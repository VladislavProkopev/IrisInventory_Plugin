#include "IrisInventoryTypes.h"
#include "IrisInventoryComponent.h"
#include "Inventory/Items/IrisInventoryFragment_Stackable.h"


// =========================================================
// FIrisInventoryEntry: УПРАВЛЕНИЕ СТАТАМИ
// =========================================================
int32 FIrisInventoryEntry::GetStatValue(FGameplayTag StatTag) const
{
	for (const FIrisInventoryStatValue& Stat : DynamicStats)
	{
		if (Stat.StatTag == StatTag) return Stat.Value;
	}
	return 0;
}

void FIrisInventoryEntry::AddStat(FGameplayTag StatTag, int32 ValueAmount)
{
	if (ValueAmount == 0) return;
	
	for (FIrisInventoryStatValue& Stat : DynamicStats)
	{
		if (Stat.StatTag == StatTag)
		{
			Stat.Value += ValueAmount;
			return;
		}
	}
	
	FIrisInventoryStatValue& NewStat = DynamicStats.AddDefaulted_GetRef();
	NewStat.StatTag = StatTag;
	NewStat.Value = ValueAmount;
}

void FIrisInventoryEntry::RemoveStat(FGameplayTag StatTag, int32 ValueAmount)
{
	if (ValueAmount == 0) return;
	
	for (int32 i = 0; i < DynamicStats.Num(); i++)
	{
		if (DynamicStats[i].StatTag == StatTag)
		{
			DynamicStats[i].Value -= ValueAmount;
			
			//GC-free удаление: 0(1) если стат исчерпан
			if (DynamicStats[i].Value <= 0)
			{
				DynamicStats.RemoveAtSwap(i);
			}
			return;
		}
	}
}

void FIrisInventoryEntry::SetStat(FGameplayTag StatTag, int32 NewValue)
{
	if (NewValue <= 0)
	{
		RemoveStat(StatTag,INT_MAX);
		return;
	}
	
	for (FIrisInventoryStatValue& Stat : DynamicStats)
	{
		if (Stat.StatTag == StatTag)
		{
			Stat.Value = NewValue;
			return;
		}
	}
	
	FIrisInventoryStatValue& NewStat = DynamicStats.AddDefaulted_GetRef();
	NewStat.StatTag = StatTag;
	NewStat.Value = NewValue;
}

// =========================================================
// FIrisInventoryList: УПРАВЛЕНИЕ МАССИВОМ И СЕТЬЮ
// =========================================================

/*TODO To Delete?
 *void FIrisInventoryList::BroadcastChange(const UIrisInventoryItemDefinition* ItemDef, int32 NewCount)
{
	//Безопасно делегируем оповещение в L3-компонент (если он существует)
	//Позже добавим в компонент метод BroadcastInventoryUpdate, который дернет GameplayMessageRouter
	if (OwnerComponent)
	{
		//Расскоментируем позже как будет реализована логика
		OwnerComponent->BroadcastInventoryUpdate(ItemDef,NewCount);
	}
}*/

void FIrisInventoryList::AddAmountToEntry(FIrisInventoryEntry& Entry, int32 Amount)
{
	Entry.StackCount += Amount;
	MarkItemDirty(Entry);
	
	//СИММЕТРИЯ: Сервер обновляет свой UI не дожидаясь сети
	BroadcastChange(Entry.ItemDef,Entry.StackCount,EIrisInventoryChangeType::Updated);
}

void FIrisInventoryList::CreateNewEntry(const UIrisInventoryItemDefinition* ItemDef, int32 Amount)
{
	FIrisInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.ItemDef = ItemDef;
	NewEntry.StackCount = Amount;
	
	if (OwnerComponent && OwnerComponent->HasAuthority())
	{
		NewEntry.InstanceID = OwnerComponent->GenerateInstanceID();
	}
	
	MarkItemDirty(NewEntry);
	MarkArrayDirty();
	
	//СИММЕТРИЯ
	BroadcastChange(ItemDef,Amount,EIrisInventoryChangeType::Added);
}

int32 FIrisInventoryList::SplitEntry(int32 SourceInstanceID, int32 AmountToSplit)
{
	if (AmountToSplit<=0||!OwnerComponent) return INDEX_NONE;
	
	for (int32 i=0; i<Entries.Num(); i++)
	{
		FIrisInventoryEntry& SourceEntry = Entries[i];
		
		if (SourceEntry.InstanceID == SourceInstanceID)
		{
			//Валидация: нельзя отщепить больше, чем есть, и нельзя оставлять 0 без удаления
			if (AmountToSplit>=SourceEntry.StackCount) return INDEX_NONE;
			
			//1. Уменьшаем оригинальный стак
			SourceEntry.StackCount -= AmountToSplit;
			MarkItemDirty(SourceEntry);
			
			//2. Генерируем новый ID и создаем новый стак
			int32 NewInstanceID = OwnerComponent->GenerateInstanceID();
			
			FIrisInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
			NewEntry.ItemDef = SourceEntry.ItemDef;
			NewEntry.StackCount = AmountToSplit;
			NewEntry.InstanceID = NewInstanceID;
			
			//Если использются DynamicStats (прочность/модификации), здесь их тоже нужно скопировать
			//NewEntry.DynamicStats = SourceEntry.DynamicStats
			
			MarkItemDirty(NewEntry);

			if (OwnerComponent)
			{
				OwnerComponent->BroadcastInventoryUpdate(SourceEntry.ItemDef,SourceEntry.StackCount,EIrisInventoryChangeType::Updated,SourceEntry.InstanceID);
				OwnerComponent->BroadcastInventoryUpdate(NewEntry.ItemDef,NewEntry.StackCount,EIrisInventoryChangeType::Added,NewEntry.InstanceID);
			}
			
			return NewInstanceID;
		}
	}
	return INDEX_NONE;
}

bool FIrisInventoryList::MergeEntries(int32 SourceInstanceID, int32 TargetInstanceID)
{
	if (SourceInstanceID == TargetInstanceID) return false;
	
	FIrisInventoryEntry* SourceEntry = nullptr;
	FIrisInventoryEntry* TargetEntry = nullptr;
	
	//0(n) поиск обоих элементов за один проход
	for (FIrisInventoryEntry& Entry : Entries)
	{
		if (Entry.InstanceID == SourceInstanceID) SourceEntry = &Entry;
		else if (Entry.InstanceID == TargetInstanceID) TargetEntry = &Entry;
		
		if (SourceEntry && TargetEntry) break;
	}
	if (!SourceEntry || !TargetEntry) return false;
	if (SourceEntry->ItemDef != TargetEntry->ItemDef) return false;
	
	//Получаем MaxStackSize из L1 фрагмента. Если фрагментов нет, считаем не стакаемым (Max = 1)
	int32 MaxStack = 1;
	if (const UIrisInventoryFragment_Stackable* StackFrag = TargetEntry->ItemDef->FindFragmentByClass<UIrisInventoryFragment_Stackable>())
	{
		MaxStack = StackFrag->MaxStackSize;
	}
	
	int32 SpaceLeft = MaxStack - TargetEntry->StackCount;
	if (SpaceLeft <= 0) return false; //Таргет полон
	
	//Вычисляем, сколько реально можем переместить
	int32 AmountToMove = FMath::Min(SpaceLeft,SourceEntry->StackCount);
	
	//1. Увеличиваем таргет
	TargetEntry->StackCount += AmountToMove;
	MarkItemDirty(*TargetEntry);

	if (OwnerComponent)
	{
		OwnerComponent->BroadcastInventoryUpdate(TargetEntry->ItemDef,TargetEntry->StackCount,EIrisInventoryChangeType::Updated,TargetEntry->InstanceID);
	}
	
	//2. Уменьшаем/удаляем сурс(переиспользуем готовую 0(1) GC-Free функцию
	//RemoveEntryByID внутри сама вызовет MarkItemDirty или MarkArrayDirty
	RemoveEntryByID(SourceInstanceID,TargetInstanceID);
	
	return true;
}

// =========================================================
// --------- Хуки клиента
// =========================================================
void FIrisInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FIrisInventoryEntry& Entry = Entries[Index];
		BroadcastChange(Entry.ItemDef,Entry.StackCount,EIrisInventoryChangeType::Added);
	}
}
void FIrisInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FIrisInventoryEntry& Entry = Entries[Index];
		//Сообщаем UI, что количество стало 0 (предмет удален)
		BroadcastChange(Entry.ItemDef,0,EIrisInventoryChangeType::Removed);
	}
}

void FIrisInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FIrisInventoryEntry& Entry = Entries[Index];
		BroadcastChange(Entry.ItemDef,Entry.StackCount,EIrisInventoryChangeType::Updated);
	}
}

void FIrisInventoryList::RemoveEntryByID(int32 InstanceID, int32 CountToRemove)
{
	if (CountToRemove <= 0) return;
	
	for (int32 i=0; i<Entries.Num(); i++)
	{
		if (Entries[i].InstanceID == InstanceID)
		{
			FIrisInventoryEntry& Entry = Entries[i];
			const UIrisInventoryItemDefinition* CachedItemDef = Entry.ItemDef;
			
			if (Entry.StackCount <= CountToRemove)
			{
				//Полное удаление стака (0(1) GC-Free)
				int32 RemovedCount = Entry.StackCount;
				
				Entries.RemoveAtSwap(i);
				MarkArrayDirty();
				
				if (OwnerComponent)
				{
					OwnerComponent->BroadcastInventoryUpdate(CachedItemDef,0,EIrisInventoryChangeType::Removed,Entry.InstanceID);
				}
			}
			else
			{
				//Частичное уменьшение стака (Delta Update)
				Entry.StackCount -= CountToRemove;
				MarkItemDirty(Entry);

				if (OwnerComponent)
				{
					OwnerComponent->BroadcastInventoryUpdate(CachedItemDef,Entry.StackCount,EIrisInventoryChangeType::Updated,Entry.InstanceID);
				}
			}
			return;
		}
	}
}



