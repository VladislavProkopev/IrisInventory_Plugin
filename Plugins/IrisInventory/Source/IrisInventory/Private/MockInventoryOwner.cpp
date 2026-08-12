// Fill out your copyright notice in the Description page of Project Settings.


#include "MockInventoryOwner.h"

#include "CoreGameplayTags.h"
#include "IrisEquipmentManagerComponent.h"
#include "IrisInventoryComponent.h"
#include "Components/GameFrameworkComponentManager.h"

DEFINE_LOG_CATEGORY_STATIC(Log_MockInventoryOwner, All, All);
// Sets default values
AMockInventoryOwner::AMockInventoryOwner()
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	
	RootDummy = CreateDefaultSubobject<USceneComponent>("RootDummy");
	SetRootComponent(RootDummy);
	
	RightHandSocketDummy = CreateDefaultSubobject<USceneComponent>("RightHandDummy");
	RightHandSocketDummy->SetupAttachment(RootDummy);
	RightHandSocketDummy->SetRelativeLocation(FVector(50.0f, 30.0f, 50.0f));
	
	BackSocketDummy = CreateDefaultSubobject<USceneComponent>("BackSocketDummy");
	BackSocketDummy->SetupAttachment(RootDummy);
	BackSocketDummy->SetRelativeLocation(FVector(-20.0f, 0.0f, 50.0f));
	
	InventoryComponent = CreateDefaultSubobject<UIrisInventoryComponent>("InventoryComponent");
	EquipmentComponent = CreateDefaultSubobject<UIrisEquipmentManagerComponent>("EquipmentComponent");
}

void AMockInventoryOwner::BeginPlay()
{
	Super::BeginPlay();
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this,UGameFrameworkComponentManager::NAME_GameActorReady);
}

void AMockInventoryOwner::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AMockInventoryOwner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

FName AMockInventoryOwner::GetFeatureName() const
{
	return IGameFrameworkInitStateInterface::GetFeatureName();
}

bool AMockInventoryOwner::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	return true;
}

void AMockInventoryOwner::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	//Компоненты отреагируют сами
}

void AMockInventoryOwner::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	UE_LOG(Log_MockInventoryOwner,Warning,TEXT("CurrentState - %s"),*Params.FeatureState.ToString());
}

void AMockInventoryOwner::CheckDefaultInitialization()
{
	UGameFrameworkComponentManager* CompMng = UGameFrameworkComponentManager::GetForActor(this);
	if (CompMng)
	{
		// В тестовой среде форсируем переход сразу в GameplayReady, 
		// чтобы L2 компоненты (Inventory/Equipment) немедленно активировались.
		CompMng->ChangeFeatureInitState(
			this,                                                       // AActor* Actor
			GetFeatureName(),                                           // FName FeatureName
			this,                                                       // UObject* Implementer
			CoreGameplayTags::InitStateTags::InitState_GameplayReady    // FGameplayTag FeatureState
		);
	}
}

USceneComponent* AMockInventoryOwner::GetMountComponentForSocket_Implementation(FName SocketName) const
{
	if (SocketName == TEXT("Weapon_R"))
	{
		return RightHandSocketDummy;
	}
	if (SocketName == TEXT("Backpack"))
	{
		return BackSocketDummy;
	}
	return RootDummy;
}



