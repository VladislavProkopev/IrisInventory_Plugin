// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IrisInventoryItemFragment.h"
#include "IrisInventoryFragment_Stackable.generated.h"

/**
 * 
 */
UCLASS()
class IRISINVENTORY_API UIrisInventoryFragment_Stackable : public UIrisInventoryItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category=Inventory)
	int32 MaxStackSize = 1;
};
