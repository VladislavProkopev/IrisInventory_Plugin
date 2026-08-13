// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IrisInventoryItemFragment.h"
#include "GameplayTagContainer.h"
#include "IrisInventoryItemDefinition.generated.h"

class UIrisInventoryFragment_Stats;
class UIrisInventoryFragment_Stackable;
class UIrisInventoryFragment_InstanceState;
/**
 * 
 */
UCLASS(BlueprintType,Const)
class COREFEATURES_API UIrisInventoryItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	
	//Прямой доступ к фрагментам, которые читаются на каждое добавление предмета.
	//Остальные оставлены на FindFragmentByClass: они холодные (экипировка, спавн визуала),
	//а поле на каждый тип фрагмента превратило бы определение в свалку
	const UIrisInventoryFragment_Stats*     GetStatsFragment()     const { return CachedStats; }
	const UIrisInventoryFragment_Stackable* GetStackableFragment() const { return CachedStackable; }
	//Наличие фрагмента = предмет уникален. Читается на каждой записи стата
	//(ModifyItemStatByInstanceID), то есть на каждый выстрел - потому в кеше
	const UIrisInventoryFragment_InstanceState* GetInstanceStateFragment() const { return CachedInstanceState; }

	virtual void PostLoad() override;
	//Копия ассета (Ctrl+D) не проходит через PostLoad, а Transient-поля не переносятся
	//при дублировании: без этого у копии кеш останется пустым до перезапуска редактора
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	//Классификация предмета: Item.Type.*, Item.Rarity.*, Item.Quest.* и подобное.
	//Живёт в CDO, читается lock-free. Отдельно от InitialItemStats: там ЗНАЧЕНИЯ
	//статов (вес, размер стака), здесь ПРИНАДЛЕЖНОСТЬ к категориям.
	//Смешивать их нельзя - именно на этом ломались GetTotalItemCountByTag и ConsumeItemByTag
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Item")
	FGameplayTagContainer ItemTags;
	
	//Идентификатор типа для AssetManager
	//в ProjectSettings -> AssetManager нужно создать Primary Asset Type с именем "InventoryItem"
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		//GetFName() вернет имя самого ассета в редакторе (например, "DA_Weapon_AK47")
		return FPrimaryAssetId(FName("InventoryItem"),GetFName());
	}
	
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
	
private:
	void RebuildFragmentCache();

	UPROPERTY(Transient)
	TObjectPtr<const UIrisInventoryFragment_Stats> CachedStats = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<const UIrisInventoryFragment_Stackable> CachedStackable = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<const UIrisInventoryFragment_InstanceState> CachedInstanceState = nullptr;
};
