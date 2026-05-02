// Fill out your copyright notice in the Description page of Project Settings.


#include "PawnExtensionComponent.h"
#include "CoreGameplayTags.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"

const FName UPawnExtensionComponent::NAME_ActorFeatureName = FName("PawnExtension");

UPawnExtensionComponent::UPawnExtensionComponent(const FObjectInitializer& Initializer) : Super(Initializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

}


void UPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	//Подписываемся на изменение стейтов ВСЕХ компонентов на этом акторе (NAME_None)
	//Без этого OnActorInitStateChanged() никогда не вызовется!
	BindOnActorInitStateChanged(NAME_None,FGameplayTag(),false);
	
	//Ручной толчок
	//Явно говорим менеджеру: Переведи меня из состояния None в Spawned
	if (UGameFrameworkComponentManager* Manager = UGameFrameworkComponentManager::GetForActor(GetOwningActor()))
	{
		Manager->ChangeFeatureInitState(GetOwningActor(),GetFeatureName(),this,CoreGameplayTags::InitStateTags::InitState_Spawned);
	}
	
	//Стартовый пинок машины для перехода из Unknown в InitState_Spawned
	CheckDefaultInitialization();
}

FName UPawnExtensionComponent::GetFeatureName() const
{
	return NAME_ActorFeatureName;
}

bool UPawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	check(Manager);
    
	//Разрешаем переход из None в Spawned если есть ожидающая пешка
	if (!CurrentState.IsValid() && DesiredState == CoreGameplayTags::InitStateTags::InitState_Spawned)
	{
		return GetPawn<APawn>() !=nullptr;
	}
	
	// ... логика DataAvaliable / DataInitialized ...

	if (DesiredState == CoreGameplayTags::InitStateTags::InitState_GameplayReady)
	{
		// Хаб ждет, пока ВСЕ зарегистрированные компоненты на этом Экторе
		// (Movement, Health, Weapons и т.д.) не дойдут до DataInitialized.
		// O(n) проверка, но выполняется редко и гарантирует thread-safety.
		return Manager->HaveAllFeaturesReachedInitState(GetPawn<APawn>(), CoreGameplayTags::InitStateTags::InitState_DataInitialized);
	}
    
	return true;
}

void UPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	IGameFrameworkInitStateInterface::HandleChangeInitState(Manager, CurrentState, DesiredState);
}

void UPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	IGameFrameworkInitStateInterface::OnActorInitStateChanged(Params);

	// Если стейт изменил другой компонент (например, ADVMMovement перешел в DataInitialized),
	// мы снова проверяем свои условия.
	if (Params.FeatureName != GetFeatureName())
	{
		CheckDefaultInitialization();
	}
}

void UPawnExtensionComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = {
		CoreGameplayTags::InitStateTags::InitState_Spawned,
		CoreGameplayTags::InitStateTags::InitState_DataAvaliable,
		CoreGameplayTags::InitStateTags::InitState_DataInitialized,
		CoreGameplayTags::InitStateTags::InitState_GameplayReady };
    
	ContinueInitStateChain(StateChain);
}

void UPawnExtensionComponent::OnRegister()
{
	Super::OnRegister();
	RegisterInitStateFeature();
}

void UPawnExtensionComponent::OnUnregister()
{
	UnregisterInitStateFeature();
	Super::OnUnregister();
}



