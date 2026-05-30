// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "ActiveGameplayEffectHandle.h"
#include "CoreFeatures/Public/Inventory/Items/IrisInventoryItemFragment_Equippable.h"
#include "IrisEquipmentInstance.generated.h"

class UIrisInventoryItemDefinition;
class UAbilitySystemComponent;
class AActor;

/**
 * Транзитный обьект. Существует только пока предмет в руках (экипирован)
 * Реплицируется по сети. Управляет GAS (На сервере) и мешами (Локально)
 */
UCLASS(BlueprintType,Blueprintable)
class IRISINVENTORY_API UIrisEquipmentInstance : public UObject
{
	GENERATED_BODY()
public:
	virtual void OnEquipped();
	virtual void OnUnEquipped();
	
	// --- Инициализация сети ---
	virtual bool IsSupportedForNetworking() const override {return true;}
	virtual UWorld* GetWorld() const override;
	
	// --- GAS: Серверная логика ---
	void GrantEquipmentDef(UAbilitySystemComponent* ASC, const UIrisInventoryItemDefinition* InItemDef);
	void RevokeEquipmentDef();
	
	// --- Визуал: Локальная логика ---
	void SpawnEquipmentDef();
	void DestroyEquipmentDef();
	
	//Геттер для сравнения в менеджере
	const UIrisInventoryItemDefinition* GetItemDef() const {return SourceItemDef;}
	int32 GetInstanceID() const {return SourceInstanceID;}
	
	void SetEquipmentData(const UIrisInventoryItemDefinition* InItemDef,int32 InInstanceID);
	
	//Ссылка на заспавненный меш (Не реплицируется)
	UPROPERTY()
	TObjectPtr<AActor> SpawnedActor;
protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	//Храним Handle, чтобы отменить загрузку, если игрок дропнет оружие до ее завершения
	TSharedPtr<struct FStreamableHandle> GASLoadHandle;
private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;
	
	// КРИТИЧНО: Реплицируем, чтобы клиент знал, чей префаб спавнить
	UPROPERTY(Replicated)
	TObjectPtr<const UIrisInventoryItemDefinition> SourceItemDef;
	
	UPROPERTY(Replicated)
	int32 SourceInstanceID = INDEX_NONE;
	
	//Храним хендлы, чтобы знать, что именно нужно забрать при смене оружия
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
	TArray<FActiveGameplayEffectHandle> GrantedEffectHandles;
	
	void OnGASAssetsLoaded();
};
