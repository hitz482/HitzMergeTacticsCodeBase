#pragma once

#include "CoreMinimal.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "HMT_UnitStarVisual.generated.h"

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_UnitStarVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Visual")
	TSoftObjectPtr<USkeletalMesh> MeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Visual")
	TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;
};
