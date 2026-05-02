// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "Components/GameFrameworkComponentManager.h"
#include "ADVMGameFeatureAction_AddMovement.generated.h"

class AActor;
class UADVMMovementComponent;

/**
 * 
 */
UCLASS(MinimalAPI,meta = (DisplayName = "Add ADVM Movement Component"))
class UADVMGameFeatureAction_AddMovement : public UGameFeatureAction
{
	GENERATED_BODY()
public:
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	
	UPROPERTY(EditAnywhere,Category="Injection")
	TSoftClassPtr<AActor> TargetActorClass;
	
	UPROPERTY(EditAnywhere,Category="Injection")
	TSoftClassPtr<UADVMMovementComponent> MovementComponentClass;
	
private:
	struct FPerContextData
	{
		TArray<TSharedPtr<FComponentRequestHandle>> ExtensionRequestHandles;
		TArray<TWeakObjectPtr<AActor>> InjectedActors;
	};
	
	TMap<FGameFeatureStateChangeContext,FPerContextData> ContextData;
	
	void AddToWorld(const FWorldContext& Context,const FGameFeatureStateChangeContext& ChangeContext);
	void HandleActorExtension(AActor* Actor,FName EventName, FGameFeatureStateChangeContext ChangeContext);
};
