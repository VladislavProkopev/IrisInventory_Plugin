// Fill out your copyright notice in the Description page of Project Settings.


#include "ADVMMovementComponent.h"

#include "ADVMInputConfig.h"
#include "ADVMInputHelpers.h"
#include "CoreGameplayTags.h"
#include "EnhancedInputComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/AssetManager.h"
#include "InputActionValue.h"

const FName UADVMMovementComponent::NAME_ActorFeatureName = FName("MovementFeature");

// Sets default values for this component's properties
UADVMMovementComponent::UADVMMovementComponent(const FObjectInitializer& Initializer) : Super(Initializer)
{
}

void UADVMMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	//Плагин должен слышать координатора (PawnExtensionComponent), чтобы понять, когда тот перейдет в
	//состояние DataInitialized, поэтому тоже подписываемся на всех.
	BindOnActorInitStateChanged(NAME_None,FGameplayTag(),false);
	
	//Ручной толчок
	//Явно говорим менеджеру: Переведи меня из состояния None в Spawned
	if (UGameFrameworkComponentManager* Manager = UGameFrameworkComponentManager::GetForActor(GetOwningActor()))
	{
		Manager->ChangeFeatureInitState(GetOwningActor(),GetFeatureName(),this,CoreGameplayTags::InitStateTags::InitState_Spawned);
	}
	
	//Стартовый пинок стейт машины для плагина
	CheckDefaultInitialization();
}

FName UADVMMovementComponent::GetFeatureName() const
{
	return NAME_ActorFeatureName;
}

bool UADVMMovementComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	//Разрешаем переход из None в Spawned если есть ожидающая пешка
	if (!CurrentState.IsValid() && DesiredState == CoreGameplayTags::InitStateTags::InitState_Spawned)
	{
		return GetPawn<APawn>() !=nullptr;
	}
	if (DesiredState == CoreGameplayTags::InitStateTags::InitState_DataAvaliable)
	{
		return !InputConfigSoftPtr.IsNull();
	}
	if (DesiredState == CoreGameplayTags::InitStateTags::InitState_DataInitialized)
	{
		//Если не загружен ассет в память или контроллер не доступен запрещаем переход к следующему стейту
		return (LoadedInputConfig != nullptr) && (GetController<APlayerController>() != nullptr);
	}
	
	return true;
	
	
}

void UADVMMovementComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	if (DesiredState == CoreGameplayTags::InitStateTags::InitState_DataAvaliable)
	{
		if (!InputConfigSoftPtr.IsNull() && !LoadedInputConfig)
		{
			RequestInputConfigLoad();
		}
	}
	else if (DesiredState == CoreGameplayTags::InitStateTags::InitState_DataInitialized)
	{
		BindInputActions();
	}
}

void UADVMMovementComponent::RequestInputConfigLoad()
{
	UAssetManager& AssetManager = UAssetManager::Get();
	FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();
	
	//Запрашиваем загрузку и биндим колбек
	InputConfigLoadHandle = StreamableManager.RequestAsyncLoad(
		InputConfigSoftPtr.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this,&ThisClass::OnInputConfigLoaded));
}

void UADVMMovementComponent::BindInputActions()
{
	APawn* Pawn = GetPawn<APawn>();
	if (Pawn && Pawn->IsLocallyControlled())
	{
		
		if (UEnhancedInputComponent* InputComp = Cast<UEnhancedInputComponent>(Pawn->InputComponent))
		{
			FADVMInputHelpers::BindNativeAction(
				InputComp,
				LoadedInputConfig,
				CoreGameplayTags::ADVMInputTags::Input_Action_Move,
				ETriggerEvent::Triggered,
				this,
				&ThisClass::Input_Move,
				InputBindHandles);
			
			FADVMInputHelpers::BindNativeAction(
				InputComp,
				LoadedInputConfig,
				CoreGameplayTags::ADVMInputTags::Input_Action_Look,
				ETriggerEvent::Triggered,
				this,
				&ThisClass::Input_Look,
				InputBindHandles);
			
			FADVMInputHelpers::BindNativeAction(
				InputComp,
				LoadedInputConfig,
				CoreGameplayTags::ADVMInputTags::Input_Action_Jump,
				ETriggerEvent::Triggered,
				this,
				&ThisClass::Input_Jump,
				InputBindHandles);
		}
	}
}

void UADVMMovementComponent::OnInputConfigLoaded()
{
	if (InputConfigLoadHandle.IsValid() && InputConfigLoadHandle->HasLoadCompleted())
	{
		//Кешируем в оперативную память
		LoadedInputConfig = Cast<UADVMInputConfig>(InputConfigLoadHandle->GetLoadedAsset());
		
		//Очищаем хендл
		InputConfigLoadHandle.Reset();
		
		//Форсируем попытку продвинуть стейт машину дальше (Переход в DataInitialized)
		CheckDefaultInitialization();
	}
}

void UADVMMovementComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void UADVMMovementComponent::Input_Look(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y);
	}
}

void UADVMMovementComponent::Input_Jump(const FInputActionValue& InputActionValue)
{
	//TODO Удалить и перенести логику в GAS
	UE_LOG(LogTemp,Log,TEXT("UADVMMovementComponent::Input_Jump()"));
}

void UADVMMovementComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName != GetFeatureName())
	{
		CheckDefaultInitialization();
	}
}

void UADVMMovementComponent::CheckDefaultInitialization()
{
	//Проверяем состояние стейт машины остальных Акторов или компонентов
	CheckDefaultInitializationForImplementers();
	
	//Формируем массив состояний
	static const TArray<FGameplayTag> StateChain = {
		CoreGameplayTags::InitStateTags::InitState_Spawned,
		CoreGameplayTags::InitStateTags::InitState_DataAvaliable,
		CoreGameplayTags::InitStateTags::InitState_DataInitialized,
		CoreGameplayTags::InitStateTags::InitState_GameplayReady };
	
	ContinueInitStateChain(StateChain);
}

void UADVMMovementComponent::OnRegister()
{
	Super::OnRegister();
	//Регистирруем компонент в ComponentManager для участия в InitStates
	RegisterInitStateFeature();
}

void UADVMMovementComponent::OnUnregister()
{
	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (UEnhancedInputComponent* InputComp = Cast<UEnhancedInputComponent>(Pawn->InputComponent))
		{
			FADVMInputHelpers::RemoveBinds(InputComp,InputBindHandles);
		}
	}
	
	UnregisterInitStateFeature();
	
	Super::OnUnregister();
}


