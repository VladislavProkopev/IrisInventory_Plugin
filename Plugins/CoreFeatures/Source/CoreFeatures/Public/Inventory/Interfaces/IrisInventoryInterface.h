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
 * Контракт L1 для инвентаря. Намеренно чистый C++ без UFUNCTION:
 * методы вызываются из горячих путей (GAS CheckCost - потенциально каждый кадр),
 * а BlueprintNativeEvent там означал бы Execute_X вместо прямого виртуального
 * вызова и ProcessEvent при переопределении в BP.
 * Точки расширения для дизайнеров живут на компоненте, на холодном пути.
 */

class COREFEATURES_API IIrisInventoryInterface
{
	GENERATED_BODY()

public:
	// --- READ-ONLY OPERATIONS ---
	virtual const UIrisInventoryItemDefinition* GetItemDefAtSlot(int32 SlotIndex) const = 0;
	
	//-------------------------------------------------------------------------------------------------------
	/*TODO Refactor Comment
	UFUNCTION(BlueprintCallable,Category="IrisInventory|Read")
	virtual int32 GetItemStat(int32 SlotIndex, FGameplayTag StatTag) const = 0;
	*/
	//-------------------------------------------------------------------------------------------------------
	
	//Все публичные операции над конкретным стаком - по InstanceID.
	//SlotIndex нестабилен: RemoveAtSwap переставляет элементы, и сохранённый
	//индекс начинает указывать на чужой предмет без каких-либо признаков ошибки
	virtual int32 GetItemStatByInstanceID(int32 InstanceID, FGameplayTag StatTag) const = 0;
	
	virtual int32 FindSlotByInstanceID(int32 InstanceID) const = 0;
	
	// Подсчет всех стаков предмета с данным тегом (Для GAS CheckCost)
	// Асимптотика: 0(N), где N - количество уникальных стаков в инвентаре
	virtual int32 GetTotalItemCountByTag(FGameplayTag ItemTag) const = 0;
	
	// --- TRANSACTIONAL MUTATORS ---
	
	//-------------------------------------------------------------------------------------------------------
	/*TODO Refactor Comment
	UFUNCTION(BlueprintCallable,Category="IrisInventory|Mutate")
	virtual void ModifyItemStat(int32 SlotIndex, FGameplayTag StatTag, int32 Delta) = 0;
	*/
	//-------------------------------------------------------------------------------------------------------
	
	//Все публичные операции над конкретным стаком - по InstanceID.
	//SlotIndex нестабилен: RemoveAtSwap переставляет элементы, и сохранённый
	//индекс начинает указывать на чужой предмет без каких-либо признаков ошибки
	virtual bool ModifyItemStatByInstanceID(int32 InstanceID, FGameplayTag StatTag, int32 Delta) = 0;
	
	//Списание предметов по тегу (Для GAS ApplyCost)
	//Возвращает фактически списанное количество (защита от race conditions на сервере)
	virtual int32 ConsumeItemByTag(FGameplayTag ItemTag, int32 CountToConsume) = 0;
	
	//Физическое удаление предметов (Drop, Consume, Trade)
	//Возвращает true, если транзакция успешна и массив (FastArray) был обновлен
	virtual bool RemoveItemByInstanceID(int32 InstanceID,int32 CountToRemove) = 0;
	

};
