#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "HMT_MatchRulesAsset.generated.h"

class UHMT_CombatResolver;

UCLASS(BlueprintType)
class HMT_CORE_API UHMT_MatchRulesAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match")
	int32 MinPlayers = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match")
	int32 MaxPlayers = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match")
	bool bFillWithBotsWhenMinPlayersUnmet = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match", meta = (EditCondition = "bFillWithBotsWhenMinPlayersUnmet", ClampMin = "0.0"))
	float BotFillWaitSeconds = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|AI", meta = (MustImplement = "/Script/HMT_Core.HMT_BotDecisionStrategy"))
	TSubclassOf<UObject> BotDecisionStrategyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match")
	float PrepPhaseDuration = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match")
	int32 MaxRounds = 40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match")
	int32 StartingPlayerHP = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match")
	int32 BaseRoundDamage = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match")
	int32 DamagePerSurvivor = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match")
	int32 PvERoundInterval = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Match")
	float CombatPhaseDuration = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Stat"))
	FGameplayTag HealthStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Stat"))
	FGameplayTag AttackDamageStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Stat"))
	FGameplayTag ArmorStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Stat"))
	FGameplayTag RangeStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Stat"))
	FGameplayTag MovementSpeedStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Stat"))
	FGameplayTag AttackSpeedStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Stat"))
	FGameplayTag CritChanceStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Stat"))
	FGameplayTag CritDamageStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Stat"))
	FGameplayTag CastTimeStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Status"))
	FGameplayTag StunEffectTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Status"))
	FGameplayTag RootEffectTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Status"))
	FGameplayTag SilenceEffectTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat", meta = (Categories = "Status"))
	FGameplayTag TauntEffectTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat")
	TSubclassOf<UHMT_CombatResolver> CombatResolverClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Merge", meta = (ClampMin = "2"))
	int32 CopiesRequiredForMerge = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Merge", meta = (ClampMin = "1"))
	int32 MaxStarLevel = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	int32 StartingGold = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	int32 ShopSlotCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	int32 RerollCost = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	TArray<int32> GoldCostByTier = { 1, 2, 3, 4, 5 };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	TArray<int32> PoolSizeByTier = { 30, 25, 18, 10, 9 };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	TArray<float> ShopTierOdds = { 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	int32 IncomePerRound = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	int32 InterestDivisor = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	int32 InterestCap = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	TArray<int32> StreakBonusByCount = { 0, 1, 1, 2, 3 };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Economy")
	int32 BaseBoardCapacity = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Modifiers")
	TArray<TObjectPtr<class UHMT_GameModifierDefinition>> AvailableGameModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Modifiers", meta = (ClampMin = "0"))
	int32 ModifiersPerMatch = 2;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	int32 GetRerollCostValue() const { return RerollCost; }

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("HMT_MatchRules"), GetFName());
	}
};
