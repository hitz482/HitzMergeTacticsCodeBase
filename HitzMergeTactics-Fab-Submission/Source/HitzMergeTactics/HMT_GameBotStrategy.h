#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AI/HMT_BotDecisionStrategy.h"
#include "HMT_GameBotStrategy.generated.h"

/**
 * Reference bot brain: trait-aware auto-shopping. Each turn, scores every shop slot by how much it
 * reinforces trait synergies already present on the bot's own board (heavily weighted) plus its cost
 * tier (lightly weighted, as a rough power-level tiebreaker), buys the best-scoring affordable slot,
 * and places it on the first free board cell (bench if the board's full/capped) — repeating until the
 * board+bench are full or nothing in the shop is worth/affordable buying. No reroll, no economy
 * optimization, no positional/formation decisions: intentionally the simplest heuristic that still
 * produces a bot that visibly builds toward its own comps, not scope for a full difficulty system.
 * Assign on UHMT_MatchRulesAsset::BotDecisionStrategyClass, or subclass in Blueprint to retune the
 * weights/behavior.
 */
UCLASS(Blueprintable)
class UHMT_GameBotStrategy : public UObject, public IHMT_BotDecisionStrategy
{
	GENERATED_BODY()

public:
	/** Score contribution per already-on-board unit sharing a trait with the candidate shop unit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float TraitOverlapWeight = 10.f;

	/** Score contribution per cost tier — a light tiebreaker, not the primary decision driver. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float CostTierWeight = 1.f;

	virtual void TakeTurn_Implementation(APlayerState* SelfPlayerState, int32 RoundNumber) override;
};
