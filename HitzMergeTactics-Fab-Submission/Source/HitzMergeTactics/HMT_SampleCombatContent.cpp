#include "HMT_SampleCombatContent.h"
#include "HMT_GameState.h"
#include "Combat/HMT_CombatContext.h"
#include "Match/HMT_MatchStateComponent.h"
#include "Economy/HMT_MatchRulesAsset.h"
#include "Stats/IHMT_StatProvider.h"
#include "Units/HMT_UnitDefinition.h"
#include "Units/HMT_UnitInstanceComponent.h"
#include "Engine/World.h"

namespace
{
	/** Stat tags are buyer-authored content — read them off the active MatchRules, same source the
	 *  resolver itself uses, rather than guessing tag names in code. */
	const UHMT_MatchRulesAsset* GetActiveMatchRules(const AActor* AnyWorldActor)
	{
		const UWorld* World = AnyWorldActor ? AnyWorldActor->GetWorld() : nullptr;
		const AHMT_GameState* GameState = World ? World->GetGameState<AHMT_GameState>() : nullptr;
		return GameState && GameState->MatchStateComponent ? GameState->MatchStateComponent->GetMatchRules() : nullptr;
	}

	float GetStat(AActor* Unit, const FGameplayTag& Tag)
	{
		UActorComponent* Provider = Unit ? Unit->FindComponentByInterface(UHMT_StatProvider::StaticClass()) : nullptr;
		return Provider ? IHMT_StatProvider::Execute_GetFinalStatValue(Provider, Tag) : 0.f;
	}
}

AActor* UHMT_Targeting_LowestMaxHealth::SelectTarget_Implementation(AActor* SelfActor, const TArray<AActor*>& Candidates, UHMT_CombatContext* Context)
{
	const UHMT_MatchRulesAsset* Rules = GetActiveMatchRules(SelfActor);
	if (!Rules || Candidates.Num() == 0)
	{
		return Candidates.Num() > 0 ? Candidates[0] : nullptr;
	}

	AActor* Best = nullptr;
	float BestHealth = 0.f;
	for (AActor* Candidate : Candidates)
	{
		const float Health = GetStat(Candidate, Rules->HealthStatTag);
		if (!Best || Health < BestHealth)
		{
			Best = Candidate;
			BestHealth = Health;
		}
	}
	return Best;
}

AActor* UHMT_Targeting_LowestCurrentHealth::SelectTarget_Implementation(AActor* SelfActor, const TArray<AActor*>& Candidates, UHMT_CombatContext* Context)
{
	if (!Context || Candidates.Num() == 0)
	{
		return Candidates.Num() > 0 ? Candidates[0] : nullptr;
	}

	AActor* Best = nullptr;
	float BestHealth = 0.f;
	for (AActor* Candidate : Candidates)
	{
		const float Health = Context->GetCurrentHealth(Candidate);
		if (!Best || Health < BestHealth)
		{
			Best = Candidate;
			BestHealth = Health;
		}
	}
	return Best;
}

bool UHMT_Ability_Cleave::CanExecuteAbility_Implementation(AActor* SelfActor)
{
	if (AttackCounter++ < AttacksBetweenCasts)
	{
		return false;
	}
	AttackCounter = 0;
	return true;
}

void UHMT_Ability_Cleave::ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context)
{
	const UHMT_MatchRulesAsset* Rules = GetActiveMatchRules(SelfActor);
	if (!Rules || !Context || !Target)
	{
		return;
	}

	const float Damage = GetStat(SelfActor, Rules->AttackDamageStatTag) * DamageMultiplier;

	// Primary target plus every OTHER enemy of ours within CleaveRadius of it. GetEnemies gives
	// "enemies of self"; distance is measured from the primary target (the cleave's center).
	Context->DealDamage(SelfActor, Target, Damage, /*bApplyArmor*/ true);
	for (AActor* Enemy : Context->GetEnemies(SelfActor))
	{
		if (Enemy == Target)
		{
			continue;
		}
		const int32 Distance = Context->GetDistanceBetween(Target, Enemy);
		if (Distance >= 0 && Distance <= CleaveRadius)
		{
			Context->DealDamage(SelfActor, Enemy, Damage, /*bApplyArmor*/ true);
		}
	}
}

bool UHMT_Ability_Knockback::CanExecuteAbility_Implementation(AActor* SelfActor)
{
	if (AttackCounter++ < AttacksBetweenCasts)
	{
		return false;
	}
	AttackCounter = 0;
	return true;
}

void UHMT_Ability_Knockback::ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context)
{
	const UHMT_MatchRulesAsset* Rules = GetActiveMatchRules(SelfActor);
	if (!Rules || !Context || !Target)
	{
		return;
	}

	Context->DealDamage(SelfActor, Target, GetStat(SelfActor, Rules->AttackDamageStatTag) * DamageMultiplier, /*bApplyArmor*/ true);

	// Push the target one cell AWAY from the caster: of the target's free neighbor cells, take the
	// one that maximizes distance from self. No free cell that increases distance = no push.
	bool bFoundSelf = false;
	bool bFoundTarget = false;
	const FHMT_GridCoord SelfCoord = Context->GetUnitCoord(SelfActor, bFoundSelf);
	const FHMT_GridCoord TargetCoord = Context->GetUnitCoord(Target, bFoundTarget);
	if (!bFoundSelf || !bFoundTarget)
	{
		return;
	}

	FHMT_GridCoord BestCell = TargetCoord;
	int32 BestDistance = Context->GetGridDistance(SelfCoord, TargetCoord);
	for (const FHMT_GridCoord& Neighbor : Context->GetNeighborCoords(TargetCoord))
	{
		if (!Context->IsCellFree(Neighbor))
		{
			continue;
		}
		const int32 Distance = Context->GetGridDistance(SelfCoord, Neighbor);
		if (Distance > BestDistance)
		{
			BestDistance = Distance;
			BestCell = Neighbor;
		}
	}

	if (!(BestCell == TargetCoord))
	{
		Context->MoveUnit(Target, BestCell);
	}
}

bool UHMT_Ability_Stun::CanExecuteAbility_Implementation(AActor* SelfActor)
{
	if (AttackCounter++ < AttacksBetweenCasts)
	{
		return false;
	}
	AttackCounter = 0;
	return true;
}

void UHMT_Ability_Stun::ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context)
{
	const UHMT_MatchRulesAsset* Rules = GetActiveMatchRules(SelfActor);
	if (!Rules || !Context || !Target)
	{
		return;
	}

	Context->DealDamage(SelfActor, Target, GetStat(SelfActor, Rules->AttackDamageStatTag) * DamageMultiplier, /*bApplyArmor*/ true);
	if (EffectTagToApply.IsValid())
	{
		Context->ApplyStatusEffect(SelfActor, Target, EffectTagToApply, StunDuration);
	}
}

bool UHMT_Ability_Summon::CanExecuteAbility_Implementation(AActor* SelfActor)
{
	if (AttackCounter++ < AttacksBetweenCasts)
	{
		return false;
	}
	AttackCounter = 0;
	return true;
}

void UHMT_Ability_Summon::ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context)
{
	if (!Context || !SummonDefinition)
	{
		return;
	}

	bool bFoundSelf = false;
	const FHMT_GridCoord SelfCoord = Context->GetUnitCoord(SelfActor, bFoundSelf);
	if (!bFoundSelf)
	{
		return;
	}

	const int32 DesiredCount = GetSummonCountForCaster(SelfActor);

	// Breadth-first ring search outward from the caster, widening past the immediate neighbors
	// only if the summon count needs more room than a single ring provides. IsCellFree only
	// checks already-resolved units (see UHMT_CombatContext::IsCellFree) — it can't see anything
	// spawned earlier in THIS SAME call (SpawnUnit stages into PendingSpawns, not drained until
	// after this tick), so cells claimed this call are tracked locally to avoid stacking summons.
	TArray<FHMT_GridCoord> ClaimedThisCall;
	TSet<FHMT_GridCoord> Visited = { SelfCoord };
	TArray<FHMT_GridCoord> Frontier = { SelfCoord };

	while (ClaimedThisCall.Num() < DesiredCount && Frontier.Num() > 0)
	{
		TArray<FHMT_GridCoord> NextFrontier;
		for (const FHMT_GridCoord& Cell : Frontier)
		{
			for (const FHMT_GridCoord& Neighbor : Context->GetNeighborCoords(Cell))
			{
				if (Visited.Contains(Neighbor))
				{
					continue;
				}
				Visited.Add(Neighbor);
				NextFrontier.Add(Neighbor);

				if (ClaimedThisCall.Num() < DesiredCount && Context->IsCellFree(Neighbor) && !ClaimedThisCall.Contains(Neighbor))
				{
					ClaimedThisCall.Add(Neighbor);
					Context->SpawnUnit(SelfActor, SummonDefinition, SummonStarLevel, Neighbor);
				}
			}
		}
		Frontier = MoveTemp(NextFrontier);
	}
	// Fewer summons than desired if the battlefield is too crowded — degrading gracefully to
	// "as many as fit" rather than failing the cast outright.
}

int32 UHMT_Ability_Summon::GetSummonCountForCaster(AActor* SelfActor) const
{
	const UHMT_UnitInstanceComponent* Instance = SelfActor ? SelfActor->FindComponentByClass<UHMT_UnitInstanceComponent>() : nullptr;
	const int32 CasterStarLevel = Instance ? Instance->GetStarLevel() : 1;
	return FMath::Max(1, SummonsAtStar1 + SummonsPerCasterStarAbove1 * FMath::Max(0, CasterStarLevel - 1));
}

bool UHMT_Ability_ManaCast::CanExecuteAbility_Implementation(AActor* SelfActor)
{
	return GetStat(SelfActor, ManaStatTag) >= ManaCost;
}

void UHMT_Ability_ManaCast::ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context)
{
	const UHMT_MatchRulesAsset* Rules = GetActiveMatchRules(SelfActor);
	if (!Rules || !Context || !Target)
	{
		return;
	}

	Context->DealDamage(SelfActor, Target, GetStat(SelfActor, Rules->AttackDamageStatTag) * DamageMultiplier, /*bApplyArmor*/ true);

	// Spend mana by setting the base stat directly — this is a resource POOL, not a combat modifier
	// (no duration/stacking semantics apply), so IHMT_StatProvider::SetBaseStatValue is the right
	// call, not ApplyStatModifier.
	UActorComponent* StatProvider = SelfActor ? SelfActor->FindComponentByInterface(UHMT_StatProvider::StaticClass()) : nullptr;
	if (StatProvider)
	{
		IHMT_StatProvider::Execute_SetBaseStatValue(StatProvider, ManaStatTag, GetStat(SelfActor, ManaStatTag) - ManaCost);
	}
}

void UHMT_Ability_ManaCast::OnCombatEvent_Implementation(AActor* SelfActor, const FHMT_CombatEvent& Event, UHMT_CombatContext* Context)
{
	if (!ManaStatTag.IsValid())
	{
		return;
	}

	UActorComponent* StatProvider = SelfActor ? SelfActor->FindComponentByInterface(UHMT_StatProvider::StaticClass()) : nullptr;
	if (!StatProvider)
	{
		return;
	}

	float ManaGain = 0.f;
	if (Event.EventType == EHMT_CombatEventType::AttackStart && Event.SourceInstanceId.IsValid())
	{
		// Only OUR OWN attack grants mana here — Context->FindUnitByInstanceId + comparing against
		// SelfActor is how a reactive hook (which receives EVERY unit's events) filters to "did I
		// personally do this," the same pattern UHMT_TraitEffect_GuardianHeal uses for its target-side check.
		if (Context && Context->FindUnitByInstanceId(Event.SourceInstanceId) == SelfActor)
		{
			ManaGain = ManaPerAttack;
		}
	}
	else if (Event.EventType == EHMT_CombatEventType::DamageDealt && Event.Magnitude > 0.f && Event.TargetInstanceId.IsValid())
	{
		if (Context && Context->FindUnitByInstanceId(Event.TargetInstanceId) == SelfActor)
		{
			ManaGain = Event.Magnitude * ManaPerDamageTaken;
		}
	}

	if (ManaGain > 0.f)
	{
		const float NewMana = FMath::Min(GetStat(SelfActor, ManaStatTag) + ManaGain, ManaCost);
		IHMT_StatProvider::Execute_SetBaseStatValue(StatProvider, ManaStatTag, NewMana);
	}
}

bool UHMT_Ability_ManaAura::CanExecuteAbility_Implementation(AActor* SelfActor)
{
	if (AttackCounter++ < AttacksBetweenCasts)
	{
		return false;
	}
	AttackCounter = 0;
	return true;
}

void UHMT_Ability_ManaAura::ExecuteAbility_Implementation(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context)
{
	if (!Context || !ManaStatTag.IsValid() || !SelfActor)
	{
		return;
	}

	// Pulse mana into the whole side, including the building itself, rather than dealing damage to
	// Target — Target only exists here because CanExecuteAbility fires through the same in-range
	// gate every attacking unit uses; a mana building ignores it entirely.
	TArray<AActor*> Recipients = Context->GetAllies(SelfActor);
	Recipients.Add(SelfActor);

	for (AActor* Ally : Recipients)
	{
		UActorComponent* StatProvider = Ally ? Ally->FindComponentByInterface(UHMT_StatProvider::StaticClass()) : nullptr;
		if (StatProvider)
		{
			IHMT_StatProvider::Execute_SetBaseStatValue(StatProvider, ManaStatTag, GetStat(Ally, ManaStatTag) + ManaPerPulse);
		}
	}
}

void UHMT_TraitEffect_GuardianHeal::OnCombatStart_Implementation(int32 OwningSide, UHMT_CombatContext* Context)
{
	Side = OwningSide;
	AlreadyHealed.Reset();
}

void UHMT_TraitEffect_GuardianHeal::OnCombatEvent_Implementation(const FHMT_CombatEvent& Event, UHMT_CombatContext* Context)
{
	// Only real damage (positive magnitude) can trip the heal — our own Heal emits a negative-
	// magnitude DamageDealt, so this guard is also what prevents infinite reaction loops.
	if (Event.EventType != EHMT_CombatEventType::DamageDealt || Event.Magnitude <= 0.f || !Context || Side == INDEX_NONE)
	{
		return;
	}
	if (AlreadyHealed.Contains(Event.TargetInstanceId))
	{
		return;
	}

	AActor* Target = Context->FindUnitByInstanceId(Event.TargetInstanceId);
	if (!Target || Context->GetUnitSide(Target) != Side)
	{
		return;
	}

	const float CurrentHealth = Context->GetCurrentHealth(Target);
	const float MaxHealth = Context->GetMaxHealth(Target);
	if (CurrentHealth <= 0.f || MaxHealth <= 0.f || CurrentHealth >= MaxHealth * HealthThresholdFraction)
	{
		return;
	}

	AlreadyHealed.Add(Event.TargetInstanceId);
	Context->Heal(nullptr, Target, HealAmount);
}
