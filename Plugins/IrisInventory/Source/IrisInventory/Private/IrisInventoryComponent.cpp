// Fill out your copyright notice in the Description page of Project Settings.


#include "IrisInventoryComponent.h"

#include "Net/UnrealNetwork.h"


UIrisInventoryComponent::UIrisInventoryComponent()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}

void UIrisInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass,Inventory);
}


void UIrisInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

