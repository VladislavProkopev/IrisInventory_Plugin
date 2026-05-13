#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "IrisInventoryTypes.h"
#include "Inventory/Interfaces/IrisInventoryInterface.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "IrisInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIrisInventoryItemRemovedSignature, const UIrisInventoryItemDefinition*, RemovedItemDef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIrisInventoryItemAddedSignature, const UIrisInventoryItemDefinition*, AddedItemDef,int32,NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIrisInventoryItemUpdatedSignature, const UIrisInventoryItemDefinition*, UpdatedItemDef,int32,NewCount);
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
	//~ End IIrisInventoryInterface
	
	static const FName NAME_ActorFeatureName;
	
	//~ IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override;
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface
	
	UFUNCTION(BlueprintCallable, Category = "IrisInventory|Operations",BlueprintAuthorityOnly)
	void AddEntry(const UIrisInventoryItemDefinition* ItemDef,int32 CountToAdd);
	
	//Делегаты для обратной совместимости в EquipmentManager
	UPROPERTY(BlueprintAssignable,Category="IrisInventory|Events")
	FIrisInventoryItemRemovedSignature OnItemRemoved;
	
	UPROPERTY(BlueprintAssignable,Category="IrisInventory|Events")
	FIrisInventoryItemAddedSignature OnItemAdded;
	
	UPROPERTY(BlueprintAssignable,Category="IrisInventory|Events")
	FIrisInventoryItemUpdatedSignature OnItemUpdated;
	
	//Метод связи с L2 (Вызывается из FIrisInventoryList)
	void BroadcastInventoryUpdate(const UIrisInventoryItemDefinition* ItemDef,int32 NewCount,EIrisInventoryChangeType ChangeType);
	
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void OnRegister() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	UPROPERTY(Replicated)
	FIrisInventoryList Inventory;
	
	int32 GetMaxStackSize(const UIrisInventoryItemDefinition* ItemDef) const;
	
};
