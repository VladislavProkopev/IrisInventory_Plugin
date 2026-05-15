#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseWeapon.generated.h"

class UIrisInventoryItemDefinition;

UCLASS()
class PETPROJECT_API ABaseWeapon : public AActor
{
	GENERATED_BODY()

public:
	ABaseWeapon();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly)
	UIrisInventoryItemDefinition* IrisInventoryItemDefinition;
};
