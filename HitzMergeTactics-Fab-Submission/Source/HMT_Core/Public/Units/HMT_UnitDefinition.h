#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Stats/HMT_StatTypes.h"
#include "Units/HMT_UnitAnimSet.h"
#include "Units/HMT_UnitFXSet.h"
#include "Units/HMT_UnitStarVisual.h"
#include "Units/HMT_UnitProjectileSet.h"
#include "HMT_UnitDefinition.generated.h"

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_StarStatOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Units")
	int32 StarLevel = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Units")
	TArray<FHMT_BaseStat> Stats;
};

UCLASS(BlueprintType)
class HMT_CORE_API UHMT_UnitDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units", meta = (Categories = "Unit"))
	FGameplayTag UnitID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	FText AbilityName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units", meta = (MultiLine = "true"))
	FText AbilityDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units", meta = (ClampMin = "1", ClampMax = "5"))
	int32 CostTier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	bool bIsBuilding = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units", meta = (Categories = "Trait"))
	TArray<FGameplayTag> Traits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	TArray<FHMT_BaseStat> BaseStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	float StarStatMultiplier = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	TArray<FHMT_StarStatOverride> PerStarStatOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	FHMT_UnitAnimSet AnimSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	FHMT_UnitFXSet FXSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units")
	FHMT_UnitAudioSet AudioSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units|Merge Feedback")
	TArray<FHMT_UnitStarVisual> VisualsByStar;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units|Merge Feedback")
	FName MergeFXSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units|Merge Feedback")
	TArray<float> ScaleByStar;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat")
	FHMT_UnitProjectileSet ProjectileSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units|Presentation")
	FVector HealthBarOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units|Presentation")
	float HealthBarZOffsetPerStar = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units", meta = (MustImplement = "/Script/HMT_Core.HMT_TargetingStrategy"))
	TSubclassOf<UObject> TargetingStrategyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Units", meta = (MustImplement = "/Script/HMT_Core.HMT_AbilityExecutor"))
	TSubclassOf<UObject> AbilityExecutorClass;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	TArray<FHMT_BaseStat> GetScaledStats(int32 StarLevel) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	FText GetDisplayNameText() const { return DisplayName; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	FText GetDescriptionText() const { return Description; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	FText GetAbilityNameText() const { return AbilityName; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	FText GetAbilityDescriptionText() const { return AbilityDescription; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	FVector GetHealthBarAnchor(float MeshBoundsTopZ, int32 StarLevel) const
	{
		FVector Anchor = HealthBarOffset.IsNearlyZero()
			? FVector(0.f, 0.f, MeshBoundsTopZ + 25.f)
			: HealthBarOffset;
		Anchor.Z += HealthBarZOffsetPerStar * FMath::Max(0, StarLevel - 1);
		return Anchor;
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	int32 GetCostTierValue() const { return CostTier; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	TSoftObjectPtr<UTexture2D> GetPortraitAsset() const { return Portrait; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	TArray<FGameplayTag> GetTraitsArray() const { return Traits; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units|Merge Feedback")
	TArray<FHMT_UnitStarVisual> GetVisualsByStar() const { return VisualsByStar; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units|Merge Feedback")
	FName GetMergeFXSocketName() const { return MergeFXSocketName; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units|Merge Feedback")
	TArray<float> GetScaleByStar() const { return ScaleByStar; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units|Merge Feedback")
	TSoftObjectPtr<UFXSystemAsset> GetMergeFXAsset() const { return FXSet.Merge; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units|Merge Feedback")
	TSoftObjectPtr<USoundBase> GetMergeSFXAsset() const { return AudioSet.Merge; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units|Merge Feedback")
	FHMT_UnitStarVisual GetVisualForStar(int32 StarLevel, bool& bOutFound) const
	{
		const int32 Index = StarLevel - 2;
		bOutFound = VisualsByStar.IsValidIndex(Index);
		return bOutFound ? VisualsByStar[Index] : FHMT_UnitStarVisual();
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units|Merge Feedback")
	TSoftObjectPtr<USkeletalMesh> GetMeshOverrideForStar(int32 StarLevel, bool& bOutFound) const
	{
		const int32 Index = StarLevel - 2;
		bOutFound = VisualsByStar.IsValidIndex(Index) && !VisualsByStar[Index].MeshOverride.IsNull();
		return bOutFound ? VisualsByStar[Index].MeshOverride : nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units|Merge Feedback")
	TArray<TSoftObjectPtr<UMaterialInterface>> GetMaterialOverridesForStar(int32 StarLevel) const
	{
		const int32 Index = StarLevel - 2;
		return VisualsByStar.IsValidIndex(Index) ? VisualsByStar[Index].MaterialOverrides : TArray<TSoftObjectPtr<UMaterialInterface>>();
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	UAnimMontage* GetIdleAnim() const { return AnimSet.Idle; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units|Merge Feedback")
	float GetScaleForStar(int32 StarLevel, bool& bOutFound) const
	{
		const int32 Index = StarLevel - 2;
		bOutFound = ScaleByStar.IsValidIndex(Index);
		return bOutFound ? ScaleByStar[Index] : 1.f;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("HMT_UnitDefinition"), GetFName());
	}
};
