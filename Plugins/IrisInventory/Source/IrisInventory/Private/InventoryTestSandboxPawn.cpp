#include "InventoryTestSandboxPawn.h"
#include "InventoryTestUserWidget.h"
#include "IrisEquipmentManagerComponent.h"
#include "IrisInventoryComponent.h"
#include "TimerManager.h"

DECLARE_CYCLE_STAT(TEXT("Inventory Stress Test Tick"), STAT_InventoryStressTestTick, STATGROUP_Game);

AInventoryTestSandboxPawn::AInventoryTestSandboxPawn()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    AutoPossessPlayer = EAutoReceiveInput::Player0;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootDummy"));
    
    RightHandSocketDummy = CreateDefaultSubobject<USceneComponent>(TEXT("RightHandSocketDummy"));
    RightHandSocketDummy->SetupAttachment(RootComponent);

    InventoryComponent = CreateDefaultSubobject<UIrisInventoryComponent>(TEXT("InventoryComponent"));
    EquipmentComponent = CreateDefaultSubobject<UIrisEquipmentManagerComponent>(TEXT("EquipmentComponent"));
}

void AInventoryTestSandboxPawn::BeginPlay()
{
    Super::BeginPlay();
    
    if (IsLocallyControlled() && TestWidgetClass)
    {
       if (APlayerController* PC = Cast<APlayerController>(GetController()))
       {
          SpawnedWidget = CreateWidget<UInventoryTestUserWidget>(PC, TestWidgetClass);
          if (SpawnedWidget)
          {
             // TODO: Подпиши здесь остальные делегаты из виджета, когда добавишь их
             SpawnedWidget->OnRunMassAdd.AddDynamic(this, &AInventoryTestSandboxPawn::HandleMassAddRequest);
             SpawnedWidget->AddToViewport();
             PC->SetShowMouseCursor(true);
          }
       }
    }
}

void AInventoryTestSandboxPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SpawnedWidget)
    {
       SpawnedWidget->OnRunMassAdd.RemoveDynamic(this, &AInventoryTestSandboxPawn::HandleMassAddRequest);
    }
    Super::EndPlay(EndPlayReason);
}

// ==========================================
// 1. CLIENT HANDLERS (Вызов с виджета)
// ==========================================
void AInventoryTestSandboxPawn::HandleMassAddRequest(const UIrisInventoryItemDefinition* ItemDef, int32 Count, float Interval, int32 OpsPerTick)
{
    Server_RunMassAdd(ItemDef, Count, Interval, OpsPerTick);
}

void AInventoryTestSandboxPawn::HandleMassSplitRequest(int32 TargetInstanceID, int32 SplitAmount, int32 Iterations)
{
    Server_RunMassSplit(TargetInstanceID, SplitAmount, Iterations);
}

void AInventoryTestSandboxPawn::HandleMassRemoveRequest(const UIrisInventoryItemDefinition* ItemDef)
{
    Server_RunMassRemove(ItemDef);
}

void AInventoryTestSandboxPawn::HandleEquipCycleRequest(const UIrisInventoryItemDefinition* EquipItemDef, int32 Iterations)
{
    Server_RunEquipCycle(EquipItemDef, Iterations);
}

void AInventoryTestSandboxPawn::HandleStartStressTestRequest(float Interval, int32 OperationPerTick, const UIrisInventoryItemDefinition* ItemDef, int32 InCount)
{
    Server_StartStressTest(Interval, OperationPerTick, ItemDef, InCount);
}

// ==========================================
// 2. SERVER RPCs (Валидация и Маршрутизация)
// ==========================================
bool AInventoryTestSandboxPawn::Server_RunMassAdd_Validate(const UIrisInventoryItemDefinition* ItemDef, int32 Count, float Interval, int32 OpsPerTick) { return ItemDef != nullptr && Count > 0; }
void AInventoryTestSandboxPawn::Server_RunMassAdd_Implementation(const UIrisInventoryItemDefinition* ItemDef, int32 Count, float Interval, int32 OpsPerTick)
{
    if (!GetWorld()->HasBegunPlay()) return; 
    RunBurstTest_MassAdd(ItemDef, Count); // Либо QueueItemsForGrant
}

bool AInventoryTestSandboxPawn::Server_RunMassSplit_Validate(int32 TargetInstanceID, int32 SplitAmount, int32 Iterations) { return TargetInstanceID > 0 && SplitAmount > 0; }
void AInventoryTestSandboxPawn::Server_RunMassSplit_Implementation(int32 TargetInstanceID, int32 SplitAmount, int32 Iterations)
{
    if (!GetWorld()->HasBegunPlay()) return;
    RunBurstTest_MassSplit(TargetInstanceID, SplitAmount, Iterations);
}

bool AInventoryTestSandboxPawn::Server_RunMassRemove_Validate(const UIrisInventoryItemDefinition* ItemDef) { return ItemDef != nullptr; }
void AInventoryTestSandboxPawn::Server_RunMassRemove_Implementation(const UIrisInventoryItemDefinition* ItemDef)
{
    if (!GetWorld()->HasBegunPlay()) return;
    RunBurstTest_MassRemove(ItemDef);
}

bool AInventoryTestSandboxPawn::Server_RunEquipCycle_Validate(const UIrisInventoryItemDefinition* EquipItemDef, int32 Iterations) { return EquipItemDef != nullptr && Iterations > 0; }
void AInventoryTestSandboxPawn::Server_RunEquipCycle_Implementation(const UIrisInventoryItemDefinition* EquipItemDef, int32 Iterations)
{
    if (!GetWorld()->HasBegunPlay()) return;
    // Обернули в старый вызов из твоего черновика, если ты тестируешь Equip без таймера
    RunBurstTest_EquipUnequipCycle(EquipItemDef, Iterations); 
}

bool AInventoryTestSandboxPawn::Server_StartStressTest_Validate(float Interval, int32 OperationPerTick, const UIrisInventoryItemDefinition* ItemDef, int32 InCount) { return ItemDef != nullptr; }
void AInventoryTestSandboxPawn::Server_StartStressTest_Implementation(float Interval, int32 OperationPerTick, const UIrisInventoryItemDefinition* ItemDef, int32 InCount)
{
    if (!GetWorld()->HasBegunPlay()) return;
    StartStressTest(Interval, OperationPerTick, ItemDef, InCount);
}

// ==========================================
// 3. SERVER BUSINESS LOGIC (Реальное исполнение)
// ==========================================
void AInventoryTestSandboxPawn::StartStressTest(float Interval, int32 OperationPerTick, const UIrisInventoryItemDefinition* ItemDef, int32 InCount)
{
    BatchSize = FMath::Clamp(OperationPerTick, 1, 100); 
    TestItemDef = ItemDef;
    TestCount = FMath::Clamp(InCount, 1, 50);
    EquipCyclesLeft = TestCount * 2; 

    GetWorld()->GetTimerManager().SetTimer(
       StressTestTimerHandle,
       this,
       &AInventoryTestSandboxPawn::ExecuteTestTick,
       Interval, 
       true);
}

void AInventoryTestSandboxPawn::ExecuteTestTick()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(AInventoryTestSandboxPawn::ExecuteTestTick);
    SCOPE_CYCLE_COUNTER(STAT_InventoryStressTestTick);

    if (!HasAuthority() || !TestItemDef || !InventoryComponent) return;

    for (int i = 0; i < BatchSize; ++i)
    {
       RunBurstTest_MassAdd(TestItemDef, TestCount);
       RunBurstTest_MassSplit(1, 1, TestCount); 
       RunBurstTest_MassRemove(TestItemDef);
    }

    if (EquipCyclesLeft > 0)
    {
       RunToggleTest_Equip(TestItemDef);
       EquipCyclesLeft--;
    }
}

void AInventoryTestSandboxPawn::RunBurstTest_MassAdd(const UIrisInventoryItemDefinition* ItemDefinition, int32 InCount)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(TEST_MassAdd);
    // InventoryComponent->AddEntry_Batched(ItemDefinition, InCount);
}

void AInventoryTestSandboxPawn::RunBurstTest_MassSplit(int32 TargetInstanceID, int32 SplitAmount, int32 Iterations)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(TEST_MassSplit);
    
    if (!InventoryComponent || TargetInstanceID <= 0 || SplitAmount <= 0) return;

    for (int32 i = 0; i < Iterations; ++i)
    {
       int32 Index = InventoryComponent->SplitStack(TargetInstanceID, SplitAmount);
       if (Index != INDEX_NONE) 
       {
          InventoryComponent->MergeStacks(Index, TargetInstanceID);
       }
    }
}

void AInventoryTestSandboxPawn::RunBurstTest_MassRemove(const UIrisInventoryItemDefinition* ItemDefinition)
{
    // TODO: Батч удаление
}

void AInventoryTestSandboxPawn::RunToggleTest_Equip(const UIrisInventoryItemDefinition* EquipItemDef)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(TEST_EquipToggle);
    if (!EquipmentComponent) return;

    bIsEquippedToggle = !bIsEquippedToggle;
    if (bIsEquippedToggle)
    {
       EquipmentComponent->EquipItemByInstance(EquipItemDef);
    }
    else
    {
       EquipmentComponent->UnequipItem();
    }
}

FName AInventoryTestSandboxPawn::GetFeatureName() const
{
    return IGameFrameworkInitStateInterface::GetFeatureName();
}

USceneComponent* AInventoryTestSandboxPawn::GetMountComponentForSocket_Implementation(FName SocketName) const
{
    if (SocketName == FName("RightHand")) return RightHandSocketDummy;
    return RootComponent;
}

void AInventoryTestSandboxPawn::RunBurstTest_EquipUnequipCycle(const UIrisInventoryItemDefinition* EquipItemDef, int32 Iterations)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(TEST_EquipCycle);
	
    if (!EquipmentComponent || !EquipItemDef) return;

    for (int32 i = 0; i < Iterations; ++i)
    {
        EquipmentComponent->EquipItemByInstance(EquipItemDef);
        EquipmentComponent->UnequipItem();
    }
}