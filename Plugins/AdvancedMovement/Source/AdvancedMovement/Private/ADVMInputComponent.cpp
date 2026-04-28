// Fill out your copyright notice in the Description page of Project Settings.


#include "ADVMInputComponent.h"


UADVMInputComponent::UADVMInputComponent(const FObjectInitializer& Initializer) : Super(Initializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}

void UADVMInputComponent::AddInputMappings(const UADVMInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);
}

void UADVMInputComponent::RemoveInputMappings(const UADVMInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);
}

void UADVMInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}


