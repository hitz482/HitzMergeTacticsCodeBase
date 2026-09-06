#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "HMT_AttributeTagMapAsset.generated.h"

USTRUCT(BlueprintType)
struct HMT_GASBRIDGE_API FHMT_AttributeTagMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|GASBridge", meta = (Categories = "Stat"))
	FGameplayTag StatTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|GASBridge")
	TSubclassOf<UAttributeSet> AttributeSetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|GASBridge")
	FName AttributePropertyName;
};

UCLASS(BlueprintType)
class HMT_GASBRIDGE_API UHMT_AttributeTagMapAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|GASBridge")
	TArray<FHMT_AttributeTagMapping> Mappings;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|GASBridge")
	FGameplayAttribute ResolveAttribute(FGameplayTag StatTag) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("HMT_AttributeTagMap"), GetFName());
	}
};
