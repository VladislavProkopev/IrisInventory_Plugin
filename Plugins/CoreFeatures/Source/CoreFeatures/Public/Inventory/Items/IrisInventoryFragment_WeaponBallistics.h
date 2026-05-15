// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "IrisInventoryItemFragment.h"
#include "IrisInventoryFragment_WeaponBallistics.generated.h"

/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew, meta=(DisplayName="Weapon Ballistics Fragment"))
class COREFEATURES_API UIrisInventoryFragment_WeaponBallistics : public UIrisInventoryItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Timing")
	float TimeBetweenShots = 0.1f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Trace")
	int32 PelletsPerShot = 1;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Trace")
	float SpreadAngle = 0.0f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Trace")
	UCurveFloat* SpreadCurve = nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Trace")
	float MaxRange = 10000.f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Damage")
	float BaseDamage = 10.f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Visuals")
	FGameplayTag FireCueTag;
};
