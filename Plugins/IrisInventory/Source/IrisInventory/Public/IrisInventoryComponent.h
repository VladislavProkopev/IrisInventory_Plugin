// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IrisInventoryEntry.h"
#include "IrisInventoryComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class IRISINVENTORY_API UIrisInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIrisInventoryComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;

	UPROPERTY(Replicated)
	FIrisInventoryList Inventory;
};
