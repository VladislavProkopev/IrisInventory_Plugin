#include "IrisEquipmentTypes.h"
#include "IrisEquipmentInstance.h"
#include "IrisEquipmentManagerComponent.h"


// ---------------------------------------------------------
// ВНУТРЕННЯЯ ЛОГИКА
// ---------------------------------------------------------
void FIrisEquipmentList::TrySpawnVisuals(FIrisEquipmentEntry& Entry)
{
	//Ждем пока движок разрезолвит UObject по сети (защита от nullptr)
	if (Entry.Instance == nullptr) return;
	
	//Гарантируем, что меш заспавнится только один раз
	if (Entry.bIsSpawnedLocally) return;
	
	//Вызываем логику инстанса (Он спавнит Актора в мире)
	Entry.Instance->SpawnEquipmentDef();
	Entry.bIsSpawnedLocally = true;
}


// ---------------------------------------------------------
// СЕРВЕР: ВЫДАЧА И ИЗЪЯТИЕ
// ---------------------------------------------------------
UIrisEquipmentInstance* FIrisEquipmentList::AddEntry(TSubclassOf<UIrisEquipmentInstance> InstanceClass)
{
	check(InstanceClass);
	check(OwnerComponent);
	
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());
	
	//Аллоцируем инстанс
	UIrisEquipmentInstance* NewInstance = NewObject<UIrisEquipmentInstance>(OwnerComponent,InstanceClass);
	
	//Добавляем в массив
	FIrisEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewInstance;
	
	//Уведомляем NetSerializer
	MarkItemDirty(NewEntry);
	MarkArrayDirty();
	
	//Сервер спавнит визуал сам себе, не дожидаясь сетевого такта
	TrySpawnVisuals(NewEntry);
	
	return NewInstance;
}

void FIrisEquipmentList::RemoveEntry(UIrisEquipmentInstance* Instance)
{
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		FIrisEquipmentEntry& Entry = Entries[i];
		
		if (Entry.Instance == Instance)
		{
			//Уничтожаем меш на сервере перед удалением памяти
			if (Entry.bIsSpawnedLocally && Entry.Instance)
			{
				Entry.Instance->DestroyEquipmentDef();
				Entry.bIsSpawnedLocally = false;
			}
			
			Entries.RemoveAtSwap(i);
			MarkArrayDirty();
			return;
		}
	}
}


// ---------------------------------------------------------
// КЛИЕНТ: ХУКИ СЕТИ
// ---------------------------------------------------------
void FIrisEquipmentList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FIrisEquipmentEntry& Entry = Entries[Index];
		
		//Очищаем мир от меша, до того как удалится структура
		if (Entry.Instance != nullptr && Entry.bIsSpawnedLocally)
		{
			Entry.Instance->DestroyEquipmentDef();
			Entry.bIsSpawnedLocally = false;
		}
	}
}

void FIrisEquipmentList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		TrySpawnVisuals(Entries[Index]);
	}
}

void FIrisEquipmentList::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		//Если указатель пришел с задержкой, он обрабатывается здесь
		TrySpawnVisuals(Entries[Index]);
	}
}


