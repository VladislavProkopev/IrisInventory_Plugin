// Fill out your copyright notice in the Description page of Project Settings.


#include "CC_WeaponFire_Hitscan.h"

#include "AbilitySystemComponent.h"
#include "CC_WeaponBase.h"
#include "CollisionQueryParams.h"
#include "CoreGameplayTags.h"
#include "IrisInventoryComponent.h"
#include "Engine/World.h"
#include "Inventory/Items/IrisInventoryFragment_WeaponBallistics.h"
#include "IrisInventory/Public/IrisEquipmentInstance.h"

UCC_WeaponFire_Hitscan::UCC_WeaponFire_Hitscan()
{
	//TODO Продолжить отсюда
}

void UCC_WeaponFire_Hitscan::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CachedBallistics || !CachedWeaponActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true,false);
		return;
	}
	
	//Коммитим кулдаун и трату патронов (GameplayEffect Cost/Cooldown)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) {EndAbility(Handle, ActorInfo, ActivationInfo, true,false); return;}
	
	//0(1) Доступ к статам
	int32 Pellets = CachedBallistics->PelletsPerShot;
	FTransform MuzzleTransform = CachedWeaponActor->GetMuzzleTransform();
	
	//Триггерим клиентский визуал (GameplayCue)
	//GAS автоматически выполнит это предсказание на клиенте и авторитарно на сервере
	FGameplayCueParameters CueParams;
	CueParams.Location = MuzzleTransform.GetLocation();
	ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(CachedBallistics->FireCueTag,CueParams);
	
	PerformFireMath(CachedWeaponActor.Get(),CachedBallistics);
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true,false);
}

void UCC_WeaponFire_Hitscan::PerformFireMath(ACC_WeaponBase* WeaponActor,
	const UIrisInventoryFragment_WeaponBallistics* Ballistics)
{
	FTransform MuzzleTransform = WeaponActor->GetMuzzleTransform();
	FVector StartLoc = MuzzleTransform.GetLocation();
	FVector ForwardDir = MuzzleTransform.GetRotation().GetForwardVector();
	
	for (int i = 0; i < Ballistics->PelletsPerShot; i++)
	{
		//Применяем Spread
		FVector TraceDir = FMath::VRandCone(ForwardDir,FMath::DegreesToRadians(Ballistics->SpreadAngle));
		FVector EndLoc = StartLoc + (TraceDir * Ballistics->MaxRange);
		
		FHitResult Hit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetAvatarActorFromActorInfo());
		
		if (GetWorld()->LineTraceSingleByChannel(Hit,StartLoc,EndLoc,ECC_Visibility,QueryParams))
		{
			//TODO Логика урона
		}
	}
}

void UCC_WeaponFire_Hitscan::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);
	
	//Достаем SourceObject
	if (UIrisEquipmentInstance* EquipInstance = Cast<UIrisEquipmentInstance>(Spec.SourceObject))
	{
		//Кешируем актор оружия (Если он заспавнен)
		CachedWeaponActor = Cast<ACC_WeaponBase>(EquipInstance->SpawnedActor);
		CachedInstanceID = EquipInstance->GetInstanceID();
		//Кешируем тяжелый поиск фрагмента (Выполняется 1 раз за всю жизнь оружия)
		if (const UIrisInventoryItemDefinition* ItemDef = EquipInstance->GetItemDef())
		{
			CachedBallistics = ItemDef->FindFragmentByClass<UIrisInventoryFragment_WeaponBallistics>();
		}
		
		if (UIrisInventoryComponent* InventoryComp = ActorInfo->AvatarActor->FindComponentByClass<UIrisInventoryComponent>())
		{
			CachedInventoryComponent = InventoryComp;
		}
	}
}

bool UCC_WeaponFire_Hitscan::CheckCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	//Оставляем базовую логику GAS (вдруг есть какие-то глобальные дебафы на стрельбу)
	const bool bBaseCostMet = Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	
	if (!bBaseCostMet) return false;
	
	if (!CachedInventoryComponent || CachedInstanceID == INDEX_NONE) return false;
	
	int32 CurrentWeaponSlotIndex = CachedInventoryComponent->FindSlotByInstanceID(CachedInstanceID);
	if (CurrentWeaponSlotIndex == INDEX_NONE) return false;
	
	const int32 CurrentAmmo = CachedInventoryComponent->GetItemStat(CurrentWeaponSlotIndex,CoreGameplayTags::InventoryTags::Item_Stat_Ammo_Current);
	
	return CurrentAmmo > 0;
}

void UCC_WeaponFire_Hitscan::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
	
	if (!CachedInventoryComponent || CachedInstanceID == INDEX_NONE) return;
	
	const int32 CurrentWeaponSlotIndex = CachedInventoryComponent->FindSlotByInstanceID(CachedInstanceID);
	if (CurrentWeaponSlotIndex == INDEX_NONE) return;
	
	CachedInventoryComponent->ModifyItemStat(CurrentWeaponSlotIndex,CoreGameplayTags::InventoryTags::Item_Stat_Ammo_Current,-1);
}
