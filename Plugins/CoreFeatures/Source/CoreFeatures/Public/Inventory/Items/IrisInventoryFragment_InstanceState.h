// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IrisInventoryItemFragment.h"
#include "GameplayTagContainer.h"
#include "IrisInventoryFragment_InstanceState.generated.h"

/**
 * Начальные значения статов, которые живут на КОНКРЕТНОМ экземпляре и меняются
 * в ходе игры: прочность, заряд, боезапас в обойме.
 *
 * Наличие этого фрагмента объявляет предмет уникальным. Стакуемость и состояние
 * экземпляра взаимно исключают друг друга: "прочность стака из десяти мечей" -
 * вопрос без ответа, а StackCount осмыслен только когда все N единиц равнозначны
 */
UCLASS(DefaultToInstanced,EditInlineNew,meta=(DisplayName="Instance State Fragment"))
class COREFEATURES_API UIrisInventoryFragment_InstanceState : public UIrisInventoryItemFragment
{
	GENERATED_BODY()
public:
	//Копируются в FIrisInventoryEntry::DynamicStats при создании записи
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="State")
	TMap<FGameplayTag,int32> InitialStats;
};