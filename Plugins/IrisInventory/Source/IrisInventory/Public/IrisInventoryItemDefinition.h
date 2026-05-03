// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IrisInventoryItemFragment.h"
#include "IrisInventoryItemDefinition.generated.h"

/**
 * 
 */
UCLASS(BlueprintType,Const)
class IRISINVENTORY_API UIrisInventoryItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	//Thread-safe поиск фрагмента. 0(n), где n - количество фрагментов (обычно 2-5)
	//Работает lock-free, так как мы только читаем const данные.
	template<typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		for (const UIrisInventoryItemFragment* Fragment : Fragments)
		{
			if (const ResultClass* FoundFragment = Cast<ResultClass>(Fragment))
			{
				return FoundFragment;
			}
		}
		return nullptr;
	}
	
protected:
	// Instanced флаг критически важен. Без него UE попытается сохранить указатели на внешние ассеты,
	// а нам нужно, чтобы фрагменты сериализовались прямо внутрь UIrisInventoryItemDefinition
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Instanced,Category=Fragments)
	TArray<TObjectPtr<UIrisInventoryItemFragment>> Fragments;
};
