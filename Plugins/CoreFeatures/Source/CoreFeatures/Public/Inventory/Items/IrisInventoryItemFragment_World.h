// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IrisInventoryItemFragment.h"
#include "IrisInventoryItemFragment_World.generated.h"

/**
 * 
 */
UCLASS(DefaultToInstanced,EditInlineNew,meta=(DisplayName="World Visuals Fragment"))
class COREFEATURES_API UIrisInventoryItemFragment_World : public UIrisInventoryItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Visuals")
	TSoftObjectPtr<UStaticMesh> WorldMesh;
};
