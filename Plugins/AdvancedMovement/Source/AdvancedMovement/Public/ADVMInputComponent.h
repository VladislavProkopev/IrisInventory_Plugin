/*
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ADVMInputConfig.h"
#include "EnhancedInputComponent.h"
#include "ADVMInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UObject;

UCLASS(Config=Input)
class ADVANCEDMOVEMENT_API UADVMInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	UADVMInputComponent(const FObjectInitializer& Initializer);
	
	void AddInputMappings(const UADVMInputConfig* InputConfig,UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;
	void RemoveInputMappings(const UADVMInputConfig* InputConfig,UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;
	
	template<class UserClass,typename FuncType>
	void BindNativeAction(const UADVMInputConfig* InputConfig, const FGameplayTag InputTag, ETriggerEvent TriggerEvent,UserClass* Object,FuncType Func,bool bLogNotFound);
	
	template<class UserClass,typename PressedFuncType,typename ReleasedFuncType>
	void BindAbilityActions(const UADVMInputConfig* InputConfig,UserClass* Object,PressedFuncType PressedFunc,ReleasedFuncType ReleasedFunc,TArray<uint32>& BindHandles);
	
	void RemoveBinds(TArray<uint32>& BindHandles);
};

template <class UserClass, typename FuncType>
void UADVMInputComponent::BindNativeAction(const UADVMInputConfig* InputConfig, const FGameplayTag InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogNotFound)
{
	check(InputConfig);
	if (const UInputAction* Action = InputConfig->FindNativeActionForTag(InputTag))
	{
		BindAction(Action,TriggerEvent,Object,Func);
	}
}

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UADVMInputComponent::BindAbilityActions(const UADVMInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);
	for (const FADVMInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction,ETriggerEvent::Triggered,Object,PressedFunc,Action.InputTag).GetHandle());
			}
			if (ReleasedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction,ETriggerEvent::Completed,Object,PressedFunc,Action.InputTag).GetHandle());
			}
		}
	}
}
*/
