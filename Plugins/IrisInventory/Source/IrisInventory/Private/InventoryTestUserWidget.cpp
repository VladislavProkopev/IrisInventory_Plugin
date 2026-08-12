#include "InventoryTestUserWidget.h"
#include "Components/Button.h"
#include "Components/SpinBox.h"

void UInventoryTestUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Безопасно подписываемся на клик кнопки
	if (Button_RunBurstTest_MassAdd)
	{
		Button_RunBurstTest_MassAdd->OnClicked.AddDynamic(this, &UInventoryTestUserWidget::OnMassAddButtonClicked);
	}
	
	// TODO: Аналогично подписать остальные кнопки, когда реализуешь их делегаты
}

void UInventoryTestUserWidget::NativeDestruct()
{
	if (Button_RunBurstTest_MassAdd)
	{
		Button_RunBurstTest_MassAdd->OnClicked.RemoveDynamic(this, &UInventoryTestUserWidget::OnMassAddButtonClicked);
	}

	Super::NativeDestruct();
}

void UInventoryTestUserWidget::OnMassAddButtonClicked()
{
	// Парсим данные из SpinBox-ов в момент клика. O(1) операция.
	int32 ParsedCount = SpinBox_Count ? static_cast<int32>(SpinBox_Count->GetValue()) : 10;
	float ParsedInterval = SpinBox_Interval ? SpinBox_Interval->GetValue() : 0.1f;
	int32 ParsedOps = SpinBox_OperationPerTick ? static_cast<int32>(SpinBox_OperationPerTick->GetValue()) : 100;

	// Отправляем данные наверх (в Pawn)
	OnRunMassAdd.Broadcast(TestItemDef, ParsedCount, ParsedInterval, ParsedOps);
}