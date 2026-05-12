// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoreFeatures/Public/Inventory/Items/IrisInventoryItemDefinition.h"
#include "IrisEquipmentInstance.h"
#include "IrisEquipmentManagerComponent.generated.h"

class UAbilitySystemComponent;

/*
 * L3 Coordinator: Управляет активной экипировкой и интеграцией с GAS
 * Инжектится в Pawn через GameFeatures
 */

UCLASS(BlueprintType,meta = (BlueprintSpawnableComponent))
class IRISINVENTORY_API UIrisEquipmentManagerComponent : public UActorComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	UIrisEquipmentManagerComponent(const FObjectInitializer& OI);

	UFUNCTION(BlueprintCallable,Category="Equipment",BlueprintAuthorityOnly)
	UIrisEquipmentInstance* EquipItem(const UIrisInventoryItemDefinition* ItemDef);
	
	UFUNCTION(BlueprintCallable,Category="Equipment",BlueprintAuthorityOnly)
	void UnequipItem(UIrisEquipmentInstance* ItemInstance);
	
	static const FName NAME_ActorFeatureName;
		
	//~ IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override;
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface
	
	virtual void OnRegister() override;
protected:
	virtual void BeginPlay() override;
	
private:
	UPROPERTY()
	TArray<TObjectPtr<UIrisEquipmentInstance>> ActiveEquipment;
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;
	
	void InitializeEquipmentSystem();
	
	UFUNCTION()
	void HandleItemRemovedFromInventory(const UIrisInventoryItemDefinition* RemovedItemDef);
};
