// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ADVMInputConfig.generated.h"

class UInputAction;
class UObject;
struct FFrame;

USTRUCT(BlueprintType)
struct FADVMInputAction
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,meta=(Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * 
 */
UCLASS(BlueprintType,Const)
class ADVANCEDMOVEMENT_API UADVMInputConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UADVMInputConfig(const FObjectInitializer& InitializerModule);
	
	UFUNCTION(BlueprintCallable,Category="AdvancedMovement|Pawn")
	const UInputAction* FindNativeActionForTag(const FGameplayTag InputTag, bool bLogNotFound = true) const;
	
	UFUNCTION(BlueprintCallable,Category="AdvancedMovement|Pawn")
	const UInputAction* FindAbilityActionForTag(const FGameplayTag InputTag, bool bLogNotFound = true) const;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,meta=(TitleProperty = "Native InputActions"))
	TArray<FADVMInputAction> NativeInputActions;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Meta = (ToolTip = "Ability InputActions"))
	TArray<FADVMInputAction> AbilityInputActions;
};
