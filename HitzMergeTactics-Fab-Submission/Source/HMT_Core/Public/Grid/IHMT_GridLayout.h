#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Grid/HMT_GridTypes.h"
#include "IHMT_GridLayout.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class UHMT_GridLayout : public UInterface
{
	GENERATED_BODY()
};

class HMT_CORE_API IHMT_GridLayout
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Grid")
	TArray<FHMT_GridCoord> GetNeighbors(FHMT_GridCoord Coord);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Grid")
	int32 GetDistance(FHMT_GridCoord A, FHMT_GridCoord B);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Grid")
	FVector GridToWorld(FHMT_GridCoord Coord);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Grid")
	FHMT_GridCoord WorldToGrid(FVector WorldLocation);
};
