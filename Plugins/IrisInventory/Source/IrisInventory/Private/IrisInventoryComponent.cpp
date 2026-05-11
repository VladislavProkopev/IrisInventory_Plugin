// Fill out your copyright notice in the Description page of Project Settings.


#include "IrisInventoryComponent.h"

#include "CoreGameplayTags.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Inventory/Items/IrisInventoryFragment_Stats.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"

const FName UIrisInventoryComponent::NAME_ActorFeatureName("IrisInventory");

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

FName UIrisInventoryComponent::GetFeatureName() const
{
	return IGameFrameworkInitStateInterface::GetFeatureName();
}

bool UIrisInventoryComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	check(Manager);
	
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return false;

	if (CurrentState == CoreGameplayTags::InitStateTags::InitState_Spawned && DesiredState == CoreGameplayTags::InitStateTags::InitState_DataAvaliable)
	{
		return Manager->HasFeatureReachedInitState(Pawn,FName("PawnExtension"),CoreGameplayTags::InitStateTags::InitState_DataAvaliable);
	}
	if (CurrentState == CoreGameplayTags::InitStateTags::InitState_DataAvaliable && DesiredState == CoreGameplayTags::InitStateTags::InitState_DataInitialized)
	{
		return true;
	}
	if (CurrentState == CoreGameplayTags::InitStateTags::InitState_DataInitialized && DesiredState == CoreGameplayTags::InitStateTags::InitState_GameplayReady)
	{
		return true;
	}
	return false;
}

void UIrisInventoryComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	if (CurrentState == CoreGameplayTags::InitStateTags::InitState_DataInitialized)
	{
		//TODO
		// КРИТИЧЕСКИЙ МОМЕНТ ДЛЯ IRIS:
		// Именно здесь мы должны заспавнить стартовый лут (если он есть).
		// Добавление в массив и MarkItemDirty произойдут ДО перехода в GameplayReady,
		// что гарантирует отсутствие race conditions в NetSerializer.
        
		// Пример: AddItemDefinition(StarterWeaponDef, 1);
	}
}

void UIrisInventoryComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		//Если PawnExtensionComponent поменял стейт, проверяем можем ли мы продвинуться дальше
		CheckDefaultInitialization();
	}
}

void UIrisInventoryComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain{
	CoreGameplayTags::InitStateTags::InitState_Spawned,
	CoreGameplayTags::InitStateTags::InitState_DataAvaliable,
	CoreGameplayTags::InitStateTags::InitState_DataInitialized,
	CoreGameplayTags::InitStateTags::InitState_GameplayReady};
	
	ContinueInitStateChain(StateChain);
}

void UIrisInventoryComponent::OnRegister()
{
	Super::OnRegister();
	
	RegisterInitStateFeature();
}

void UIrisInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

void UIrisInventoryComponent::AddEntry(UIrisInventoryItemDefinition* ItemDef, int32 CountToAdd)
{
	if (!ItemDef || CountToAdd<=0 || !HasAuthority()) return;
	
	//Читаем Data-Driven лимиты из L1
	const int32 MaxStackSize = GetMaxStackSize(ItemDef);
	
	// В будущем: Если CountToAdd огромный или слотов > 1000, 
	// здесь мы вызываем UE::Tasks::Launch(UE_SOURCE_LOCATION, [Snapshot]() { ... });
	// Но для базовой операции делаем синхронно:
	
	//Ищем неполные стаки
	for (FIrisInventoryEntry& Entry: Inventory.Entries)
	{
		if (Entry.ItemDef == ItemDef && Entry.StackCount < MaxStackSize)
		{
			const int32 AmmountToFill = FMath::Min(CountToAdd,MaxStackSize - Entry.StackCount);
			
			//Вызываем атомарную операцию из InventoryTypes->InventoryList
			Inventory.AddAmmountToEntry(Entry,AmmountToFill);
			
			CountToAdd -= AmmountToFill;
			if (CountToAdd <= 0) return;
		}
	}

	while (CountToAdd>0)
	{
		const int32 AmmountToFill = FMath::Min(CountToAdd,MaxStackSize);
		Inventory.CreateNewEntry(ItemDef,AmmountToFill);
		CountToAdd -= AmmountToFill;
	}
}

int32 UIrisInventoryComponent::GetMaxStackSize(UIrisInventoryItemDefinition* ItemDef) const
{
	if (const UIrisInventoryFragment_Stats* StatsFrag = ItemDef->FindFragmentByClass<UIrisInventoryFragment_Stats>())
	{
		return FMath::Max(1,StatsFrag->GetItemStatByTag(CoreGameplayTags::InventoryTags::Item_Stat_MaxStackSize));
	}
	return 1;
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
	
	BindOnActorInitStateChanged(NAME_None,FGameplayTag(),false);
	
	ensure(TryToChangeInitState(CoreGameplayTags::InitStateTags::InitState_Spawned));
	CheckDefaultInitialization();
}

