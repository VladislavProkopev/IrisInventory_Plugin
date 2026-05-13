#include "IrisInventoryComponent.h"
#include "CoreFeatures/Public/CoreGameplayTags.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Inventory/Items/IrisInventoryFragment_Stats.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(Log_IrisInventoryComponent,All,All)

//TODO ПРОВЕРИТЬ ВО ВСЕХ CANCHANGEINITSTATE потому что после рефактора мог изменить имя а там используется старое и логика будет сломана
const FName UIrisInventoryComponent::NAME_ActorFeatureName("IrisInventory");

UIrisInventoryComponent::UIrisInventoryComponent(const FObjectInitializer& OI) : Super(OI)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UIrisInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass,Inventory);
}

void UIrisInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
		
	Inventory.OnListChanged.AddUObject(this,&UIrisInventoryComponent::BroadcastInventoryUpdate);
	
	BindOnActorInitStateChanged(NAME_None,FGameplayTag(),false);
	ensure(TryToChangeInitState(CoreGameplayTags::InitStateTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UIrisInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

// ----------------------------------------------------------------------
// ИНТЕРФЕЙС ИНВЕНТАРЯ (ЧТЕНИЕ И МУТАЦИЯ)
// ----------------------------------------------------------------------
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
	Entry.AddStat(StatTag,Delta); //Используем метод из структуры
	
	Inventory.MarkItemDirty(Entry);
}

const UIrisInventoryItemDefinition* UIrisInventoryComponent::GetItemDefAtSlot(int32 SlotIndex) const
{
	if (Inventory.Entries.IsValidIndex(SlotIndex))
	{
		return Inventory.Entries[SlotIndex].ItemDef.Get();
	}
	return nullptr;
}

int32 UIrisInventoryComponent::GetMaxStackSize(const UIrisInventoryItemDefinition* ItemDef) const
{
	if (const UIrisInventoryFragment_Stats* StatsFrag = ItemDef->FindFragmentByClass<UIrisInventoryFragment_Stats>())
	{
		return FMath::Max(1,StatsFrag->GetItemStatByTag(CoreGameplayTags::InventoryTags::Item_Stat_MaxStackSize));
	}
	return 1;
}

// ----------------------------------------------------------------------
// ДОБАВЛЕНИЕ ЛУТА
// ----------------------------------------------------------------------
void UIrisInventoryComponent::AddEntry(const UIrisInventoryItemDefinition* ItemDef, int32 CountToAdd)
{
	if (!ItemDef || CountToAdd <= 0 || HasAuthority()) return;
	
	const int32 MaxStackSize = GetMaxStackSize(ItemDef);
	
	for (FIrisInventoryEntry& Entry : Inventory.Entries)
	{
		if (Entry.ItemDef == ItemDef && Entry.StackCount < MaxStackSize)
		{
			const int32 AmmountToFill = FMath::Min(CountToAdd,MaxStackSize - Entry.StackCount);
			Inventory.AddAmountToEntry(Entry,AmmountToFill);
		
			CountToAdd -= AmmountToFill;
			if (CountToAdd <=0) return;
		}
	}

	while (CountToAdd > 0)
	{
		const int32 AmmountToFill = FMath::Min(CountToAdd,MaxStackSize);
		Inventory.CreateNewEntry(ItemDef,AmmountToFill);
		CountToAdd -= AmmountToFill;
	}
}

// ----------------------------------------------------------------------
// ОПОВЕЩЕНИЯ ДЛЯ L3/L4 (GMR & Делегаты)
// ----------------------------------------------------------------------
void UIrisInventoryComponent::BroadcastInventoryUpdate(const UIrisInventoryItemDefinition* ItemDef, int32 NewCount,EIrisInventoryChangeType ChangeType)
{
	if (!ItemDef) return;

	switch (ChangeType)
	{
	case EIrisInventoryChangeType::Added :
		OnItemAdded.Broadcast(ItemDef,NewCount);
		break;
	case EIrisInventoryChangeType::Removed :
		OnItemRemoved.Broadcast(ItemDef);
		break;
	case EIrisInventoryChangeType::Updated :
		OnItemUpdated.Broadcast(ItemDef,NewCount);
		break;
	}
	
	//TODO в будущем здесь будет бродкаст в GameplayMessageRouter для обновления UI
}

// ----------------------------------------------------------------------
// GAME FEATURES INIT STATE
// ----------------------------------------------------------------------
FName UIrisInventoryComponent::GetFeatureName() const
{
	return NAME_ActorFeatureName;
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
	if (CurrentState == CoreGameplayTags::InitStateTags::InitState_DataInitialized){} //Добавить логику если будет необходимо
}

void UIrisInventoryComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
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




