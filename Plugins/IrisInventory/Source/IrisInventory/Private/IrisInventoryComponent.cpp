// Fill out your copyright notice in the Description page of Project Settings.


#include "IrisInventoryComponent.h"
#include "Net/UnrealNetwork.h"


UIrisInventoryComponent::UIrisInventoryComponent(const FObjectInitializer& InitializerModule) : Super(InitializerModule)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

int32 UIrisInventoryComponent::GetItemStat(int32 SlotIndex, FGameplayTag StatTag) const
{
	if (Inventory.Entries.IsValidIndex(SlotIndex))
	{
		return Inventory.Entries[SlotIndex].GetStatValue(StatTag);
	}
	return 0;
}

void UIrisInventoryComponent::ModifyItemStat(int32 SlotIndex, FGameplayTag StatTag, int32 Delta)
{
	if (!GetOwner()->HasAuthority() || !Inventory.Entries.IsValidIndex(SlotIndex)) return;
	
	FIrisInventoryEntry& Entry = Inventory.Entries[SlotIndex];
	
	for (FIrisInventoryStatValue& Stat : Entry.DynamicStats)
	{
		if (Stat.StatTag == StatTag)
		{
			Stat.Value += Delta;
			
			Inventory.MarkItemDirty(Entry);
			return;
		}
	}
}

const UIrisInventoryItemDefinition* UIrisInventoryComponent::GetItemDefAtSlot(int32 SlotIndex) const
{
	if (Inventory.Entries.IsValidIndex(SlotIndex))
	{
		return Inventory.Entries[SlotIndex].ItemDef;
	}
	return nullptr;
}

void UIrisInventoryComponent::AddItemDefinition(const UIrisInventoryItemDefinition* ItemDef, int32 Count)
{
	if (!ItemDef || Count <-0 || !GetOwner()->HasAuthority()) return;
	
	//TODO: Здесь должна быть логика стака
	// Для примера создаем новую запись
	
	FIrisInventoryEntry NewEntry;
	NewEntry.ItemDef = ItemDef;
	NewEntry.StackCount = Count;
	
	// Если это оружие, инициализируем базовые динамические статы
	// NewEntry.DynamicStats.Add({ TAG_Weapon_Ammo, 30 });
	
	int32 Index = Inventory.Entries.Add(NewEntry);
	
	// ТРИГГЕР IRIS: Мы говорим движку, что конкретно этот элемент массива изменился.
	// Iris мгновенно соберет дельту и отправит клиентам без Legacy Polling'а.
	Inventory.MarkItemDirty(Inventory.Entries[Index]);
}

void UIrisInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass,Inventory);
}


void UIrisInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

