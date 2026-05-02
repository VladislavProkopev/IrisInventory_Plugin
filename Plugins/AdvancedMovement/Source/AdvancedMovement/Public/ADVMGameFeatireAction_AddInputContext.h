// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "ADVMGameFeatireAction_AddInputContext.generated.h"

class UInputMappingContext;
struct FComponentRequestHandle;

USTRUCT()
struct FADVMInputMapping
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere,Category="Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;
	
	UPROPERTY(EditAnywhere,Category="Input")
	int32 Priority = 0;
	
};

/**
 * 
 */
UCLASS(MinimalAPI,meta = (DisplayName = "Add ADVM Input Mapping Context"))
class UADVMGameFeatireAction_AddInputContext : public UGameFeatureAction
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere,Category="Input")
	TArray<FADVMInputMapping> InputMappings;
	
	//~ UGameFeaturesAction interface
	
	
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~ End UGameFeaturesAction interface
	
private:
	void HandleControllerExtension(AActor* Actor, FName EventName);
	void AddInputMappingForPlayer(APlayerController* PC);
	void RemoveInputMappingForPlayer(APlayerController* PC);
	
	//Обработчик старта нового инстанса игры (PIE)
	void HandleGameInstanceStart(UGameInstance* GameInstance);
	FDelegateHandle GameInstanceStartHandle;
	
	//Храним хендлы подписок, чтобы корректно отписаться при деактивации фитч 
	TArray<TSharedPtr<FComponentRequestHandle>> ExtensionRequestHandles;
};
