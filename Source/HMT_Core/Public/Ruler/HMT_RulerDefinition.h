#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Units/HMT_UnitAnimSet.h"
#include "HMT_RulerDefinition.generated.h"

UCLASS(BlueprintType, Const)
class HMT_CORE_API UHMT_RulerDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ruler")
	FText RulerName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ruler")
	TSoftObjectPtr<class UTexture2D> RulerIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ruler")
	FString RulerDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ruler")
	int32 StartingHPOverride = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ruler")
	TSubclassOf<UObject> RulerPassiveClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ruler|Presentation")
	TSoftObjectPtr<class USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ruler|Presentation")
	FHMT_UnitAnimSet AnimSet;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UFUNCTION(BlueprintCallable, Category = "Ruler")
	FText GetRulerNameText() const { return RulerName; }

	UFUNCTION(BlueprintCallable, Category = "Ruler")
	TSoftObjectPtr<UTexture2D> GetRulerIconAsset() const { return RulerIcon; }

	UFUNCTION(BlueprintCallable, Category = "Ruler")
	FText GetRulerDescriptionText() const { return FText::FromString(RulerDescription); }
};
