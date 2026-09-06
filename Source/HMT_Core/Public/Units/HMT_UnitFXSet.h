#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"
#include "HMT_UnitFXSet.generated.h"

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_UnitFXSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|FX")
	TSoftObjectPtr<UFXSystemAsset> Spawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|FX")
	TSoftObjectPtr<UFXSystemAsset> Attack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|FX")
	TSoftObjectPtr<UFXSystemAsset> Ability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|FX")
	TSoftObjectPtr<UFXSystemAsset> Death;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|FX")
	TSoftObjectPtr<UFXSystemAsset> Merge;
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_UnitAudioSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Audio")
	TSoftObjectPtr<USoundBase> Spawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Audio")
	TSoftObjectPtr<USoundBase> Attack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Audio")
	TSoftObjectPtr<USoundBase> Ability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Audio")
	TSoftObjectPtr<USoundBase> Death;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Audio")
	TSoftObjectPtr<USoundBase> Merge;
};
