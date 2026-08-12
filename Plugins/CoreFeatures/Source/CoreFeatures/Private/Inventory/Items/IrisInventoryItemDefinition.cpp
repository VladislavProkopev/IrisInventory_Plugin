// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreFeatures/Public/Inventory/Items/IrisInventoryItemDefinition.h"

void UIrisInventoryItemDefinition::PostLoad()
{
	Super::PostLoad();
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
}
