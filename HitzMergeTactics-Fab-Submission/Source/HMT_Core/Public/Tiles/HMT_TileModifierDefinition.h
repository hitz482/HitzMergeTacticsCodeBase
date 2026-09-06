#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HMT_TileModifierDefinition.generated.h"

UCLASS(BlueprintType, Const)
class HMT_CORE_API UHMT_TileModifierDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Modifier")
	FText TileName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Modifier")
	FString Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Modifier")
	TSoftObjectPtr<class UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Modifier", meta = (MustImplement = "/Script/HMT_Core.HMT_TileEffect"))
	TSubclassOf<UObject> TileEffectClass;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
