// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "Components/GameFrameworkComponentManager.h"
#include "UObject/SoftObjectPtr.h"
#include "GameFeatureAction_AddIrisLoadout.generated.h"

class UIrisLoadoutDataAsset;

/**
 * GameFeatureAction: Отвечает за сервероориентированную инжекцию стартового лута
 * в UIrisEquipmentManagerComponent
 */
UCLASS(MinimalAPI,meta=(DisplayName = "Add Iris Loadout"))
class UGameFeatureAction_AddIrisLoadout : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	//Список конфигураций лоадаута, применяемый этой фитчей
	UPROPERTY(EditAnywhere,Category="Loadout")
	TArray<TSoftObjectPtr<UIrisLoadoutDataAsset>> LoadoutConfigs;

	//~ UGameFeatureAction interface
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~ End UGameFeatureAction interface

private:
	/*
	 *Контейнер состояния для одного конкретного мира/контекста (PIE-safe)
	 *Предотвращает утечки и перекрестное загрязнение данных между клиентами и серверами в редакторе
	 */
	struct FPerContextData
	{
		FDelegateHandle GameInstanceStartHandle;
		TArray<TSharedPtr<FComponentRequestHandle>> ExtensionHandles;
		//TMap<TWeakObjectPtr<AActor>,TWeakObjectPtr<const UIrisLoadoutDataAsset>> PendingActors;
	};

	//Изолированное хранилище данных под каждый контекст выполнения
	TMap<FGameFeatureStateChangeContext,FPerContextData> ContextData;

	//Обработчик старта GameInstance
	void HandleGameInstanceStart(UGameInstance* GameInstance,FGameFeatureStateChangeContext ChangeContext);

	//Коллбэк от UGameFrameworkComponentManager при спавне целевого Актора
	void HandleActorExtension(AActor* Actor,FName EventName,TWeakObjectPtr<const UIrisLoadoutDataAsset> Config, FGameFeatureStateChangeContext ChangeContext);

	//Ожидание завершения BeginPlay актора перед выдачей лута
	void OnTargetActorBeginPlay(AActor* Actor, FGameFeatureStateChangeContext ChangeContext);

	//Выполнение вызова метода выдачи на Authority
	void ApplyLoadout(AActor* Actor,const UIrisLoadoutDataAsset* Config);
};
