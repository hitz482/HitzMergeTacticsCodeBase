#include "HMT_GameMode.h"
#include "HMT_PlayerState.h"
#include "HMT_GameState.h"
#include "HMT_PlayerController.h"
#include "HMT_TopDownPawn.h"
#include "Economy/HMT_ShopPoolComponent.h"
#include "Match/HMT_MatchStateComponent.h"
#include "Synergy/HMT_SynergyManagerComponent.h"
#include "Units/HMT_UnitCollectionComponent.h"
#include "AI/HMT_BotDecisionStrategy.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Grid/HMT_BoardComponent.h"
#include "Tiles/HMT_TileModifierDefinition.h"
#include "Ruler/HMT_RulerActor.h"
#include "HMT_CombatPlaybackDriver.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Loads a data asset by path in the CDO constructor, returning nullptr (never crashing) if the
	// buyer's project doesn't have it. Keeps the many finder declarations below to one line each.
	template <typename T>
	T* HMT_FindDefaultAsset(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<T> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}
}

AHMT_GameMode::AHMT_GameMode()
{
	PlayerStateClass = AHMT_PlayerState::StaticClass();
	GameStateClass = AHMT_GameState::StaticClass();
	PlayerControllerClass = AHMT_PlayerController::StaticClass();
	DefaultPawnClass = AHMT_TopDownPawn::StaticClass();

	// Sample content defaults, resolved from /Game so the demo map plays immediately without any
	// hand-wiring on the GameMode's Class Defaults. These are ordinary EditDefaultsOnly properties,
	// so any Blueprint child (GM_MergeTactics, or a buyer's own subclass) can still override every
	// one of them in Class Defaults — an override always wins over these. A buyer project that
	// doesn't ship these exact assets simply gets nullptr/empty here and wires their own instead.
	// (These live in C++ rather than only on the Blueprint because inherited-property overrides on a
	// Blueprint get wiped whenever this parent class's header changes and triggers a reinstance —
	// putting the defaults here makes the sample robust against that.)
	MatchRules = HMT_FindDefaultAsset<UHMT_MatchRulesAsset>(TEXT("/Game/HitzMergeTactics/DataAssets/MatchRules/DA_MatchRules"));
	BoardLayout = HMT_FindDefaultAsset<UHMT_BoardLayoutAsset>(TEXT("/Game/HitzMergeTactics/DataAssets/BoardLayout/DA_BoardLayout"));

	const TCHAR* UnitPaths[] = {
		TEXT("/Game/HitzMergeTactics/DataAssets/Units/DA_Unit1"),
		TEXT("/Game/HitzMergeTactics/DataAssets/Units/DA_Unit2"),
		TEXT("/Game/HitzMergeTactics/DataAssets/Units/DA_Unit3"),
		TEXT("/Game/HitzMergeTactics/DataAssets/Units/DA_Unit4"),
		TEXT("/Game/HitzMergeTactics/DataAssets/Units/DA_Unit5"),
	};
	for (const TCHAR* Path : UnitPaths)
	{
		if (UHMT_UnitDefinition* Unit = HMT_FindDefaultAsset<UHMT_UnitDefinition>(Path))
		{
			AvailableUnits.Add(Unit);
		}
	}

	const TCHAR* TraitPaths[] = {
		TEXT("/Game/HitzMergeTactics/DataAssets/Traits/DA_Trait_Warrior"),
		TEXT("/Game/HitzMergeTactics/DataAssets/Traits/DA_Trait_Ranger"),
		TEXT("/Game/HitzMergeTactics/DataAssets/Traits/DA_Trait_Assassin"),
		TEXT("/Game/HitzMergeTactics/DataAssets/Traits/DA_Trait_Brawler"),
		TEXT("/Game/HitzMergeTactics/DataAssets/Traits/DA_Trait_Guardian"),
	};
	for (const TCHAR* Path : TraitPaths)
	{
		if (UHMT_TraitDefinition* Trait = HMT_FindDefaultAsset<UHMT_TraitDefinition>(Path))
		{
			RegisteredTraits.Add(Trait);
		}
	}

	const TCHAR* RulerPaths[] = {
		TEXT("/Game/HitzMergeTactics/DataAssets/Rulers/DA_Ruler_Ironclad"),
		TEXT("/Game/HitzMergeTactics/DataAssets/Rulers/DA_Ruler_Nightwhisper"),
	};
	for (const TCHAR* Path : RulerPaths)
	{
		if (UHMT_RulerDefinition* Ruler = HMT_FindDefaultAsset<UHMT_RulerDefinition>(Path))
		{
			AvailableRulers.Add(Ruler);
		}
	}
}

void AHMT_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	TryStartMatch();

	// Only ever schedule this once — TryStartMatch already fires on every PostLogin, so if enough
	// real players show up before the timer fires, bMatchStarted is already true by then and
	// FillRemainingSlotsWithBots no-ops (see its own guard).
	if (!bMatchStarted && !bBotFillTimerStarted && MatchRules && MatchRules->bFillWithBotsWhenMinPlayersUnmet)
	{
		bBotFillTimerStarted = true;
		GetWorldTimerManager().SetTimer(BotFillTimerHandle, this, &AHMT_GameMode::FillRemainingSlotsWithBots, MatchRules->BotFillWaitSeconds, false);
	}
}

void AHMT_GameMode::TryStartMatch()
{
	if (bMatchStarted)
	{
		UE_LOG(LogTemp, Log, TEXT("[HMT Sample] TryStartMatch: already started, skipping."));
		return;
	}

	if (!MatchRules)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] TryStartMatch: MatchRules is not assigned on this GameMode's Class Defaults — nothing will happen until it is."));
		return;
	}

	AHMT_GameState* SampleGameState = GetGameState<AHMT_GameState>();
	if (!SampleGameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] TryStartMatch: GameState is not an AHMT_GameState — check GameStateClass."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] TryStartMatch: %d/%d players joined."), SampleGameState->PlayerArray.Num(), MatchRules->MinPlayers);
	if (SampleGameState->PlayerArray.Num() < MatchRules->MinPlayers)
	{
		return;
	}

	if (!BoardLayout)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] TryStartMatch: BoardLayout is not assigned on this GameMode's Class Defaults — boards will not initialize."));
	}

	// Pool must exist before any player's shop can roll from it.
	SampleGameState->ShopPoolComponent->InitializePool(MatchRules, AvailableUnits);

	TArray<APlayerState*> Players;
	int32 PlayerIndex = 0;
	for (APlayerState* PlayerStateBase : SampleGameState->PlayerArray)
	{
		if (AHMT_PlayerState* SamplePlayerState = Cast<AHMT_PlayerState>(PlayerStateBase))
		{
			// Spaces each player's board apart in world X so they don't overlap — debug-only sample concern.
			const FVector BoardWorldOrigin(PlayerIndex * 2000.f, 0.f, 0.f);
			SamplePlayerState->InitializeForMatch(BoardLayout, BenchSlotCount, MatchRules, AvailableUnits, SampleGameState->ShopPoolComponent, BoardWorldOrigin);
			if (SamplePlayerState->SynergyManagerComponent)
			{
				SamplePlayerState->SynergyManagerComponent->RegisteredTraits = RegisteredTraits;
			}
			Players.Add(PlayerStateBase);
			++PlayerIndex;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] TryStartMatch: a PlayerState in PlayerArray is not an AHMT_PlayerState — check PlayerStateClass."));
		}
	}


	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] TryStartMatch: starting match with %d players."), Players.Num());

	TSet<APlayerState*> BotPlayers;
	for (const TObjectPtr<APlayerState>& Bot : KnownBotPlayers)
	{
		BotPlayers.Add(Bot);
	}

	// Bind BEFORE StartMatch, not after: BeginPreparationPhase's round-1 OnRoundStart broadcasts
	// synchronously from inside StartMatch, before it returns — binding afterward would silently
	// skip every bot's very first turn.
	// Merge sweep binds first so deferred mid-combat merges settle before bots take their turn.
	SampleGameState->MatchStateComponent->OnRoundStart.AddDynamic(this, &AHMT_GameMode::HandleRoundStartMergeSweep);
	SampleGameState->MatchStateComponent->OnRoundStart.AddDynamic(this, &AHMT_GameMode::HandleRoundStartForBots);
	SampleGameState->MatchStateComponent->OnRoundStart.AddDynamic(this, &AHMT_GameMode::HandleRoundStartRulerPresentation);
	SampleGameState->MatchStateComponent->OnCombatEnd.AddDynamic(this, &AHMT_GameMode::HandleCombatEndRulerReaction);
	SampleGameState->MatchStateComponent->OnRoundStart.AddDynamic(this, &AHMT_GameMode::HandleRoundStartClearStalePlayback);
	SampleGameState->MatchStateComponent->StartMatch(MatchRules, SampleGameState->ShopPoolComponent, Players, PvERounds, BotPlayers, AvailableRulers);

	bMatchStarted = true;
}

void AHMT_GameMode::FillRemainingSlotsWithBots()
{
	if (bMatchStarted || !MatchRules)
	{
		return;
	}

	AHMT_GameState* SampleGameState = GetGameState<AHMT_GameState>();
	if (!SampleGameState)
	{
		return;
	}

	const int32 ShortBy = MatchRules->MinPlayers - SampleGameState->PlayerArray.Num();
	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < ShortBy; ++Index)
	{
		if (AHMT_PlayerState* Bot = GetWorld()->SpawnActor<AHMT_PlayerState>(PlayerStateClass))
		{
			SampleGameState->AddPlayerState(Bot);
			KnownBotPlayers.Add(Bot);
			++SpawnedCount;
			// Never named otherwise (no PlayerController ever logs in for a bot) — without this,
			// GetPlayerName() falls back to APlayerState's blank default, which is why bot names
			// showed up empty in merge/opponent-tile log lines and would do the same in any HUD text.
			Bot->SetPlayerName(FString::Printf(TEXT("Bot %d"), SpawnedCount));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] FillRemainingSlotsWithBots: spawned %d bot(s) to reach MinPlayers=%d, retrying match start."), SpawnedCount, MatchRules->MinPlayers);
	TryStartMatch();
}

void AHMT_GameMode::HandleRoundStartMergeSweep(int32 RoundNumber)
{
	AHMT_GameState* SampleGameState = GetGameState<AHMT_GameState>();
	if (!SampleGameState)
	{
		return;
	}

	for (APlayerState* PlayerStateBase : SampleGameState->PlayerArray)
	{
		if (AHMT_PlayerState* SamplePlayerState = Cast<AHMT_PlayerState>(PlayerStateBase))
		{
			SamplePlayerState->UnitCollectionComponent->RunMergeCascade();
			SamplePlayerState->SynergyManagerComponent->RecomputeSynergies();
		}
	}
}

void AHMT_GameMode::HandleRoundStartRulerPresentation(int32 RoundNumber)
{
	AHMT_GameState* SampleGameState = GetGameState<AHMT_GameState>();
	if (!SampleGameState)
	{
		return;
	}

	for (APlayerState* PlayerStateBase : SampleGameState->PlayerArray)
	{
		if (AHMT_PlayerState* SamplePlayerState = Cast<AHMT_PlayerState>(PlayerStateBase))
		{
			SamplePlayerState->EnsureRulerActorSpawned();
			PushRulerHealth(SamplePlayerState);
		}
	}
}

void AHMT_GameMode::HandleCombatEndRulerReaction(APlayerState* Player, bool bWon)
{
	AHMT_PlayerState* SamplePlayerState = Cast<AHMT_PlayerState>(Player);
	if (SamplePlayerState && SamplePlayerState->RulerActor)
	{
		SamplePlayerState->RulerActor->SetPose(bWon ? EHMT_RulerPose::Victory : EHMT_RulerPose::Defeat);
	}
	PushRulerHealth(SamplePlayerState);
}

void AHMT_GameMode::PushRulerHealth(AHMT_PlayerState* SamplePlayerState) const
{
	if (!SamplePlayerState || !SamplePlayerState->RulerActor)
	{
		return;
	}

	const AHMT_GameState* SampleGameState = GetGameState<AHMT_GameState>();
	if (!SampleGameState || !SampleGameState->MatchStateComponent)
	{
		return;
	}

	UHMT_RulerDefinition* RulerDef = nullptr;
	int32 CurrentHP = 0;
	if (!SampleGameState->MatchStateComponent->GetPlayerRuler(SamplePlayerState, RulerDef, CurrentHP))
	{
		return;
	}

	const int32 MaxHP = (RulerDef && RulerDef->StartingHPOverride > 0)
		? RulerDef->StartingHPOverride
		: (MatchRules ? MatchRules->StartingPlayerHP : CurrentHP);
	SamplePlayerState->RulerActor->SetHealth(CurrentHP, MaxHP);
}

void AHMT_GameMode::HandleRoundStartClearStalePlayback(int32 RoundNumber)
{
	AHMT_GameState* SampleGameState = GetGameState<AHMT_GameState>();
	if (!SampleGameState)
	{
		return;
	}

	for (APlayerState* PlayerStateBase : SampleGameState->PlayerArray)
	{
		if (AHMT_PlayerState* SamplePlayerState = Cast<AHMT_PlayerState>(PlayerStateBase))
		{
			if (SamplePlayerState->CombatPlaybackDriver)
			{
				SamplePlayerState->CombatPlaybackDriver->EnsureNotPlaying();
			}
			SamplePlayerState->ClearSpectateGhosts();
		}
	}
}

void AHMT_GameMode::HandleRoundStartForBots(int32 RoundNumber)
{
	if (!MatchRules || !MatchRules->BotDecisionStrategyClass)
	{
		return;
	}

	AHMT_GameState* SampleGameState = GetGameState<AHMT_GameState>();
	if (!SampleGameState || !SampleGameState->MatchStateComponent)
	{
		return;
	}

	for (APlayerState* PlayerStateBase : SampleGameState->PlayerArray)
	{
		if (!SampleGameState->MatchStateComponent->IsPlayerBotControlled(PlayerStateBase))
		{
			continue;
		}

		TObjectPtr<UObject>& Strategy = BotStrategyInstances.FindOrAdd(PlayerStateBase);
		if (!Strategy)
		{
			Strategy = NewObject<UObject>(PlayerStateBase, MatchRules->BotDecisionStrategyClass);
		}
		IHMT_BotDecisionStrategy::Execute_TakeTurn(Strategy, PlayerStateBase, RoundNumber);
	}
}
