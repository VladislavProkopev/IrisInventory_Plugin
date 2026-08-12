// Fill out your copyright notice in the Description page of Project Settings.


#include "IrisEquipmentInstance.h"
#include "AbilitySystemComponent.h"
#include "GameFeatureAction_AddIrisLoadout.h"
#include "GameplayAbilitySpec.h"
#include "Engine/AssetManager.h"
#include "CoreFeatures/Public/Inventory/Interfaces/IrisEquipmentMountInterface.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/Items/IrisInventoryItemDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(Log_IrisEquipmentInstance, All, All);

UWorld* UIrisEquipmentInstance::GetWorld() const
{
	//UObject получает доступ к миру через своего владельца (Outer)
	if (const UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}
	return nullptr;
}

void UIrisEquipmentInstance::SetEquipmentData(const UIrisInventoryItemDefinition* InItemDef, int32 InInstanceID)
{
	SourceItemDef = InItemDef;
	SourceInstanceID = InInstanceID;
}

void UIrisEquipmentInstance::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass,SourceItemDef);
	DOREPLIFETIME(ThisClass,SourceInstanceID);
}

void UIrisEquipmentInstance::OnGASAssetsLoaded()
{
	//---------------------------------------------------------------------------------------------------------------
	//TODO Refactor comment
	
	/*
	*Сейчас функция выходит на первой строке всегда, кроме одного случая:
	*если ассеты уже в памяти, стример дёргает делегат **синхронно внутри**
	*`RequestAsyncLoad`, и присваивание `GASLoadHandle = ...` ещё не произошло. Тогда код отрабатывает.
	*То есть поведение зависит от того, грузил ли кто-то эти абилки раньше.
	*На тестах будет «иногда работает» — хуже стабильного отказа.
	 */
	
	//Если предмет сняли или прервали загрузку - прерываемся
	//if (!IsValid(this) || !CachedASC || GASLoadHandle.IsValid()) return;
	//---------------------------------------------------------------------------------------------------------------
	
	//Если предмет сняли - прерываемся.
	//Проверки GASLoadHandle здесь быть не должно: это колбэк самой загрузки, хендл
	//на этот момент всегда валиден (сбрасывается ниже), и условие всегда истинно.
	//Отмену уже покрывает RevokeEquipmentDef - он зовёт CancelHandle и Reset
	if (!IsValid(this) || !CachedASC) return;
	
	const UIrisInventoryItemFragment_Equippable* EquipDef = SourceItemDef->FindFragmentByClass<UIrisInventoryItemFragment_Equippable>();
	if (!EquipDef) return;
	
	//Выдаем абилки (теперь они гарантированно в памяти)
	for (const FEqupmentAbilitySet& AbilitySet : EquipDef->GrantedAbilities)
	{
		if (UClass* AbilityClass = AbilitySet.Ability.Get())
		{
			//TODO продумать выдачу абилок любого уровня (пока только 1й)
			FGameplayAbilitySpec Spec(AbilityClass,1,INDEX_NONE,this);
			Spec.GetDynamicSpecSourceTags().AddTag(AbilitySet.InputTag);
			
			FGameplayAbilitySpecHandle Handle = CachedASC->GiveAbility(Spec);
			GrantedAbilityHandles.Add(Handle);
		}
	}
	
	//Накладываем эффекты
	for (const TSoftClassPtr<UGameplayEffect>& EffectClassPtr : EquipDef->PassiveEffects)
	{
		if (UClass* EffectClass = EffectClassPtr.Get())
		{
			FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
			Context.AddInstigator(CachedASC->GetOwner(),CachedASC->GetOwner());
			Context.AddSourceObject(this);
			
			UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
			
			//TODO Продумать накладывание эффектов любого уровня (пока только 1й)
			FActiveGameplayEffectHandle Handle = CachedASC->ApplyGameplayEffectToSelf(EffectCDO,1.f,Context);
			GrantedEffectHandles.Add(Handle);
		}
	}
	
	GASLoadHandle.Reset();
	
}

void UIrisEquipmentInstance::OnEquipped(){}
void UIrisEquipmentInstance::OnUnEquipped(){}

// ----------------------------------------------------------------------
// GAS (Выполняется СТРОГО на сервере)
// ----------------------------------------------------------------------
void UIrisEquipmentInstance::GrantEquipmentDef(UAbilitySystemComponent* ASC,
                                               const UIrisInventoryItemDefinition* InItemDef)
{
	if (!ASC || !InItemDef || !ASC->GetOwnerActor()->HasAuthority()) return;
	
	CachedASC = ASC;
	SourceItemDef = InItemDef; //Клиенты получат реплицируемое значение
	
	//Достаем фрагмент с правилами GAS из CDO
	const UIrisInventoryItemFragment_Equippable* EquipDef = InItemDef->FindFragmentByClass<UIrisInventoryItemFragment_Equippable>();
	if (!EquipDef)
	{
		UE_LOG(Log_IrisEquipmentInstance, Error,TEXT("[%s] Item %s has no Equippable Fragment!"),ANSI_TO_TCHAR(__FUNCTION__),*InItemDef->GetName());
		return;
	}
	
	//Собираем все пути для асинхронной загрузки
	TArray<FSoftObjectPath> AssetsToLoad;
	
	for (const FEqupmentAbilitySet& AbilitySet : EquipDef->GrantedAbilities)
	{
		if (!AbilitySet.Ability.IsNull())
		{
			AssetsToLoad.AddUnique(AbilitySet.Ability.ToSoftObjectPath());
		}
	}
	
	for (const TSoftClassPtr<UGameplayEffect>& EffectClassPtr : EquipDef->PassiveEffects)
	{
		if (!EffectClassPtr.IsNull())
		{
			AssetsToLoad.AddUnique(EffectClassPtr.ToSoftObjectPath());
		}
	}
	
	//Если грузить нечего выходим
	if (AssetsToLoad.IsEmpty()) return;
	
	//Запускаем пакетную загрузку
	GASLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		AssetsToLoad,
		FStreamableDelegate::CreateUObject(this,&UIrisEquipmentInstance::OnGASAssetsLoaded)
		);
	
}

void UIrisEquipmentInstance::RevokeEquipmentDef()
{
	if (!CachedASC || !CachedASC->GetOwnerActor()->HasAuthority()) return;
	
	//Отменяем загрузку ассетов, если предмет сняли до ее завершения
	if (GASLoadHandle.IsValid() && GASLoadHandle->IsActive())
	{
		GASLoadHandle->CancelHandle();
		GASLoadHandle.Reset();
	}
	
	for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		CachedASC->ClearAbility(Handle);
	}
	GrantedAbilityHandles.Empty();
	
	for (const FActiveGameplayEffectHandle& Handle : GrantedEffectHandles)
	{
		CachedASC->RemoveActiveGameplayEffect(Handle);
	}
	GrantedEffectHandles.Empty();
	
	CachedASC = nullptr;
}

// ----------------------------------------------------------------------
// ВИЗУАЛ (Выполняется ЛОКАЛЬНО на всех машинах через хуки Iris)
// ----------------------------------------------------------------------
void UIrisEquipmentInstance::SpawnEquipmentDef()
{
	if (SpawnedActor) return; //Защита
	if (!SourceItemDef) return; //Ждем репликации от сервера
	
	const UIrisInventoryItemFragment_Equippable* EquipDef = SourceItemDef->FindFragmentByClass<UIrisInventoryItemFragment_Equippable>();
	if (!EquipDef || EquipDef->EquipmentPrefab.IsNull()) return;
	
	UWorld* World = GetWorld();
	
	UActorComponent* ManagerComponent = Cast<UActorComponent>(GetOuter());
	if (!ManagerComponent) return;
	
	AActor* OwningActor = ManagerComponent->GetOwner();
	if (!World || !OwningActor) return;
	
	//TODO поменять на асинхронный
	if (UClass* ActorClass = EquipDef->EquipmentPrefab.LoadSynchronous())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwningActor;
		SpawnParams.Instigator = Cast<APawn>(OwningActor);
		//Меш оружия не должен блокировать спавн
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedActor = World->SpawnActor<AActor>(ActorClass,SpawnParams);
		
		//Аттачим к сокету
		if (SpawnedActor)
		{
			USceneComponent* AttachTarget = nullptr;
			FName SocketName = EquipDef->AttachSocket;
			
			if (OwningActor->Implements<UIrisEquipmentMountInterface>())
			{
				AttachTarget = IIrisEquipmentMountInterface::Execute_GetMountComponentForSocket(OwningActor,SocketName);
			}
			else
			{
				AttachTarget = OwningActor->GetRootComponent();
				UE_LOG(Log_IrisEquipmentInstance,Warning,TEXT("[%s] OwningActor %s dosen't implement IIrisEquipmentMountInterface ot returned null. Attached to Root."),ANSI_TO_TCHAR(__FUNCTION__),*OwningActor->GetName());
				
			}
			
			SpawnedActor->AttachToComponent(AttachTarget,FAttachmentTransformRules::SnapToTargetIncludingScale,SocketName);
			OnEquipped(); //Сигнал для BP (Проиграть звук и т.д)
			
			//Старая реализация не использующая интерфейсы
			/*if (ACharacter* Char = Cast<ACharacter>(OwningActor))
			{
				FName SocketName = EquipDef->AttachSocket;
				SpawnedActor->AttachToComponent(Char->GetMesh(),FAttachmentTransformRules::SnapToTargetIncludingScale,SocketName);
			}
			OnEquipped(); // Сигнал для Blueprint (Проиграть звук и т.д)*/
		}
	}
}

void UIrisEquipmentInstance::DestroyEquipmentDef()
{
	if (SpawnedActor)
	{
		OnUnEquipped(); //Сигнал для Blueprint
		SpawnedActor->Destroy();
		SpawnedActor = nullptr;
	}
}
