#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EnhancedInputComponent.h"
#include "ADVMInputConfig.h"

struct ADVANCEDMOVEMENT_API FADVMInputHelpers
{
public:
	template<class UserClass, typename FuncType>
	static void BindNativeAction(UEnhancedInputComponent* InputComp, const UADVMInputConfig* InputConfig, const FGameplayTag InputTag, ETriggerEvent TriggerEvent, UserClass* Object,FuncType Func,TArray<uint32>& BindHandles)
	{
		check(InputComp);
		check(InputConfig);

		if (const UInputAction* Action = InputConfig->FindNativeActionForTag(InputTag))
		{
			BindHandles.Add(InputComp->BindAction(Action,TriggerEvent,Object,Func).GetHandle());
		}
	}
	
	template<class UserClass,typename PressedFuncType,typename ReleasedFuncType>
	static void BindAbilityActions(UEnhancedInputComponent* InputComp, const UADVMInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,TArray<uint32>& BindHandles)
	{
		check(InputComp);
		check(InputConfig);
		
		for (const FADVMInputAction& Action : InputConfig->AbilityInputActions)
		{
			if (Action.InputAction && Action.InputTag.IsValid())
			{
				if (PressedFunc)
				{
					BindHandles.Add(InputComp->BindAction(Action.InputAction,ETriggerEvent::Triggered,Object,PressedFunc,Action.InputTag).GetHandle());
				}
				if (ReleasedFunc)
				{
					BindHandles.Add(InputComp->BindAction(Action.InputAction,ETriggerEvent::Completed,Object,PressedFunc,Action.InputTag).GetHandle());
				}
			}
		}
	}
	
	static void RemoveBinds(UEnhancedInputComponent* InputComp,TArray<uint32>& BindHandles)
	{
		check(InputComp);
		
		for (uint32 Handle : BindHandles)
		{
			InputComp->RemoveBindingByHandle(Handle);
		}
		
		BindHandles.Reset();
	}
};
