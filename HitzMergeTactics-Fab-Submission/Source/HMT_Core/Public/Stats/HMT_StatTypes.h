#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "HMT_StatTypes.generated.h"

UENUM(BlueprintType)
enum class EHMT_StatModOp : uint8
{
	Additive,
	PercentAdditive,
	Multiplicative,
	Override
};

UENUM(BlueprintType)
enum class EHMT_ModifierDuration : uint8
{
	Permanent,
	UntilCombatEnd,
	Timed
};

UENUM(BlueprintType)
enum class EHMT_StackingRule : uint8
{
	Stack,
	Refresh,
	Ignore
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_BaseStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Stats", meta = (Categories = "Stat"))
	FGameplayTag StatTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Stats")
	float Value = 0.f;
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_StatModifier : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Stats", meta = (Categories = "Stat"))
	FGameplayTag StatTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Stats")
	EHMT_StatModOp ModOp = EHMT_StatModOp::Additive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Stats")
	float Magnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Stats")
	EHMT_ModifierDuration Duration = EHMT_ModifierDuration::Permanent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Stats")
	float TimedDurationSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Stats")
	EHMT_StackingRule StackingRule = EHMT_StackingRule::Stack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Stats", meta = (Categories = "Source"))
	FGameplayTag SourceTag;

	UPROPERTY()
	int32 ModifierHandle = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_ModifierArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FHMT_StatModifier> Modifiers;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FHMT_StatModifier, FHMT_ModifierArray>(Modifiers, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FHMT_ModifierArray> : public TStructOpsTypeTraitsBase2<FHMT_ModifierArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
