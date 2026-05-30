// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "IrisInventoryItemFragment.h"
#include "IrisInventoryItemFragment_Equippable.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class AActor;

/**
 * DTO для выдачи абилок через систему Equipment
 * Позволяет биндить абилку к конкретному инпут-тегу при экипировке
 */
USTRUCT(BlueprintType)
struct COREFEATURES_API FEqupmentAbilitySet
{
	GENERATED_BODY()
	
	//Асинхронная загрузка абилки (OOM Protection)
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="GAS")
	TSoftClassPtr<UGameplayAbility> Ability;
	
	//Тег инпута
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="GAS")
	FGameplayTag InputTag;
};

/*
 * L1 Core: Immutable Equippable Fragment
 * Диктует, какие GAS-ассеты и акторы спавнятся при экипировке фрагмента
 */
UCLASS(DefaultToInstanced,EditInlineNew,meta=(DisplayName="Equippable Fragment"))
class COREFEATURES_API UIrisInventoryItemFragment_Equippable : public UIrisInventoryItemFragment
{
	GENERATED_BODY()
public:
	//Представление в мире
	//Спавнится через EquipmentManager и аттачится к сокету
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Visuals")
	TSoftClassPtr<AActor> EquipmentPrefab;
	
	//Имя сокета для аттача
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Visuals")
	FName AttachSocket = FName("WeaponSocket");
	
	//Активные способности
	//Выдаются (GiveAbility) при экипировке, забираются при снятии
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="GAS|Abilities")
	TArray<FEqupmentAbilitySet> GrantedAbilities;
	
	//Пассивные баффы
	//Накладываются (ApplyGameplayEffect) при экипировке, снимаются при снятии экипировки
	TArray<TSoftClassPtr<UGameplayEffect>> PassiveEffects;
	
	//Слот экипировки
	//Использует теги из CoreGameplayTags
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Equip")
	FGameplayTag EquipSlotTag;
};
