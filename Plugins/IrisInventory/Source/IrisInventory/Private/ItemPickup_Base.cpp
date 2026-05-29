// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemPickup_Base.h"

#include "IrisInventoryComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/AssetManager.h"
#include "Inventory/Items/IrisInventoryItemFragment_World.h"
#include "Net/UnrealNetwork.h"


AItemPickup_Base::AItemPickup_Base()
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates = true;
	SetReplicatingMovement(true);
	
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(FName("InteractionSphere"));
	RootComponent = InteractionSphere;
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool AItemPickup_Base::TakePickup(AActor* Receiver)
{
	if (!HasAuthority() || !Receiver || ItemDef.IsNull() || StackCount <=0) return false;
	
	if (UIrisInventoryComponent* Inventory = Receiver->FindComponentByClass<UIrisInventoryComponent>())
	{
		//Похоже тут тоже переделаем
		const UIrisInventoryItemDefinition* LoadedDef = ItemDef.LoadSynchronous();
		
		Inventory->AddEntry(LoadedDef,StackCount);
		
		Destroy();
		return true;
	}
	return false;
}

void AItemPickup_Base::InitializePickup(const UIrisInventoryItemDefinition* InItemDef, int32 InStackCount)
{
	ItemDef = InItemDef;
	StackCount = InStackCount;
	
	//Обновляем свойства немедленно для клиентов
	MARK_PROPERTY_DIRTY_FROM_NAME(AItemPickup_Base, ItemDef,this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AItemPickup_Base, StackCount,this);
	
	UpdateVisuals();
}

void AItemPickup_Base::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass,ItemDef);
	DOREPLIFETIME(ThisClass,StackCount);
}

void AItemPickup_Base::OnRep_PickupData()
{
	UpdateVisuals();
}

void AItemPickup_Base::UpdateVisuals()
{
	if (!ItemDef.IsNull()) return;
	
	//Шаг 1: Асинхронно грузим CDO
	UAssetManager::GetStreamableManager().RequestAsyncLoad(
		ItemDef.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this,&AItemPickup_Base::OnItemDefLoaded)
		);
	
}

void AItemPickup_Base::OnItemDefLoaded()
{
	const UIrisInventoryItemDefinition* LoadedDef = ItemDef.Get();
	if (!LoadedDef) return;
	
	//Ищем фрагмент с 3D визуальной частью
	if (const UIrisInventoryItemFragment_World* VisualFrag = LoadedDef->FindFragmentByClass<UIrisInventoryItemFragment_World>())
	{
		if (!VisualFrag->WorldMesh.IsValid())
		{
			//Шаг 2: Асинхронно грузим саму тяжелую геометрию
			MeshLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
				VisualFrag->WorldMesh.ToSoftObjectPath(),
				FStreamableDelegate::CreateUObject(this,&AItemPickup_Base::OnMeshLoaded));
		}
	}
}

void AItemPickup_Base::OnMeshLoaded()
{
	//Проверяем, не был ли уничтожен пикап например (кто-то поднял его пока грузился меш)
	if (!IsValid(this) || ItemDef.IsNull()) return;
	
	if (const UIrisInventoryItemFragment_World* VisualFrag = ItemDef.Get()->FindFragmentByClass<UIrisInventoryItemFragment_World>())
	{
		UStaticMesh* LoadedMesh = VisualFrag->WorldMesh.Get();
		if (LoadedMesh && VisualMesh)
		{
			VisualMesh->SetStaticMesh(LoadedMesh);
		}
	}
	
	//Очищаем Handle
	MeshLoadHandle.Reset();
}

void AItemPickup_Base::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority()) UpdateVisuals();
}


