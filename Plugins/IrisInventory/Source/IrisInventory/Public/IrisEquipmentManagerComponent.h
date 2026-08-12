// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoreFeatures/Public/Inventory/Items/IrisInventoryItemDefinition.h"
#include "IrisEquipmentInstance.h"
#include "IrisEquipmentTypes.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "IrisEquipmentManagerComponent.generated.h"

class UAbilitySystemComponent;
class UIrisEquipmentInstance;
/*
 * L3 Coordinator: Управляет активной экипировкой (Слотами)
 * Инжектится в Pawn через GameFeatures
 */

UCLASS(BlueprintType,meta = (BlueprintSpawnableComponent))
class IRISINVENTORY_API UIrisEquipmentManagerComponent : public UActorComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	UIrisEquipmentManagerComponent(const FObjectInitializer& OI);

	UFUNCTION(BlueprintCallable,Category="Equipment",BlueprintAuthorityOnly)
	UIrisEquipmentInstance* EquipItemByInstance(const UIrisInventoryItemDefinition* ItemDef);
	
	UFUNCTION(BlueprintCallable,Category="Equipment",BlueprintAuthorityOnly)
	UIrisEquipmentInstance* EquipItemByID(int32 InventoryInstanceID);
	
	UFUNCTION(BlueprintCallable,Category="Equipment",BlueprintAuthorityOnly)
	void UnequipItem();
	
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
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Обязательный хук для репликации UObject-инстансов внутри компонента
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
private:
	//L2 База данных
	UPROPERTY(Replicated)
	FIrisEquipmentList EquipmentList;
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;
	
	void InitializeEquipmentSystem();
	
	UPROPERTY()
	UIrisEquipmentInstance* EquippedItemInstance;
};
