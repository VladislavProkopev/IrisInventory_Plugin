#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryTestUserWidget.generated.h"

class USpinBox;
class UButton;
class UIrisInventoryItemDefinition;

// Объявляем сигнатуру делегата (совпадает с тем, что ждет Pawn)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnRunMassAddSignature, const UIrisInventoryItemDefinition*, ItemDef, int32, Count, float, Interval, int32, OpsPerTick);

UCLASS()
class IRISINVENTORY_API UInventoryTestUserWidget : public UUserWidget
{
	GENERATED_BODY()
    
public:
	// Делегат, на который подписывается Sandbox Pawn
	UPROPERTY(BlueprintAssignable, Category = "Test | Events")
	FOnRunMassAddSignature OnRunMassAdd;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// Внутренний обработчик клика кнопки
	UFUNCTION()
	void OnMassAddButtonClicked();

protected:
	// Эта ссылка назначается в Blueprint-е виджета, чтобы знать, КАКОЙ предмет спамить
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Test | Parameters")
	const UIrisInventoryItemDefinition* TestItemDef;

	// --- UI Bindings ---
	UPROPERTY(meta = (BindWidget))
	USpinBox* SpinBox_Iterations;

	UPROPERTY(meta = (BindWidget))
	USpinBox* SpinBox_Count;

	UPROPERTY(meta = (BindWidget))
	USpinBox* SpinBox_Interval;

	UPROPERTY(meta = (BindWidget))
	USpinBox* SpinBox_OperationPerTick;

	UPROPERTY(meta = (BindWidget))
	UButton* Button_RunBurstTest_MassAdd;
    
	UPROPERTY(meta = (BindWidget))
	UButton* Button_RunBurstTest_MassSplit;
    
	UPROPERTY(meta = (BindWidget))
	UButton* Button_RunBurstTest_MassRemove;
    
	UPROPERTY(meta = (BindWidget))
	UButton* Button_RunBurstTest_EquipUnequipCycle;
    
	UPROPERTY(meta = (BindWidget))
	UButton* Button_StartStressTest;
    
	UPROPERTY(meta = (BindWidget))
	UButton* Button_StopStressTest;
};