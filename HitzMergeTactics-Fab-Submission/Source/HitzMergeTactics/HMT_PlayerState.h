#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Grid/HMT_BoardLayoutAsset.h"
#include "Grid/HMT_GridTypes.h"
#include "Economy/HMT_MatchRulesAsset.h"
#include "Units/HMT_UnitDefinition.h"
#include "HMT_PlayerState.generated.h"

class UMaterialInterface;
class UHMT_BoardComponent;
class UHMT_BenchComponent;
class UHMT_UnitCollectionComponent;
class UHMT_SynergyManagerComponent;
class UHMT_ShopComponent;
class UHMT_CombatPlaybackComponent;
class UHMT_ShopPoolComponent;

/** Identifies one board cell or one bench slot — the common currency for drag-drop moves/swaps,
 *  since a single move can originate/land on either. Not BlueprintType: built purely from
 *  AHMT_BoardTile fields by the C++ mouse-picking code, never authored in Blueprint/UMG. */
USTRUCT()
struct FHMT_PlacementSlot
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsBoard = false;

	UPROPERTY()
	FHMT_GridCoord BoardCoord;

	UPROPERTY()
	int32 BenchSlot = INDEX_NONE;
};

/**
 * Reference wiring for a HitzMergeTactics player: every per-player HMT_Core component,
 * initialized from whatever the GameMode hands it. This is sample/glue code, not
 * framework code — a real game would likely customize this class directly.
 */
UCLASS()
class AHMT_PlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AHMT_PlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HMT Sample")
	TObjectPtr<UHMT_BoardComponent> BoardComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HMT Sample")
	TObjectPtr<UHMT_BenchComponent> BenchComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HMT Sample")
	TObjectPtr<UHMT_UnitCollectionComponent> UnitCollectionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HMT Sample")
	TObjectPtr<UHMT_SynergyManagerComponent> SynergyManagerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HMT Sample")
	TObjectPtr<UHMT_ShopComponent> ShopComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HMT Sample")
	TObjectPtr<UHMT_CombatPlaybackComponent> CombatPlaybackComponent;

	/** UFUNCTION wrappers — the reflection-based Blueprint tooling used to build sample UI (Phase 9)
	 *  can only create nodes for callable functions, not bare property reads on a non-self object. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	UHMT_BoardComponent* GetBoard() const { return BoardComponent; }

	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	UHMT_BenchComponent* GetBench() const { return BenchComponent; }

	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	UHMT_UnitCollectionComponent* GetUnitCollection() const { return UnitCollectionComponent; }

	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	UHMT_SynergyManagerComponent* GetSynergyManager() const { return SynergyManagerComponent; }

	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	UHMT_ShopComponent* GetShop() const { return ShopComponent; }

	/** First unoccupied, valid board cell in row-major scan order (respecting the level-gated
	 *  MaxBoardUnits cap), or false if the board is at/over cap or has no free cell at all. Shared by
	 *  ServerBuyAndPlaceUnit's board-first placement and any bot-controlled player's own placement
	 *  logic (see UHMT_GameBotStrategy) so both paths agree on "first free cell" exactly. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	bool FindFirstFreeBoardCoord(FHMT_GridCoord& OutCoord) const;

	/** Called by AHMT_GameMode once the match config is known — ShopPool must already be
	 *  initialized by then (this rolls the initial shop). BoardWorldOrigin offsets this player's
	 *  board in world space so multiple players' boards don't overlap. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	void InitializeForMatch(UHMT_BoardLayoutAsset* BoardLayout, int32 BenchSlotCount, UHMT_MatchRulesAsset* MatchRules, const TArray<UHMT_UnitDefinition*>& AvailableUnits, UHMT_ShopPoolComponent* ShopPool, FVector BoardWorldOrigin);

	/** Genre-standard bench-first buy: purchases from ShopSlot, spawns the unit on the first free
	 *  bench slot (rejected before spending gold if the bench is full), then runs the merge
	 *  cascade. Placement onto the board is a separate drag-drop action (ServerMoveOrSwapUnit).
	 *  Server RPC: called from AHMT_PlayerController's local input handling, since the owning
	 *  client has no authority to mutate gold/pool state directly. */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "HMT Sample")
	void ServerBuyAndPlaceUnit(int32 ShopSlotIndex);

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "HMT Sample")
	void ServerRerollShop();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "HMT Sample")
	void ServerForceEndPreparation();

	/** Moves the occupant at From to To, board/bench in any combination — swaps if To is occupied.
	 *  Called from AHMT_PlayerController's mouse drag-drop; the client only ever resolves
	 *  local tile-actor hits into slots, all validation and the actual data mutation is server-side. */
	UFUNCTION(Server, Reliable, WithValidation, Category = "HMT Sample")
	void ServerMoveOrSwapUnit(FHMT_PlacementSlot From, FHMT_PlacementSlot To);

	/** Sells the occupant of Slot back to the pool (refund via UHMT_ShopComponent::SellUnit). */
	UFUNCTION(Server, Reliable, WithValidation, Category = "HMT Sample")
	void ServerSellUnitAt(FHMT_PlacementSlot Slot);

	/** Convenience wrapper over ServerSellUnitAt for callers holding a unit ACTOR reference but not
	 *  its slot (e.g. a Sell button reacting to the currently-selected unit) — finds Unit's current
	 *  board/bench slot and sells it there. No-op if Unit isn't currently placed by this player. */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "HMT Sample")
	void ServerSellUnitByActor(AActor* Unit);

	/** Toggles ShopComponent::IsLocked() — a locked shop skips the free per-round reroll. */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "HMT Sample")
	void ServerToggleShopLock();

	/** World-space origin of this player's board (replicated) — used by tiles, spawns, and playback. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	FVector GetBoardWorldOrigin() const { return ReplicatedBoardOrigin; }

	/** World-space position of a bench slot — a fixed row behind the board, same pattern as board
	 *  cells. Bench has no adjacency/topology of its own (see UHMT_BenchComponent), so this is
	 *  purely a presentation choice, not a framework concept. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	FVector GetBenchSlotWorldLocation(int32 SlotIndex) const;

	/** World-space center of this player's board — the framing anchor for the camera pawn
	 *  (AHMT_TopDownPawn::FrameBoard), both for the initial placement and spectate switches. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	FVector GetBoardCenterWorld() const;

	/** Local-only spectate toggle. Spectating REUSES the local player's own board — no second tile
	 *  set is spawned and the camera never moves: this (spectated) player's units are projected
	 *  onto the local board/bench positions and re-projected on a short timer (their owner can
	 *  still drag them during THEIR preparation; each replicated move would otherwise land back at
	 *  their real far-away origin). Pure presentation — reads only replicated state. */
	void SetLocallySpectated(bool bSpectated);

	/** Re-runs RefreshLocalVisibility on every AHMT_GameUnitActor owned by this PlayerState.
	 *  Public so the controller can re-evaluate the LOCAL player's own units on spectate toggles
	 *  (they hide while someone else's board is being viewed). */
	void RefreshOwnedUnitVisibility() const;

	/** Re-evaluates this player's Ruler visibility on the LOCAL machine (see AHMT_GameRulerActor::
	 *  RefreshLocalVisibility) — same spectate-toggle hook as RefreshOwnedUnitVisibility, called
	 *  alongside it. No-op if RulerActor isn't spawned yet, or isn't an AHMT_GameRulerActor. */
	void RefreshRulerVisibility() const;

	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	bool IsLocallySpectated() const { return bLocallySpectated; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HMT Sample")
	TObjectPtr<class UHMT_CombatPlaybackDriver> CombatPlaybackDriver;

	/** Class spawned for each bought unit. Defaults to a Blueprint at /Game/Blueprints/BP_HMT_SampleUnit
	 *  if one exists (set in the constructor via FClassFinder — see AHMT_UnitActor::OnStarLevelChanged
	 *  for why a BP subclass is where merge feedback actually lives), else falls back to the plain
	 *  C++ AHMT_GameUnitActor. Override on a Blueprint child of this class to point elsewhere. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HMT Sample")
	TSubclassOf<class AHMT_UnitActor> UnitActorClass;

	/** Class spawned for this player's Ruler world presence. Defaults to the plain C++
	 *  AHMT_RulerActor in the constructor — override on a Blueprint child of this class to point at
	 *  a buyer-authored subclass instead. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HMT Sample")
	TSubclassOf<class AHMT_RulerActor> RulerActorClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HMT Sample")
	TObjectPtr<AHMT_RulerActor> RulerActor;

	/** Spawns (first call) or re-initializes this player's Ruler actor from whatever Ruler
	 *  MatchStateComponent has assigned. No-ops if the player has no Ruler assigned — RulerDefinition
	 *  unset is a valid, supported state, same "unset = feature does nothing" convention used
	 *  elsewhere. Called from AHMT_GameMode's OnRoundStart handler, once per round (idempotent after
	 *  the first call — later calls just reset pose to Idle, they don't re-spawn). */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	void EnsureRulerActorSpawned();

	/** Fixed spot behind the bench row (one more row-depth back), centered on the board — see
	 *  GetBenchSlotWorldLocation's comment for the same axis convention. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	FVector GetRulerWorldLocation() const;

private:


	/** Replicated so every client (and the listen-server host) can locally spawn its own debug
	 *  tiles — AStaticMeshActor's mesh component isn't marked replicated by the engine (it assumes
	 *  level-placed meshes, known to clients from loading the same level), so a server-spawned,
	 *  server-configured mesh actor never reaches remote clients. Data-driven client-local spawning
	 *  sidesteps that entirely, same pattern as the combat event log (replicate data, not actors). */
	UPROPERTY(ReplicatedUsing = OnRep_DebugBoardLayout)
	TObjectPtr<UHMT_BoardLayoutAsset> ReplicatedBoardLayout;

	UPROPERTY(Replicated)
	FVector ReplicatedBoardOrigin = FVector::ZeroVector;

	// Not its own ReplicatedUsing — deliberately piggybacks on OnRep_DebugBoardLayout (see that
	// function) rather than adding a second OnRep with its own ordering-vs-ReplicatedBoardLayout
	// question to answer.
	UPROPERTY(Replicated)
	int32 ReplicatedBenchSlotCount = 0;

	UFUNCTION()
	void OnRep_DebugBoardLayout();

public:
	/** Tri-state locality check: is this the ONE PlayerState the local machine is playing as?
	 *  Returns false (undeterminable) while the local PlayerController->PlayerState link hasn't
	 *  replicated yet — which can be AFTER this PlayerState's own properties OnRep, so callers must
	 *  retry later rather than treat "don't know yet" as "no" (getting that wrong silently skipped
	 *  all board visuals on clients). On a dedicated server it's always determinable: never local.
	 *  Why locality matters at all: other players' boards/units stay hidden during placement —
	 *  combat (and a future spectator mode) shows opponents via the snapshot-driven playback
	 *  proxies, not by making every board globally visible. */
	bool TryDetermineLocallyViewed(bool& bOutIsLocal) const;

	/** Destroys and clears every ghost from SpawnSpectateGhosts. Public so AHMT_GameMode's
	 *  OnRoundStart handler can force-clear stale ghosts as a safety net at the start of every
	 *  round, regardless of whether the normal stop-spectating/re-apply path already cleared them —
	 *  see that handler's comment for why this defense-in-depth exists. */
	void ClearSpectateGhosts();

private:
	/** Server-side debug feedback for merges — logs and (on whichever machine actually ran the
	 *  merge, i.e. the server/listen-host) shows an on-screen message naming the unit and new star
	 *  level. Bound to UnitCollectionComponent::OnUnitMerged in the constructor. Note this delegate
	 *  itself never reaches remote clients (see AHMT_UnitActor::OnStarLevelChanged for the hook that
	 *  does) — this is dev/debug visibility, not player-facing UI. */
	UFUNCTION()
	void HandleUnitMerged(AActor* SurvivorUnit, FGameplayTag UnitID, int32 NewStarLevel);

	/** Single entry point for all local-only presentation (board tiles, bench tiles, pawn
	 *  reposition), called from both the server-direct path and OnRep. Defers itself on a short
	 *  timer while TryDetermineLocallyViewed can't answer yet — see that function. */
	void TrySpawnLocalBoardVisuals();

	/** Tile visuals come from the layout asset's TileMesh/materials (buyer-authored, defaults to
	 *  the engine Plane) — HMT_Core deliberately ships no visuals, this is sample-only glue.
	 *  Spawned tiles are appended to OutTiles so spectate-spawned sets can be torn down later.
	 *  TileMaterialOverride, if set, replaces the layout's own TileMaterial for normal (non-blocked)
	 *  cells only — used for UHMT_BoardLayoutAsset::OpponentTileMaterial. */
	void SpawnDebugBoardTiles(UHMT_BoardLayoutAsset* BoardLayout, FVector BoardWorldOrigin, TArray<TObjectPtr<class AHMT_BoardTile>>& OutTiles, UMaterialInterface* TileMaterialOverride = nullptr);

	/** Polled on a repeating timer once the local board is ready (see TrySpawnLocalBoardVisuals) —
	 *  re-derives who this round's paired opponent is via UHMT_MatchStateComponent::
	 *  GetCurrentOpponentFor, and (re)spawns/tears down OpponentTiles when it changes. No-op if
	 *  UHMT_BoardLayoutAsset::bShowOpponentTiles is false. Tiles-only (not units) — matches the
	 *  audit ask literally and avoids fighting the separate Tab-spectate unit-visibility system,
	 *  which OwningPlayer-based interactability already keeps opponent tiles un-placeable on anyway. */
	void TryUpdateOpponentBoardVisuals();

	/** Same idea as SpawnDebugBoardTiles but for the bench row behind the board. */
	void SpawnDebugBenchTiles(int32 SlotCount, FVector BoardWorldOrigin, TArray<TObjectPtr<class AHMT_BoardTile>>& OutTiles);

	/** Recenters the local machine's own top-down pawn above this player's board, once the board
	 *  origin is known — pawn possession happens at PostLogin, before InitializeForMatch computes
	 *  BoardWorldOrigin, so the pawn's spawn-time location is just whatever PlayerStart gave it. */
	void RepositionLocalPawnAboveBoard(FVector BoardWorldOrigin) const;

	/** While locally spectated: (re)builds this player's ghost proxies on the LOCAL player's board
	 *  and reschedules itself (0.5s) — see SetLocallySpectated's comment for why this must repeat
	 *  (the target can keep placing/moving units during their own Preparation). */
	void ApplySpectateRemap();

	/** Compact string of this player's current board occupancy (coord + unit id + star level) — see
	 *  LastSpectateSignature. Equal across two calls iff the projected formation a spectator sees is
	 *  unchanged, which is the condition for skipping a ghost rebuild. */
	FString ComputeSpectateBoardSignature() const;

	/** Spawns one local-only, non-replicated ghost actor (mesh + position, no gameplay behavior)
	 *  per BOARD occupant this player has, positioned at OriginOwner's board world coords —
	 *  board only, deliberately: prep-spectating shows scouting-relevant placed formation, never
	 *  bench. Deliberately never touches the real unit actors' transforms: on a LISTEN SERVER
	 *  those are the actual authoritative actors, not a harmless replicated proxy, so moving them
	 *  directly (the previous approach) corrupted real game state instead of just drawing wrong —
	 *  ghosts sidestep that by construction, same pattern as the opponent-preview tiles. */
	void SpawnSpectateGhosts(const AHMT_PlayerState* OriginOwner);

	/** True only during Preparation (world/gamestate/matchstate all resolved) — the phase gate
	 *  for spectate's ghost projection. Defaults to false (don't project) if anything is
	 *  unresolved, since projecting prep-only data outside Preparation is exactly the bug this
	 *  guards against. */
	bool IsPreparationPhaseNow() const;

	/** Local-only ghost proxies from SpawnSpectateGhosts — never replicated, torn down the moment
	 *  spectating this player stops or re-applies (see ApplySpectateRemap/ClearSpectateGhosts). */
	UPROPERTY()
	TArray<TObjectPtr<class ASkeletalMeshActor>> SpectateGhostProxies;

	/** Signature of this player's board occupancy (coords + unit id + star) from the last time the
	 *  spectate ghosts were (re)built. ApplySpectateRemap rebuilds only when this changes, so a static
	 *  formation's looping idle ghosts are left running untouched instead of being destroyed and
	 *  respawned every refresh (which restarted every animation from frame 0 — the spectate flicker).
	 *  Emptied whenever spectating this player stops, so the next spectate always rebuilds fresh. */
	FString LastSpectateSignature;

	bool bLocalBoardVisualsSpawned = false;
	bool bLocallySpectated = false;
	FTimerHandle LocalVisualsRetryHandle;
	FTimerHandle SpectateRemapHandle;

	/** The local player's own permanent tiles (never torn down). */
	UPROPERTY()
	TArray<TObjectPtr<class AHMT_BoardTile>> OwnBoardTiles;

	/** Tiles spawned on this machine for the current round's paired opponent, when
	 *  UHMT_BoardLayoutAsset::bShowOpponentTiles is enabled. Independent of spectating — a
	 *  different concept (this round's opponent vs. whoever Tab-cycle happens to be viewing). */
	UPROPERTY()
	TArray<TObjectPtr<class AHMT_BoardTile>> OpponentTiles;

	/** Which opponent OpponentTiles was last spawned for — TryUpdateOpponentBoardVisuals only
	 *  respawns when this changes, since it polls on a repeating timer every round. Null while the
	 *  PLACEHOLDER grid is up (no live opponent), so bOpponentTilesSpawned disambiguates
	 *  "placeholder showing" from "nothing spawned yet". */
	TWeakObjectPtr<APlayerState> CachedOpponentForTiles;
	bool bOpponentTilesSpawned = false;

	FTimerHandle OpponentTilesPollHandle;
};
