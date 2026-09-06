#include "Combat/HMT_CombatContext.h"
#include "Combat/HMT_CombatResolver.h"
#include "Combat/HMT_CombatUnitActor.h"
#include "Stats/IHMT_StatProvider.h"
#include "Stats/HMT_StatComponent.h"
#include "Units/HMT_UnitInstanceComponent.h"
#include "Units/HMT_UnitDefinition.h"
#include "Engine/World.h"

FHMT_ResolverUnitState* UHMT_CombatContext::FindState(AActor* Unit) const
{
	if (!Units || !Unit)
	{
		return nullptr;
	}
	return Units->FindByPredicate([Unit](const FHMT_ResolverUnitState& State) { return State.Actor.Get() == Unit; });
}

TArray<AActor*> UHMT_CombatContext::GetEnemies(AActor* SelfActor) const
{
	TArray<AActor*> Result;
	const FHMT_ResolverUnitState* Self = FindState(SelfActor);
	if (!Self || !Units)
	{
		return Result;
	}

	for (const FHMT_ResolverUnitState& State : *Units)
	{
		if (State.IsAlive() && State.Side != Self->Side && State.Actor.IsValid())
		{
			Result.Add(State.Actor.Get());
		}
	}
	return Result;
}

TArray<AActor*> UHMT_CombatContext::GetAllies(AActor* SelfActor) const
{
	TArray<AActor*> Result;
	const FHMT_ResolverUnitState* Self = FindState(SelfActor);
	if (!Self || !Units)
	{
		return Result;
	}

	for (const FHMT_ResolverUnitState& State : *Units)
	{
		if (State.IsAlive() && State.Side == Self->Side && State.Actor.Get() != SelfActor && State.Actor.IsValid())
		{
			Result.Add(State.Actor.Get());
		}
	}
	return Result;
}

FHMT_GridCoord UHMT_CombatContext::GetUnitCoord(AActor* Unit, bool& bOutFound) const
{
	const FHMT_ResolverUnitState* State = FindState(Unit);
	bOutFound = State != nullptr;
	return State ? State->Coord : FHMT_GridCoord();
}

int32 UHMT_CombatContext::GetDistanceBetween(AActor* UnitA, AActor* UnitB) const
{
	const FHMT_ResolverUnitState* StateA = FindState(UnitA);
	const FHMT_ResolverUnitState* StateB = FindState(UnitB);
	if (!StateA || !StateB || !GridLayoutObject)
	{
		return -1;
	}
	return IHMT_GridLayout::Execute_GetDistance(GridLayoutObject, StateA->Coord, StateB->Coord);
}

TArray<AActor*> UHMT_CombatContext::GetUnitsInRangeOf(AActor* CenterUnit, int32 Range, bool bEnemiesOfCenterOnly) const
{
	TArray<AActor*> Result;
	const FHMT_ResolverUnitState* Center = FindState(CenterUnit);
	if (!Center || !Units || !GridLayoutObject)
	{
		return Result;
	}

	for (const FHMT_ResolverUnitState& State : *Units)
	{
		if (!State.IsAlive() || State.Actor.Get() == CenterUnit || !State.Actor.IsValid())
		{
			continue;
		}
		if (bEnemiesOfCenterOnly && State.Side == Center->Side)
		{
			continue;
		}
		if (IHMT_GridLayout::Execute_GetDistance(GridLayoutObject, Center->Coord, State.Coord) <= Range)
		{
			Result.Add(State.Actor.Get());
		}
	}
	return Result;
}

float UHMT_CombatContext::GetCurrentHealth(AActor* Unit) const
{
	const FHMT_ResolverUnitState* State = FindState(Unit);
	return State ? State->CurrentHealth : 0.f;
}

float UHMT_CombatContext::GetMaxHealth(AActor* Unit) const
{
	const FHMT_ResolverUnitState* State = FindState(Unit);
	if (!State || !State->Actor.IsValid() || !Resolver)
	{
		return 0.f;
	}
	return IHMT_StatProvider::Execute_GetFinalStatValue(State->Actor->StatComponent, Resolver->HealthStatTag);
}

AActor* UHMT_CombatContext::FindUnitByInstanceId(FGuid InstanceId) const
{
	if (!Units)
	{
		return nullptr;
	}
	const FHMT_ResolverUnitState* State = Units->FindByPredicate(
		[&InstanceId](const FHMT_ResolverUnitState& Candidate) { return Candidate.InstanceId == InstanceId; });
	return State && State->Actor.IsValid() ? State->Actor.Get() : nullptr;
}

int32 UHMT_CombatContext::GetUnitSide(AActor* Unit) const
{
	const FHMT_ResolverUnitState* State = FindState(Unit);
	return State ? State->Side : INDEX_NONE;
}

void UHMT_CombatContext::DealDamage(AActor* SourceUnit, AActor* TargetUnit, float Damage, bool bApplyArmor)
{
	FHMT_ResolverUnitState* Source = FindState(SourceUnit);
	FHMT_ResolverUnitState* Target = FindState(TargetUnit);
	if (!Target || !Target->IsAlive() || Damage <= 0.f || !EmitEvent)
	{
		return;
	}

	float FinalDamage = Damage;
	if (bApplyArmor && Resolver && Target->Actor.IsValid())
	{
		const float Armor = IHMT_StatProvider::Execute_GetFinalStatValue(Target->Actor->StatComponent, Resolver->ArmorStatTag);
		FinalDamage = Damage * (100.f / (100.f + FMath::Max(Armor, 0.f)));
	}

	Target->CurrentHealth -= FinalDamage;

	FHMT_CombatEvent DamageEvent;
	DamageEvent.EventType = EHMT_CombatEventType::DamageDealt;
	DamageEvent.Timestamp = CurrentSimTime;
	DamageEvent.SourceInstanceId = Source ? Source->InstanceId : FGuid();
	DamageEvent.TargetInstanceId = Target->InstanceId;
	DamageEvent.Magnitude = FinalDamage;
	EmitEvent(DamageEvent);

	if (Target->CurrentHealth <= 0.f)
	{
		FHMT_CombatEvent DeathEvent;
		DeathEvent.EventType = EHMT_CombatEventType::UnitDeath;
		DeathEvent.Timestamp = CurrentSimTime;
		DeathEvent.SourceInstanceId = Source ? Source->InstanceId : FGuid();
		DeathEvent.TargetInstanceId = Target->InstanceId;
		EmitEvent(DeathEvent);
	}
}

void UHMT_CombatContext::Heal(AActor* SourceUnit, AActor* TargetUnit, float Amount)
{
	FHMT_ResolverUnitState* Source = FindState(SourceUnit);
	FHMT_ResolverUnitState* Target = FindState(TargetUnit);
	if (!Target || !Target->IsAlive() || Amount <= 0.f || !EmitEvent)
	{
		return;
	}

	float MaxHealth = Target->CurrentHealth;
	if (Resolver && Target->Actor.IsValid())
	{
		MaxHealth = IHMT_StatProvider::Execute_GetFinalStatValue(Target->Actor->StatComponent, Resolver->HealthStatTag);
	}
	const float Healed = FMath::Min(Amount, FMath::Max(MaxHealth - Target->CurrentHealth, 0.f));
	if (Healed <= 0.f)
	{
		return;
	}

	Target->CurrentHealth += Healed;

	FHMT_CombatEvent HealEvent;
	HealEvent.EventType = EHMT_CombatEventType::DamageDealt;
	HealEvent.Timestamp = CurrentSimTime;
	HealEvent.SourceInstanceId = Source ? Source->InstanceId : FGuid();
	HealEvent.TargetInstanceId = Target->InstanceId;
	HealEvent.Magnitude = -Healed;
	EmitEvent(HealEvent);
}

bool UHMT_CombatContext::IsCellFree(FHMT_GridCoord Coord) const
{
	if (Coord.Q < 0 || Coord.Q >= ArenaColumns || Coord.R < 0 || Coord.R >= ArenaRows || !Units)
	{
		return false;
	}
	for (const FHMT_ResolverUnitState& State : *Units)
	{
		if (State.IsAlive() && State.Coord == Coord)
		{
			return false;
		}
	}
	return true;
}

TArray<FHMT_GridCoord> UHMT_CombatContext::GetNeighborCoords(FHMT_GridCoord Coord) const
{
	return GridLayoutObject ? IHMT_GridLayout::Execute_GetNeighbors(GridLayoutObject, Coord) : TArray<FHMT_GridCoord>();
}

int32 UHMT_CombatContext::GetGridDistance(FHMT_GridCoord CoordA, FHMT_GridCoord CoordB) const
{
	return GridLayoutObject ? IHMT_GridLayout::Execute_GetDistance(GridLayoutObject, CoordA, CoordB) : -1;
}

bool UHMT_CombatContext::MoveUnit(AActor* Unit, FHMT_GridCoord ToCoord)
{
	FHMT_ResolverUnitState* State = FindState(Unit);
	if (!State || !State->IsAlive() || !EmitEvent || !IsCellFree(ToCoord))
	{
		return false;
	}

	FHMT_CombatEvent MoveEvent;
	MoveEvent.EventType = EHMT_CombatEventType::UnitMoved;
	MoveEvent.Timestamp = CurrentSimTime;
	MoveEvent.SourceInstanceId = State->InstanceId;
	MoveEvent.FromCoord = State->Coord;
	MoveEvent.ToCoord = ToCoord;
	MoveEvent.MoveDuration = 0.15f;
	MoveEvent.bIsForcedDisplacement = true;
	EmitEvent(MoveEvent);

	State->Coord = ToCoord;
	State->BusyUntilTime = FMath::Max(State->BusyUntilTime, CurrentSimTime + MoveEvent.MoveDuration);
	return true;
}

void UHMT_CombatContext::ApplyStatusEffect(AActor* SourceUnit, AActor* TargetUnit, FGameplayTag EffectTag, float DurationSeconds)
{
	FHMT_ResolverUnitState* Target = FindState(TargetUnit);
	if (!Target || !Target->IsAlive() || !EffectTag.IsValid() || !EmitEvent)
	{
		return;
	}

	const float ExpireAt = DurationSeconds > 0.f ? CurrentSimTime + DurationSeconds : CurrentSimTime + (Resolver ? Resolver->MaxCombatDuration : 999.f) + 1.f;
	Target->ActiveStatusEffects.Add(EffectTag, ExpireAt);

	if (Resolver && EffectTag == Resolver->TauntEffectTag)
	{
		const FHMT_ResolverUnitState* SourceState = FindState(SourceUnit);
		Target->TauntedBy = SourceState ? SourceState->Actor : nullptr;
		Target->CurrentTarget = Target->TauntedBy;
	}

	FHMT_CombatEvent StatusEvent;
	StatusEvent.EventType = EHMT_CombatEventType::StatusApplied;
	StatusEvent.Timestamp = CurrentSimTime;
	StatusEvent.SourceInstanceId = FindState(SourceUnit) ? FindState(SourceUnit)->InstanceId : FGuid();
	StatusEvent.TargetInstanceId = Target->InstanceId;
	StatusEvent.EffectTag = EffectTag;
	EmitEvent(StatusEvent);
}

void UHMT_CombatContext::RemoveStatusEffect(AActor* Unit, FGameplayTag EffectTag)
{
	FHMT_ResolverUnitState* State = FindState(Unit);
	if (!State || !State->ActiveStatusEffects.Remove(EffectTag) || !EmitEvent)
	{
		return;
	}

	if (Resolver && EffectTag == Resolver->TauntEffectTag)
	{
		State->TauntedBy = nullptr;
	}

	FHMT_CombatEvent StatusEvent;
	StatusEvent.EventType = EHMT_CombatEventType::StatusExpired;
	StatusEvent.Timestamp = CurrentSimTime;
	StatusEvent.TargetInstanceId = State->InstanceId;
	StatusEvent.EffectTag = EffectTag;
	EmitEvent(StatusEvent);
}

bool UHMT_CombatContext::HasStatusEffect(AActor* Unit, FGameplayTag EffectTag) const
{
	const FHMT_ResolverUnitState* State = FindState(Unit);
	return State && State->HasStatus(EffectTag);
}

AActor* UHMT_CombatContext::SpawnUnit(AActor* Summoner, UHMT_UnitDefinition* Definition, int32 StarLevel, FHMT_GridCoord Coord)
{
	const FHMT_ResolverUnitState* SummonerState = FindState(Summoner);
	if (!SummonerState || !Definition || !Units || !World || !EmitEvent || !IsCellFree(Coord))
	{
		return nullptr;
	}
	const bool bPendingCollision = PendingSpawns.ContainsByPredicate([&Coord](const FHMT_ResolverUnitState& Pending) { return Pending.Coord == Coord; });
	if (bPendingCollision)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AHMT_CombatUnitActor* Actor = World->SpawnActor<AHMT_CombatUnitActor>(SpawnParams);
	if (!Actor)
	{
		return nullptr;
	}

	Actor->UnitInstanceComponent->InitializeUnit(Definition, StarLevel);

	FHMT_ResolverUnitState NewState;
	NewState.Actor = Actor;
	NewState.Coord = Coord;
	NewState.Side = SummonerState->Side;
	NewState.InstanceId = FGuid::NewGuid();
	NewState.CurrentHealth = Resolver ? IHMT_StatProvider::Execute_GetFinalStatValue(Actor->StatComponent, Resolver->HealthStatTag) : 0.f;
	if (Definition->TargetingStrategyClass)
	{
		NewState.TargetingStrategy = NewObject<UObject>(Actor, Definition->TargetingStrategyClass);
	}
	if (Definition->AbilityExecutorClass)
	{
		NewState.AbilityExecutor = NewObject<UObject>(Actor, Definition->AbilityExecutorClass);
	}
	PendingSpawns.Add(NewState);

	FHMT_CombatEvent SpawnEvent;
	SpawnEvent.EventType = EHMT_CombatEventType::UnitSpawned;
	SpawnEvent.Timestamp = CurrentSimTime;
	SpawnEvent.SourceInstanceId = SummonerState->InstanceId;
	SpawnEvent.TargetInstanceId = NewState.InstanceId;
	SpawnEvent.ToCoord = Coord;
	SpawnEvent.SpawnedUnitDefinition = Definition;
	SpawnEvent.SpawnedStarLevel = StarLevel;
	EmitEvent(SpawnEvent);

	return Actor;
}
