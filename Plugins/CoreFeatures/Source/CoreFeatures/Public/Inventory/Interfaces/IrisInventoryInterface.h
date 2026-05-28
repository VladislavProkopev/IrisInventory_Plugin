// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "Inventory/Items/IrisInventoryItemDefinition.h"
#include "IrisInventoryInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI,BlueprintType,NotBlueprintable)
class UIrisInventoryInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class COREFEATURES_API IIrisInventoryInterface
{
	GENERATED_BODY()

public:
	// --- READ-ONLY OPERATIONS ---
	UFUNCTION(BlueprintCallable,Category="IrisInventory|Read")
	virtual const UIrisInventoryItemDefinition* GetItemDefAtSlot(int32 SlotIndex) const = 0;
	
	UFUNCTION(BlueprintCallable,Category="IrisInventory|Read")
	virtual int32 GetItemStat(int32 SlotIndex, FGameplayTag StatTag) const = 0;
	
	UFUNCTION(BlueprintCallable,Category="IrisInventory|Read")
	virtual int32 FindSlotByInstanceID(int32 InstanceID) const = 0;
	
	// Подсчет всех стаков предмета с данным тегом (Для GAS CheckCost)
	// Асимптотика: 0(N), где N - количество уникальных стаков в инвентаре
	UFUNCTION(Blueprintable,Category="IrisInventory|Read")
	virtual int32 GetTotalItemCountByTag(FGameplayTag ItemTag) const = 0;
	
	// --- TRANSACTIONAL MUTATORS ---
	UFUNCTION(BlueprintCallable,Category="IrisInventory|Mutate")
    	virtual void ModifyItemStat(int32 SlotIndex, FGameplayTag StatTag, int32 Delta) = 0;
	
	//Списание предметов по тегу (Для GAS ApplyCost)
	//Возвращает фактически списанное количество (защита от race conditions на сервере)
	UFUNCTION(BlueprintCallable,Category="IrisInventory|Mutate")
	virtual int32 ConsumeItemByTag(FGameplayTag ItemTag, int32 CountToConsume) = 0;
	
	//Физическое удаление предметов (Drop, Consume, Trade)
	//Возвращает true, если транзакция успешна и массив (FastArray) был обновлен
	UFUNCTION(BlueprintCallable,Category="IrisInventory|Mutate")
	virtual bool RemoveItemByInstanceID(int32 InstanceID,int32 CountToRemove) = 0;
	

};
