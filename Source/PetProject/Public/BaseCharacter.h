#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include "BaseCharacter.generated.h"

class UPawnExtensionComponent;

UCLASS()
class ABaseCharacter : public AModularCharacter
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	
	virtual void BeginPlay() override;
	
	UPawnExtensionComponent* GetPawnExtensionComponent() const {return PawnExtensionComponent;}
	
	virtual void SetupPlayerInputComponents();
private:
	UPROPERTY(EditDefaultsOnly,Category="GameFeatures")
	TObjectPtr<UPawnExtensionComponent> PawnExtensionComponent;
};


