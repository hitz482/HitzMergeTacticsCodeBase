#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Units/IHMT_TargetingStrategy.h"
#include "Units/IHMT_AbilityExecutor.h"
#include "Synergy/IHMT_TraitEffect.h"
#include "HMT_SampleCombatContent.generated.h"

/**
 * Reference combat behaviors exercising the full UHMT_CombatContext surface (AoE, multi-target,
 * forced displacement, status effects, summons, resource costs, live-HP targeting). Assign these
 * classes on a UHMT_UnitDefinition's TargetingStrategyClass / AbilityExecutorClass fields, or a
 * UHMT_TraitDefinition breakpoint's TraitEffectClass — or subclass any of them in Blueprint and
 * tweak the exposed numbers/logic. Sample-layer content, not framework.
 */

/** Targets the squishiest enemy by max Health stat instead of the nearest. */
UCLASS(Blueprintable)
class UHMT_Targeting_LowestMaxHealth : public UObject, public IHMT_TargetingStrategy
{
	GENERATED_BODY()

public:
	virtual AActor* SelectTarget_Implementation(AActor* SelfActor, const TArray<AActor*>& Candidates, UHMT_CombatContext* Context) override;
};

/** Targets whoever is closest to dying RIGHT NOW (live sim health, not max) — an execute/kill-
 *  priority strategy. Only possible because SelectTarget receives Context: prior to that, targeting
 *  strategies could only read a unit's max/base stats, never its actual in-fight health. */
UCLASS(Blueprintable)
class UHMT_Targeting_LowestCurrentHealth : public UObject, public IHMT_TargetingStrategy
{
	GENERATED_BODY()

public:
	virtual AActor* SelectTarget_Implementation(AActor* SelfActor, const TArray<AActor*>& Candidates, UHMT_CombatContext* Context) override;
};

/** Every Nth attack opportunity becomes a cleave: weapon damage x multiplier to the target AND
 *  every enemy within CleaveRadius of it — the minimal AoE/multi-target reference. */
UCLASS(Blueprintable)
class UHMT_Ability_Cleave : public UObject, public IHMT_AbilityExecutor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 AttacksBetweenCasts = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float DamageMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 CleaveRadius = 1;

	virtual bool CanExecuteAbility_Implementation(AActor* SelfActor) override;
	virtual void ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context) override;

private:
	/** Per-combat state is safe here: the resolver news one executor instance per unit per fight. */
	int32 AttackCounter = 0;
};

/** Every Nth attack opportunity: weapon damage x multiplier plus a 1-cell knockback away from
 *  the caster — the forced-displacement reference (emits UnitMoved with bIsForcedDisplacement). */
UCLASS(Blueprintable)
class UHMT_Ability_Knockback : public UObject, public IHMT_AbilityExecutor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 AttacksBetweenCasts = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float DamageMultiplier = 1.f;

	virtual bool CanExecuteAbility_Implementation(AActor* SelfActor) override;
	virtual void ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context) override;

private:
	int32 AttackCounter = 0;
};

/** Every Nth attack opportunity: weapon damage x multiplier plus a Stun on the target — the
 *  status-effect/CC reference. Requires UHMT_MatchRulesAsset::StunEffectTag to be set to whatever
 *  tag this ability applies, or the stun replays visually (StatusApplied event) but doesn't
 *  actually gate the target's turn — see UHMT_CombatResolver's CC-tag fields. */
UCLASS(Blueprintable)
class UHMT_Ability_Stun : public UObject, public IHMT_AbilityExecutor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 AttacksBetweenCasts = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float DamageMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float StunDuration = 1.5f;

	/** The tag this ability applies — must match UHMT_MatchRulesAsset::StunEffectTag to actually
	 *  gate the target's turn; left as its own field (not hardcoded) so the same ability class
	 *  could apply Root/Silence instead just by pointing this at a different configured tag. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample", meta = (Categories = "Status"))
	FGameplayTag EffectTagToApply;

	virtual bool CanExecuteAbility_Implementation(AActor* SelfActor) override;
	virtual void ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context) override;

private:
	int32 AttackCounter = 0;
};

/** Every Nth attack opportunity: summons copies of SummonDefinition on empty neighboring cells
 *  instead of attacking — the summon-primitive reference (UHMT_CombatContext::SpawnUnit). Summon
 *  COUNT scales with the CASTER's own star level (SummonsAtStar1, +SummonsPerCasterStarAbove1 for
 *  each star beyond 1 — e.g. defaults 1/+1 give a 1-star caster 1 summon, 2-star 2, 3-star 3),
 *  so merging the summoner is what grows the summon squad, not the summoned unit's own star.
 *  Summons are pure combat-sim entities (UHMT_CombatContext::SpawnUnit never touches real board/
 *  bench state) — they exist only for this fight and are gone the moment it ends, same as every
 *  other ephemeral combat proxy. */
UCLASS(Blueprintable)
class UHMT_Ability_Summon : public UObject, public IHMT_AbilityExecutor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 AttacksBetweenCasts = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	TObjectPtr<class UHMT_UnitDefinition> SummonDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 SummonStarLevel = 1;

	/** Summons granted at caster star level 1 (the un-merged baseline). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 SummonsAtStar1 = 1;

	/** Additional summons granted per caster star level above 1 — default 1 means star 2 grants 2
	 *  total, star 3 grants 3 total. Set to 0 for a flat, non-scaling summon count. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 SummonsPerCasterStarAbove1 = 1;

	virtual bool CanExecuteAbility_Implementation(AActor* SelfActor) override;
	virtual void ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context) override;

private:
	int32 AttackCounter = 0;

	/** SummonsAtStar1 + SummonsPerCasterStarAbove1 * (CasterStarLevel - 1), floored at 1. Reads the
	 *  caster's UHMT_UnitInstanceComponent directly — SelfActor is the ephemeral combat proxy, but
	 *  it carries the same component setup as any placed unit (see resolver unit spawn). */
	int32 GetSummonCountForCaster(AActor* SelfActor) const;
};

/** A resource-cost ("mana") reference: NOT a hardcoded mana system — mana is just a regular
 *  tag-driven stat (author a "Stat.Mana" tag and give units a base value for it), and this class
 *  is the pattern for spending/gaining it. OnCombatEvent (new on IHMT_AbilityExecutor, mirrors the
 *  trait-effect hook) is what makes "gain mana when I attack or take damage" possible — before it,
 *  an ability executor only ever heard about its OWN attack-opportunity moment. */
UCLASS(Blueprintable)
class UHMT_Ability_ManaCast : public UObject, public IHMT_AbilityExecutor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample", meta = (Categories = "Stat"))
	FGameplayTag ManaStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float ManaCost = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float ManaPerAttack = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float ManaPerDamageTaken = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float DamageMultiplier = 2.f;

	virtual bool CanExecuteAbility_Implementation(AActor* SelfActor) override;
	virtual void ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context) override;
	virtual void OnCombatEvent_Implementation(AActor* SelfActor, const FHMT_CombatEvent& Event, UHMT_CombatContext* Context) override;
};

/** Building-style passive reference: every AttacksBetweenCasts attack-opportunities, grants
 *  ManaPerPulse of ManaStatTag to every ally (including itself) instead of attacking — reuses the
 *  exact same "cast cadence" gate as UHMT_Ability_Summon/Cleave, so a building just needs a Range
 *  large enough to always have SOME enemy "in range" (it never actually hits one) to keep pulsing
 *  all fight. Pair with UHMT_UnitDefinition::bIsBuilding=true and a high Range stat. */
UCLASS(Blueprintable)
class UHMT_Ability_ManaAura : public UObject, public IHMT_AbilityExecutor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 AttacksBetweenCasts = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample", meta = (Categories = "Stat"))
	FGameplayTag ManaStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float ManaPerPulse = 10.f;

	virtual bool CanExecuteAbility_Implementation(AActor* SelfActor) override;
	virtual void ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context) override;

private:
	int32 AttackCounter = 0;
};

/** The REACTIVE trait-effect reference (Guardian synergy): the first time each ally drops below
 *  HealthThresholdFraction of max HP during a fight, heal them for HealAmount. Assign on a
 *  UHMT_TraitDefinition breakpoint's TraitEffectClass (or a Blueprint subclass with tweaked
 *  numbers/logic). Exercises OnCombatStart's side handshake + acting on the sim through
 *  UHMT_CombatContext from an event reaction. */
UCLASS(Blueprintable)
class UHMT_TraitEffect_GuardianHeal : public UObject, public IHMT_TraitEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealthThresholdFraction = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	float HealAmount = 200.f;

	virtual void OnCombatStart_Implementation(int32 OwningSide, UHMT_CombatContext* Context) override;
	virtual void OnCombatEvent_Implementation(const FHMT_CombatEvent& Event, UHMT_CombatContext* Context) override;

private:
	/** Per-fight state is safe: the resolver news one effect instance per side per fight. */
	int32 Side = INDEX_NONE;
	TSet<FGuid> AlreadyHealed;
};
