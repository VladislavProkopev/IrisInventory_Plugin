#include "IrisEquipmentManagerComponent.h"
#include "AbilitySystemGlobals.h"
#include "CoreGameplayTags.h"
#include "IrisInventoryComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Net/UnrealNetwork.h"
#include "Logging/LogMacros.h"
#include "IrisInventoryComponent.h"
#include "IrisInventoryTypes.h"


DEFINE_LOG_CATEGORY_STATIC(Log_IrisEquipmentManagerComponent, All, All);
	
const FName UIrisEquipmentManagerComponent::NAME_ActorFeatureName("EquipmentManager");

UIrisEquipmentManagerComponent::UIrisEquipmentManagerComponent(const FObjectInitializer& OI) : Super(OI)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
	
}

void UIrisEquipmentManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	//DI: Привязываем структуру к компоненту
	EquipmentList.OwnerComponent = this;
}

void UIrisEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass,EquipmentList);
}

// ----------------------------------------------------------------------
// МУТАТОРЫ ЭКИПИРОВКИ (Сервер)
// ----------------------------------------------------------------------

UIrisEquipmentInstance* UIrisEquipmentManagerComponent::EquipItem(const UIrisInventoryItemDefinition* ItemDef)
{
	if(!ItemDef || !CachedASC || !GetOwner()->HasAuthority()) return nullptr;
	
	//TODO Дописать логику UnequipItem Если слот уже занят
	
	//Делигируем создание инстанса в L2 (FastArray)
	//В будущем мы сможем читать класс инстанса (TSubclassOf) из ItemDef (из фрагмента)
	//Пока используем базовый класс
	UIrisEquipmentInstance* NewInstance = EquipmentList.AddEntry(UIrisEquipmentInstance::StaticClass());
	
	//Инициализируем UObject данными До того, как он попробует заспавнить меш
	if (NewInstance)
	{
		NewInstance->GrantEquipmentDef(CachedASC,ItemDef);
		NewInstance->SpawnEquipmentDef();
	}
	
	return NewInstance;
}

void UIrisEquipmentManagerComponent::UnequipItem(UIrisEquipmentInstance* ItemInstance)
{
	if (!ItemInstance || !GetOwner()->HasAuthority()) return;
	
	//Делигируем удаление в L2
	//Метод RemoveEntry сам вызовет DestroyEquipmentDef перед очисткой памяти.
	EquipmentList.RemoveEntry(ItemInstance);
}

// ----------------------------------------------------------------------
// ИНИЦИАЛИЗАЦИЯ И GFCM
// ----------------------------------------------------------------------

void UIrisEquipmentManagerComponent::InitializeEquipmentSystem()
{
	//Безопасное кеширование ASC (Теперь он на 100% готов)
	CachedASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	check(CachedASC);
		
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


