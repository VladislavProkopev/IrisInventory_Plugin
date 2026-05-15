// Fill out your copyright notice in the Description page of Project Settings.


#include "IrisEquipmentInstance.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameFramework/Character.h"
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
}

void UIrisEquipmentInstance::OnEquipped(){}
void UIrisEquipmentInstance::OnUnEquipped(){}

// ----------------------------------------------------------------------
// GAS (Выполняется СТРОГО на сервере)
// ----------------------------------------------------------------------
void UIrisEquipmentInstance::GrantEquipmentDef(UAbilitySystemComponent* ASC,
                                               const UIrisInventoryItemDefinition* InItemDef)
{
	if (!ASC || !InItemDef || ASC->GetOwnerActor()->HasAuthority()) return;
	
	CachedASC = ASC;
	SourceItemDef = InItemDef; //Клиенты получат реплицируемое значение
	
	//Достаем фрагмент с правилами GAS из CDO
	const UIrisInventoryItemFragment_Equippable* EquipDef = InItemDef->FindFragmentByClass<UIrisInventoryItemFragment_Equippable>();
	if (!EquipDef)
	{
		UE_LOG(Log_IrisEquipmentInstance, Error,TEXT("[%s] Item %s has no Equippable Fragment!"),ANSI_TO_TCHAR(__FUNCTION__),*InItemDef->GetName());
		return;
	}
	
	//Выдаем активные способности (Стрельба, Перезарядка)
	for (const FEqupmentAbilitySet& AbilitySet : EquipDef->GrantedAbilities)
	{
		//TODO Переделать в дальнейшем на StreamableManager.RequestAsyncLoad() пока снхронно
		if (UClass* AbilityClass = AbilitySet.Ability.LoadSynchronous())
		{
			FGameplayAbilitySpec Spec(AbilityClass,1,INDEX_NONE,this);
			//Биндим инпут по тегу (Пример Item.Type.Weapon)
			Spec.GetDynamicSpecSourceTags().AddTag(AbilitySet.InputTag);
			
			//Получаем и сохраняем Handle, чтобы позже сделать ClearAbility при Unequip
			FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
			GrantedAbilityHandles.Add(Handle);
		}
	}
	
	//Накидываем пассивные эффекты (Штраф к скорости, бонус к броне)
	for (const TSoftClassPtr<UGameplayEffect>& EffectClassPtr : EquipDef->PassiveEffects)
	{
		//TODO Переделать в дальнейшем на StreamableManager.RequestAsyncLoad() пока снхронно
		if (UClass* EffectClass = EffectClassPtr.LoadSynchronous())
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddInstigator(ASC->GetOwner(),ASC->GetOwner());
			Context.AddSourceObject(this);
			
			UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
			
			//Накидываем эффект и сохраняем Handle для RemoveActiveGameplayEffect при Unequip
			FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(EffectCDO,1.f,Context);
			GrantedEffectHandles.Add(Handle);
		}
	}
}

void UIrisEquipmentInstance::RevokeEquipmentDef()
{
	if (!CachedASC || !CachedASC->GetOwnerActor()->HasAuthority()) return;
	
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
	AActor* OwningActor = Cast<AActor>(GetOuter());
	if (!World || !OwningActor) return;
	
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
			if (ACharacter* Char = Cast<ACharacter>(OwningActor))
			{
				//TODO Продумать систему в которой будет фрагмент с сокетом для аттача и логики если он отсутствует
				SpawnedActor->AttachToComponent(Char->GetMesh(),FAttachmentTransformRules::SnapToTargetIncludingScale,"WeaponSocket"); //TODO Затычка переделать после
			}
			OnEquipped(); // Сигнал для Blueprint (Проиграть звук и т.д)
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
