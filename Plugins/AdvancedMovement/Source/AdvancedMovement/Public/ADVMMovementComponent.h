// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"

#include "ADVMMovementComponent.generated.h"


struct FInputActionValue;
struct FStreamableHandle;
class UADVMInputConfig;

UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent), ClassGroup=(Custom))
class ADVANCEDMOVEMENT_API UADVMMovementComponent : public UPawnComponent , public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UADVMMovementComponent(const FObjectInitializer& Initializer);
	
	virtual void BeginPlay() override;
	
	static const FName NAME_ActorFeatureName;
	
	//~ IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override;
	bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	
	void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface
	
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category=Input)
	TSoftObjectPtr<UADVMInputConfig> InputConfigSoftPtr;
	
	UPROPERTY(Transient)
	TObjectPtr<UADVMInputConfig> LoadedInputConfig = nullptr;
	
	TSharedPtr<FStreamableHandle> InputConfigLoadHandle;
	
	//////////////////////////////////////////////////////////////////////
	void RequestInputConfigLoad();
	void BindInputActions();
	void OnInputConfigLoaded();
	
protected:
	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_Look(const FInputActionValue& InputActionValue);
	void Input_Jump(const FInputActionValue& InputActionValue);
	
private:
	UPROPERTY(Transient)
	TArray<uint32> InputBindHandles;
};


