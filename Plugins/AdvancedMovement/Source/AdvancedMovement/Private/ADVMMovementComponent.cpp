// Fill out your copyright notice in the Description page of Project Settings.


#include "ADVMMovementComponent.h"

// Sets default values for this component's properties
UADVMMovementComponent::UADVMMovementComponent(const FObjectInitializer& Initializer) : Super(Initializer)
{
}

FName UADVMMovementComponent::GetFeatureName() const
{
	return IGameFrameworkInitStateInterface::GetFeatureName();
}

bool UADVMMovementComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	//if (DesiredState == InitStateTags)
	{
		
	}
	return true;
}

void UADVMMovementComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	IGameFrameworkInitStateInterface::HandleChangeInitState(Manager, CurrentState, DesiredState);
}

void UADVMMovementComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	IGameFrameworkInitStateInterface::OnActorInitStateChanged(Params);
}

void UADVMMovementComponent::CheckDefaultInitialization()
{
	IGameFrameworkInitStateInterface::CheckDefaultInitialization();
}

void UADVMMovementComponent::OnRegister()
{
	Super::OnRegister();
	//Регистирруем компонент в ComponentManager для участия в InitStates
	RegisterInitStateFeature();
}


