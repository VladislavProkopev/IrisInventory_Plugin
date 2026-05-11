// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IrisInventoryItemFragment.h"
#include "GameplayTagContainer.h"
#include "IrisInventoryFragment_Stats.generated.h"

/**
 * L1 Core: Immutable Stats Data Fragment
 * Contains Const Parameters (MaxStackSize, Weight, BaseDamage)
 */
UCLASS(DefaultToInstanced,EditInlineNew,meta=(DisplayName="Stats Fragment"))
class COREFEATURES_API UIrisInventoryFragment_Stats : public UIrisInventoryItemFragment
{
	GENERATED_BODY()
public:
	//TMap безопасен здесь, так как это Read-Only CDO данные (нет гонок на запись)
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Stats")
	TMap<FGameplayTag,int32> InitialItemStats;
	
	//0(1) Fetch
	int32 GetItemStatByTag(FGameplayTag Tag) const
	{
		if (const int32* StatPtr = InitialItemStats.Find(Tag))
		{
			return *StatPtr;
		}
		return 0;
	}
};
