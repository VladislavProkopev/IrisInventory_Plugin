#include "IrisInventoryComponent.h"

#include "ItemPickup_Base.h"
#include "CoreFeatures/Public/CoreGameplayTags.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"
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

void UIrisInventoryComponent::DropItem(int32 InstanceID, int32 CountToDrop)
{
	if (!HasAuthority() || CountToDrop <=0 || !DefaultPickupClass) return;
	
	//1. Ищем что выбрасываем
	int32 SlotIndex = FindSlotByInstanceID(InstanceID);
	if (SlotIndex == INDEX_NONE) return;
	
	const UIrisInventoryItemDefinition* ItemDefToDrop = Inventory.Entries[SlotIndex].ItemDef;
	
	//2. Уничтожаем данные в памяти (GC-Free)
	//метод вернет true или false, если транзакция удалась. Для простоты опустим чек
	Inventory.RemoveEntryByID(InstanceID,CountToDrop);
	
	//3. Материализация в мире
	AActor* OwnerActor = GetOwner();
	//TODO Посмотреть как будет в игре и нужно ли выносить в параметр
	FVector DropLocation = OwnerActor->GetActorLocation() + OwnerActor->GetActorForwardVector() * 100.f;
	FTransform SpawnTransform(OwnerActor->GetActorRotation(),DropLocation);
	
	AItemPickup_Base* SpawnedPickup = GetWorld()->SpawnActorDeferred<AItemPickup_Base>(
		DefaultPickupClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	
	if (SpawnedPickup)
	{
		//Инжектим данные в сосуд до BeginPlay и репликации
		SpawnedPickup->InitializePickup(ItemDefToDrop,CountToDrop);
		
		SpawnedPickup->FinishSpawning(SpawnTransform);
	}
}

bool UIrisInventoryComponent::CanRemoveItem_Implementation(int32 InstanceID, int32 CountToRemove) const
{
	//По умолчанию разрешаем удалять все
	//Дизайнер в BP может добавить проверку тега "Item.Tag.Quest" и вернуть false
	return true;
}

bool UIrisInventoryComponent::CanMergeItems_Implementation(int32 SourceInstanceID, int32 TargetInstanceID) const
{
	//C++ ядро проверит совпадение ItemDef
	//Здесь мы по умолчанию просто разрешаем операцию
	return true;
}

bool UIrisInventoryComponent::CanSplitItem_Implementation(int32 InstanceID) const
{
	//По умолчанию разрешаем делить все
	return true;
}

int32 UIrisInventoryComponent::GetItemWeight(const UIrisInventoryItemDefinition* ItemDef) const
{
	if (!ItemDef) return 0;
	
	//0(1) lock-free чтение из CDO
	if (const UIrisInventoryFragment_Stats* StatsFrag = ItemDef->FindFragmentByClass<UIrisInventoryFragment_Stats>())
	{
		return StatsFrag->GetItemStatByTag(CoreGameplayTags::InventoryTags::Item_Stat_Weight);
	}
	
	return 0; //Если статов нет, предмет ничего не весит
}

void UIrisInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	BindOnActorInitStateChanged(NAME_None,FGameplayTag(),false);
	ensure(TryToChangeInitState(CoreGameplayTags::InitStateTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UIrisInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Inventory.OwnerComponent = nullptr;
	Inventory.OnListChanged.RemoveAll(this);
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

int32 UIrisInventoryComponent::FindSlotByInstanceID(int32 InstanceID) const
{
	if (InstanceID == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	
	for (int32 i=0;i<Inventory.Entries.Num();i++)
	{
		if (Inventory.Entries[i].InstanceID == InstanceID)
		{
			return i;
		}
	}
	
	return INDEX_NONE;
}

bool UIrisInventoryComponent::RemoveItemByInstanceID(int32 InstanceID, int32 CountToRemove)
{
	if (!HasAuthority()) return false;
	
	//Проверяем политику
	if (!CanRemoveItem(InstanceID, CountToRemove)) return false;
	
	int32 SlotIndex = FindSlotByInstanceID(InstanceID);
	if (SlotIndex != INDEX_NONE)
	{
		Inventory.RemoveEntryByID(InstanceID,CountToRemove);
		return true;
	}
	return false;
}

int32 UIrisInventoryComponent::GetTotalItemCountByTag(FGameplayTag ItemTag) const
{
	int32 TotalCount = 0;
	
	//0(n) проход по кешу. Lock-Free, так как мы только читаем
	for (const FIrisInventoryEntry& Entry : Inventory.Entries)
	{
		if (Entry.IsValid())
		{
			//Проверяем фрагмент статов на наличие тега(например, Item_Type_Consumable)
			if (const UIrisInventoryFragment_Stats* StatsFrag = Entry.ItemDef->FindFragmentByClass<UIrisInventoryFragment_Stats>())
			{
				if (StatsFrag->InitialItemStats.Contains(ItemTag))
				{
					TotalCount+=Entry.StackCount;
				}
			}
		}
	}
	return TotalCount;
}

int32 UIrisInventoryComponent::ConsumeItemByTag(FGameplayTag ItemTag, int32 CountToConsume)
{
	if (!HasAuthority() || CountToConsume<=0) return 0;
	
	int32 RemainingToConsume = 0;
	int32 ActuallyConsumed = 0;
	
	//Идем с конца, так как RemoveAtSwap меняет индексы, если мы будем удалять предметы
	//Обратный цикл - стандартный паттерн безопасной итерации с удалением
	for (int32 i = Inventory.Entries.Num() - 1; i>=0;--i)
	{
		FIrisInventoryEntry& Entry = Inventory.Entries[i];
		
		if (Entry.IsValid())
		{
			if (const UIrisInventoryFragment_Stats* StatsFrag = Entry.ItemDef->FindFragmentByClass<UIrisInventoryFragment_Stats>())
			{
				if (StatsFrag->InitialItemStats.Contains(ItemTag))
				{
					int32 ConsumeFromStack = FMath::Min(RemainingToConsume,Entry.StackCount);
					Inventory.RemoveEntryByID(Entry.InstanceID,ConsumeFromStack);
					
					RemainingToConsume -= ConsumeFromStack;
					ActuallyConsumed += ConsumeFromStack;
					
					if (RemainingToConsume <= 0)
					{
						break;
					}
				}
			}
		}
	}
	return ActuallyConsumed;
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
FIrisInventoryAddResult UIrisInventoryComponent::AddEntry(const UIrisInventoryItemDefinition* ItemDef, int32 CountToAdd)
{
	FIrisInventoryAddResult Result;
	Result.RequestedCount = CountToAdd;
	
	if (!HasAuthority() || !ItemDef || CountToAdd <=0) return Result;
	
	//Спрашиваем политику: Сколько можно положить
	int32 AllowedCount = CalculateAllowedAddAmount(ItemDef,CountToAdd);
	
	Result.ActuallyAdded = AllowedCount;
	Result.RejectedCount = CountToAdd - AllowedCount;
	
	//Если что-то не влезло - передаем в сетевой стейт L2
	if (AllowedCount > 0)
	{
		Inventory.CreateNewEntry(ItemDef,AllowedCount);
	}
	
	//Компонент больше не занимается спавном Drop-акторов
	//Он просто возвращает чек
	
	return Result;
	
	
	/*
	if (!ItemDef || CountToAdd <= 0 || HasAuthority()) return;
	
	/*TODO Добавить проверку веса и допустимого лимита перед добавлением и продумать универсальную логику,
	которую будет реализовывать пользователь#1# 
	
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
	}*/
}

// ----------------------------------------------------------------------
// ОПОВЕЩЕНИЯ ДЛЯ L3/L4 (GMR & Делегаты)
// ----------------------------------------------------------------------
void UIrisInventoryComponent::BroadcastInventoryUpdate(const UIrisInventoryItemDefinition* ItemDef, int32 NewCount,EIrisInventoryChangeType ChangeType,int32 InstanceID)
{
	if (!ItemDef) return;

	//Формируем DTO
	FSFInventoryChangeMessage Message;
	Message.ItemDef = ItemDef;
	Message.ChangeType = ChangeType;
	Message.InstanceID = InstanceID;
	Message.NewCount = NewCount;
	
	//0(1) бродкаст в пустоту
	//Используем тег CoreGameplayTags::GMR::Inventory_Message_Updated
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(CoreGameplayTags::GMR::Inventory_Message_Updated,Message);
}

bool UIrisInventoryComponent::MergeStacks(int32 SourceInstanceID, int32 TargetInstanceID)
{
	if (!HasAuthority()) return false;
	
	//Проверяем политику
	if(!CanMergeItems(SourceInstanceID,TargetInstanceID)) return false;
	
	return Inventory.MergeEntries(SourceInstanceID,TargetInstanceID);
}

int32 UIrisInventoryComponent::SplitStack(int32 SourceInstanceID, int32 AmountToSplit)
{
	if (!HasAuthority()) return INDEX_NONE;
	
	//Проверяем политику
	if (!CanSplitItem(SourceInstanceID)) return INDEX_NONE;
	
	return Inventory.SplitEntry(SourceInstanceID,AmountToSplit);
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

int32 UIrisInventoryComponent::CalculateAllowedAddAmount_Implementation(const UIrisInventoryItemDefinition* ItemDef,
	int32 RequestedCount) const
{
	if (!ItemDef || RequestedCount <=0) return 0;
	//TODO Подумать где будем хранить переменные и добавить фрагмент веса
	//Если отключен (MaxWeight <=0), разрешаем взять все
	//Если разработчик переопределит этот метод в Блюпринте и просто воткнет
	//входной RequestCount в Return Node - вес перестанет работать
	if (MaxWeight<=0) return RequestedCount;
	
	float ItemWeight = GetItemWeight(ItemDef);
	if (ItemWeight <= 0) return RequestedCount; //Предмет ничего не весит
	
	int32 FreeWeight = FMath::Max(0.f,MaxWeight-CurrentWeight);
	int32 AllowedByWeight = FreeWeight/ItemWeight;
	
	return FMath::Min(RequestedCount,AllowedByWeight);
}

void UIrisInventoryComponent::OnRegister()
{
	Super::OnRegister();
	Inventory.OwnerComponent = this;
	//Inventory.OnListChanged.AddUObject(this,&UIrisInventoryComponent::BroadcastInventoryUpdate);
	RegisterInitStateFeature();
}




