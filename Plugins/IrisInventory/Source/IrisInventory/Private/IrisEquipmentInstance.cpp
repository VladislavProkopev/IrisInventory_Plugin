// Fill out your copyright notice in the Description page of Project Settings.


#include "IrisEquipmentInstance.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "CoreFeatures/Public/Inventory/Items/IrisInventoryItemFragment_Equippable.h"
#include "Inventory/Items/IrisInventoryItemDefinition.h"
#include "Net/RepLayout.h"

DEFINE_LOG_CATEGORY_STATIC(Log_IrisEquipmentInstance, All, All);

void UIrisEquipmentInstance::OnEquipped()
{
}

void UIrisEquipmentInstance::OnUnEquipped()
{
}

void UIrisEquipmentInstance::GrantEquipmentDef(UAbilitySystemComponent* ASC,
	const UIrisInventoryItemDefinition* InItemDef)
{
	if (!ASC || !InItemDef) return;
	
	CachedASC = ASC;
	SourceItemDef = InItemDef;
	
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
			Spec.DynamicAbilityTags.AddTag(AbilitySet.InputTag);
			
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
	if (!CachedASC) return;
	
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

void UIrisEquipmentInstance::SpawnEquipmentDef()
{
}

void UIrisEquipmentInstance::DestroyEquipmentDef()
{
}
