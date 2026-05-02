// Fill out your copyright notice in the Description page of Project Settings.


#include "ADVMGameFeatureAction_AddMovement.h"
#include "ADVMMovementComponent.h"
#include "Engine/World.h"
#include "Components/GameFrameworkComponentManager.h"

#define LOCTEXT_NAMESPACE "ADVMGameFeatures"

void UADVMGameFeatureAction_AddMovement::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);

	if (!ensureAlways(ContextData.Contains(Context)))
	{
		ContextData.Add(Context,FPerContextData());
	}
}

void UADVMGameFeatureAction_AddMovement::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	if (FPerContextData* ActiveData = ContextData.Find(Context))
	{
		ActiveData->ExtensionRequestHandles.Empty();
		
		for (TWeakObjectPtr<AActor>& ActorPtr : ActiveData->InjectedActors)
		{
			if (AActor* Actor = ActorPtr.Get())
			{
				if (UADVMMovementComponent* Comp = Actor->FindComponentByClass<UADVMMovementComponent>())
				{
					Comp->DestroyComponent();
				}
			}
		}
		ContextData.Remove(Context);
	}
}

void UADVMGameFeatureAction_AddMovement::AddToWorld(const FWorldContext& Context,
	const FGameFeatureStateChangeContext& ChangeContext)
{
	UWorld* World = Context.World();
	UGameInstance* GameInstance = Context.OwningGameInstance;
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);

	if (GameInstance && World && World->IsGameWorld())
	{
		if (UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance))
		{
			if (!TargetActorClass.IsNull())
			{
				TSharedPtr<FComponentRequestHandle> ExtensionHandle = ComponentManager->AddExtensionHandler(
					TargetActorClass.LoadSynchronous(),
					UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(this,&ThisClass::HandleActorExtension,ChangeContext));
				ActiveData.ExtensionRequestHandles.Add(ExtensionHandle);
			}
		}
	}
}

void UADVMGameFeatureAction_AddMovement::HandleActorExtension(AActor* Actor, FName EventName,
	FGameFeatureStateChangeContext ChangeContext)
{
	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		return;
	}
	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_ReceiverAdded)
	{
		FPerContextData* ActiveData = ContextData.Find(ChangeContext);
		if (!ActiveData || !Actor) return;
		
		if (Actor->FindComponentByClass<UADVMMovementComponent>()) return;
		
		UClass* CompClass = MovementComponentClass.LoadSynchronous();
		if (!CompClass) return;
		
		UADVMMovementComponent* NewComp = NewObject<UADVMMovementComponent>(Actor,CompClass);
		NewComp->RegisterComponent();
		
		NewComp->SetIsReplicated(true);
		
		Actor->AddInstanceComponent(NewComp);
		ActiveData->InjectedActors.Add(Actor);
	}
}
