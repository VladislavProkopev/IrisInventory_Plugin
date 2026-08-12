#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "CoreFeatures/Public/Inventory/Interfaces/IrisEquipmentMountInterface.h"
#include "InventoryTestSandboxPawn.generated.h"

class UIrisInventoryItemDefinition;
class UIrisInventoryComponent;
class UIrisEquipmentManagerComponent;
class UInventoryTestUserWidget;

UCLASS(BlueprintType, Blueprintable)
class IRISINVENTORY_API AInventoryTestSandboxPawn : public APawn, public IGameFrameworkInitStateInterface, public IIrisEquipmentMountInterface
{
    GENERATED_BODY()

public:
    AInventoryTestSandboxPawn();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // --- L2 Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test Subject | Components")
    TObjectPtr<UIrisInventoryComponent> InventoryComponent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test Subject | Components")
    TObjectPtr<UIrisEquipmentManagerComponent> EquipmentComponent;

    // --- Visuals ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test Subject | Visuals")
    TObjectPtr<USceneComponent> RightHandSocketDummy;

protected:
    // --- UI Setup ---
    UPROPERTY(EditDefaultsOnly, Category = "Test | UI")
    TSubclassOf<UInventoryTestUserWidget> TestWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<UInventoryTestUserWidget> SpawnedWidget;

private:
    // --- CLIENT HANDLERS (Маршрутизация из UI) ---
    UFUNCTION()
    void HandleMassAddRequest(const UIrisInventoryItemDefinition* ItemDef, int32 Count, float Interval, int32 OpsPerTick);
    
    UFUNCTION()
    void HandleMassSplitRequest(int32 TargetInstanceID, int32 SplitAmount, int32 Iterations);

    UFUNCTION()
    void HandleMassRemoveRequest(const UIrisInventoryItemDefinition* ItemDef);

    UFUNCTION()
    void HandleEquipCycleRequest(const UIrisInventoryItemDefinition* EquipItemDef, int32 Iterations);

    UFUNCTION()
    void HandleStartStressTestRequest(float Interval, int32 OperationPerTick, const UIrisInventoryItemDefinition* ItemDef, int32 InCount);


    // --- SERVER RPCs (Переброс на авторитет) ---
    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Test | Execution")
    void Server_RunMassAdd(const UIrisInventoryItemDefinition* ItemDef, int32 Count, float Interval, int32 OpsPerTick);
    
    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Test | Execution")
    void Server_RunMassSplit(int32 TargetInstanceID, int32 SplitAmount, int32 Iterations);

    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Test | Execution")
    void Server_RunMassRemove(const UIrisInventoryItemDefinition* ItemDef);

    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Test | Execution")
    void Server_RunEquipCycle(const UIrisInventoryItemDefinition* EquipItemDef, int32 Iterations);

    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Test | Execution")
    void Server_StartStressTest(float Interval, int32 OperationPerTick, const UIrisInventoryItemDefinition* ItemDef, int32 InCount);


    // --- SERVER BUSINESS LOGIC (Реальное выполнение тестов) ---
    void StartStressTest(float Interval, int32 OperationPerTick, const UIrisInventoryItemDefinition* ItemDef, int32 InCount);
    void RunBurstTest_EquipUnequipCycle(const UIrisInventoryItemDefinition* EquipItemDef, int32 Iterations);
    
    // Обязателен UFUNCTION, чтобы таймер мог его вызвать
    UFUNCTION()
    void ExecuteTestTick();

    void RunBurstTest_MassAdd(const UIrisInventoryItemDefinition* ItemDefinition, int32 InCount);
    void RunBurstTest_MassSplit(int32 TargetInstanceID, int32 SplitAmount, int32 Iterations);
    void RunBurstTest_MassRemove(const UIrisInventoryItemDefinition* ItemDefinition);
    void RunToggleTest_Equip(const UIrisInventoryItemDefinition* EquipItemDef);


private:
    // --- Stress Test State ---
    FTimerHandle StressTestTimerHandle;
    
    UPROPERTY(Transient)
    const UIrisInventoryItemDefinition* TestItemDef = nullptr;
    
    int32 TestCount = 0;
    int32 BatchSize = 0;
    
    bool bIsEquippedToggle = false;
    int32 EquipCyclesLeft = 0;

public:
    //~ IGameFrameworkInitStateInterface
    virtual FName GetFeatureName() const override;
    //~End IGameFrameworkInitStateInterface

    //~ IIrisEquipmentMountInterface
    virtual USceneComponent* GetMountComponentForSocket_Implementation(FName SocketName) const override;
    //~End IIrisEquipmentMountInterface
};