// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IrisInventoryTypes.h"
#include "Inventory/Items/Interfaces/IrisInventoryInterface.h"
#include "IrisInventoryComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class IRISINVENTORY_API UIrisInventoryComponent : public UActorComponent , public IIrisInventoryInterface
{
	GENERATED_BODY()

public:
	UIrisInventoryComponent(const FObjectInitializer& ObjectInitializer);
	
	//~ IIrisInventoryInterface
	virtual int32 GetItemStat(int32 SlotIndex, FGameplayTag StatTag) const override;
	virtual void ModifyItemStat(int32 SlotIndex, FGameplayTag StatTag, int32 Delta) override;
	virtual const UIrisInventoryItemDefinition* GetItemDefAtSlot(int32 SlotIndex) const override;
	//~ End IIrisInventoryInterface
	
	UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="IrisInventory")
	void AddItemDefinition(const UIrisInventoryItemDefinition* ItemDef,int32 Count);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(Replicated)
	FIrisInventoryList Inventory;
};
