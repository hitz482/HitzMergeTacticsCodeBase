#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HMT_PlayerController.generated.h"

enum class EHMT_MatchPhase : uint8;

/** Fired on press+release over the same tile (no drag): the unit sitting there — own, or a
 *  spectated player's — or null for a click on empty space/no tile (the HUD's close-panel cue). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHMT_OnUnitClickedSignature, class AHMT_GameUnitActor*, Unit);

/**
 * Debug-only input for exercising the sample match flow before real UI exists:
 * number keys 1-5 buy+place from that shop slot, R rerolls the shop, Enter force-ends
 * the preparation phase timer. Pure input relay — the actual logic (and the Server RPC
 * boundary) lives on AHMT_PlayerState, which owns the relevant components.
 * Also spawns ShopWidgetClass (if assigned) into the local viewport on BeginPlay.
 */
UCLASS()
class AHMT_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Assign a Designer-built WBP (plain UUserWidget, no C++ parent class needed — read its
	 *  gold/level/shop-slot/round/phase state via the already-BlueprintCallable getters on
	 *  ShopComponent/BoardComponent/GameState) to auto-spawn it into the local viewport. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	TSubclassOf<class UUserWidget> ShopWidgetClass;

	/** Unit-info-panel hook: a HUD widget binds this and shows/refreshes (unit != null) or hides
	 *  (null) its detail panel. Read the unit's UnitInstanceComponent/StatComponent for content. */
	UPROPERTY(BlueprintAssignable, Category = "HMT Sample")
	FHMT_OnUnitClickedSignature OnUnitClicked;

	/** Polling twin of OnUnitClicked for widgets built on the standard 0.2s self-refresh pattern:
	 *  the unit whose info panel should currently be open, or null for none. Cleared automatically
	 *  if the actor dies/despawns (sold, combat teardown). */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	class AHMT_GameUnitActor* GetSelectedUnit() const;

	/** Trait-info-panel selection, set by a trait row's OnClicked. Mutually exclusive with the
	 *  unit selection — one info panel at a time: selecting a trait clears the unit and vice
	 *  versa; clicking empty ground clears both. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	void SetSelectedTrait(class UHMT_TraitDefinition* Trait);

	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	class UHMT_TraitDefinition* GetSelectedTrait() const;

	/** Who this machine is currently spectating, or null when viewing your own board — the HUD's
	 *  show/hide signal for a spectate indicator, and the visibility policy's army selector
	 *  (see AHMT_GameUnitActor::RefreshLocalVisibility). Public: read externally by unit actors. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	class AHMT_PlayerState* GetSpectateTarget() const { return SpectateTarget; }

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;

private:
	void BuySlot(int32 SlotIndex);

	void DebugBuySlot0();
	void DebugBuySlot1();
	void DebugBuySlot2();
	void DebugBuySlot3();
	void DebugBuySlot4();
	void DebugReroll();
	void DebugForceEndPreparation();
	void DebugToggleShopLock();

	/** X while hovering an occupied tile sells that unit. */
	void SellHoveredUnit();

	/** Tab (or a UI Spectate button bound to this) cycles the local view: own board -> each other
	 *  player's board (spectate) -> own board. Spectating reveals that player's tiles/units on this
	 *  machine only (see SetLocallySpectated). */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	void CycleSpectateTarget();

	/** Shared target-swap logic for CycleSpectateTarget (manual) and Tick's auto-stop-on-phase-
	 *  change (automatic) — both just need to name the new target (or null for "back to own
	 *  board"), never duplicate the toggle-old/toggle-new/refresh-visibility sequence. */
	void SetSpectateTarget(class AHMT_PlayerState* NewTarget);

	/** Current match phase, or Preparation if the world/gamestate/matchstate chain isn't resolved
	 *  yet — a safe default that never accidentally treats "unknown" as Combat, which would
	 *  misfire the mid-combat playback-source switch in SetSpectateTarget. */
	EHMT_MatchPhase GetCurrentMatchPhase() const;


	/** Drag-drop board/bench moves: mouse-down over one of the local player's occupied tiles lifts
	 *  that unit — it follows the cursor while the drag is active (Tick), with the hovered candidate
	 *  tile highlighted. Mouse-up over a valid drop tile snaps the unit there and fires the move/swap
	 *  Server RPC; anywhere else (empty ground, a spectated player's board, a cap-violating cell) the
	 *  unit glides back to where it was picked up. Validity is client-PREDICTED from replicated state
	 *  purely to choose snap-vs-return — the server RPC revalidates everything authoritatively. */
	void OnDragStart();
	void OnDragEnd();
	bool TraceTileUnderCursor(class AHMT_BoardTile*& OutTile) const;
	void SetHoveredTile(class AHMT_BoardTile* NewHovered);

	/** The local player's own unit occupying Tile, or null (empty tile / someone else's tile). */
	class AHMT_GameUnitActor* FindOwnUnitOnTile(const class AHMT_BoardTile* Tile) const;

	/** Any player's unit occupying Tile (resolved via the tile's own OwningPlayer, so it also works
	 *  on a spectated board), or null for an empty tile. Click/info-panel lookups only — move/sell
	 *  paths keep using FindOwnUnitOnTile's ownership gate. */
	class AHMT_GameUnitActor* FindUnitOnTile(const class AHMT_BoardTile* Tile) const;

	/** Client-side mirror of ServerMoveOrSwapUnit's checks (occupancy, bounds, board cap — all
	 *  replicated), used only to pick drop-vs-return feedback. */
	bool CanDropOn(const class AHMT_BoardTile* FromTile, const class AHMT_BoardTile* ToTile) const;

public:
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<class AHMT_BoardTile> DragFromTile;

	/** Tile under the cursor at mouse-down that held ANY unit — if mouse-up lands on this same
	 *  tile, that's a click (OnUnitClicked), not a drag. */
	UPROPERTY()
	TObjectPtr<class AHMT_BoardTile> ClickCandidateTile;

	/** Current info-panel selection (see GetSelectedUnit). Weak: units are destroyed by sells and
	 *  combat teardown, and a stale panel target should read as "nothing selected", not crash. */
	TWeakObjectPtr<class AHMT_GameUnitActor> SelectedUnit;

	/** See SetSelectedTrait. Weak for symmetry (data assets rarely die, but never crash on it). */
	TWeakObjectPtr<class UHMT_TraitDefinition> SelectedTrait;

	UPROPERTY()
	TObjectPtr<class AHMT_BoardTile> HoveredTile;

	/** Own tiles marked Occupied for the duration of the current drag — restored to Base on drag
	 *  end. Weak: tiles are local-only actors that can be torn down mid-drag (e.g. spectate swap). */
	TArray<TWeakObjectPtr<class AHMT_BoardTile>> OccupiedDuringDrag;

	UPROPERTY()
	TObjectPtr<class AHMT_PlayerState> SpectateTarget;

	/** The unit visually following the cursor during an active drag (local-only movement — its
	 *  collision is disabled for the duration so it doesn't block the cursor's tile trace). */
	UPROPERTY()
	TObjectPtr<class AHMT_GameUnitActor> DraggedUnit;

	/** Where the dragged unit was picked up — the return point for an invalid drop. */
	FVector DragOriginLocation = FVector::ZeroVector;

	/** Last match phase observed via Tick — spectating auto-stops the instant this changes (see
	 *  Tick). Undefined/unused until bHasObservedMatchPhase is set on the first successful read. */
	EHMT_MatchPhase LastObservedMatchPhase;
	bool bHasObservedMatchPhase = false;
};
