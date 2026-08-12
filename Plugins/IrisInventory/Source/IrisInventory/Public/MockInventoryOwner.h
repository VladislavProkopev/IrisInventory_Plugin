// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "CoreFeatures/Public/Inventory/Interfaces/IrisEquipmentMountInterface.h"
#include "MockInventoryOwner.generated.h"

class UIrisInventoryComponent;
class UIrisEquipmentManagerComponent;
class UGameFrameworkComponentManager;

UCLASS(BlueprintType,Blueprintable)
class IRISINVENTORY_API AMockInventoryOwner : public AActor , public IGameFrameworkInitStateInterface, public IIrisEquipmentMountInterface
{
	GENERATED_BODY()

public:
	AMockInventoryOwner();

protected:
	virtual void BeginPlay() override;
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	// --- L2 Components ---
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Test Subject| Components")
	TObjectPtr<UIrisInventoryComponent> InventoryComponent;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="TestSubject| Components")
	TObjectPtr<UIrisEquipmentManagerComponent> EquipmentComponent;
	
	// --- Dummy Visual Hierarchy (Zero Cpu Cost) ---
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="TestSubject| Visuals")
	TObjectPtr<USceneComponent> RootDummy;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="TestSubject| Visuals")
	TObjectPtr<USceneComponent> RightHandSocketDummy;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="TestSubject| Visuals")
	TObjectPtr<USceneComponent> BackSocketDummy;
	
	//~ IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override;
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~End IGameFrameworkInitStateInterface
	
	//~ IIrisEquipmentMountInterface
	virtual USceneComponent* GetMountComponentForSocket_Implementation(FName SocketName) const override;
	//~End IIrisEquipmentMountInterface
};
