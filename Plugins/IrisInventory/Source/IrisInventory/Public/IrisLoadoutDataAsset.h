// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "IrisLoadoutDataAsset.generated.h"

class UIrisInventoryItemDefinition;

/*
 *Переопределение базовых статов предмета при спавне (Tag Stack Pattern)
 *Позволяет изменить дефолтные параметры (Например прочность) для конкретного Loadout
 */

USTRUCT(BlueprintType)
struct FIrisStatOverrideSpec
{
	GENERATED_BODY()
	
	//Тег стата например (Item.Stat.MaxStackSize)
	UPROPERTY(EditDefaultsOnly,Category="Stat")
	FGameplayTag StatTag;
	
	//Значение, которое перекроет дефолтное значение из UIrisInventoryItemDefinition
	UPROPERTY(EditDefaultsOnly,Category="Stat")
	int32 InitialValue = 0;
	
};

USTRUCT(BlueprintType)
struct FIrisLoadoutEntry
{
	GENERATED_BODY()
	
	//Soft-ссылка предотвращает Hard-Load ассетов при загрузке плагина в память
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category="Loadout")
	TSoftObjectPtr<UIrisInventoryItemDefinition> ItemDef;
	
	//Колличество предметов в стаке
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category="Loadout", meta = (ClampMin = "1"))
	int32 StackCount = 1;
	
	//Автоматически экипировать предмет после добавления в инвентарь
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category="Loadout")
	bool bAutoEquip = false;
	
	//Массив кастомных статов для этого конкретного экземпляра
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category="Loadout")
	TArray<FIrisStatOverrideSpec> StatOverrides;
};

/*
 *DataAsset - конфиг, связывающий целевой класс Актора со стартовым набором предметов
 */
UCLASS()
class IRISINVENTORY_API UIrisLoadoutDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	//Класс Актора (Pawn/Character), которому предназначается данный лоадаут
	UPROPERTY(EditDefaultsOnly,Category="Target")
	TSoftClassPtr<AActor> TargetActorClass;

	//Список предметов для выдачи
	UPROPERTY(EditDefaultsOnly,Category="Loadout")
	TArray<FIrisLoadoutEntry> DefaultItems;
};
