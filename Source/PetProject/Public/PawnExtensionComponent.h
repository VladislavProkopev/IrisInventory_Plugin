// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "PawnExtensionComponent.generated.h"

//#define UE_API PETPROJECT_API

UCLASS(MinimalAPI)
class UPawnExtensionComponent : public UPawnComponent , public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPawnExtensionComponent(const FObjectInitializer& Initializer);
	static const FName NAME_ActorFeatureName;
protected:
	virtual void BeginPlay() override;

	//~ IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override;
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface
	
	UFUNCTION(BlueprintPure, Category = "Prpject|PawnExtensionComponent")
	static UPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor){return Actor?Actor->FindComponentByClass<UPawnExtensionComponent>():nullptr;}
	
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
};
