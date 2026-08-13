#include "IrisInventoryComponent.h"

#include "ItemPickup_Base.h"
#include "TimerManager.h"
#include "CoreFeatures/Public/CoreGameplayTags.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Inventory/Items/IrisInventoryFragment_InstanceState.h"
#include "Inventory/Items/IrisInventoryFragment_Stackable.h"
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

int32 UIrisInventoryComponent::GetItemStatByInstanceID(int32 InstanceID, FGameplayTag StatTag) const
{
	const int32 SlotIndex = FindSlotByInstanceID(InstanceID);
	if (!Inventory.Entries.IsValidIndex(SlotIndex)) return 0;

	return Inventory.Entries[SlotIndex].GetStatValue(StatTag);
}

bool UIrisInventoryComponent::ModifyItemStatByInstanceID(int32 InstanceID, FGameplayTag StatTag, int32 Delta)
{
	if (!HasAuthority() || !StatTag.IsValid()) return false;
	
	//Ищем ОДИН раз и сразу проверяем результат: FindSlotByInstanceID возвращает
	//INDEX_NONE (-1), а Entries[-1] это выход за границы массива
	const int32 SlotIndex = FindSlotByInstanceID(InstanceID);
	if (!Inventory.Entries.IsValidIndex(SlotIndex)) return false;

	FIrisInventoryEntry& Entry = Inventory.Entries[SlotIndex];

	//Состояние экземпляра есть только у уникальных предметов: у стака из 30 стрел
	//нет "своей" прочности. Чаще всего сюда попадают с перепутанным InstanceID
	if (!Entry.ItemDef || !Entry.ItemDef->GetInstanceStateFragment())
	{
		UE_LOG(Log_IrisInventoryComponent,Warning,
			TEXT("[%s] Attempt to write stat to item %s without Instance State Fragment."),
			ANSI_TO_TCHAR(__FUNCTION__),*GetNameSafe(Entry.ItemDef));
		return false;
	}

	Entry.AddStat(StatTag,Delta);
	Inventory.MarkItemDirty(Entry);

	return true;
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
	
	//Зажимаем по фактическому размеру стака: RemoveEntryByID при CountToRemove >= StackCount
	//снимает весь стак, сколько есть, а в пикап уходил запрошенный CountToDrop -
	//на земле оказывалось больше, чем было в инвентаре
	const int32 ActualDropCount = FMath::Min(CountToDrop,Inventory.Entries[SlotIndex].StackCount);
	
	//Политику спрашиваем здесь же - см. раздел 4
	if (!CanRemoveItem(InstanceID,ActualDropCount)) return;
	
	//2. Уничтожаем данные в памяти (GC-Free)
	//метод вернет true или false, если транзакция удалась. Для простоты опустим чек
	Inventory.RemoveEntryByID(InstanceID,ActualDropCount);
	
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
		SpawnedPickup->InitializePickup(ItemDefToDrop,ActualDropCount);
		
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
	if (const UIrisInventoryFragment_Stats* StatsFrag = ItemDef->GetStatsFragment())
	{
		return StatsFrag->GetItemStatByTag(CoreGameplayTags::InventoryTags::Item_Stat_Weight);
	}
	
	return 0; //Если статов нет, предмет ничего не весит
}

void UIrisInventoryComponent::QueueItemsForGrant(const UIrisInventoryItemDefinition* ItemDef, int32 Count)
{
	if (!ItemDef || Count <= 0 || !GetOwner()->HasAuthority()) return;

	GrantQueue.Enqueue({ItemDef, Count});

	// Запускаем таймер дозатора (20 FPS), если он спит
	if (!GetWorld()->GetTimerManager().IsTimerActive(GrantQueueTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			GrantQueueTimerHandle, this, &UIrisInventoryComponent::ProcessGrantQueue, 0.05f, true);
	}
}

void UIrisInventoryComponent::ProcessGrantQueue()
{
	int32 ProcessedCount = 0;
	FIrisPendingGrant Request;

	while (ProcessedCount < MaxGrantsPerTick && GrantQueue.Dequeue(Request))
	{
		Inventory.AddEntry_Batched(Request.ItemDef, Request.Count);
		ProcessedCount++;
	}

	if (GrantQueue.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(GrantQueueTimerHandle);
	}
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
			//------------------------------------------------------------------------------------------------------------------------
			/*TODO Refactor Coment
			//Проверяем фрагмент статов на наличие тега(например, Item_Type_Consumable)
			if (const UIrisInventoryFragment_Stats* StatsFrag = Entry.ItemDef->FindFragmentByClass<UIrisInventoryFragment_Stats>())
			{
				if (StatsFrag->InitialItemStats.Contains(ItemTag))
				{
					TotalCount+=Entry.StackCount;
				}
			}
			*/
			//------------------------------------------------------------------------------------------------------------------------
			
			//Классификация лежит в ItemTags, а не в карте статов.
			//Заодно уходит FindFragmentByClass из цикла: метод зовётся из GAS CheckCost,
			//потенциально каждый кадр при зажатой кнопке
			if (Entry.ItemDef->ItemTags.HasTagExact(ItemTag))
			{
				TotalCount+=Entry.StackCount;
			}
		}
	}
	return TotalCount;
}

int32 UIrisInventoryComponent::ConsumeItemByTag(FGameplayTag ItemTag, int32 CountToConsume)
{
	if (!HasAuthority() || CountToConsume<=0) return 0;
	
	int32 RemainingToConsume = CountToConsume;
	int32 ActuallyConsumed = 0;
	
	//Идем с конца, так как RemoveAtSwap меняет индексы, если мы будем удалять предметы
	//Обратный цикл - стандартный паттерн безопасной итерации с удалением
	for (int32 i = Inventory.Entries.Num() - 1; i>=0;--i)
	{
		FIrisInventoryEntry& Entry = Inventory.Entries[i];
		
		if (!Entry.IsValid()) continue;
		if (!Entry.ItemDef->ItemTags.HasTagExact(ItemTag)) continue;
		
		const int32 ConsumeFromStack = FMath::Min(RemainingToConsume,Entry.StackCount);
		if (!CanRemoveItem(Entry.InstanceID,ConsumeFromStack)) continue;
		
		Inventory.RemoveEntryByID(Entry.InstanceID,ConsumeFromStack);
		
		RemainingToConsume -= ConsumeFromStack;
		ActuallyConsumed += ConsumeFromStack;
		
		if (RemainingToConsume <=0) break;
	}
	return ActuallyConsumed;
}

int32 UIrisInventoryComponent::GetMaxStackSize(const UIrisInventoryItemDefinition* ItemDef) const
{
	//-----------------------------------------------------------------------------------------------------------------------
	/*TODO RefactorComment
	if (const UIrisInventoryFragment_Stats* StatsFrag = ItemDef->FindFragmentByClass<UIrisInventoryFragment_Stats>())
	{
		return FMath::Max(1,StatsFrag->GetItemStatByTag(CoreGameplayTags::InventoryTags::Item_Stat_MaxStackSize));
	}
	return 1;
	*/
	//-----------------------------------------------------------------------------------------------------------------------
	
	if (!ItemDef) return 1;
	
	//Единственный источник истины - Stackable-фрагмент. Тег Item.Stat.MaxStackSize
	//из Stats-фрагмента больше не читаем: два источника расходились между
	//GetMaxStackSize и AddEntry_Batched/MergeEntries
	if (const UIrisInventoryFragment_Stackable* StackFrag = ItemDef->GetStackableFragment())
	{
		return FMath::Max(1,StackFrag->MaxStackSize);
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
	
	//---------------------------------------------------------------------------------------------
	/*TODO Refactor Commit
	Result.ActuallyAdded = AllowedCount;
	Result.RejectedCount = CountToAdd - AllowedCount;

	//Если что-то не влезло - передаем в сетевой стейт L2
	if (AllowedCount > 0)
	{
		Inventory.CreateNewEntry(ItemDef,AllowedCount);
	}
	*/
	//Компонент больше не занимается спавном Drop-акторов
	//Он просто возвращает чек
	//return Result;
	//---------------------------------------------------------------------------------------------
	
	

	//Раскладку по стакам делает L2: он уважает MaxStackSize и доливает в неполные.
	//CreateNewEntry здесь нельзя - он свалит весь AllowedCount в одну запись
	if (AllowedCount > 0)
	{
		Result = Inventory.AddEntry_Batched(ItemDef,AllowedCount);
	}
	
	//Восстанавливаем внешний контекст запроса: L2 знает только про количество,
	//разрешённое политикой, и посчитал бы RejectedCount от него
	Result.RequestedCount = CountToAdd;
	Result.RejectedCount = CountToAdd - Result.ActuallyAdded;

	/*
	if (!ItemDef || CountToAdd <= 0 || HasAuthority()) return;
	
	TODO Добавить проверку веса и допустимого лимита перед добавлением и продумать универсальную логику,
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

	return Result;
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
	
	//TODO Refactor Comment
	//float ItemWeight = GetItemWeight(ItemDef);
	const int32 ItemWeight = GetItemWeight(ItemDef);
	if (ItemWeight <= 0) return RequestedCount; //Предмет ничего не весит
	
	const int32 FreeWeight = FMath::Max(0.f,MaxWeight-CurrentWeight);
	const int32 AllowedByWeight = FreeWeight/ItemWeight;
	
	return FMath::Min(RequestedCount,AllowedByWeight);
}

void UIrisInventoryComponent::OnRegister()
{
	Super::OnRegister();
	Inventory.OwnerComponent = this;
	//Inventory.OnListChanged.AddUObject(this,&UIrisInventoryComponent::BroadcastInventoryUpdate);
	RegisterInitStateFeature();
}




