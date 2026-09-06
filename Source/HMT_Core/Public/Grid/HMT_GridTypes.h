#pragma once

#include "CoreMinimal.h"
#include "HMT_GridTypes.generated.h"

UENUM(BlueprintType)
enum class EHMT_GridTopology : uint8
{
	Hex,
	Square
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_GridCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Grid")
	int32 Q = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Grid")
	int32 R = 0;

	FHMT_GridCoord() = default;
	FHMT_GridCoord(int32 InQ, int32 InR) : Q(InQ), R(InR) {}

	bool operator==(const FHMT_GridCoord& Other) const
	{
		return Q == Other.Q && R == Other.R;
	}

	bool operator!=(const FHMT_GridCoord& Other) const
	{
		return !(*this == Other);
	}

	friend uint32 GetTypeHash(const FHMT_GridCoord& Coord)
	{
		return HashCombine(GetTypeHash(Coord.Q), GetTypeHash(Coord.R));
	}
};
