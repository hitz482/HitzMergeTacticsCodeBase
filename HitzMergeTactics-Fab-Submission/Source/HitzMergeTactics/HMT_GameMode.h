#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Economy/HMT_MatchRulesAsset.h"
#include "Grid/HMT_BoardLayoutAsset.h"
#include "Units/HMT_UnitDefinition.h"
#include "Match/HMT_PvERoundDefinition.h"
#include "Synergy/HMT_TraitDefinition.h"
#include "Ruler/HMT_RulerDefinition.h"
#include "HMT_GameMode.generated.h"

class AHMT_PlayerState;

/**
 * Reference wiring for a minimal playable HitzMergeTactics match: assign the data
 * assets below (on a Blueprint subclass's Class Defaults, or directly if you prefer a
 * C++-only sample), set this as the map's GameMode, and press Play. Starts the match
 * automatically once MatchRules->MinPlayers have logged in.
 */
UCLASS()
class AHMT_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHMT_GameMode();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	TObjectPtr<UHMT_MatchRulesAsset> MatchRules;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	TObjectPtr<UHMT_BoardLayoutAsset> BoardLayout;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	TArray<TObjectPtr<UHMT_UnitDefinition>> AvailableUnits;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	TArray<TObjectPtr<UHMT_PvERoundDefinition>> PvERounds;

	/** Copied onto each player's UHMT_SynergyManagerComponent at match start — same pattern as
	 *  AvailableUnits/MatchRules/BoardLayout above. Without this, no synergy in the match will ever
	 *  activate even if units carry Trait.* tags and UHMT_TraitDefinition assets exist, since
	 *  RegisteredTraits is otherwise never populated anywhere. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	TArray<TObjectPtr<UHMT_TraitDefinition>> RegisteredTraits;

	/** Pool StartMatch auto-assigns from (random pick per player, bots included). Empty leaves every
	 *  player's Ruler unset — matches the framework's "unset = feature does nothing" convention. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	TArray<TObjectPtr<UHMT_RulerDefinition>> AvailableRulers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 BenchSlotCount = 9;

	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	bool bMatchStarted = false;
	bool bBotFillTimerStarted = false;
	FTimerHandle BotFillTimerHandle;

	/** One lazily-created MatchRules->BotDecisionStrategyClass instance per bot-controlled player,
	 *  reused round to round so a stateful strategy can remember its own decisions across a match. */
	UPROPERTY()
	TMap<TObjectPtr<APlayerState>, TObjectPtr<UObject>> BotStrategyInstances;

	/** Every PlayerState this GameMode has spawned as a bot-fill — read by TryStartMatch when it
	 *  calls StartMatch so bot-controlled flags are correct from round 1 onward (see
	 *  UHMT_MatchStateComponent::StartMatch's InBotPlayers comment for why this can't be set after
	 *  the fact). */
	UPROPERTY()
	TSet<TObjectPtr<APlayerState>> KnownBotPlayers;

	void TryStartMatch();

	/** Timer callback (see MatchRules->bFillWithBotsWhenMinPlayersUnmet/BotFillWaitSeconds): spawns
	 *  enough bot-controlled AHMT_PlayerStates to reach MinPlayers, then retries TryStartMatch.
	 *  No-ops if the match already started (e.g. enough real players joined before the timer fired). */
	void FillRemainingSlotsWithBots();

	UFUNCTION()
	void HandleRoundStartForBots(int32 RoundNumber);

	/** Runs the merge cascade + synergy recompute for every player at Preparation start — merges
	 *  deferred by mid-combat bench buys (see ServerBuyAndPlaceUnit's phase gate) complete here. */
	UFUNCTION()
	void HandleRoundStartMergeSweep(int32 RoundNumber);

	/** Spawns each player's Ruler actor the first round it's called (once MatchStateComponent has
	 *  resolved Rulers), and resets it to the Idle pose on every later round — see
	 *  AHMT_PlayerState::EnsureRulerActorSpawned. */
	UFUNCTION()
	void HandleRoundStartRulerPresentation(int32 RoundNumber);

	/** Plays the Victory/Defeat pose on Player's Ruler actor after each round's combat resolves. */
	UFUNCTION()
	void HandleCombatEndRulerReaction(APlayerState* Player, bool bWon);

	/** Safety net: force-clears every player's combat playback proxies and spectate ghosts at the
	 *  start of every round, regardless of whether the normal teardown paths already ran. Without
	 *  this, a combat proxy whose automatic post-fight teardown (UHMT_CombatPlaybackDriver's
	 *  victory-lap-end + IsRoundPastCombat check) never satisfies both conditions simultaneously —
	 *  or a spectate ghost whose stop-spectating path gets skipped — stays alive, visible and
	 *  un-interactable, scattered off the actual board/bench, for the rest of the match (only the
	 *  NEXT combat this player personally fights in would eventually clear it, via StartPlayback's
	 *  own FinishPlayback-if-already-playing guard). Both underlying calls are no-ops when nothing
	 *  needs clearing, so this never disrupts a legitimately fresh round. */
	UFUNCTION()
	void HandleRoundStartClearStalePlayback(int32 RoundNumber);

	/** Reads SamplePlayerState's current HP off UHMT_MatchStateComponent::GetPlayerRuler and pushes
	 *  it (with the authored max — RulerDefinition's StartingHPOverride if set, else MatchRules->
	 *  StartingPlayerHP) to their Ruler actor's overhead widget. Called at every round start (so the
	 *  bar is correct from the very first Preparation, before any combat) and after every combat
	 *  (HP just changed). No-op if the player has no Ruler actor yet. */
	void PushRulerHealth(AHMT_PlayerState* SamplePlayerState) const;
};
