// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "IrisInventoryTypes.h"
#include "Inventory/Interfaces/IrisInventoryInterface.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "IrisInventoryComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class IRISINVENTORY_API UIrisInventoryComponent : public UPawnComponent , public IIrisInventoryInterface, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	UIrisInventoryComponent(const FObjectInitializer& ObjectInitializer);
	
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
	
	UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="IrisInventory")
	void AddItemDefinition(const UIrisInventoryItemDefinition* ItemDef,int32 Count);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void OnRegister() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION(BlueprintCallable,Category="IrisInventory|Operations",BlueprintAuthorityOnly)
	void AddEntry(UIrisInventoryItemDefinition* ItemDef,int32 CountToAdd);
	
	UPROPERTY(Replicated)
	FIrisInventoryList Inventory;
	
private:
	int32 GetMaxStackSize(UIrisInventoryItemDefinition* ItemDef) const;
};
