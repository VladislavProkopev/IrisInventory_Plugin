// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"

#include "ADVMMovementComponent.generated.h"

#define UE_API ADVANCEDMOVEMENT_API

struct FStreamableHandle;
class UADVMInputConfig;

UCLASS(MinimalAPI)
class UADVMMovementComponent : public UPawnComponent , public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UADVMMovementComponent(const FObjectInitializer& Initializer);
	
	static UE_API const FName NAME_ActorFeatureName;
	
	//~ IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override;
	UE_API bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	UE_API void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	UE_API void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	UE_API void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface
	
	virtual void OnRegister() override;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category=Input)
	TSoftObjectPtr<UADVMInputConfig> InputConfigSoftPtr;
	
	UPROPERTY(Transient)
	TObjectPtr<UADVMInputConfig> LoadedInputConfig;
	
	TSharedPtr<FStreamableHandle> InputConfigLoadedHandle;
};
