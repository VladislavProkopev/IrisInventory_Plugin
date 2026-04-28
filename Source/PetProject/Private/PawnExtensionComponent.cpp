// Fill out your copyright notice in the Description page of Project Settings.


#include "PawnExtensionComponent.h"
#include "PawnExtCompStatesTags.h"

UPawnExtensionComponent::UPawnExtensionComponent(const FObjectInitializer& Initializer) : Super(Initializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

}


void UPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

FName UPawnExtensionComponent::GetFeatureName() const
{
	return IGameFrameworkInitStateInterface::GetFeatureName();
}

bool UPawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	check(Manager);
	
	//TODO затычка
	return true;
	
	/*APawn* Pawn = GetPawn<APawn>();
	if (!CurrentState.IsValid() && DesiredState == InitStateTags::InitState_Spawned)
	{
		if (Pawn)
		{
			return true;
		}
	}
	if (CurrentState == InitStateTags::InitState_Spawned && DesiredState == InitStateTags::InitState_DataAvaliable)
	{
		if (!PawnData)
	}	*/
}

void UPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	IGameFrameworkInitStateInterface::HandleChangeInitState(Manager, CurrentState, DesiredState);
}

void UPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	IGameFrameworkInitStateInterface::OnActorInitStateChanged(Params);
}

void UPawnExtensionComponent::CheckDefaultInitialization()
{
	IGameFrameworkInitStateInterface::CheckDefaultInitialization();
}


