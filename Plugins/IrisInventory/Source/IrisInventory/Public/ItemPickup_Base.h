// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"
#include "ItemPickup_Base.generated.h"

class UIrisInventoryItemDefinition;
class UStaticMeshComponent;
class USphereComponent;

UCLASS(Abstract)
class IRISINVENTORY_API AItemPickup_Base : public AActor
{
	GENERATED_BODY()

public:
	AItemPickup_Base();

	UFUNCTION(BlueprintAuthorityOnly,BlueprintCallable,Category="Loot")
	bool TakePickup(AActor* Receiver);
	
	//Метод инициализации при спавне(Drop)
	void InitializePickup(const UIrisInventoryItemDefinition* InItemDef,int32 InStackCount);
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Components")
	TObjectPtr<USphereComponent> InteractionSphere;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;
	
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_PickupData)
	TSoftObjectPtr<const UIrisInventoryItemDefinition> ItemDef;
	
	UPROPERTY(Replicated)
	int32 StackCount = 1;
	
	UFUNCTION()
	void OnRep_PickupData();
	void UpdateVisuals();
private:
	void OnItemDefLoaded();
	void OnMeshLoaded();
	
	TSharedPtr<struct FStreamableHandle> MeshLoadHandle;
};

inline void AItemPickup_Base::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//Защита от утечек: Отменяем асинхронную загрузку при уничтожении актора
	if (MeshLoadHandle.IsValid() && MeshLoadHandle->IsActive())
	{
		MeshLoadHandle->CancelHandle();
	}
	
	Super::EndPlay(EndPlayReason);
}
