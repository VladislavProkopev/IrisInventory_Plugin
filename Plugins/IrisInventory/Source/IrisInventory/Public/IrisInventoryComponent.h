#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "IrisInventoryTypes.h"
#include "Inventory/Interfaces/IrisInventoryInterface.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "IrisInventoryComponent.generated.h"

/* Переход на GMR
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIrisInventoryItemRemovedSignature, const UIrisInventoryItemDefinition*, RemovedItemDef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIrisInventoryItemAddedSignature, const UIrisInventoryItemDefinition*, AddedItemDef,int32,NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIrisInventoryItemUpdatedSignature, const UIrisInventoryItemDefinition*, UpdatedItemDef,int32,NewCount);
*/

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class IRISINVENTORY_API UIrisInventoryComponent : public UPawnComponent, public IGameFrameworkInitStateInterface,public IIrisInventoryInterface
{
	GENERATED_BODY()

public:
	UIrisInventoryComponent(const FObjectInitializer& OI);
	
	//~ IIrisInventoryInterface
	virtual int32 GetItemStat(int32 SlotIndex, FGameplayTag StatTag) const override;
	virtual void ModifyItemStat(int32 SlotIndex, FGameplayTag StatTag, int32 Delta) override;
	virtual const UIrisInventoryItemDefinition* GetItemDefAtSlot(int32 SlotIndex) const override;
	virtual int32 FindSlotByInstanceID(int32 InstanceID) const override;
	virtual bool RemoveItemByInstanceID(int32 InstanceID,int32 CountToRemove) override;
	virtual int32 GetTotalItemCountByTag(FGameplayTag ItemTag) const override;
	virtual int32 ConsumeItemByTag(FGameplayTag ItemTag, int32 CountToConsume) override;
	//~ End IIrisInventoryInterface
	
	static const FName NAME_ActorFeatureName;
	
	//~ IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override;
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface
	
	//Хук для определения, сколько предметов мы имеем право взять
	//BlueprintNativeEvent позволяет написать базовую С++ логику, которую можно полностью стереть в BP
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="IrisInventory|Policy")
	int32 CalculateAllowedAddAmount(const UIrisInventoryItemDefinition* ItemDef, int32 RequestedCount) const;
	
	UFUNCTION(BlueprintCallable, Category = "IrisInventory|Operations",BlueprintAuthorityOnly)
	FIrisInventoryAddResult AddEntry(const UIrisInventoryItemDefinition* ItemDef,int32 CountToAdd);
	
	/* Переход на GMR
	//Делегаты для обратной совместимости в EquipmentManager
	UPROPERTY(BlueprintAssignable,Category="IrisInventory|Events")
	FIrisInventoryItemRemovedSignature OnItemRemoved;
	
	UPROPERTY(BlueprintAssignable,Category="IrisInventory|Events")
	FIrisInventoryItemAddedSignature OnItemAdded;
	
	UPROPERTY(BlueprintAssignable,Category="IrisInventory|Events")
	FIrisInventoryItemUpdatedSignature OnItemUpdated;
	*/
	//Метод связи с L2 (Вызывается из FIrisInventoryList)
	void BroadcastInventoryUpdate(const UIrisInventoryItemDefinition* ItemDef,int32 NewCount,EIrisInventoryChangeType ChangeType,int32 InstanceID);
	
	int32 GenerateInstanceID()
	{
		check(HasAuthority());
		return NextInstanceIU++;
	}
	
	UFUNCTION(Blueprintable,Category="IrisInventory|Operations",BlueprintAuthorityOnly)
	bool MergeStacks(int32 SourceInstanceID, int32 TargetInstanceID);
	
	UFUNCTION(Blueprintable,Category="IrisInventory|Operations",BlueprintAuthorityOnly)
	int32 SplitStack(int32 SourceInstanceID, int32 AmountToSplit);
	
	UPROPERTY(EditDefaultsOnly,Category="IrisInventory|Config")
	TSubclassOf<class AItemPickup_Base> DefaultPickupClass;
	
	UFUNCTION(BlueprintCallable, Category="IrisInventory|Operations",BlueprintInternalUseOnly)
	void DropItem(int32 InstanceID, int32 CountToDrop);
	
	// --- ПРЕДИКАТЫ (POLICIES) ---
	// Переопределяются в Blueprint дочернего класса для кастомной логики
	
	//Запрет на удаление (Например, квестовые предметы или заблокированные слоты)
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="IrisInventory|Policy")
	bool CanRemoveItem(int32 InstanceID, int32 CountToRemove) const;
	
	//Запрет на слияние (Например, предметы с разной прочностью/зачарованиями)
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="IrisInventory|Policy")
	bool CanMergeItems(int32 SourceInstanceID, int32 TargetInstanceID) const;
	
	//Запрет на разделение (Например, предмет физически неделим, хотя MaxStackSize > 1)
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="IrisInventory|Policy")
	bool CanSplitItem(int32 InstanceID) const;
	
	UFUNCTION(BlueprintPure,Category="IrisInventory|State")
	int32 GetCurrentWeight() const {return CurrentWeight;}
	
	UFUNCTION(BlueprintPure,Category="IrisInventory|State")
	int32 GetMaxWeight() const {return MaxWeight;}
	
	//Вспомогательный метод для получения веса 1 штуки из L1 CDO
	virtual int32 GetItemWeight(const UIrisInventoryItemDefinition* ItemDef) const;
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void OnRegister() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	//Базовая вместимость в граммах
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IrisInventory|Config")
	int32 MaxWeight = 50000;
	
	//Реплицируем только для отображения UI (прогресс бар веса)
	UPROPERTY(Replicated,Transient,BlueprintReadOnly,Category="IrisInventory|State")
	int32 CurrentWeight = 0;
private:
	UPROPERTY(Replicated)
	FIrisInventoryList Inventory;
	
	int32 GetMaxStackSize(const UIrisInventoryItemDefinition* ItemDef) const;
	
	//Серверный счетчик. Не имеет UPROPERTY(), не репличируется
	int32 NextInstanceIU = 1;
};
