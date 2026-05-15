// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/Items/IrisInventoryItemDefinition.h"
#include "CC_WeaponBase.generated.h"

UCLASS()
class COMBATCOREGASIRIS_API ACC_WeaponBase : public AActor
{
	GENERATED_BODY()

public:
	ACC_WeaponBase();
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Patameters")
	FName MuzzleSocketName;
	
	UPROPERTY(Transient,BlueprintReadOnly,Category="Patameters")
	TObjectPtr<const UIrisInventoryItemDefinition> WeaponDef;
	
	FTransform GetMuzzleTransform() const;
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleDefaultsOnly,BlueprintReadOnly,Category="Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
};
