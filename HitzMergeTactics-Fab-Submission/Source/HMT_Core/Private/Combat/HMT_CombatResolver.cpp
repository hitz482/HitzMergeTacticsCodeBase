#include "Combat/HMT_CombatResolver.h"
#include "Combat/HMT_CombatUnitActor.h"
#include "Combat/HMT_CombatContext.h"
#include "Grid/HMT_HexGridLayout.h"
#include "Grid/HMT_SquareGridLayout.h"
#include "Units/IHMT_TargetingStrategy.h"
#include "Units/IHMT_AbilityExecutor.h"
#include "Units/HMT_UnitInstanceComponent.h"
#include "Synergy/IHMT_TraitEffect.h"
#include "Ruler/HMT_RulerPassive.h"
#include "Modifiers/IHMT_ModifierEffect.h"
#include "Tiles/IHMT_TileEffect.h"
#include "Stats/IHMT_StatProvider.h"
#include "Stats/HMT_StatComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"

namespace
{
	using FCombatUnitState = FHMT_ResolverUnitState;

	bool IsCoordOccupiedByLivingUnit(const TArray<FCombatUnitState>& AllUnits, const FHMT_GridCoord& Coord, const FCombatUnitState& Ignoring)
	{
		for (const FCombatUnitState& Unit : AllUnits)
		{
			if (Unit.IsAlive() && Unit.InstanceId != Ignoring.InstanceId && Unit.Coord == Coord)
			{
				return true;
			}
		}
		return false;
	}

	FHMT_GridCoord GetNextStepToward(const TScriptInterface<IHMT_GridLayout>& GridLayout, const FCombatUnitState& Mover, const FHMT_GridCoord& Target, const TArray<FCombatUnitState>& AllUnits)
	{
		UObject* Layout = GridLayout.GetObject();
		const TArray<FHMT_GridCoord> Neighbors = IHMT_GridLayout::Execute_GetNeighbors(Layout, Mover.Coord);

		FHMT_GridCoord Best = Mover.Coord;
		int32 BestDistance = IHMT_GridLayout::Execute_GetDistance(Layout, Mover.Coord, Target);
		bool bFoundUnoccupied = false;

		for (const FHMT_GridCoord& Candidate : Neighbors)
		{
			if (IsCoordOccupiedByLivingUnit(AllUnits, Candidate, Mover))
			{
				continue;
			}

			const int32 CandidateDistance = IHMT_GridLayout::Execute_GetDistance(Layout, Candidate, Target);
			if (!bFoundUnoccupied || CandidateDistance < BestDistance)
			{
				Best = Candidate;
				BestDistance = CandidateDistance;
				bFoundUnoccupied = true;
			}
		}

		return Best;
	}

}

float UHMT_CombatResolver::ComputeAttackDamage_Implementation(AActor* Attacker, AActor* Defender, bool& bOutCritical)
{
	bOutCritical = false;

	UActorComponent* AttackerStats = Attacker ? Attacker->FindComponentByInterface(UHMT_StatProvider::StaticClass()) : nullptr;
	UActorComponent* DefenderStats = Defender ? Defender->FindComponentByInterface(UHMT_StatProvider::StaticClass()) : nullptr;
	if (!AttackerStats || !DefenderStats)
	{
		return 0.f;
	}

	float Damage = IHMT_StatProvider::Execute_GetFinalStatValue(AttackerStats, AttackDamageStatTag);

	if (CritChanceStatTag.IsValid())
	{
		const float CritChancePercent = IHMT_StatProvider::Execute_GetFinalStatValue(AttackerStats, CritChanceStatTag);
		if (CritChancePercent > 0.f && FMath::FRand() * 100.f < CritChancePercent)
		{
			bOutCritical = true;
			float CritMultiplier = 1.5f;
			if (CritDamageStatTag.IsValid())
			{
				const float StatMultiplier = IHMT_StatProvider::Execute_GetFinalStatValue(AttackerStats, CritDamageStatTag);
				if (StatMultiplier > 0.f)
				{
					CritMultiplier = StatMultiplier;
				}
			}
			Damage *= CritMultiplier;
		}
	}

	const float Armor = IHMT_StatProvider::Execute_GetFinalStatValue(DefenderStats, ArmorStatTag);
	return Damage * (100.f / (100.f + FMath::Max(Armor, 0.f)));
}

FHMT_CombatEventLog UHMT_CombatResolver::ResolveCombat(UObject* WorldContextObject, const FHMT_BoardSnapshot& SideA, const FHMT_BoardSnapshot& SideB)
{
	FHMT_CombatEventLog Log;

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return Log;
	}

	TScriptInterface<IHMT_GridLayout> GridLayout;
	if (SideA.Topology == EHMT_GridTopology::Hex)
	{
		UHMT_HexGridLayout* Hex = NewObject<UHMT_HexGridLayout>(this);
		Hex->CellSize = SideA.CellSize;
		GridLayout = TScriptInterface<IHMT_GridLayout>(Hex);
	}
	else
	{
		UHMT_SquareGridLayout* Square = NewObject<UHMT_SquareGridLayout>(this);
		Square->CellSize = SideA.CellSize;
		GridLayout = TScriptInterface<IHMT_GridLayout>(Square);
	}

	TArray<FCombatUnitState> AllUnits;
	TArray<TPair<UObject*, int32>> ReactiveTraitEffects;

	TArray<TPair<UObject*, APlayerState*>> ReactiveRulerPassives;
	TArray<TPair<UObject*, APlayerState*>> ReactiveModifierEffects;

	struct FActiveResolverTile
	{
		FHMT_GridCoord ArenaCoord;
		UObject* EffectInstance = nullptr;
	};
	TArray<FActiveResolverTile> ActiveTiles;

	const int32 ArenaColumns = HMT_CombatArena::GetArenaColumns(SideA, SideB);
	const int32 ArenaRows = HMT_CombatArena::GetArenaRows(SideA, SideB);

	auto SpawnSide = [&](const FHMT_BoardSnapshot& Snapshot, int32 Side)
	{
		for (const FHMT_SnapshotUnit& SnapshotUnit : Snapshot.Units)
		{
			if (!SnapshotUnit.UnitDefinition)
			{
				continue;
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AHMT_CombatUnitActor* Actor = World->SpawnActor<AHMT_CombatUnitActor>(SpawnParams);
			if (!Actor)
			{
				continue;
			}

			Actor->UnitInstanceComponent->InitializeUnit(SnapshotUnit.UnitDefinition, SnapshotUnit.StarLevel);

			for (const FHMT_StatModifier& Modifier : SnapshotUnit.ActiveModifiers)
			{
				IHMT_StatProvider::Execute_ApplyStatModifier(Actor->StatComponent,
					Modifier.StatTag, Modifier.ModOp, Modifier.Magnitude, Modifier.Duration,
					Modifier.TimedDurationSeconds, Modifier.StackingRule, Modifier.SourceTag);
			}

			FCombatUnitState State;
			State.Actor = Actor;
			State.Coord = Side == 0 ? SnapshotUnit.Coord : HMT_CombatArena::MirrorAcrossArena(SnapshotUnit.Coord, ArenaColumns, ArenaRows);
			State.Side = Side;
			State.InstanceId = SnapshotUnit.InstanceId;
			State.CurrentHealth = IHMT_StatProvider::Execute_GetFinalStatValue(Actor->StatComponent, HealthStatTag);
			if (State.CurrentHealth <= 0.f)
			{
				UE_LOG(LogTemp, Warning, TEXT("[HMT Combat] %s spawned with %.0f HP (side %d) — check its BaseStats row uses the exact HealthStatTag from MatchRules."),
					*SnapshotUnit.UnitDefinition->UnitID.ToString(), State.CurrentHealth, Side);
			}

			if (SnapshotUnit.UnitDefinition->TargetingStrategyClass)
			{
				State.TargetingStrategy = NewObject<UObject>(this, SnapshotUnit.UnitDefinition->TargetingStrategyClass);
			}
			if (SnapshotUnit.UnitDefinition->AbilityExecutorClass)
			{
				State.AbilityExecutor = NewObject<UObject>(this, SnapshotUnit.UnitDefinition->AbilityExecutorClass);
			}

			AllUnits.Add(State);
		}

		for (const TSubclassOf<UObject>& EffectClass : Snapshot.ActiveTraitEffectClasses)
		{
			if (EffectClass)
			{
				ReactiveTraitEffects.Add(TPair<UObject*, int32>(NewObject<UObject>(this, EffectClass), Side));
			}
		}

		if (Snapshot.ActiveRulerPassiveClass)
		{
			ReactiveRulerPassives.Add(TPair<UObject*, APlayerState*>(NewObject<UObject>(this, Snapshot.ActiveRulerPassiveClass), Snapshot.OwningPlayer));
		}
		for (const TSubclassOf<UObject>& ModifierClass : Snapshot.ActiveModifierEffectClasses)
		{
			if (ModifierClass)
			{
				ReactiveModifierEffects.Add(TPair<UObject*, APlayerState*>(NewObject<UObject>(this, ModifierClass), Snapshot.OwningPlayer));
			}
		}

		for (const FHMT_TileModifier& Tile : Snapshot.TileModifiers)
		{
			if (Tile.TileEffectClass)
			{
				FActiveResolverTile ActiveTile;
				ActiveTile.ArenaCoord = Side == 0 ? Tile.Coord : HMT_CombatArena::MirrorAcrossArena(Tile.Coord, ArenaColumns, ArenaRows);
				ActiveTile.EffectInstance = NewObject<UObject>(this, Tile.TileEffectClass);
				ActiveTiles.Add(ActiveTile);
			}
		}
	};

	SpawnSide(SideA, 0);
	SpawnSide(SideB, 1);

	UHMT_CombatContext* Context = NewObject<UHMT_CombatContext>(this);
	Context->Units = &AllUnits;
	Context->GridLayoutObject = GridLayout.GetObject();
	Context->Resolver = this;
	Context->World = World;
	Context->ArenaColumns = ArenaColumns;
	Context->ArenaRows = ArenaRows;

	auto EmitEvent = [&](const FHMT_CombatEvent& Event)
	{
		Log.Events.Add(Event);
		for (const TPair<UObject*, int32>& Effect : ReactiveTraitEffects)
		{
			IHMT_TraitEffect::Execute_OnCombatEvent(Effect.Key, Event, Context);
		}
		for (const TPair<UObject*, APlayerState*>& Ruler : ReactiveRulerPassives)
		{
			if (Ruler.Key->Implements<UHMT_RulerPassive>())
			{
				IHMT_RulerPassive::Execute_OnCombatEvent(Ruler.Key, Ruler.Value, Event);
			}
		}
		for (const TPair<UObject*, APlayerState*>& Modifier : ReactiveModifierEffects)
		{
			if (Modifier.Key->Implements<UHMT_ModifierEffect>())
			{
				IHMT_ModifierEffect::Execute_OnCombatEvent(Modifier.Key, Modifier.Value, Event, Context);
			}
		}
		for (FCombatUnitState& Listener : AllUnits)
		{
			if (Listener.IsAlive() && Listener.AbilityExecutor.IsValid())
			{
				IHMT_AbilityExecutor::Execute_OnCombatEvent(Listener.AbilityExecutor.Get(), Listener.Actor.Get(), Event, Context);
			}
		}
	};
	Context->EmitEvent = EmitEvent;

	for (const TPair<UObject*, int32>& Effect : ReactiveTraitEffects)
	{
		IHMT_TraitEffect::Execute_OnCombatStart(Effect.Key, Effect.Value, Context);
	}
	for (const TPair<UObject*, APlayerState*>& Modifier : ReactiveModifierEffects)
	{
		if (Modifier.Key->Implements<UHMT_ModifierEffect>())
		{
			IHMT_ModifierEffect::Execute_OnCombatStart(Modifier.Key, Modifier.Value, Context);
		}
	}

	auto AnySideAlive = [&AllUnits](int32 Side)
	{
		for (const FCombatUnitState& Unit : AllUnits)
		{
			if (Unit.Side == Side && Unit.IsAlive())
			{
				return true;
			}
		}
		return false;
	};

	float ElapsedTime = 0.f;
	while (ElapsedTime < MaxCombatDuration && AnySideAlive(0) && AnySideAlive(1))
	{
		for (FCombatUnitState& Unit : AllUnits)
		{
			if (!Unit.IsAlive())
			{
				continue;
			}

			for (const FActiveResolverTile& Tile : ActiveTiles)
			{
				if (Tile.EffectInstance && Tile.ArenaCoord == Unit.Coord && Tile.EffectInstance->Implements<UHMT_TileEffect>())
				{
					Context->CurrentSimTime = ElapsedTime;
					IHMT_TileEffect::Execute_OnUnitOnTile(Tile.EffectInstance, Unit.Actor.Get(), Context);
				}
			}

			if (Unit.bIsCasting)
			{
				if (StunEffectTag.IsValid() && Unit.HasStatus(StunEffectTag))
				{
					Unit.bIsCasting = false;
					Unit.BusyUntilTime = ElapsedTime;

					FHMT_CombatEvent InterruptEvent;
					InterruptEvent.EventType = EHMT_CombatEventType::CastInterrupted;
					InterruptEvent.Timestamp = ElapsedTime;
					InterruptEvent.SourceInstanceId = Unit.InstanceId;
					EmitEvent(InterruptEvent);
					continue;
				}

				if (ElapsedTime < Unit.CastEndTime)
				{
					continue;
				}

				Unit.bIsCasting = false;

				FCombatUnitState* CastTarget = AllUnits.FindByPredicate([&Unit](const FCombatUnitState& Candidate)
				{
					return Candidate.Actor == Unit.CurrentTarget;
				});

				if (CastTarget && CastTarget->IsAlive())
				{
					FHMT_CombatEvent AbilityEvent;
					AbilityEvent.EventType = EHMT_CombatEventType::AbilityCast;
					AbilityEvent.Timestamp = ElapsedTime;
					AbilityEvent.SourceInstanceId = Unit.InstanceId;
					AbilityEvent.TargetInstanceId = CastTarget->InstanceId;
					EmitEvent(AbilityEvent);

					Context->CurrentSimTime = ElapsedTime;
					IHMT_AbilityExecutor::Execute_ExecuteAbility(Unit.AbilityExecutor.Get(), Unit.Actor.Get(), CastTarget->Actor.Get(), Context);
				}

				continue;
			}

			if (ElapsedTime < Unit.BusyUntilTime)
			{
				continue;
			}

			for (auto It = Unit.ActiveStatusEffects.CreateIterator(); It; ++It)
			{
				if (It->Value <= ElapsedTime)
				{
					const FGameplayTag ExpiredTag = It->Key;
					It.RemoveCurrent();
					if (ExpiredTag == TauntEffectTag)
					{
						Unit.TauntedBy = nullptr;
					}
					FHMT_CombatEvent ExpiredEvent;
					ExpiredEvent.EventType = EHMT_CombatEventType::StatusExpired;
					ExpiredEvent.Timestamp = ElapsedTime;
					ExpiredEvent.TargetInstanceId = Unit.InstanceId;
					ExpiredEvent.EffectTag = ExpiredTag;
					EmitEvent(ExpiredEvent);
				}
			}

			if (StunEffectTag.IsValid() && Unit.HasStatus(StunEffectTag))
			{
				continue;
			}

			if (TauntEffectTag.IsValid() && Unit.TauntedBy.IsValid() && Unit.HasStatus(TauntEffectTag))
			{
				Unit.CurrentTarget = Unit.TauntedBy;
			}
			else if (!Unit.CurrentTarget.IsValid())
			{
				TArray<AActor*> EnemyActors;
				FCombatUnitState* NearestEnemy = nullptr;
				int32 NearestDistance = MAX_int32;

				for (FCombatUnitState& Candidate : AllUnits)
				{
					if (Candidate.Side == Unit.Side || !Candidate.IsAlive())
					{
						continue;
					}
					EnemyActors.Add(Candidate.Actor.Get());

					const int32 Distance = IHMT_GridLayout::Execute_GetDistance(GridLayout.GetObject(), Unit.Coord, Candidate.Coord);
					if (!NearestEnemy || Distance < NearestDistance)
					{
						NearestEnemy = &Candidate;
						NearestDistance = Distance;
					}
				}

				if (Unit.TargetingStrategy.IsValid())
				{
					AActor* Chosen = IHMT_TargetingStrategy::Execute_SelectTarget(Unit.TargetingStrategy.Get(), Unit.Actor.Get(), EnemyActors, Context);
					for (FCombatUnitState& Candidate : AllUnits)
					{
						if (Candidate.Actor.Get() == Chosen)
						{
							Unit.CurrentTarget = Candidate.Actor;
							break;
						}
					}
				}
				else if (NearestEnemy)
				{
					Unit.CurrentTarget = NearestEnemy->Actor;
				}
			}

			if (!Unit.CurrentTarget.IsValid())
			{
				continue;
			}

			FCombatUnitState* Target = AllUnits.FindByPredicate([&Unit](const FCombatUnitState& Candidate)
			{
				return Candidate.Actor == Unit.CurrentTarget;
			});

			if (!Target || !Target->IsAlive())
			{
				Unit.CurrentTarget.Reset();
				continue;
			}

			const int32 Distance = IHMT_GridLayout::Execute_GetDistance(GridLayout.GetObject(), Unit.Coord, Target->Coord);
			const float Range = IHMT_StatProvider::Execute_GetFinalStatValue(Unit.Actor->StatComponent, RangeStatTag);

			if (Distance <= Range)
			{
				if (ElapsedTime < Unit.NextAttackTime)
				{
					continue;
				}
				const float AttackSpeed = AttackSpeedStatTag.IsValid()
					? IHMT_StatProvider::Execute_GetFinalStatValue(Unit.Actor->StatComponent, AttackSpeedStatTag)
					: 0.f;
				Unit.NextAttackTime = AttackSpeed > 0.f ? ElapsedTime + (1.f / AttackSpeed) : ElapsedTime;

				const bool bSilenced = SilenceEffectTag.IsValid() && Unit.HasStatus(SilenceEffectTag);
				const bool bCanUseAbility = !bSilenced && Unit.AbilityExecutor.IsValid()
					&& IHMT_AbilityExecutor::Execute_CanExecuteAbility(Unit.AbilityExecutor.Get(), Unit.Actor.Get());

				if (bCanUseAbility)
				{
					const float CastTime = CastTimeStatTag.IsValid()
						? IHMT_StatProvider::Execute_GetFinalStatValue(Unit.Actor->StatComponent, CastTimeStatTag)
						: 0.f;

					if (CastTime > 0.f)
					{
						Unit.bIsCasting = true;
						Unit.CastEndTime = ElapsedTime + CastTime;
						Unit.BusyUntilTime = Unit.CastEndTime;

						FHMT_CombatEvent CastStartEvent;
						CastStartEvent.EventType = EHMT_CombatEventType::CastStart;
						CastStartEvent.Timestamp = ElapsedTime;
						CastStartEvent.SourceInstanceId = Unit.InstanceId;
						CastStartEvent.TargetInstanceId = Target->InstanceId;
						CastStartEvent.CastDuration = CastTime;
						EmitEvent(CastStartEvent);
					}
					else
					{
						FHMT_CombatEvent AbilityEvent;
						AbilityEvent.EventType = EHMT_CombatEventType::AbilityCast;
						AbilityEvent.Timestamp = ElapsedTime;
						AbilityEvent.SourceInstanceId = Unit.InstanceId;
						AbilityEvent.TargetInstanceId = Target->InstanceId;
						EmitEvent(AbilityEvent);

						Context->CurrentSimTime = ElapsedTime;
						IHMT_AbilityExecutor::Execute_ExecuteAbility(Unit.AbilityExecutor.Get(), Unit.Actor.Get(), Target->Actor.Get(), Context);
					}
				}
				else
				{
					FHMT_CombatEvent AttackEvent;
					AttackEvent.EventType = EHMT_CombatEventType::AttackStart;
					AttackEvent.Timestamp = ElapsedTime;
					AttackEvent.SourceInstanceId = Unit.InstanceId;
					AttackEvent.TargetInstanceId = Target->InstanceId;
					AttackEvent.ProjectileTravelTime = Distance > 1 ? FMath::Min(Distance * 0.05f, TickInterval) : 0.f;
					EmitEvent(AttackEvent);

					bool bCritical = false;
					const float Damage = ComputeAttackDamage(Unit.Actor.Get(), Target->Actor.Get(), bCritical);
					Target->CurrentHealth -= Damage;

					FHMT_CombatEvent DamageEvent;
					DamageEvent.EventType = EHMT_CombatEventType::DamageDealt;
					DamageEvent.Timestamp = ElapsedTime + AttackEvent.ProjectileTravelTime;
					DamageEvent.SourceInstanceId = Unit.InstanceId;
					DamageEvent.TargetInstanceId = Target->InstanceId;
					DamageEvent.Magnitude = Damage;
					DamageEvent.bIsCritical = bCritical;
					EmitEvent(DamageEvent);

					if (Target->CurrentHealth <= 0.f)
					{
						FHMT_CombatEvent DeathEvent;
						DeathEvent.EventType = EHMT_CombatEventType::UnitDeath;
						DeathEvent.Timestamp = ElapsedTime + AttackEvent.ProjectileTravelTime;
						DeathEvent.SourceInstanceId = Unit.InstanceId;
						DeathEvent.TargetInstanceId = Target->InstanceId;
						EmitEvent(DeathEvent);
					}
				}
			}
			else if (RootEffectTag.IsValid() && Unit.HasStatus(RootEffectTag))
			{
			}
			else
			{
				const FHMT_GridCoord NextCoord = GetNextStepToward(GridLayout, Unit, Target->Coord, AllUnits);
				if (NextCoord != Unit.Coord)
				{
					const float MovementSpeed = FMath::Max(IHMT_StatProvider::Execute_GetFinalStatValue(Unit.Actor->StatComponent, MovementSpeedStatTag), KINDA_SMALL_NUMBER);
					const float MoveDuration = FMath::Max(1.f / MovementSpeed, TickInterval);

					FHMT_CombatEvent MoveEvent;
					MoveEvent.EventType = EHMT_CombatEventType::UnitMoved;
					MoveEvent.Timestamp = ElapsedTime;
					MoveEvent.SourceInstanceId = Unit.InstanceId;
					MoveEvent.FromCoord = Unit.Coord;
					MoveEvent.ToCoord = NextCoord;
					MoveEvent.MoveDuration = MoveDuration;
					EmitEvent(MoveEvent);

					Unit.Coord = NextCoord;
					Unit.BusyUntilTime = ElapsedTime + MoveDuration;
				}
			}
		}

		if (Context->PendingSpawns.Num() > 0)
		{
			AllUnits.Append(Context->PendingSpawns);
			Context->PendingSpawns.Reset();
		}

		ElapsedTime += TickInterval;
	}

	FHMT_CombatEvent EndEvent;
	EndEvent.EventType = EHMT_CombatEventType::CombatEnd;
	EndEvent.Timestamp = ElapsedTime;
	const bool bSideAAlive = AnySideAlive(0);
	const bool bSideBAlive = AnySideAlive(1);
	if (bSideAAlive != bSideBAlive)
	{
		EndEvent.Magnitude = bSideAAlive ? 0.f : 1.f;
	}
	else if (!bSideAAlive)
	{
		EndEvent.Magnitude = -1.f;
	}
	else
	{
		int32 SurvivorCount[2] = { 0, 0 };
		float SurvivorHealth[2] = { 0.f, 0.f };
		for (const FCombatUnitState& Unit : AllUnits)
		{
			if (Unit.IsAlive())
			{
				SurvivorCount[Unit.Side] += 1;
				SurvivorHealth[Unit.Side] += Unit.CurrentHealth;
			}
		}
		if (SurvivorCount[0] != SurvivorCount[1])
		{
			EndEvent.Magnitude = SurvivorCount[0] > SurvivorCount[1] ? 0.f : 1.f;
		}
		else if (!FMath::IsNearlyEqual(SurvivorHealth[0], SurvivorHealth[1]))
		{
			EndEvent.Magnitude = SurvivorHealth[0] > SurvivorHealth[1] ? 0.f : 1.f;
		}
		else
		{
			EndEvent.Magnitude = -1.f;
		}
	}
	EmitEvent(EndEvent);

	for (FCombatUnitState& Unit : AllUnits)
	{
		if (Unit.Actor.IsValid())
		{
			Unit.Actor->Destroy();
		}
	}

	return Log;
}
