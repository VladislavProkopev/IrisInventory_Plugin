// Fill out your copyright notice in the Description page of Project Settings.


#include "ADVMInputConfig.h"

UADVMInputConfig::UADVMInputConfig(const FObjectInitializer& InitializerModule)
{
}

const UInputAction* UADVMInputConfig::FindNativeActionForTag(const FGameplayTag InputTag, bool bLogNotFound) const
{
	for (const FADVMInputAction& Action : NativeInputActions)
	{
		if (Action.InputTag == InputTag && Action.InputAction)
		{
			return Action.InputAction;
		}
	}
	return nullptr;
}

const UInputAction* UADVMInputConfig::FindAbilityActionForTag(const FGameplayTag InputTag, bool bLogNotFound) const
{
	for (const FADVMInputAction& Action : AbilityInputActions)
	{
		if (Action.InputTag == InputTag && Action.InputAction)
		{
			return Action.InputAction;
		}
	}
	return nullptr;
}
