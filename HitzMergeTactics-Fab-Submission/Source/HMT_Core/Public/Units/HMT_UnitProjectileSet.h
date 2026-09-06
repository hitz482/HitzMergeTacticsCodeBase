#pragma once

#include "CoreMinimal.h"
#include "Combat/HMT_ProjectileActor.h"
#include "HMT_UnitProjectileSet.generated.h"

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_UnitProjectileSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	TSubclassOf<AHMT_ProjectileActor> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FName MuzzleSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FName TargetSocketName;
};
