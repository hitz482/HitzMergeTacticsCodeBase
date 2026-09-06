#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Grid/IHMT_GridLayout.h"
#include "HMT_HexGridLayout.generated.h"

UCLASS(BlueprintType, Blueprintable)
class HMT_CORE_API UHMT_HexGridLayout : public UObject, public IHMT_GridLayout
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Grid")
	float CellSize = 100.f;

	virtual TArray<FHMT_GridCoord> GetNeighbors_Implementation(FHMT_GridCoord Coord) override;
	virtual int32 GetDistance_Implementation(FHMT_GridCoord A, FHMT_GridCoord B) override;
	virtual FVector GridToWorld_Implementation(FHMT_GridCoord Coord) override;
	virtual FHMT_GridCoord WorldToGrid_Implementation(FVector WorldLocation) override;
};
