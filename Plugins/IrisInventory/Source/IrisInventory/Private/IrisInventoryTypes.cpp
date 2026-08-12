#include "IrisInventoryTypes.h"
#include "IrisInventoryComponent.h"
#include "Inventory/Items/IrisInventoryFragment_Stackable.h"

DEFINE_LOG_CATEGORY_STATIC(Log_IrisInventoryTypes,All,All);

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

void FIrisInventoryList::ApplyWeightForItems(const UIrisInventoryItemDefinition* ItemDef, int32 CountDelta)
{
	if (!OwnerComponent || !ItemDef || CountDelta == 0) return;

	OwnerComponent->ApplyWeightDelta(OwnerComponent->GetItemWeight(ItemDef) * CountDelta);
}

void FIrisInventoryList::AddAmountToEntry(FIrisInventoryEntry& Entry, int32 Amount)
{
	Entry.StackCount += Amount;
	
	if (OwnerComponent)
	{
		ApplyWeightForItems(Entry.ItemDef,Amount);
	}
	
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

	if (OwnerComponent)
	{
		ApplyWeightForItems(ItemDef,Amount);
	}
	
	MarkItemDirty(NewEntry);
	MarkArrayDirty();

	
	
	//СИММЕТРИЯ
	BroadcastChange(ItemDef,Amount,EIrisInventoryChangeType::Added);
}

int32 FIrisInventoryList::SplitEntry(int32 SourceInstanceID, int32 AmountToSplit)
{
	//TODO Вернуться после разбора аудита
	if (AmountToSplit<=0||!OwnerComponent) return INDEX_NONE;
	
	for (int32 i=0; i<Entries.Num(); i++)
	{
		FIrisInventoryEntry& SourceEntry = Entries[i];
		
		if (SourceEntry.InstanceID == SourceInstanceID)
		{
			//Валидация: нельзя отщепить больше, чем есть, и нельзя оставлять 0 без удаления
			if (AmountToSplit>=SourceEntry.StackCount) return INDEX_NONE;
			
			//См. комментарий в MergeEntries: разделение стака со статами так же
			//не определено, как и слияние - копирование дублировало бы состояние
			if (!SourceEntry.DynamicStats.IsEmpty()) return INDEX_NONE;
			
			//1. Уменьшаем оригинальный стак
			SourceEntry.StackCount -= AmountToSplit;
			MarkItemDirty(SourceEntry);
			
			//2. Генерируем новый ID и создаем новый стак
			int32 NewInstanceID = OwnerComponent->GenerateInstanceID();
			
			FIrisInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
			NewEntry.ItemDef = SourceEntry.ItemDef;
			NewEntry.StackCount = AmountToSplit;
			NewEntry.InstanceID = NewInstanceID;
			
			//---------------------------------------------------------------------------------------------
			//TODO Refactor Coment
			//Если использются DynamicStats (прочность/модификации), здесь их тоже нужно скопировать
			//NewEntry.DynamicStats = SourceEntry.DynamicStats
			//---------------------------------------------------------------------------------------------
			
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
	/*TODO Вернуться после разбора аудита
	 *Пока иду по плану разбора но помню что пригли к элегантному решению
	*## Почему я выбрал запрет, а не копирование или усреднение
	*Дело не в том, какую формулу выбрать, а в том, что вопрос поставлен некорректно. Спроси себя: **какова прочность стака из десяти мечей?**
	*На этот вопрос нет ответа. Если два экземпляра предмета различимы по состоянию — они не взаимозаменяемы, а «стак из N» именно взаимозаменяемость и означает. Число в `StackCount` имеет смысл только тогда, когда все N единиц одинаковы.
	*Отсюда правило: **предмет либо стакуется, либо имеет состояние экземпляра. Одновременно — нет.**
	*Что было бы не так с альтернативами:
	*| Вариант | Почему отверг |
	*|---|---|
	*| Копировать статы в новый стак (`//NewEntry.DynamicStats = SourceEntry.DynamicStats`) | дублирует состояние из воздуха: разделил стак — получил две прочности вместо одной |
	*| Брать минимум при слиянии | тихо уничтожает ценность предмета, игрок теряет вещи «сложив их в кучу» |
	*| Взвешенное среднее | арифметически честно, но объяснить игроку невозможно, и всё равно не отвечает на вопрос «сколько прочности у одного меча из стака» |
	*| Отдать на откуп BP-политике `CanMergeItems` | дизайнер вернёт `true`, не зная про эту тонкость, и данные пропадут молча |
	*Последний пункт — ключевой для выбора **места** проверки. Это не правило игры, которое настраивают, а инвариант целостности данных. Инварианты живут в C++ и не переопределяются из блюпринта. `CanMergeItems` остаётся для игровых правил вроде «нельзя складывать привязанные к персонажу предметы» и вызывается **после** этой проверки.
	*## Практическая цена: ноль
	*Предметы с `DynamicStats` в твоём дизайне — оружие с прочностью и зарядом, а у них `MaxStackSize = 1`. Стак из одного нельзя ни слить в полный стак, ни разделить. То есть гвард не срабатывает никогда в нормальном сценарии — он ловит только ошибки конфигурации ассетов, когда предмету со статами по недосмотру поставили стак больше единицы.
	*Ровно то, чего хочется от инварианта: бесплатен в работе, громок при ошибке.
	 */
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
	
	//Инвариант: стакуемость и состояние экземпляра несовместимы.
	//"Прочность стака из десяти мечей" - вопрос без ответа, поэтому слияние
	//стаков с непринадлежащими им статами запрещаем в C++, а не в BP-политике:
	//это целостность данных, а не игровое правило
	if (!SourceEntry->DynamicStats.IsEmpty() || !TargetEntry->DynamicStats.IsEmpty())
		{
			UE_LOG(Log_IrisInventoryTypes,Warning,
				TEXT("[%s] Merge rejected: item %s carries per-instance stats but has MaxStackSize > 1. Check the asset."),
				ANSI_TO_TCHAR(__FUNCTION__),*GetNameSafe(SourceEntry->ItemDef));
			return false;
		}
	
	//Получаем MaxStackSize из L1 фрагмента. Если фрагментов нет, считаем не стакаемым (Max = 1)
	int32 MaxStack = 1;
	if (const UIrisInventoryFragment_Stackable* StackFrag = TargetEntry->ItemDef->GetStackableFragment())
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
	
	//------------------------------------------------------------------------------------------------------------
	/*TODO Ошибка логики!!! Проверить логику AmountToMove
	 *
	*Но рядом есть настоящая недоработка:** метод возвращает `true` и при частичном слиянии,
	*когда `SpaceLeft < Source->StackCount`. Вызывающий не может отличить «слилось целиком»
	*от «слилось сколько влезло, источник ещё есть». Для drag-and-drop в UI это важно
	*— виджет источника нельзя убирать.
	*Не чиню сейчас: это меняет сигнатуру публичного метода, а UI ещё не написан. 
	*Но когда возьмёшься за UI — возвращай не `bool`, а количество перенесённого.
	 */
	RemoveEntryByID(SourceInstanceID,AmountToMove);
	
	return true;
	//------------------------------------------------------------------------------------------------------------
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
			
			//InstanceID кешируем по той же причине, что и ItemDef: после RemoveAtSwap
			//ссылка Entry указывает на переехавший последний элемент, а при удалении
			//последнего - за границу логического размера массива
			const int32 CachedInstanceID = Entry.InstanceID;
			
			if (Entry.StackCount <= CountToRemove)
			{
				//Полное удаление стака (0(1) GC-Free)
				
				//------------------------------------------------------------------------
				//TODO Refactor comment
				//int32 RemovedCount = Entry.StackCount;
				//------------------------------------------------------------------------
				
				//Снимается СТОЛЬКО, СКОЛЬКО ЕСТЬ, а не сколько попросили:
				//условие ветки допускает CountToRemove больше содержимого стака.
				//Для веса важно именно фактическое количество
				const int32 RemovedCount = Entry.StackCount;
				
				Entries.RemoveAtSwap(i);
				
				//Минус: предметы ушли из инвентаря
				ApplyWeightForItems(CachedItemDef,-RemovedCount);
				
				//Элемент, переехавший в освободившийся слот, обязан быть помечен отдельно -
				//MarkArrayDirty взводит бит массива, но не бит элемента (этап 9, пункт I5)
				if (Entries.IsValidIndex(i))
				{
					MarkItemDirty(Entries[i]);
				}
				
				MarkArrayDirty();
				
				if (OwnerComponent)
				{
					OwnerComponent->BroadcastInventoryUpdate(CachedItemDef,0,EIrisInventoryChangeType::Removed,CachedInstanceID);
				}
			}
			else
			{
				//Частичное уменьшение стака (Delta Update)
				Entry.StackCount -= CountToRemove;
				
				//Здесь фактическое и запрошенное совпадают: условие ветки
				//гарантирует, что в стаке было больше, чем снимаем
				ApplyWeightForItems(CachedItemDef,-CountToRemove);
				
				MarkItemDirty(Entry);				
			}
			return;
		}
	}
}

FIrisInventoryAddResult FIrisInventoryList::AddEntry_Batched(const UIrisInventoryItemDefinition* ItemDef, int32 Count)
{
	FIrisInventoryAddResult Result;
	Result.RequestedCount = Count;
	
	if (!ItemDef || Count <= 0) return Result;

	int32 RemainingCount = Count;
	int32 MaxStackSize = 1; // Дефолтное значение для не-стакаемых предметов (оружие, броня)

	if (const UIrisInventoryFragment_Stackable* StackFragment = ItemDef->GetStackableFragment())
	{
		MaxStackSize = StackFragment->MaxStackSize;
	} 
	
	// 1. Доливаем в существующие неполные стаки
	for (FIrisInventoryEntry& Entry : Entries)
	{
		if (Entry.ItemDef == ItemDef && Entry.StackCount < MaxStackSize)
		{
			int32 AmountToAdd = FMath::Min(RemainingCount, MaxStackSize - Entry.StackCount);
			Entry.StackCount += AmountToAdd;
			RemainingCount -= AmountToAdd;
			Result.ActuallyAdded += AmountToAdd;

			MarkItemDirty(Entry); // Помечаем обновленный элемент
			if (RemainingCount <= 0) break;
		}
	}

	// 2. Создаем новые стаки для остатка
	while (RemainingCount > 0)
	{
		int32 AmountForNewStack = FMath::Min(RemainingCount, MaxStackSize);

		FIrisInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
		NewEntry.ItemDef = ItemDef;
		NewEntry.StackCount = AmountForNewStack;
		NewEntry.InstanceID = OwnerComponent->GenerateInstanceID(); // Вызов генератора ID

		RemainingCount -= AmountForNewStack;
		Result.ActuallyAdded += AmountForNewStack;
		
		//TODO Проверить Начисление веса в игре, потому что вроде как логика верна
		ApplyWeightForItems(ItemDef,1);
		MarkItemDirty(NewEntry); // Помечаем новый элемент
	}

	// 3. Единственный вызов сериализатора на весь батч
	if (Result.ActuallyAdded > 0)
	{
		MarkArrayDirty();
		BroadcastChange(ItemDef, Result.ActuallyAdded, EIrisInventoryChangeType::Added);
	}

	return Result;
}



