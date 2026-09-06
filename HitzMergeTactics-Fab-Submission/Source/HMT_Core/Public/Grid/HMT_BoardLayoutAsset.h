#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Grid/HMT_GridTypes.h"
#include "HMT_BoardLayoutAsset.generated.h"

UCLASS(BlueprintType)
class HMT_CORE_API UHMT_BoardLayoutAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UHMT_BoardLayoutAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board")
	EHMT_GridTopology Topology = EHMT_GridTopology::Hex;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board")
	float CellSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board")
	int32 Columns = 7;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board")
	int32 Rows = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board")
	TArray<FHMT_GridCoord> BlockedCells;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation")
	TObjectPtr<class UStaticMesh> TileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation")
	TObjectPtr<class UMaterialInterface> TileMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TileVisualScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation")
	TObjectPtr<class UMaterialInterface> HighlightMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation")
	TObjectPtr<class UMaterialInterface> BlockedTileMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation")
	bool bShowOpponentTiles = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation")
	TObjectPtr<class UMaterialInterface> OpponentTileMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation")
	TObjectPtr<class UMaterialInterface> ValidDropMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation")
	TObjectPtr<class UMaterialInterface> InvalidDropMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation")
	TObjectPtr<class UMaterialInterface> SelectedTileMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Board|Presentation")
	TObjectPtr<class UMaterialInterface> OccupiedTileMaterial;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("HMT_BoardLayout"), GetFName());
	}
};
