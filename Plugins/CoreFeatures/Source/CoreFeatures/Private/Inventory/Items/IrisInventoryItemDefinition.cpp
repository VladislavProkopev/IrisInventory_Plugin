// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreFeatures/Public/Inventory/Items/IrisInventoryItemDefinition.h"
#include "CoreFeatures/Public/Inventory/Items/IrisInventoryFragment_Stats.h"
#include "CoreFeatures/Public/Inventory/Items/IrisInventoryFragment_Stackable.h"
#include "CoreFeatures/Public/Inventory/Items/IrisInventoryFragment_InstanceState.h"


void UIrisInventoryItemDefinition::PostLoad()
{
	Super::PostLoad();
	RebuildFragmentCache();
}

void UIrisInventoryItemDefinition::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	RebuildFragmentCache();
}

#if WITH_EDITOR
void UIrisInventoryItemDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	//Фрагменты правятся инлайном в редакторе. Без перестроения указатели
	//протухнут до перезапуска, и правка ассета не подействует
	RebuildFragmentCache();
}
#endif


void UIrisInventoryItemDefinition::RebuildFragmentCache()
{
	CachedStats     = FindFragmentByClass<UIrisInventoryFragment_Stats>();
	CachedStackable = FindFragmentByClass<UIrisInventoryFragment_Stackable>();
	CachedInstanceState = FindFragmentByClass<UIrisInventoryFragment_InstanceState>();
}
