// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryTestController.h"
#include "MockInventoryOwner.h"
#include "TimerManager.h"
#include "IrisInventoryComponent.h"
#include "IrisEquipmentManagerComponent.h"

//Создаем статическую группу для Unreal Insights
DECLARE_CYCLE_STAT(TEXT("Inventory Stress Test Tick"),STAT_InventoryStressTestTick,STATGROUP_Game);

AInventoryTestController::AInventoryTestController()
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;
}

void AInventoryTestController::BeginPlay()
{
	Super::BeginPlay();
	
	if (!TargetOwner && HasAuthority())
	{
		TargetOwner = GetWorld()->SpawnActor<AMockInventoryOwner>();
	}
}

void AInventoryTestController::StartStressTest(float InInterval, int32 InOperationPerTick)
{
	if (!HasAuthority()) return;
	
	BatchSize = InOperationPerTick;
	GetWorld()->GetTimerManager().SetTimer(
		StressTestTimerHandle,
		this,
		&AInventoryTestController::ExecuteTestTick,
		InInterval,
		true);
}

void AInventoryTestController::StopStressTest()
{
	GetWorld()->GetTimerManager().ClearTimer(StressTestTimerHandle);
}

void AInventoryTestController::RunBurstTest_MassAdd(const UIrisInventoryItemDefinition* InItemDefinition, int32 InCount)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEST_MassAdd);
	
	if (!HasAuthority() || !IsValid(TargetOwner) || !InItemDefinition) return;
	
	//Генерируем мгновенную нагрузку (1кадр)
	//Проверим, насколько быстро работает поиск свободного слота
	for (int32 i = 0; i < InCount; ++i)
	{
		TargetOwner->InventoryComponent->AddEntry(InItemDefinition,1);
	}
}

void AInventoryTestController::RunBurstTest_MassSplit(int32 TargetInstanceID, int32 SplitAmount, int32 InIterations)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEST_MassSplit);
	if (!HasAuthority() || !IsValid(TargetOwner) || !TargetInstanceID || SplitAmount<=0 || InIterations<=0) return;
	
	for (int32 i = 0; i < InIterations; ++i)
	{
		int32 Index = 0; //Просто дефолтное значение которое будет перезаписано (защита от дурака)
		//Делим стак и записываем индекс нового элемента
		Index = TargetOwner->InventoryComponent->SplitStack(TargetInstanceID,SplitAmount);
		//Обьеденяем обратно
		TargetOwner->InventoryComponent->MergeStacks(Index,TargetInstanceID);
		
	}
}

void AInventoryTestController::RunBurstTest_MassRemove(const UIrisInventoryItemDefinition* InItemDefinition)
{
	/*TODO Сначала добавить методы в инвентарь, затем реализовать
	 *if (HasAuthority() || !IsValid(TargetOwner) || !ItemDefinition) return;
	
	TargetOwner->InventoryComponent->*/
}

void AInventoryTestController::RunBurstTest_EquipUnequipCycle(const UIrisInventoryItemDefinition* EquipItemDef,
	int32 InIterations)
{
	if (!HasAuthority() || !IsValid(TargetOwner) || !EquipItemDef || InIterations < 1) return;
	
	for (int32 i = 0; i < InIterations; ++i)
	{
		TargetOwner->EquipmentComponent->EquipItemByInstance(EquipItemDef);
		TargetOwner->EquipmentComponent->UnequipItem();
	}
	
}

void AInventoryTestController::ExecuteTestTick()
{
	//Оборачиваем вызов для трейсинга. Эти макросы покажут чистую стоимость операций
	TRACE_CPUPROFILER_EVENT_SCOPE(AInventoryTestController::ExecuteTestTick);
	SCOPE_CYCLE_COUNTER(STAT_InventoryStressTestTick);
	
	if (!HasAuthority() || !IsValid(TargetOwner)) return;

	for (int i = 0; i < BatchSize; ++i)
	{
		RunBurstTest_MassAdd(ItemDefinition,Count);
		RunBurstTest_EquipUnequipCycle(ItemDefinition,Iterations);
		RunBurstTest_MassSplit(1,1,Iterations); //TargetImstanceID 1 потому что с 1го начинается индексирование ID
		RunBurstTest_MassRemove(ItemDefinition);
	}
}


