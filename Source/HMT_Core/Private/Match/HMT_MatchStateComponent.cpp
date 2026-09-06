#include "Match/HMT_MatchStateComponent.h"
#include "Grid/HMT_BoardComponent.h"
#include "Units/HMT_UnitInstanceComponent.h"
#include "Units/HMT_UnitCollectionComponent.h"
#include "Synergy/HMT_SynergyManagerComponent.h"
#include "Economy/HMT_ShopComponent.h"
#include "Combat/HMT_CombatPlaybackComponent.h"
#include "Stats/IHMT_StatProvider.h"
#include "Ruler/HMT_RulerDefinition.h"
#include "Ruler/HMT_RulerPassive.h"
#include "Modifiers/IHMT_ModifierEffect.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"

UHMT_MatchStateComponent::UHMT_MatchStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHMT_MatchStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHMT_MatchStateComponent, CurrentPhase);
	DOREPLIFETIME(UHMT_MatchStateComponent, PhaseTimeRemaining);
	DOREPLIFETIME(UHMT_MatchStateComponent, PhaseEndServerTime);
	DOREPLIFETIME(UHMT_MatchStateComponent, RoundNumber);
	DOREPLIFETIME(UHMT_MatchStateComponent, CurrentRoundType);
	DOREPLIFETIME(UHMT_MatchStateComponent, Players);
	DOREPLIFETIME(UHMT_MatchStateComponent, MatchRules);
	DOREPLIFETIME(UHMT_MatchStateComponent, ActiveGameModifiers);
}

void UHMT_MatchStateComponent::StartMatch(UHMT_MatchRulesAsset* Rules, UHMT_ShopPoolComponent* Pool, const TArray<APlayerState*>& InPlayers, const TArray<UHMT_PvERoundDefinition*>& InPvEPool, const TSet<APlayerState*>& InBotPlayers, const TArray<UHMT_RulerDefinition*>& InAvailableRulers)
{
	if (!Rules)
	{
		return;
	}

	MatchRules = Rules;
	ShopPool = Pool;
	PvERoundPool = InPvEPool;
	UClass* ResolverClass = Rules->CombatResolverClass ? Rules->CombatResolverClass.Get() : UHMT_CombatResolver::StaticClass();
	CombatResolver = NewObject<UHMT_CombatResolver>(this, ResolverClass);

	CombatResolver->HealthStatTag = Rules->HealthStatTag;
	CombatResolver->AttackDamageStatTag = Rules->AttackDamageStatTag;
	CombatResolver->ArmorStatTag = Rules->ArmorStatTag;
	CombatResolver->RangeStatTag = Rules->RangeStatTag;
	CombatResolver->MovementSpeedStatTag = Rules->MovementSpeedStatTag;
	CombatResolver->AttackSpeedStatTag = Rules->AttackSpeedStatTag;
	CombatResolver->CritChanceStatTag = Rules->CritChanceStatTag;
	CombatResolver->CritDamageStatTag = Rules->CritDamageStatTag;
	CombatResolver->CastTimeStatTag = Rules->CastTimeStatTag;
	CombatResolver->StunEffectTag = Rules->StunEffectTag;
	CombatResolver->RootEffectTag = Rules->RootEffectTag;
	CombatResolver->SilenceEffectTag = Rules->SilenceEffectTag;
	CombatResolver->TauntEffectTag = Rules->TauntEffectTag;

	TArray<FString> UnsetTags;
	if (!CombatResolver->HealthStatTag.IsValid()) UnsetTags.Add(TEXT("HealthStatTag"));
	if (!CombatResolver->AttackDamageStatTag.IsValid()) UnsetTags.Add(TEXT("AttackDamageStatTag"));
	if (!CombatResolver->ArmorStatTag.IsValid()) UnsetTags.Add(TEXT("ArmorStatTag"));
	if (!CombatResolver->RangeStatTag.IsValid()) UnsetTags.Add(TEXT("RangeStatTag"));
	if (!CombatResolver->MovementSpeedStatTag.IsValid()) UnsetTags.Add(TEXT("MovementSpeedStatTag"));
	if (!CombatResolver->AttackSpeedStatTag.IsValid()) UnsetTags.Add(TEXT("AttackSpeedStatTag"));
	if (UnsetTags.Num() > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[HMT Combat] MatchRules has UNSET tag fields: %s — every unit will spawn at 0 HP and combat will resolve as an instant draw. Open DA_MatchRules, Combat category, and assign these."),
			*FString::Join(UnsetTags, TEXT(", ")));
	}

	Players.Reset();
	for (APlayerState* Player : InPlayers)
	{
		if (!Player)
		{
			continue;
		}

		FHMT_PlayerMatchState State;
		State.Player = Player;
		State.HP = Rules->StartingPlayerHP;
		State.bEliminated = false;
		State.bIsBotControlled = InBotPlayers.Contains(Player);

		if (InAvailableRulers.Num() > 0)
		{
			State.RulerDefinition = InAvailableRulers[FMath::RandHelper(InAvailableRulers.Num())];
			if (State.RulerDefinition->StartingHPOverride > 0)
			{
				State.HP = State.RulerDefinition->StartingHPOverride;
			}
		}
		Players.Add(State);
	}

	for (const FHMT_PlayerMatchState& PlayerState : Players)
	{
		if (PlayerState.RulerDefinition && PlayerState.RulerDefinition->RulerPassiveClass && PlayerState.Player)
		{
			UObject* PassiveInstance = NewObject<UObject>(PlayerState.Player, PlayerState.RulerDefinition->RulerPassiveClass);
			RulerPassiveInstances.Add(PlayerState.Player, PassiveInstance);
			if (PassiveInstance->Implements<UHMT_RulerPassive>())
			{
				IHMT_RulerPassive::Execute_OnMatchStart(PassiveInstance, PlayerState.Player);
			}
		}
	}

	ActiveGameModifiers.Reset();
	TArray<UHMT_GameModifierDefinition*> ModifierPool = Rules->AvailableGameModifiers;
	const int32 RollCount = FMath::Min(Rules->ModifiersPerMatch, ModifierPool.Num());
	for (int32 Index = 0; Index < RollCount; ++Index)
	{
		const int32 PickIndex = FMath::RandHelper(ModifierPool.Num());
		ActiveGameModifiers.Add(ModifierPool[PickIndex]);
		ModifierPool.RemoveAtSwap(PickIndex);
	}
	ActivateModifiersForTrigger(EHMT_ModifierTriggerType::OnMatchStart);
	ActivateModifiersForTrigger(EHMT_ModifierTriggerType::Persistent);

	RoundNumber = 0;
	BeginPreparationPhase();
}

void UHMT_MatchStateComponent::ActivateModifiersForTrigger(EHMT_ModifierTriggerType TriggerType)
{
	for (UHMT_GameModifierDefinition* Modifier : ActiveGameModifiers)
	{
		if (!Modifier || !Modifier->ModifierEffectClass || Modifier->TriggerType != TriggerType)
		{
			continue;
		}

		for (const FHMT_PlayerMatchState& PlayerState : Players)
		{
			if (PlayerState.bEliminated || !PlayerState.Player)
			{
				continue;
			}

			TArray<TObjectPtr<UObject>>& PlayerInstances = ModifierEffectInstances.FindOrAdd(PlayerState.Player).Instances;
			UObject* EffectInstance = nullptr;
			for (UObject* Existing : PlayerInstances)
			{
				if (Existing && Existing->GetClass() == Modifier->ModifierEffectClass)
				{
					EffectInstance = Existing;
					break;
				}
			}
			if (!EffectInstance)
			{
				EffectInstance = NewObject<UObject>(PlayerState.Player, Modifier->ModifierEffectClass);
				PlayerInstances.Add(EffectInstance);
			}
			if (EffectInstance->Implements<UHMT_ModifierEffect>())
			{
				IHMT_ModifierEffect::Execute_OnActivate(EffectInstance, PlayerState.Player);
			}
		}
	}
}

void UHMT_MatchStateComponent::BeginPreparationPhase()
{
	CurrentPhase = EHMT_MatchPhase::Preparation;
	++RoundNumber;
	CurrentRoundType = (MatchRules->PvERoundInterval > 0 && RoundNumber % MatchRules->PvERoundInterval == 0)
		? EHMT_RoundType::PvE
		: EHMT_RoundType::PvP;
	PhaseTimeRemaining = MatchRules->PrepPhaseDuration;
	if (const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		PhaseEndServerTime = GameState->GetServerWorldTimeSeconds() + MatchRules->PrepPhaseDuration;
	}

	if (ShopPool)
	{
		for (const FHMT_PlayerMatchState& PlayerState : Players)
		{
			if (!PlayerState.bEliminated && PlayerState.Player)
			{
				if (UHMT_ShopComponent* Shop = PlayerState.Player->FindComponentByClass<UHMT_ShopComponent>())
				{
					Shop->RollShop(ShopPool);
				}
			}
		}
	}

	for (const auto& PassivePair : RulerPassiveInstances)
	{
		if (PassivePair.Value && PassivePair.Value->Implements<UHMT_RulerPassive>())
		{
			IHMT_RulerPassive::Execute_OnRoundStart(PassivePair.Value, PassivePair.Key, RoundNumber);
		}
	}

	ActivateModifiersForTrigger(EHMT_ModifierTriggerType::OnRoundStart);

	OnRoundStart.Broadcast(RoundNumber);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PhaseTimerHandle, this, &UHMT_MatchStateComponent::BeginCombatLockPhase, PhaseTimeRemaining, false);
	}
}

void UHMT_MatchStateComponent::ForceEndPreparationPhase()
{
	if (CurrentPhase != EHMT_MatchPhase::Preparation)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
	}

	BeginCombatLockPhase();
}

void UHMT_MatchStateComponent::BeginCombatLockPhase()
{
	CurrentPhase = EHMT_MatchPhase::CombatLock;

	for (const FHMT_PlayerMatchState& PlayerState : Players)
	{
		if (PlayerState.bEliminated || !PlayerState.Player)
		{
			continue;
		}
		LastKnownSnapshot.Add(PlayerState.Player, BuildSnapshotForPlayer(PlayerState.Player));
	}

	BeginCombatPhase();
}

void UHMT_MatchStateComponent::BeginCombatPhase()
{
	CurrentPhase = EHMT_MatchPhase::Combat;

	float LongestLogDuration = 0.f;

	if (CurrentRoundType == EHMT_RoundType::PvE)
	{
		if (PvERoundPool.Num() > 0)
		{
			UHMT_PvERoundDefinition* Encounter = PvERoundPool[FMath::RandRange(0, PvERoundPool.Num() - 1)];
			for (const FHMT_PlayerMatchState& PlayerState : Players)
			{
				if (!PlayerState.bEliminated && PlayerState.Player)
				{
					LongestLogDuration = FMath::Max(LongestLogDuration, ResolvePvERound(PlayerState.Player, Encounter));
				}
			}
		}
	}
	else
	{
		for (const FHMT_RoundPairing& Pairing : BuildPairings())
		{
			LongestLogDuration = FMath::Max(LongestLogDuration, ResolvePairing(Pairing));
		}
	}

	constexpr float ResultsBufferSeconds = 2.f;
	constexpr float MinCombatPhaseSeconds = 3.f;
	float EffectiveDuration = FMath::Max(LongestLogDuration + ResultsBufferSeconds, MinCombatPhaseSeconds);
	if (MatchRules->CombatPhaseDuration > 0.f)
	{
		EffectiveDuration = FMath::Min(EffectiveDuration, MatchRules->CombatPhaseDuration);
	}

	PhaseTimeRemaining = EffectiveDuration;
	if (const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		PhaseEndServerTime = GameState->GetServerWorldTimeSeconds() + EffectiveDuration;
	}
	if (UWorld* World = GetWorld(); World && MatchRules->CombatPhaseDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(PhaseTimerHandle, this, &UHMT_MatchStateComponent::BeginResultsPhase, EffectiveDuration, false);
	}
	else
	{
		BeginResultsPhase();
	}
}

TArray<UHMT_MatchStateComponent::FHMT_RoundPairing> UHMT_MatchStateComponent::BuildPairings() const
{
	TArray<APlayerState*> Live;
	for (const FHMT_PlayerMatchState& PlayerState : Players)
	{
		if (!PlayerState.bEliminated && PlayerState.Player)
		{
			Live.Add(PlayerState.Player);
		}
	}

	for (int32 Index = Live.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		Live.Swap(Index, SwapIndex);
	}

	TArray<FHMT_RoundPairing> Pairings;
	TArray<APlayerState*> Unpaired = Live;

	while (Unpaired.Num() >= 2)
	{
		APlayerState* PlayerA = Unpaired[0];
		Unpaired.RemoveAt(0);

		const TMap<TWeakObjectPtr<APlayerState>, int32>* History = RoundLastPaired.Find(PlayerA);
		int32 PartnerIndex = 0;
		int32 BestRecency = MAX_int32;
		for (int32 Index = 0; Index < Unpaired.Num(); ++Index)
		{
			const int32* LastRound = History ? History->Find(Unpaired[Index]) : nullptr;
			const int32 Recency = LastRound ? *LastRound : MIN_int32;
			if (Recency < BestRecency)
			{
				BestRecency = Recency;
				PartnerIndex = Index;
			}
		}

		FHMT_RoundPairing Pairing;
		Pairing.PlayerA = PlayerA;
		Pairing.PlayerB = Unpaired[PartnerIndex];
		Unpaired.RemoveAt(PartnerIndex);
		Pairings.Add(Pairing);
	}

	if (Unpaired.Num() == 1)
	{
		FHMT_RoundPairing GhostPairing;
		GhostPairing.PlayerA = Unpaired[0];
		GhostPairing.PlayerB = nullptr;
		Pairings.Add(GhostPairing);
	}

	return Pairings;
}

float UHMT_MatchStateComponent::ResolvePairing(const FHMT_RoundPairing& Pairing)
{
	APlayerState* PlayerA = Pairing.PlayerA.Get();
	if (!PlayerA || !CombatResolver)
	{
		return 0.f;
	}

	const FHMT_BoardSnapshot SnapshotA = LastKnownSnapshot.FindRef(PlayerA);
	FHMT_BoardSnapshot SnapshotB;

	if (FHMT_PlayerMatchState* PlayerAState = FindPlayerState(PlayerA))
	{
		PlayerAState->CurrentOpponent = nullptr;
	}

	APlayerState* PlayerB = Pairing.PlayerB.Get();
	if (PlayerB)
	{
		SnapshotB = LastKnownSnapshot.FindRef(PlayerB);
		RoundLastPaired.FindOrAdd(PlayerA).Add(PlayerB, RoundNumber);
		RoundLastPaired.FindOrAdd(PlayerB).Add(PlayerA, RoundNumber);

		if (FHMT_PlayerMatchState* PlayerAState = FindPlayerState(PlayerA))
		{
			PlayerAState->CurrentOpponent = PlayerB;
		}
		if (FHMT_PlayerMatchState* PlayerBState = FindPlayerState(PlayerB))
		{
			PlayerBState->CurrentOpponent = PlayerA;
		}
	}
	else
	{
		TArray<APlayerState*> Candidates;
		for (const TPair<TObjectPtr<APlayerState>, FHMT_BoardSnapshot>& Entry : LastKnownSnapshot)
		{
			if (Entry.Key != PlayerA)
			{
				Candidates.Add(Entry.Key);
			}
		}
		if (Candidates.Num() > 0)
		{
			SnapshotB = LastKnownSnapshot.FindRef(Candidates[FMath::RandRange(0, Candidates.Num() - 1)]);
		}
		else if (PvERoundPool.Num() > 0)
		{
			UHMT_PvERoundDefinition* BotEncounter = PvERoundPool[FMath::RandRange(0, PvERoundPool.Num() - 1)];
			SnapshotB.Topology = SnapshotA.Topology;
			SnapshotB.CellSize = SnapshotA.CellSize;
			SnapshotB.Columns = SnapshotA.Columns;
			SnapshotB.Rows = SnapshotA.Rows;
			for (const FHMT_SnapshotUnit& Template : BotEncounter->EncounterUnits)
			{
				FHMT_SnapshotUnit Unit = Template;
				Unit.InstanceId = FGuid::NewGuid();
				SnapshotB.Units.Add(Unit);
			}
		}
	}

	const FHMT_CombatEventLog Log = CombatResolver->ResolveCombat(this, SnapshotA, SnapshotB);

	if (UHMT_CombatPlaybackComponent* PlaybackA = PlayerA->FindComponentByClass<UHMT_CombatPlaybackComponent>())
	{
		PlaybackA->SetCombatLog(Log, SnapshotA, SnapshotB,  true);
	}
	if (PlayerB)
	{
		if (UHMT_CombatPlaybackComponent* PlaybackB = PlayerB->FindComponentByClass<UHMT_CombatPlaybackComponent>())
		{
			PlaybackB->SetCombatLog(Log, SnapshotA, SnapshotB,  false);
		}
	}

	float Winner = -1.f;
	TSet<FGuid> DeadIds;
	for (const FHMT_CombatEvent& Event : Log.Events)
	{
		if (Event.EventType == EHMT_CombatEventType::CombatEnd)
		{
			Winner = Event.Magnitude;
		}
		else if (Event.EventType == EHMT_CombatEventType::UnitDeath)
		{
			DeadIds.Add(Event.SourceInstanceId);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[HMT Combat] Round %d: %s (%d units) vs %s (%d units) — %d events, %d deaths, winner=%s"),
		RoundNumber,
		*PlayerA->GetPlayerName(), SnapshotA.Units.Num(),
		PlayerB ? *PlayerB->GetPlayerName() : TEXT("<ghost>"), SnapshotB.Units.Num(),
		Log.Events.Num(), DeadIds.Num(),
		Winner == 0.f ? TEXT("SideA") : Winner == 1.f ? TEXT("SideB") : TEXT("draw/timeout"));

	auto CountSurvivors = [&DeadIds](const FHMT_BoardSnapshot& Snapshot)
	{
		int32 Count = 0;
		for (const FHMT_SnapshotUnit& Unit : Snapshot.Units)
		{
			if (!DeadIds.Contains(Unit.InstanceId) && (!Unit.UnitDefinition || !Unit.UnitDefinition->bIsBuilding))
			{
				++Count;
			}
		}
		return Count;
	};

	auto ApplyIncome = [](APlayerState* Player, bool bWon)
	{
		if (UHMT_ShopComponent* Shop = Player->FindComponentByClass<UHMT_ShopComponent>())
		{
			Shop->ApplyRoundIncome(bWon);
		}
	};

	if (PlayerB)
	{
		if (Winner == 0.f)
		{
			const int32 Damage = MatchRules->BaseRoundDamage + MatchRules->DamagePerSurvivor * CountSurvivors(SnapshotA);
			if (FHMT_PlayerMatchState* BState = FindPlayerState(PlayerB))
			{
				BState->HP -= Damage;
			}
			ApplyIncome(PlayerA, true);
			ApplyIncome(PlayerB, false);
			OnCombatEnd.Broadcast(PlayerA, true);
			OnCombatEnd.Broadcast(PlayerB, false);
		}
		else if (Winner == 1.f)
		{
			const int32 Damage = MatchRules->BaseRoundDamage + MatchRules->DamagePerSurvivor * CountSurvivors(SnapshotB);
			if (FHMT_PlayerMatchState* AState = FindPlayerState(PlayerA))
			{
				AState->HP -= Damage;
			}
			ApplyIncome(PlayerA, false);
			ApplyIncome(PlayerB, true);
			OnCombatEnd.Broadcast(PlayerA, false);
			OnCombatEnd.Broadcast(PlayerB, true);
		}
		else
		{
			if (FHMT_PlayerMatchState* AState = FindPlayerState(PlayerA))
			{
				AState->HP -= 1;
			}
			if (FHMT_PlayerMatchState* BState = FindPlayerState(PlayerB))
			{
				BState->HP -= 1;
			}
			OnCombatEnd.Broadcast(PlayerA, false);
			OnCombatEnd.Broadcast(PlayerB, false);
		}
	}
	else
	{
		const bool bWon = (Winner == 0.f);
		ApplyIncome(PlayerA, bWon);
		if (!bWon)
		{
			const int32 Damage = MatchRules->BaseRoundDamage + MatchRules->DamagePerSurvivor * CountSurvivors(SnapshotB);
			if (FHMT_PlayerMatchState* AState = FindPlayerState(PlayerA))
			{
				AState->HP -= Damage;
			}
		}
		OnCombatEnd.Broadcast(PlayerA, bWon);
	}

	return Log.Events.Num() > 0 ? Log.Events.Last().Timestamp : 0.f;
}

float UHMT_MatchStateComponent::ResolvePvERound(APlayerState* Player, UHMT_PvERoundDefinition* Encounter)
{
	if (!Player || !Encounter || !CombatResolver)
	{
		return 0.f;
	}

	const FHMT_BoardSnapshot SideA = BuildSnapshotForPlayer(Player);

	FHMT_BoardSnapshot SideB;
	SideB.Topology = SideA.Topology;
	SideB.CellSize = SideA.CellSize;
	SideB.Columns = SideA.Columns;
	SideB.Rows = SideA.Rows;
	for (const FHMT_SnapshotUnit& Template : Encounter->EncounterUnits)
	{
		FHMT_SnapshotUnit Unit = Template;
		Unit.InstanceId = FGuid::NewGuid();
		SideB.Units.Add(Unit);
	}

	const FHMT_CombatEventLog Log = CombatResolver->ResolveCombat(this, SideA, SideB);

	if (UHMT_CombatPlaybackComponent* Playback = Player->FindComponentByClass<UHMT_CombatPlaybackComponent>())
	{
		Playback->SetCombatLog(Log, SideA, SideB,  true);
	}

	float Winner = -1.f;
	for (const FHMT_CombatEvent& Event : Log.Events)
	{
		if (Event.EventType == EHMT_CombatEventType::CombatEnd)
		{
			Winner = Event.Magnitude;
		}
	}

	const bool bWon = (Winner == 0.f);
	if (bWon)
	{
		if (UHMT_ShopComponent* Shop = Player->FindComponentByClass<UHMT_ShopComponent>())
		{
			Shop->AddGold(Encounter->ClearBonusGold);
		}
	}
	OnCombatEnd.Broadcast(Player, bWon);

	return Log.Events.Num() > 0 ? Log.Events.Last().Timestamp : 0.f;
}

void UHMT_MatchStateComponent::BeginResultsPhase()
{
	CurrentPhase = EHMT_MatchPhase::Results;

	for (const FHMT_PlayerMatchState& PlayerState : Players)
	{
		if (PlayerState.bEliminated || !PlayerState.Player)
		{
			continue;
		}
		if (const UHMT_UnitCollectionComponent* Collection = PlayerState.Player->FindComponentByClass<UHMT_UnitCollectionComponent>())
		{
			for (AActor* Unit : Collection->GetAllOwnedUnits())
			{
				if (UActorComponent* StatProvider = Unit ? Unit->FindComponentByInterface(UHMT_StatProvider::StaticClass()) : nullptr)
				{
					IHMT_StatProvider::Execute_RemoveModifiersByDuration(StatProvider, EHMT_ModifierDuration::UntilCombatEnd);
				}
			}
		}
	}

	for (FHMT_PlayerMatchState& PlayerState : Players)
	{
		if (!PlayerState.bEliminated && PlayerState.HP <= 0)
		{
			PlayerState.bEliminated = true;
			PlayerState.HP = 0;
		}
	}

	CheckForMatchEnd();

	if (CurrentPhase != EHMT_MatchPhase::MatchEnded)
	{
		BeginRoundEndPhase();
	}
}

void UHMT_MatchStateComponent::BeginRoundEndPhase()
{
	CurrentPhase = EHMT_MatchPhase::RoundEnd;
	BeginPreparationPhase();
}

void UHMT_MatchStateComponent::CheckForMatchEnd()
{
	int32 LiveCount = 0;
	for (const FHMT_PlayerMatchState& PlayerState : Players)
	{
		if (!PlayerState.bEliminated)
		{
			++LiveCount;
		}
	}

	if (LiveCount <= 1 || (MatchRules && RoundNumber >= MatchRules->MaxRounds))
	{
		CurrentPhase = EHMT_MatchPhase::MatchEnded;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PhaseTimerHandle);
		}
	}
}

FHMT_BoardSnapshot UHMT_MatchStateComponent::BuildSnapshotForPlayer(APlayerState* Player) const
{
	FHMT_BoardSnapshot Snapshot;
	if (!Player)
	{
		return Snapshot;
	}

	UHMT_BoardComponent* Board = Player->FindComponentByClass<UHMT_BoardComponent>();
	if (!Board)
	{
		return Snapshot;
	}

	Snapshot.Topology = Board->GetTopology();
	Snapshot.CellSize = Board->GetCellSize();
	Snapshot.Columns = Board->GetColumns();
	Snapshot.Rows = Board->GetRows();
	Snapshot.TileModifiers = Board->GetActiveTileModifiers();

	for (AActor* Occupant : Board->GetAllOccupants())
	{
		const UHMT_UnitInstanceComponent* Instance = Occupant ? Occupant->FindComponentByClass<UHMT_UnitInstanceComponent>() : nullptr;
		if (!Instance || !Instance->GetUnitDefinition())
		{
			continue;
		}

		bool bFound = false;
		const FHMT_GridCoord Coord = Board->GetCoordOfOccupant(Occupant, bFound);
		if (!bFound)
		{
			continue;
		}

		FHMT_SnapshotUnit SnapshotUnit;
		SnapshotUnit.Coord = Coord;
		SnapshotUnit.UnitDefinition = Instance->GetUnitDefinition();
		SnapshotUnit.StarLevel = Instance->GetStarLevel();
		SnapshotUnit.InstanceId = Instance->GetInstanceId();
		if (UActorComponent* StatProvider = Occupant->FindComponentByInterface(UHMT_StatProvider::StaticClass()))
		{
			SnapshotUnit.ActiveModifiers = IHMT_StatProvider::Execute_GetActiveModifiers(StatProvider);
		}
		Snapshot.Units.Add(SnapshotUnit);
	}

	if (UHMT_SynergyManagerComponent* Synergy = Player->FindComponentByClass<UHMT_SynergyManagerComponent>())
	{
		Snapshot.ActiveTraitEffectClasses = Synergy->GetActiveTraitEffectClasses();
	}

	Snapshot.OwningPlayer = Player;
	if (const FHMT_PlayerMatchState* PlayerState = Players.FindByPredicate([Player](const FHMT_PlayerMatchState& State) { return State.Player == Player; }))
	{
		if (PlayerState->RulerDefinition && PlayerState->RulerDefinition->RulerPassiveClass)
		{
			Snapshot.ActiveRulerPassiveClass = PlayerState->RulerDefinition->RulerPassiveClass;
		}
	}
	for (UHMT_GameModifierDefinition* Modifier : ActiveGameModifiers)
	{
		if (Modifier && Modifier->ModifierEffectClass &&
			(Modifier->TriggerType == EHMT_ModifierTriggerType::OnCombatStart ||
			 Modifier->TriggerType == EHMT_ModifierTriggerType::Persistent ||
			 Modifier->TriggerType == EHMT_ModifierTriggerType::OnKill))
		{
			Snapshot.ActiveModifierEffectClasses.Add(Modifier->ModifierEffectClass);
		}
	}

	return Snapshot;
}

float UHMT_MatchStateComponent::GetPhaseTimeRemaining() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState || PhaseEndServerTime <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(PhaseEndServerTime - GameState->GetServerWorldTimeSeconds(), 0.f);
}

FHMT_PlayerMatchState* UHMT_MatchStateComponent::FindPlayerState(APlayerState* Player)
{
	return Players.FindByPredicate([Player](const FHMT_PlayerMatchState& PlayerState) { return PlayerState.Player == Player; });
}
