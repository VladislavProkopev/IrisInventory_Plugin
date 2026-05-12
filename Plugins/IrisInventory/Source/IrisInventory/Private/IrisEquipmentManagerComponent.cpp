#include "IrisEquipmentManagerComponent.h"

#include "AbilitySystemGlobals.h"
#include "CoreGameplayTags.h"
#include "IrisInventoryComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(Log_IrisEquipmentManagerComponent,All,All);
	
const FName UIrisEquipmentManagerComponent::NAME_ActorFeatureName("EquipmentManager");

UIrisEquipmentManagerComponent::UIrisEquipmentManagerComponent(const FObjectInitializer& OI) : Super(OI)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
	
}

UIrisEquipmentInstance* UIrisEquipmentManagerComponent::EquipItem(const UIrisInventoryItemDefinition* ItemDef)
{
	if (!ItemDef || !CachedASC) return nullptr;
	
	//Ищем фрагмент с правилами экипировки
	const UIrisInventoryItemFragment_Equippable* EquipFrag = ItemDef->FindFragmentByClass<UIrisInventoryItemFragment_Equippable>();
	if (!EquipFrag) return nullptr;
	
	//TODO Доделать логику проверки не занят ли слот и перед экипировкой вызвать UnequipItem
	
	//Инстансируем транзитный контроллер (UObject)
	UIrisEquipmentInstance* NewInstance = NewObject<UIrisEquipmentInstance>(this);
	
	//Спавним визуальную часть (Оружие) если она есть
	if (!EquipFrag->EquipmentPrefab.IsNull())
	{
		//TODO переделать на асинхронную загрузку позже
		if (UClass* ActorClass = EquipFrag->EquipmentPrefab.LoadSynchronous())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = Cast<APawn>(GetOwner());
			
			NewInstance->GrantEquipmentDef(CachedASC,ItemDef);
			
			//TODO Добавить логику аттача к сокету
		}
	}
	
	//Инжектим GAS (абилки, эффекты)
	NewInstance->GrantEquipmentDef(CachedASC,ItemDef);
	
	//Сохраняем стейт
	ActiveEquipment.Add(NewInstance);
	
	return NewInstance;
}

void UIrisEquipmentManagerComponent::UnequipItem(UIrisEquipmentInstance* ItemInstance)
{
	if (!ItemInstance || !ActiveEquipment.Contains(ItemInstance)) return;
	
	//Отвязываем GAS (забираем абилки, снимаем баффы)
	ItemInstance->RevokeEquipmentDef();
	
	//Уничтожаем визуальную часть (Actor)
	if (ItemInstance->SpawnedActor)
	{
		ItemInstance->SpawnedActor->Destroy();
		ItemInstance->SpawnedActor = nullptr;
	}
	
	//Убираем из трекинга (GC очистит UObject в следующем цикле)
	ActiveEquipment.Remove(ItemInstance);
}

void UIrisEquipmentManagerComponent::InitializeEquipmentSystem()
{
	//Безопасное кеширование ASC (Теперь он на 100% готов)
	CachedASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	check(CachedASC);
	
	//Подписка на ивенты инвентаря
	UIrisInventoryComponent* InventoryComponent = GetOwner()->FindComponentByClass<UIrisInventoryComponent>();
	if (InventoryComponent)
	{
		//Делегат сработает если предмет пропал (уничтожен/выброшен/продан)
		InventoryComponent->OnItemRemoved.AddDynamic(this,&ThisClass::HandleItemRemovedFromInventory);
	}
	//Оповкщаем GFCM, что EquipmentManager готов к геймплею
	UGameFrameworkComponentManager* GFCM = UGameFrameworkComponentManager::GetForActor(GetOwner());
	if (GFCM)
	{
		GFCM->ChangeFeatureInitState(
			GetOwner(),
			UIrisEquipmentManagerComponent::NAME_ActorFeatureName,
			this,
			CoreGameplayTags::InitStateTags::InitState_GameplayReady);
		
	}
}

void UIrisEquipmentManagerComponent::HandleItemRemovedFromInventory(const UIrisInventoryItemDefinition* RemovedItemDef)
{
	//Логика защиты: если удаленный предмет сейчас в руках - снимаем его
	for (UIrisEquipmentInstance* ItemInstance : ActiveEquipment)
	{
		if (ItemInstance && ItemInstance->GetItemDef() == RemovedItemDef)
		{
			UnequipItem(ItemInstance);
			break;
		}
	}
}

void UIrisEquipmentManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

FName UIrisEquipmentManagerComponent::GetFeatureName() const
{
	return NAME_ActorFeatureName;
}

bool UIrisEquipmentManagerComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);
	
	if (!CurrentState.IsValid() && DesiredState == CoreGameplayTags::InitStateTags::InitState_DataAvaliable)
	{
		return true;
	}
	if (CurrentState == CoreGameplayTags::InitStateTags::InitState_Spawned && DesiredState == CoreGameplayTags::InitStateTags::InitState_DataAvaliable)
	{
		//Проверяем существует ли компонент инвентаря на акторе
		UIrisInventoryComponent* InventoryComponent = GetOwner()->FindComponentByClass<UIrisInventoryComponent>();
		if (!InventoryComponent)
		{
			UE_LOG(Log_IrisEquipmentManagerComponent,Error,TEXT("[%s] Missing UIrisInventoryComponent on Actor %s. Ensure GameFeatures injects BOTH components."),ANSI_TO_TCHAR(__FUNCTION__),*GetNameSafe(GetOwner()));
			return false;
		}
		return Manager->HasFeatureReachedInitState(GetOwner(),UIrisInventoryComponent::NAME_ActorFeatureName,CoreGameplayTags::InitStateTags::InitState_DataAvaliable);
		
	}
	return true;
}

void UIrisEquipmentManagerComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (DesiredState == CoreGameplayTags::InitStateTags::InitState_DataAvaliable)
	{
		//В этом стейте базопастно кешировать инвентарь, ASC начинает слушать ивенты экипировки
		InitializeEquipmentSystem();
	}
}

void UIrisEquipmentManagerComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	//TODO
}

void UIrisEquipmentManagerComponent::CheckDefaultInitialization()
{
	//TODO
}

void UIrisEquipmentManagerComponent::OnRegister()
{
	Super::OnRegister();
	
	RegisterInitStateFeature();
}


