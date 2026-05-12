// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "CoreFeatures/Public/Inventory/Items/IrisInventoryItemFragment_Equippable.h"
#include "IrisEquipmentInstance.generated.h"

class UIrisInventoryItemDefinition;
class UAbilitySystemComponent;

/**
 * Транзитный обьект. Существует только пока предмет в руках (экипирован)
 * Управляет жизненым циклом GAS для конкретного оружия/предмета
 */
UCLASS(BlueprintType,Blueprintable)
class IRISINVENTORY_API UIrisEquipmentInstance : public UObject
{
	GENERATED_BODY()
public:
	virtual void OnEquipped();
	virtual void OnUnEquipped();
	
	//TODO посмотреть что нужно сделать с ним
	//Вызывается из EquipmentManager на основе данных из ItemFragment_Equippable
	void GrantEquipmentDef(UAbilitySystemComponent* ASC, const UIrisInventoryItemDefinition* InItemDef);
	
	void RevokeEquipmentDef();
	
	//Ссылка на заспавненный меш
	UPROPERTY()
	TObjectPtr<AActor> SpawnedActor;

	//Геттер для сравнения в менеджере
	const UIrisInventoryItemDefinition* GetItemDef() const {return SourceItemDef;}
	
	void SpawnEquipmentDef();
	void DestroyEquipmentDef();
private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;
	
	UPROPERTY()
	TObjectPtr<const UIrisInventoryItemDefinition> SourceItemDef;
	
	//Храним хендлы, чтобы знать, что именно нужно забрать при смене оружия
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
	TArray<FActiveGameplayEffectHandle> GrantedEffectHandles;
};
