#pragma once
#include "CoreMinimal.h"
#include "IrisInventoryItemDefinition.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "IrisInventoryEntry.generated.h"

USTRUCT(BlueprintType)
struct FIrisInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	//Ссылка на DataAsset. По сети полетит только его PrimaryAssetId (строка\хеш)
	UPROPERTY()
	TObjectPtr<const UIrisInventoryItemDefinition> ItemDef = nullptr;
	
	//Динамические данные (Instance)
	UPROPERTY()
	int32 StackCount = 0;
	
	//Для Iris ксли предмет пустой (удален), возвращаем false
	bool IsValid() const {return  ItemDef != nullptr && StackCount > 0;}
};

USTRUCT()
struct FIrisInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FIrisInventoryEntry> Entries;
	
	//Обязательный контракт движка для сериализации
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FIrisInventoryEntry,FIrisInventoryList>(Entries, DeltaParams,*this);
	}
};

// Регистрация Трейта (Только для контейнера)
template<>
struct TStructOpsTypeTraits<FIrisInventoryList> : public TStructOpsTypeTraitsBase2<FIrisInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};