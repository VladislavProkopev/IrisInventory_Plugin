// Fill out your copyright notice in the Description page of Project Settings.
#include "CC_WeaponBase.h"


ACC_WeaponBase::ACC_WeaponBase()
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;
}

FTransform ACC_WeaponBase::GetMuzzleTransform() const
{
	return WeaponMesh->GetSocketTransform(MuzzleSocketName);
}

void ACC_WeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
}


