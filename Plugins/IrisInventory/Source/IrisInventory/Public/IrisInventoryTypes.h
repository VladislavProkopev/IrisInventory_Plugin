#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "CoreFeatures/Public/Inventory/Items/IrisInventoryItemDefinition.h"
#include "IrisInventoryTypes.generated.h"

class UIrisInventoryComponent;

UENUM(BlueprintType)
enum EIrisInventoryChangeType : uint8
{
	Added, //Создали новый стак
	Updated, //Изменилось количество
	Removed //Предмет полностью удален
};

USTRUCT(BlueprintType)
struct FSFInventoryChangeMessage
{
	GENERATED_BODY()
	
	//ReadOnly, чтобы UI не мог случайно изменить стейт
	UPROPERTY(BlueprintReadOnly,Category="IrisInventory|Message")
	TObjectPtr<const UIrisInventoryItemDefinition> ItemDef = nullptr;
	
	//Критично: Виджет слота должен знать, какой именно стак обновился
	UPROPERTY(BlueprintReadOnly,Category="IrisInventory|Message")
	int32 InstanceID = INDEX_NONE;
	
	UPROPERTY(BlueprintReadOnly,Category="IrisInventory|Message")
	int32 NewCount = 0;
	
	UPROPERTY(BlueprintReadOnly,Category="IrisInventory|Message")
	TEnumAsByte<EIrisInventoryChangeType> ChangeType = EIrisInventoryChangeType::Updated;
};

USTRUCT(BlueprintType)
struct FIrisInventoryAddResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly,Category="IrisInventory|Result")
	int32 RequestedCount = 0;
	
	//Сколько реально было добавлено в инвентарь
	UPROPERTY(BlueprintReadOnly,Category="IrisInventory|Result")
	int32 ActuallyAdded = 0;
	
	//Сколько не влезло (из-за лимита веса или кастомной логики)
	UPROPERTY(BlueprintReadOnly,Category="IrisInventory|Result")
	int32 RejectedCount = 0;

	bool IsFullySuccessful() const {return RequestedCount > 0 && RequestedCount == ActuallyAdded;}
	bool IsPartiallySuccessful() const {return ActuallyAdded > 0 && RejectedCount > 0;}
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FIrisInventoryListChangedSignature, const UIrisInventoryItemDefinition*, int32,EIrisInventoryChangeType);

class UIrisInventoryComponent;

// ---------------------------------------------------------
// 1. ДИНАМИЧЕСКИЙ СТАТ (Tag Stack Pattern)
// ---------------------------------------------------------
USTRUCT(BlueprintType)
struct FIrisInventoryStatValue
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite,Category="IrisInventory|Stat")
	FGameplayTag StatTag;
	
	UPROPERTY(BlueprintReadWrite,Category="IrisInventory|Stat")
	int32 Value = 0;
};

// ---------------------------------------------------------
// 2. ЭЛЕМЕНТ (Один слот в рюкзаке)
// ---------------------------------------------------------
USTRUCT(BlueprintType)
struct FIrisInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	//Ссылка на DataAsset (по сети летит как PrimaryAssetId)
	UPROPERTY()
	TObjectPtr<const UIrisInventoryItemDefinition> ItemDef = nullptr;
	
	UPROPERTY()
	int32 StackCount = 0;
	
	UPROPERTY()
	TArray<FIrisInventoryStatValue> DynamicStats;
	
	UPROPERTY()
	int32 InstanceID = INDEX_NONE;
	
	bool IsValid() const {return ItemDef != nullptr && StackCount > 0;}
	
	//Мутаторы статов
	int32 GetStatValue(FGameplayTag StatTag) const;
	void AddStat(FGameplayTag StatTag, int32 ValueAmount);
	void RemoveStat(FGameplayTag StatTag, int32 ValueAmount);
	void SetStat(FGameplayTag StatTag, int32 NewValue);
};

// ---------------------------------------------------------
// 3. МАССИВ ИНВЕНТАРЯ (L2 Контейнер)
// ---------------------------------------------------------
USTRUCT(BlueprintType)
struct FIrisInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FIrisInventoryEntry> Entries;
	
	FIrisInventoryListChangedSignature OnListChanged;
	
	/*
	 *Учёт веса при изменении количества. CountDelta знаковая:
	 *положительная при добавлении, отрицательная при снятии.
	 *
	 *Отдельный метод, чтобы на местах вызова нельзя было перепутать знак
	 *и чтобы формула веса лежала в одном месте
	 */
	void ApplyWeightForItems(const UIrisInventoryItemDefinition* ItemDef, int32 CountDelta);
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FIrisInventoryEntry, FIrisInventoryList>(Entries, DeltaParams,*this);
	}
	
	// --- СЕРВЕРНЫЕ МУТАТОРЫ ---
	void AddAmountToEntry(FIrisInventoryEntry& Entry, int32 Amount);
	void CreateNewEntry(const UIrisInventoryItemDefinition* ItemDef,int32 Amount);
	//Разделяет стак. Возвращает InstanveID нового стака или INDEX_NONE при провале
	int32 SplitEntry(int32 SourceInstanceID, int32 AmountToSplit);
	//Сливает Source в Target. Возвращает true, если хоть что-то было перемещено
	bool MergeEntries(int32 SourceInstanceID, int32 TargetInstanceID);
	
	// --- КЛИЕНТСКИЕ ХУКИ ---
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices,int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices,int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices,int32 FinalSize);
	
	//TODO Разобраться с циклическими зависимостями, возможно вынести логику в L0 интерфейс
	//Единое окно оповещения (для UI и стейт машины)
	void BroadcastChange(const UIrisInventoryItemDefinition* ItemDef,int32 NewCount,EIrisInventoryChangeType ChangeType) const
	{
		OnListChanged.Broadcast(ItemDef,NewCount,ChangeType);
	}
	
	void RemoveEntryByID(int32 InstanceID,int32 CountToRemove);
	
	//NotReplicated обязательно, иначе Iris попытается сериализовать весь компонент
	UPROPERTY(NotReplicated)
	TObjectPtr<UIrisInventoryComponent> OwnerComponent = nullptr;
	
	FIrisInventoryAddResult AddEntry_Batched(const UIrisInventoryItemDefinition* ItemDef, int32 Count);
	
		
};

// ---------------------------------------------------------
// 4. РЕГИСТРАЦИЯ ТРЕЙТА
// ---------------------------------------------------------
template<>
struct TStructOpsTypeTraits<FIrisInventoryList> : public TStructOpsTypeTraitsBase2<FIrisInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

