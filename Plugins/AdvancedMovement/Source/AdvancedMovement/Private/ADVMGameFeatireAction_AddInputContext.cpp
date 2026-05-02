// Fill out your copyright notice in the Description page of Project Settings.


#include "ADVMGameFeatireAction_AddInputContext.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "GameFeaturesSubsystem.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"


void UADVMGameFeatireAction_AddInputContext::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);
	//TODO Доделать логику
	
	// Подумать над #if ineditoronly
	//Подписываемся на глобальный делегат создания Новых GameInstance(Решает проблему PIE)
	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this,&ThisClass::HandleGameInstanceStart);
	
	//Так как UGameFrameworkComponentManager синглтон мы не можем проверить контекст через GameWorld
	//Так как сервер может крутить контексты разных мирова
	//и нужно итерироваться по всем акторам и опрашивать их о том нужна ли им конкретная фитча
	
	// Итерируемся по всем активным мирам (Client,Server,PIE)
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		// Проверяем, должна ли фитча работать в данном контексте
		if (Context.ShouldApplyToWorldContext(WorldContext))
		{
			if (UGameInstance* GameInstance = WorldContext.OwningGameInstance)
			{
				HandleGameInstanceStart(GameInstance);
			}
		}
	}
	
}

void UADVMGameFeatireAction_AddInputContext::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	
	FWorldDelegates::OnStartGameInstance.Remove(GameInstanceStartHandle);
	
	//Так как UGameFrameworkComponentManager синглтон мы не можем проверить контекст через GameWorld
	//Так как сервер может крутить контексты разных мирова
	//и нужно итерироваться по всем акторам и опрашивать их о том нужна ли им конкретная фитча
	
	// Итерируемся по всем активным мирам (Client,Server,PIE)
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		// Проверяем, должна ли фитча работать в данном контексте
		if (Context.ShouldApplyToWorldContext(WorldContext))
		{
			if (UWorld* World = WorldContext.World())
			{
				//Получаем итератор контроллеров этого мира
				for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
				{
					if (APlayerController* PC = Iterator->Get())
					{
						RemoveInputMappingForPlayer(PC);
					}
				}
			}
		}
	}
	
	//Очищаем хендлы подписок ComponentManager
	ExtensionRequestHandles.Empty();
	
}

void UADVMGameFeatireAction_AddInputContext::HandleControllerExtension(AActor* Actor, FName EventName)
{
	APlayerController* PC = CastChecked<APlayerController>(Actor);
	
	//В момент когда контроллер получает свой LocalPlayer (а значит и сабсистему инпута)
	if (EventName == UGameFrameworkComponentManager::NAME_GameActorReady || EventName == FName("LocalPlayerReady"))
	{
		AddInputMappingForPlayer(PC);
	}
	else if (EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveInputMappingForPlayer(PC);
	}
}

void UADVMGameFeatireAction_AddInputContext::AddInputMappingForPlayer(APlayerController* PC)
{
	if (!PC || !PC->IsLocalController()) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		for (const FADVMInputMapping& Mapping : InputMappings)
		{
			if (!Mapping.InputMapping.IsNull())
			{
				if (UInputMappingContext* LoadedIMC = Mapping.InputMapping.LoadSynchronous())
				{
					Subsystem->AddMappingContext(LoadedIMC,Mapping.Priority);
				}
			}
		}
	}
}

void UADVMGameFeatireAction_AddInputContext::RemoveInputMappingForPlayer(APlayerController* PC)
{
	if (!PC || !PC->IsLocalController()) return;
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		for (const FADVMInputMapping& Mapping : InputMappings)
		{
			if (UInputMappingContext* LoadedIMC = Mapping.InputMapping.Get())
			{
				Subsystem->RemoveMappingContext(LoadedIMC);
			}
		}
	}
}

void UADVMGameFeatireAction_AddInputContext::HandleGameInstanceStart(UGameInstance* GameInstance)
{
	if (UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance))
	{
		TSharedPtr<FComponentRequestHandle> Handle = ComponentManager->AddExtensionHandler(
			APlayerController::StaticClass(),
			UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(this,&ThisClass::HandleControllerExtension));
		
		ExtensionRequestHandles.Add(Handle);
		
		//Отрабатывает случаи когда контроллер заспавнился раньше 
		if (UWorld* World = GameInstance->GetWorld())
		{
			for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				if (APlayerController* PC = Iterator->Get())
				{
					AddInputMappingForPlayer(PC);
				}
			}
		}
	}
}
