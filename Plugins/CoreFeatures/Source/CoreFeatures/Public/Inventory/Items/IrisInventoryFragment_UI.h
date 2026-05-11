// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Items/IrisInventoryItemFragment.h"
#include "IrisInventoryFragment_UI.generated.h"

class UTexture2D;

/**
 * L1 Core: Immutable UI Data Fragment
 * Injected via Editor into UIrisInventoryItemDefinition
 */
UCLASS(DefaultToInstanced,EditInlineNew,meta=(DisplayName="UI Fragment"))
class COREFEATURES_API UIrisInventoryFragment_UI : public UIrisInventoryItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IrisInventory|UI")
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IrisInventory|UI")
	FText Description;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="IrisInventory|UI")
	TSoftObjectPtr<UTexture2D> Icon;
};
