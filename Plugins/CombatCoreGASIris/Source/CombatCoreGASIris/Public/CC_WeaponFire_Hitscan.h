// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CC_WeaponFire_Hitscan.generated.h"

class ACC_WeaponBase;
class UIrisInventoryFragment_WeaponBallistics;
class UIrisInventoryComponent;
/**
 * 
 */
UCLASS()
class COMBATCOREGASIRIS_API UCC_WeaponFire_Hitscan : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UCC_WeaponFire_Hitscan();
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	virtual void PerformFireMath(ACC_WeaponBase* WeaponActor,const UIrisInventoryFragment_WeaponBallistics* Ballistics);
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
	const UIrisInventoryFragment_WeaponBallistics* CachedBallistics = nullptr;
	TWeakObjectPtr<ACC_WeaponBase> CachedWeaponActor = nullptr;
	const UIrisInventoryComponent* CachedInventoryComponent = nullptr;
};
