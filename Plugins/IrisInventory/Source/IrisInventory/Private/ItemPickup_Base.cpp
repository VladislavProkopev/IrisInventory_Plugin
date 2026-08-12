// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemPickup_Base.h"

#include "IrisInventoryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
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
		//CDO уже должен быть в памяти (загружен через UpdateVisuals), поэтому LoadSynchronous тут безопасен 0(1) чтение из кеша
		const UIrisInventoryItemDefinition* LoadedDef = ItemDef.LoadSynchronous();
		
		//Вызываем новый API, который возвращает структуру-отчет
		FIrisInventoryAddResult Result = Inventory->AddEntry(LoadedDef, StackCount);
		
		if (Result.IsFullySuccessful())
		{
			//Влезло абсолютно все. Сосуд больше не нужен
			Destroy();
			return true;
		}
		else if (Result.IsPartiallySuccessful())
		{
			//Влезла только часть (из-за лимита веса/слотов)
			//Обновляем количество в сосуде на земле и говорим Iris разослать изменения
			StackCount = Result.RejectedCount;
			MARK_PROPERTY_DIRTY_FROM_NAME(AItemPickup_Base,StackCount,this);
			return true; //Транзакция все равно успешна, так как мы хоть что-то подобрали
		}
		
		//Если дошли сюда, значит Result.ActuallyAdded == 0
		//У игрока нет места даже для 1 единицы лута. Ничего не делаем, сосуд останется нетронутым
		return false;
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
	if (ItemDef.IsNull()) return;
	
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
		if (VisualFrag->WorldMesh.IsValid())
		{
			//Меш уже загружен в RAM (другим пикапом) Применяем мгновенно
			if (VisualMesh)
			{
				VisualMesh->SetStaticMesh(VisualFrag->WorldMesh.Get());
			}
		}
		else if (!VisualFrag->WorldMesh.IsValid())
		{
			//Меша нет в RAM, но путь указан корректно
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


