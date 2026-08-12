// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IrisEquipmentMountInterface.generated.h"

UINTERFACE()
class UIrisEquipmentMountInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class COREFEATURES_API IIrisEquipmentMountInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="Equipment")
	USceneComponent* GetMountComponentForSocket(FName SocketName) const;
};
