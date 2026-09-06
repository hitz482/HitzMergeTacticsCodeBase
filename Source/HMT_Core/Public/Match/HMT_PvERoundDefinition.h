#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/HMT_CombatTypes.h"
#include "HMT_PvERoundDefinition.generated.h"

UCLASS(BlueprintType)
class HMT_CORE_API UHMT_PvERoundDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|PvE")
	TArray<FHMT_SnapshotUnit> EncounterUnits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|PvE")
	int32 ClearBonusGold = 1;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("HMT_PvERoundDefinition"), GetFName());
	}
};
