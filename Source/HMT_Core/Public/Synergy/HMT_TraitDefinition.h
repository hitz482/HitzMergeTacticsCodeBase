#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Stats/HMT_StatTypes.h"
#include "HMT_TraitDefinition.generated.h"

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_TraitStatGrant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Synergy", meta = (Categories = "Stat"))
	FGameplayTag StatTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Synergy")
	EHMT_StatModOp ModOp = EHMT_StatModOp::Additive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Synergy")
	float Magnitude = 0.f;
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_TraitBreakpoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Synergy")
	int32 RequiredCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Synergy")
	TArray<FHMT_TraitStatGrant> StatGrants;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Synergy", meta = (MustImplement = "/Script/HMT_Core.HMT_TraitEffect"))
	TSubclassOf<UObject> TraitEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Synergy")
	FText Description;
};

UCLASS(BlueprintType)
class HMT_CORE_API UHMT_TraitDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Synergy", meta = (Categories = "Trait"))
	FGameplayTag TraitTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Synergy")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Synergy")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Synergy")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Synergy")
	TArray<FHMT_TraitBreakpoint> Breakpoints;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	int32 GetHighestMetBreakpointIndex(int32 Count) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	FText GetDisplayNameText() const { return DisplayName; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	int32 GetBreakpointRequiredCount(int32 BreakpointIndex) const { return Breakpoints.IsValidIndex(BreakpointIndex) ? Breakpoints[BreakpointIndex].RequiredCount : 0; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	FGameplayTag GetTraitTagValue() const { return TraitTag; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	TSoftObjectPtr<UTexture2D> GetIconAsset() const { return Icon; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	FText GetDescriptionText() const { return Description; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	FText GetBreakpointDescription(int32 BreakpointIndex) const { return Breakpoints.IsValidIndex(BreakpointIndex) ? Breakpoints[BreakpointIndex].Description : FText::GetEmpty(); }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	int32 GetBreakpointCount() const { return Breakpoints.Num(); }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	FText GetTierListText() const
	{
		TArray<FString> Lines;
		if (!Description.IsEmpty())
		{
			Lines.Add(Description.ToString());
		}
		for (const FHMT_TraitBreakpoint& Breakpoint : Breakpoints)
		{
			Lines.Add(FString::Printf(TEXT("(%d)  %s"), Breakpoint.RequiredCount, *Breakpoint.Description.ToString()));
		}
		return FText::FromString(FString::Join(Lines, TEXT("\n")));
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	FText GetTooltipText() const
	{
		TArray<FString> Lines;
		Lines.Add(DisplayName.ToString());
		if (!Description.IsEmpty())
		{
			Lines.Add(Description.ToString());
		}
		for (const FHMT_TraitBreakpoint& Breakpoint : Breakpoints)
		{
			Lines.Add(FString::Printf(TEXT("(%d)  %s"), Breakpoint.RequiredCount, *Breakpoint.Description.ToString()));
		}
		return FText::FromString(FString::Join(Lines, TEXT("\n")));
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("HMT_TraitDefinition"), GetFName());
	}
};
