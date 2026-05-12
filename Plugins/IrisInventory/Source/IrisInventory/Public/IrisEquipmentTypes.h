#pragma once
#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "IrisEquipmentTypes.generated.h"

class UIrisEquipmentInstance;
class UIrisEquipmentManagerComponent;

// ---------------------------------------------------------
// 1. ЭЛЕМЕНТ ЭКИПИРОВКИ (Один слот в руках/на теле)
// ---------------------------------------------------------
USTRUCT(BlueprintType)
struct FIrisEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	//Указатель на транзитный контроллер оружия
	UPROPERTY()
	TObjectPtr<UIrisEquipmentInstance> Instance = nullptr;
	
	//Локальный стейт (не реплицируется).
	//Защищает от двойного спавна меша на клиенте когда данные прийдут по сети
	bool bIsSpawnedLocally = false;
};

// ---------------------------------------------------------
// 2. МАССИВ ЭКИПИРОВКИ (Контейнер)
// ---------------------------------------------------------
USTRUCT(BlueprintType)
struct FIrisEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FIrisEquipmentEntry> Entries;
	
	//Ссылка на компонент, который владеет этим массивом
	UPROPERTY(NotReplicated)
	TObjectPtr<UIrisEquipmentManagerComponent> OwnerComponent = nullptr;
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FIrisEquipmentEntry,FIrisEquipmentList>(Entries, DeltaParams,*this);
	}
	
	// --- СЕРВЕРНЫЕ МУТАТОРЫ ---
	UIrisEquipmentInstance* AddEntry(TSubclassOf<UIrisEquipmentInstance> InstanceClass);
	void RemoveEntry(UIrisEquipmentInstance* Instance);
	
	// --- КЛИЕНТСКИЕ МУТАТОРЫ ---
	void PreReplicationRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	
private:
	//Внутренний метод для безопасного спавна визуала
	void TrySpawnVisuals(FIrisEquipmentEntry& Entry);
};

// ---------------------------------------------------------
// 3. РЕГИСТРАЦИЯ ТРЕЙТА
// ---------------------------------------------------------
template<>
struct TStructOpsTypeTraits<FIrisEquipmentList> : public TStructOpsTypeTraitsBase2<FIrisEquipmentList>
{
	enum {WithNetDeltaSerializer = true};
};