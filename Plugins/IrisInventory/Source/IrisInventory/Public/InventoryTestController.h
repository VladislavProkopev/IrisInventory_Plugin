// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#pragma warning( disable : 4458 )

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryTestController.generated.h"

class UIrisInventoryItemDefinition;
class AMockInventoryOwner;

UCLASS(BlueprintType,Blueprintable)
class IRISINVENTORY_API AInventoryTestController : public AActor
{
	GENERATED_BODY()

public:
	AInventoryTestController();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Test | Context")
	TObjectPtr<AMockInventoryOwner> TargetOwner;
	
	UFUNCTION(BlueprintCallable,Category="Test | Execution",BlueprintAuthorityOnly)
	void StartStressTest(float Interval = 0.1f, int32 OperationPerTick = 1000);
	
	UFUNCTION(BlueprintCallable,Category="Test | Execution",BlueprintAuthorityOnly)
	void StopStressTest();
	
	UFUNCTION(Blueprintable,Category="Test | Execution",BlueprintAuthorityOnly)
	void RunBurstTest_MassAdd(const UIrisInventoryItemDefinition* ItemDefinition,int32 Count);
	
	UFUNCTION(BlueprintCallable,Category="Test | Execution",BlueprintAuthorityOnly)
	void RunBurstTest_MassSplit(int32 TargetInstanceID,int32 SplitAmount,int32 Iterations);
	
	UFUNCTION(BlueprintCallable,Category="Test | Execution",BlueprintAuthorityOnly)
	void RunBurstTest_MassRemove(const UIrisInventoryItemDefinition* ItemDefinition);
	
	UFUNCTION(BlueprintCallable,Category="Test | Execution",BlueprintAuthorityOnly)
	void RunBurstTest_EquipUnequipCycle(const UIrisInventoryItemDefinition* EquipItemDef,int32 Iterations);
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Test | Parameters")
	const UIrisInventoryItemDefinition* ItemDefinition;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Test | Parameters")
	int32 Iterations = 10;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Test | Parameters")
	int32 Count = 10;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Test | Parameters")
	float Interval = 0.1f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Test | Parameters")
	int32 OperationPerTick = 100;
private:
	void ExecuteTestTick();
	
	FTimerHandle StressTestTimerHandle;
	int32 BatchSize = 1;
	
	
};
