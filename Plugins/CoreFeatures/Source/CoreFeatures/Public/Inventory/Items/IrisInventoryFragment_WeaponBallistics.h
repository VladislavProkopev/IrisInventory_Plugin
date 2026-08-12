// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "IrisInventoryItemFragment.h"
#include "IrisInventoryFragment_WeaponBallistics.generated.h"

class UCurveFloat;
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
	TObjectPtr<UCurveFloat> SpreadCurve = nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Trace")
	float MaxRange = 10000.f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Damage")
	float BaseDamage = 10.f;
	
	//Ёмкость магазина. На этапе 6 читается при экипировке в атрибут MagazineSize,
	//по нему клампится AmmoLoaded
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Ammo",meta=(ClampMin="1"))
	int32 MagazineSize = 30;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Visuals")
	FGameplayTag FireCueTag;
};
