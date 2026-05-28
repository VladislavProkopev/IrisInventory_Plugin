// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IrisInventoryItemFragment.generated.h"

/**
 * DefaultToInstanced: Каждый обьявленный фрагмент будет уникальным обьектом в памяти ассета
 * EditInlineNew: Позволяет создавать наследников этого класса прямо в окне свойств (кнопка "+")
 */
UCLASS(DefaultToInstanced, EditInlineNew,Abstract)
class COREFEATURES_API UIrisInventoryItemFragment : public UObject
{
	GENERATED_BODY()
};
