// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFeatureAction_AddIrisLoadout.h"
#include "IrisLoadoutDataAsset.h"
#include "IrisEquipmentManagerComponent.h"
#include "CoreFeatures/Public/Inventory/Items/IrisInventoryItemDefinition.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UObject/FastReferenceCollector.h"

void UGameFeatureAction_AddIrisLoadout::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);

	//1. Выделяем или находим изолированные данные под текущий контекст активации
	FPerContextData& ActiveData = ContextData.FindOrAdd(Context);

	//2. Подписываемся на запуск GameInstance для регистрации хуков в мире
	ActiveData.GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(
		this, &ThisClass::HandleGameInstanceStart, static_cast<FGameFeatureStateChangeContext>(Context));

	//3. Если мир уже поднят (Например, фича включена на лету в PIE), обрабатываем имеющиеся GameInstance
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (Context.ShouldApplyToWorldContext(WorldContext) && WorldContext.OwningGameInstance)
		{
			HandleGameInstanceStart(WorldContext.OwningGameInstance, Context);
		}
	}
}

void UGameFeatureAction_AddIrisLoadout::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	if (FPerContextData* ActiveData = ContextData.Find(Context))
	{
		FWorldDelegates::OnStartGameInstance.Remove(ActiveData->GameInstanceStartHandle);

		//RAII-хендлы отписывают GFCM автоматически при очистке
		ActiveData->ExtensionHandles.Empty();
		ContextData.Remove(Context);
	}
}

void UGameFeatureAction_AddIrisLoadout::HandleGameInstanceStart(UGameInstance* GameInstance,
	FGameFeatureStateChangeContext ChangeContext)
{
	UWorld* World = GameInstance->GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	UGameFrameworkComponentManager* GFCM = GameInstance->GetSubsystem<UGameFrameworkComponentManager>();
	FPerContextData* ActiveData = ContextData.Find(ChangeContext);

	if (!GFCM || !ActiveData)
	{
		return;
	}

	for (const TSoftObjectPtr<UIrisLoadoutDataAsset>& ConfigPtr : LoadoutConfigs)
	{
		const UIrisLoadoutDataAsset* Config = ConfigPtr.LoadSynchronous();
		if (!Config || Config->TargetActorClass.IsNull())
		{
			continue;
		}

		UClass* TargetClass = Config->TargetActorClass.LoadSynchronous();
		if (!TargetClass)
		{
			continue;
		}

		TWeakObjectPtr<const UIrisLoadoutDataAsset> WeakConfig = Config;

		TSharedPtr<FComponentRequestHandle> Handle = GFCM->AddExtensionHandler(
			TargetClass,
			UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(
				this,&ThisClass::HandleActorExtension,WeakConfig,ChangeContext));

		ActiveData->ExtensionHandles.Add(Handle);
	}
}

void UGameFeatureAction_AddIrisLoadout::HandleActorExtension(AActor* Actor, FName EventName,
	TWeakObjectPtr<const UIrisLoadoutDataAsset> Config, FGameFeatureStateChangeContext ChangeContext)
{
	if (!Actor || !Config.IsValid())
	{
		return;
	}
	
	//Игнорируем события удаления актора/компонента
	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved ||
		EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		return;
	}
	
	//Реплицируемая экипировка выдается строго на Authority
	if (!Actor->HasAuthority())
	{
		return;
	}
	
	//Проверяем наличие целевого компонента управления экипировкой
	if (!Actor->FindComponentByClass<UIrisEquipmentManagerComponent>())
	{
		return;
	}
	
	//Валидация точки жизненного цикла:
	//Событие NAME_ReceiverAdded генерируется в С++ BeginPlay актора при вызове AddGameplayFrameworkComponentReceiver
	//Если актор уже прошел BeginPlay, используем проверку HasActorBeginPlay()
	if (Actor->HasActorBegunPlay() || EventName == UGameFrameworkComponentManager::NAME_ReceiverAdded)
	{
		ApplyLoadout(Actor, Config.Get());
	}
}

//TODO Провсотреть вариант удаления
void UGameFeatureAction_AddIrisLoadout::OnTargetActorBeginPlay(AActor* Actor,
	FGameFeatureStateChangeContext ChangeContext)
{
}

void UGameFeatureAction_AddIrisLoadout::ApplyLoadout(AActor* Actor, const UIrisLoadoutDataAsset* Config)
{
	UIrisEquipmentManagerComponent* EquipmentManager = Actor->FindComponentByClass<UIrisEquipmentManagerComponent>();
	if (!EquipmentManager) return;
	
	for (const FIrisLoadoutEntry& Entry : Config->DefaultItems)
	{
		const UIrisInventoryItemDefinition* ItemDef =Entry.ItemDef.LoadSynchronous();
		if (!ItemDef) continue;
		
		//Вызов бизнес логики выдачи предметов в Iris-совместимый компонент
		if (Entry.bAutoEquip)
		{
			EquipmentManager->EquipItemByInstance(ItemDef);
		}
		
	}
}
