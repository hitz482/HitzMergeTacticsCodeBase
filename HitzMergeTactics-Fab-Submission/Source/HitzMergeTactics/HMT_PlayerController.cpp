#include "HMT_PlayerController.h"
#include "HMT_PlayerState.h"
#include "HMT_GameState.h"
#include "HMT_BoardTile.h"
#include "HMT_TopDownPawn.h"
#include "HMT_GameUnitActor.h"
#include "Match/HMT_MatchStateComponent.h"
#include "Match/HMT_MatchTypes.h"
#include "HMT_CombatPlaybackDriver.h"
#include "Combat/HMT_CombatPlaybackComponent.h"
#include "Grid/HMT_BoardComponent.h"
#include "Grid/HMT_BenchComponent.h"
#include "Economy/HMT_ShopComponent.h"
#include "Economy/HMT_MatchRulesAsset.h"
#include "Synergy/HMT_TraitDefinition.h"
#include "Components/InputComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/HitResult.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"

namespace
{
	/** How far above the board a dragged unit hovers while following the cursor. */
	constexpr float HMT_DragLiftHeight = 80.f;

	/** Units sit +50 above a cell's base; tiles sit +5 — so a unit rests +45 above its tile actor. */
	constexpr float HMT_UnitAboveTileOffset = 45.f;
}

void AHMT_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// If the class logged below is AHMT_PlayerController (the raw C++ class) instead of
	// your BP subclass, the GameMode override didn't take — check the World Settings GameMode's
	// PlayerControllerClass, not just a GameMode asset that isn't the one this level uses.
	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] PC BeginPlay: class=%s, IsLocalController=%d, ShopWidgetClass=%s"),
		*GetClass()->GetName(), IsLocalController() ? 1 : 0,
		ShopWidgetClass ? *ShopWidgetClass->GetName() : TEXT("NULL"));

	if (IsLocalController())
	{
		bShowMouseCursor = true;
		bEnableClickEvents = true;
	}

	if (IsLocalController() && ShopWidgetClass)
	{
		if (UUserWidget* Widget = CreateWidget<UUserWidget>(this, ShopWidgetClass))
		{
			Widget->AddToViewport();
			UE_LOG(LogTemp, Log, TEXT("[HMT Sample] PC BeginPlay: shop widget created and added to viewport."));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] PC BeginPlay: CreateWidget returned null."));
		}
	}
}

void AHMT_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AHMT_PlayerController::DebugBuySlot0);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AHMT_PlayerController::DebugBuySlot1);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AHMT_PlayerController::DebugBuySlot2);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AHMT_PlayerController::DebugBuySlot3);
	InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AHMT_PlayerController::DebugBuySlot4);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AHMT_PlayerController::DebugReroll);
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AHMT_PlayerController::DebugForceEndPreparation);
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AHMT_PlayerController::DebugToggleShopLock);
	InputComponent->BindKey(EKeys::X, IE_Pressed, this, &AHMT_PlayerController::SellHoveredUnit);
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AHMT_PlayerController::CycleSpectateTarget);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AHMT_PlayerController::OnDragStart);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AHMT_PlayerController::OnDragEnd);
}

void AHMT_PlayerController::DebugToggleShopLock()
{
	if (AHMT_PlayerState* SamplePlayerState = GetPlayerState<AHMT_PlayerState>())
	{
		SamplePlayerState->ServerToggleShopLock();
	}
}

void AHMT_PlayerController::SellHoveredUnit()
{
	// View-only while spectating — X over a projected unit must not sell OUR unit at that coord.
	if (SpectateTarget)
	{
		return;
	}

	AHMT_BoardTile* Tile = nullptr;
	if (!TraceTileUnderCursor(Tile))
	{
		return;
	}

	AHMT_PlayerState* SamplePlayerState = GetPlayerState<AHMT_PlayerState>();
	// Ownership check matters: a spectated player's tiles share coordinates with our own board,
	// so without it, X over their unit would sell OURS at the same coordinate.
	if (!SamplePlayerState || Tile->OwningPlayer != SamplePlayerState)
	{
		return;
	}

	FHMT_PlacementSlot Slot;
	Slot.bIsBoard = Tile->bIsBoardCell;
	Slot.BoardCoord = Tile->BoardCoord;
	Slot.BenchSlot = Tile->BenchSlotIndex;
	SamplePlayerState->ServerSellUnitAt(Slot);
}

void AHMT_PlayerController::CycleSpectateTarget()
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	AHMT_PlayerState* OwnPlayerState = GetPlayerState<AHMT_PlayerState>();
	if (!GameState || !OwnPlayerState)
	{
		return;
	}

	// No elimination gate: any player (alive or eliminated) can spectate any other player's board.
	TArray<AHMT_PlayerState*> Others;
	for (APlayerState* PlayerStateBase : GameState->PlayerArray)
	{
		if (AHMT_PlayerState* Other = Cast<AHMT_PlayerState>(PlayerStateBase); Other && Other != OwnPlayerState)
		{
			Others.Add(Other);
		}
	}

	// Advance: own -> Others[0] -> Others[1] -> ... -> own.
	AHMT_PlayerState* Next = nullptr;
	if (!SpectateTarget)
	{
		Next = Others.Num() > 0 ? Others[0] : nullptr;
	}
	else
	{
		const int32 CurrentIndex = Others.IndexOfByKey(SpectateTarget.Get());
		Next = Others.IsValidIndex(CurrentIndex + 1) ? Others[CurrentIndex + 1] : nullptr;
	}

	SetSpectateTarget(Next);

	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] Spectate: now viewing %s."),
		SpectateTarget ? *SpectateTarget->GetPlayerName() : *OwnPlayerState->GetPlayerName());
}

void AHMT_PlayerController::SetSpectateTarget(AHMT_PlayerState* NewTarget)
{
	if (SpectateTarget == NewTarget)
	{
		return;
	}

	// ORDER MATTERS: SpectateTarget must point at the NEW target before either toggle runs —
	// SetLocallySpectated(false) refreshes the old target's unit visibility, and that policy
	// (AHMT_GameUnitActor::RefreshLocalVisibility) reads GetSpectateTarget(). With the old value
	// still set, the old target's units stayed visible forever ("a copy stays in the scene").
	AHMT_PlayerState* Previous = SpectateTarget;
	SpectateTarget = NewTarget;

	if (Previous)
	{
		Previous->SetLocallySpectated(false);
	}
	if (SpectateTarget)
	{
		SpectateTarget->SetLocallySpectated(true);
	}

	// The camera never moves: spectating projects the target's army onto OUR board (see
	// SetLocallySpectated), so the local player's own units must re-evaluate visibility too —
	// they hide while someone else's board is being viewed and reappear on cycling back.
	if (AHMT_PlayerState* OwnPlayerState = GetPlayerState<AHMT_PlayerState>())
	{
		OwnPlayerState->RefreshOwnedUnitVisibility();
		OwnPlayerState->RefreshRulerVisibility();

		// Mid-combat retarget: an already-running playback doesn't react to SpectateTarget on its
		// own (see UHMT_CombatPlaybackDriver::SwitchPlaybackSource's own comment) — tell it
		// directly. Preparation-phase spectating never reaches here needing anything from the
		// driver; SetLocallySpectated's own phase gate already handles that case.
		if (GetCurrentMatchPhase() == EHMT_MatchPhase::Combat)
		{
			if (UHMT_CombatPlaybackDriver* Driver = OwnPlayerState->CombatPlaybackDriver)
			{
				UHMT_CombatPlaybackComponent* NewSource = SpectateTarget
					? SpectateTarget->FindComponentByClass<UHMT_CombatPlaybackComponent>()
					: nullptr;
				Driver->SwitchPlaybackSource(NewSource);
			}
		}
	}
}

EHMT_MatchPhase AHMT_PlayerController::GetCurrentMatchPhase() const
{
	const AHMT_GameState* SampleGameState = GetWorld() ? GetWorld()->GetGameState<AHMT_GameState>() : nullptr;
	const UHMT_MatchStateComponent* MatchState = SampleGameState ? SampleGameState->GetMatchState() : nullptr;
	return MatchState ? MatchState->GetCurrentPhase() : EHMT_MatchPhase::Preparation;
}

bool AHMT_PlayerController::TraceTileUnderCursor(AHMT_BoardTile*& OutTile) const
{
	OutTile = nullptr;

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		return false;
	}

	OutTile = Cast<AHMT_BoardTile>(Hit.GetActor());
	if (OutTile && !OutTile->bInteractable)
	{
		OutTile = nullptr;   // rendered-but-blocked cells are scenery, not drag targets
	}
	return OutTile != nullptr;
}

void AHMT_PlayerController::SetHoveredTile(AHMT_BoardTile* NewHovered)
{
	if (HoveredTile == NewHovered)
	{
		return;
	}
	if (HoveredTile)
	{
		// Un-hover restores whatever drag-context state the tile was showing before the cursor
		// arrived (Occupied for other held cells during a drag), not blindly Base.
		HoveredTile->SetVisualState(OccupiedDuringDrag.Contains(HoveredTile)
			? EHMT_SampleTileVisualState::Occupied
			: EHMT_SampleTileVisualState::Base);
	}
	HoveredTile = NewHovered;
	if (HoveredTile)
	{
		// Valid/invalid drop feedback instead of a binary highlight — falls back to the old look
		// when the layout asset authors no ValidDrop/InvalidDrop materials.
		HoveredTile->SetVisualState(CanDropOn(DragFromTile, HoveredTile)
			? EHMT_SampleTileVisualState::ValidDrop
			: EHMT_SampleTileVisualState::InvalidDrop);
	}
}

void AHMT_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Spectating never survives a match-phase change: Preparation -> CombatLock drops a prep-
	// spectate so the player sees their own fight start; Combat -> Results drops a combat-
	// spectate so their own board is restored before the next round. A cheap enum compare, safe
	// every tick.
	{
		const EHMT_MatchPhase CurrentPhase = GetCurrentMatchPhase();
		if (bHasObservedMatchPhase && CurrentPhase != LastObservedMatchPhase && SpectateTarget)
		{
			SetSpectateTarget(nullptr);
			UE_LOG(LogTemp, Log, TEXT("[HMT Sample] Spectate: auto-stopped on phase change, viewing own board."));
		}
		LastObservedMatchPhase = CurrentPhase;
		bHasObservedMatchPhase = true;
	}

	// Hover feedback only while dragging — no highlight churn during normal mouse movement.
	if (DragFromTile)
	{
		AHMT_BoardTile* Tile = nullptr;
		TraceTileUnderCursor(Tile);
		SetHoveredTile(Tile != DragFromTile ? Tile : nullptr);
	}

	// The lifted unit rides the cursor: intersect the mouse ray with the board's ground plane
	// (no collision involved, so nothing occludes it) and hover the unit above that point.
	if (DraggedUnit)
	{
		FVector RayOrigin, RayDirection;
		if (DeprojectMousePositionToWorld(RayOrigin, RayDirection) && FMath::Abs(RayDirection.Z) > KINDA_SMALL_NUMBER)
		{
			const FVector GroundPoint = FMath::LinePlaneIntersection(
				RayOrigin, RayOrigin + RayDirection * 100000.f,
				FVector(0.f, 0.f, DragOriginLocation.Z), FVector::UpVector);
			DraggedUnit->SetActorLocation(GroundPoint + FVector(0.f, 0.f, HMT_DragLiftHeight));
		}
	}
}

AHMT_GameUnitActor* AHMT_PlayerController::GetSelectedUnit() const
{
	return SelectedUnit.Get();
}

void AHMT_PlayerController::SetSelectedTrait(UHMT_TraitDefinition* Trait)
{
	SelectedTrait = Trait;
	if (Trait)
	{
		SelectedUnit = nullptr;
	}
}

UHMT_TraitDefinition* AHMT_PlayerController::GetSelectedTrait() const
{
	return SelectedTrait.Get();
}

AHMT_GameUnitActor* AHMT_PlayerController::FindUnitOnTile(const AHMT_BoardTile* Tile) const
{
	// Resolved via the TILE's owner, not the local player — this is what lets clicks work on a
	// spectated board, whose tiles share coordinates with the local player's own.
	AHMT_PlayerState* TileOwner = Tile ? Cast<AHMT_PlayerState>(Tile->OwningPlayer) : nullptr;
	// While spectating, the local board DISPLAYS the spectated player's army (see
	// SetLocallySpectated's projection) — clicks on our own tiles must resolve against THEIR
	// occupancy at the same coordinate, or the info panel would read our hidden unit instead.
	if (TileOwner && SpectateTarget && TileOwner == GetPlayerState<AHMT_PlayerState>())
	{
		TileOwner = SpectateTarget.Get();
	}
	if (!TileOwner)
	{
		return nullptr;
	}

	AActor* Occupant = Tile->bIsBoardCell
		? TileOwner->BoardComponent->GetOccupantAt(Tile->BoardCoord)
		: TileOwner->BenchComponent->GetOccupantAt(Tile->BenchSlotIndex);
	return Cast<AHMT_GameUnitActor>(Occupant);
}

AHMT_GameUnitActor* AHMT_PlayerController::FindOwnUnitOnTile(const AHMT_BoardTile* Tile) const
{
	AHMT_PlayerState* SamplePlayerState = GetPlayerState<AHMT_PlayerState>();
	if (!Tile || !SamplePlayerState || Tile->OwningPlayer != SamplePlayerState)
	{
		return nullptr;
	}

	AActor* Occupant = Tile->bIsBoardCell
		? SamplePlayerState->BoardComponent->GetOccupantAt(Tile->BoardCoord)
		: SamplePlayerState->BenchComponent->GetOccupantAt(Tile->BenchSlotIndex);
	return Cast<AHMT_GameUnitActor>(Occupant);
}

bool AHMT_PlayerController::CanDropOn(const AHMT_BoardTile* FromTile, const AHMT_BoardTile* ToTile) const
{
	AHMT_PlayerState* SamplePlayerState = GetPlayerState<AHMT_PlayerState>();
	if (!SamplePlayerState || !ToTile || !ToTile->bInteractable || ToTile->OwningPlayer != SamplePlayerState)
	{
		return false;
	}
	if (ToTile->bIsBoardCell && !SamplePlayerState->BoardComponent->IsValidCoord(ToTile->BoardCoord))
	{
		return false;
	}

	// Mirrors ServerMoveOrSwapUnit's cap rule: only a NET board increase (bench -> empty board
	// cell) is capacity-gated. All inputs here are replicated, so the prediction matches the server.
	if (!FromTile->bIsBoardCell && ToTile->bIsBoardCell && !SamplePlayerState->BoardComponent->GetOccupantAt(ToTile->BoardCoord))
	{
		const UHMT_MatchRulesAsset* MatchRules = SamplePlayerState->ShopComponent->GetMatchRules();
		const int32 MaxBoardUnits = MatchRules && MatchRules->BaseBoardCapacity > 0
			? MatchRules->BaseBoardCapacity + SamplePlayerState->BoardComponent->GetBonusUnitCapacity()
			: TNumericLimits<int32>::Max();
		if (SamplePlayerState->BoardComponent->GetAllOccupants().Num() >= MaxBoardUnits)
		{
			return false;
		}
	}

	return true;
}

void AHMT_PlayerController::OnDragStart()
{
	AHMT_BoardTile* Tile = nullptr;
	TraceTileUnderCursor(Tile);

	// ANY occupied tile (own or spectated) is a click candidate for the unit-info panel —
	// resolved against the release position in OnDragEnd.
	ClickCandidateTile = FindUnitOnTile(Tile) ? Tile : nullptr;

	// Spectating is view-only: clicks (info panel) still work, but no drags — the local player's
	// own units are hidden while viewing someone else's board, and an invisible drag would still
	// fire real move RPCs against the local board.
	if (SpectateTarget)
	{
		return;
	}

	// A drag only starts on one of OUR OWN occupied tiles — empty tiles have nothing to lift, and
	// a spectated player's tiles share coordinates with ours (see AHMT_BoardTile::OwningPlayer).
	AHMT_GameUnitActor* Unit = FindOwnUnitOnTile(Tile);
	if (!Unit)
	{
		return;
	}

	DragFromTile = Tile;
	DraggedUnit = Unit;
	DragOriginLocation = Unit->GetActorLocation();
	// Collision off while lifted, so the cursor's tile trace sees through the carried unit.
	Unit->SetActorEnableCollision(false);

	// Drag-context tile states: the pickup tile reads as Selected; every OTHER of our own tiles
	// currently holding a unit reads as Occupied (a drop there swaps). All local-only actors, so
	// TActorIterator is cheap here (a few dozen tiles, once per drag start).
	Tile->SetVisualState(EHMT_SampleTileVisualState::Selected);
	if (AHMT_PlayerState* SamplePlayerState = GetPlayerState<AHMT_PlayerState>())
	{
		for (TActorIterator<AHMT_BoardTile> It(GetWorld()); It; ++It)
		{
			AHMT_BoardTile* Candidate = *It;
			if (Candidate == Tile || Candidate->OwningPlayer != SamplePlayerState || !Candidate->bInteractable)
			{
				continue;
			}
			if (FindOwnUnitOnTile(Candidate))
			{
				Candidate->SetVisualState(EHMT_SampleTileVisualState::Occupied);
				OccupiedDuringDrag.Add(Candidate);
			}
		}
	}
}

void AHMT_PlayerController::OnDragEnd()
{
	SetHoveredTile(nullptr);

	// Click-vs-drag: released over the same tile the press found a unit on = a click — surface
	// that unit to the HUD. Anywhere else with no unit = empty-space click, surfaced as null (the
	// HUD's cue to close its panel). Runs before the drag handling below so clicking your own unit
	// both selects it AND leaves it exactly where it was (same-tile drops were no-ops anyway).
	{
		AHMT_BoardTile* ReleaseTile = nullptr;
		TraceTileUnderCursor(ReleaseTile);
		if (ClickCandidateTile && ReleaseTile == ClickCandidateTile)
		{
			SelectedUnit = FindUnitOnTile(ReleaseTile);
			SelectedTrait = nullptr;   // one info panel at a time
			OnUnitClicked.Broadcast(SelectedUnit.Get());
		}
		else if (!FindUnitOnTile(ReleaseTile))
		{
			SelectedUnit = nullptr;
			SelectedTrait = nullptr;
			OnUnitClicked.Broadcast(nullptr);
		}
		ClickCandidateTile = nullptr;
	}

	// Clear every drag-context state back to base — Selected on the origin, Occupied on the rest.
	for (const TWeakObjectPtr<AHMT_BoardTile>& Held : OccupiedDuringDrag)
	{
		if (Held.IsValid())
		{
			Held->SetVisualState(EHMT_SampleTileVisualState::Base);
		}
	}
	OccupiedDuringDrag.Reset();
	if (DragFromTile)
	{
		DragFromTile->SetVisualState(EHMT_SampleTileVisualState::Base);
	}

	AHMT_BoardTile* FromTile = DragFromTile;
	AHMT_GameUnitActor* Unit = DraggedUnit;
	DragFromTile = nullptr;
	DraggedUnit = nullptr;
	if (!FromTile)
	{
		return;
	}
	if (Unit)
	{
		Unit->SetActorEnableCollision(true);
	}

	AHMT_BoardTile* ToTile = nullptr;
	TraceTileUnderCursor(ToTile);

	AHMT_PlayerState* SamplePlayerState = GetPlayerState<AHMT_PlayerState>();
	const bool bValidDrop = SamplePlayerState && ToTile && ToTile != FromTile && CanDropOn(FromTile, ToTile);
	if (!bValidDrop)
	{
		// Nowhere legal to put it down — glide back to the pickup point.
		if (Unit)
		{
			Unit->MoveSmoothly(DragOriginLocation);
		}
		return;
	}

	// Optimistic local snap onto the drop tile; the server places it at the same spot instantly
	// (see ServerMoveOrSwapUnit), so the replicated update confirms rather than corrects.
	if (Unit)
	{
		Unit->SetActorLocation(ToTile->GetActorLocation() + FVector(0.f, 0.f, HMT_UnitAboveTileOffset));
	}

	FHMT_PlacementSlot From;
	From.bIsBoard = FromTile->bIsBoardCell;
	From.BoardCoord = FromTile->BoardCoord;
	From.BenchSlot = FromTile->BenchSlotIndex;

	FHMT_PlacementSlot To;
	To.bIsBoard = ToTile->bIsBoardCell;
	To.BoardCoord = ToTile->BoardCoord;
	To.BenchSlot = ToTile->BenchSlotIndex;

	SamplePlayerState->ServerMoveOrSwapUnit(From, To);
}

void AHMT_PlayerController::BuySlot(int32 SlotIndex)
{
	if (AHMT_PlayerState* SamplePlayerState = GetPlayerState<AHMT_PlayerState>())
	{
		SamplePlayerState->ServerBuyAndPlaceUnit(SlotIndex);
	}
}

void AHMT_PlayerController::DebugBuySlot0() { BuySlot(0); }
void AHMT_PlayerController::DebugBuySlot1() { BuySlot(1); }
void AHMT_PlayerController::DebugBuySlot2() { BuySlot(2); }
void AHMT_PlayerController::DebugBuySlot3() { BuySlot(3); }
void AHMT_PlayerController::DebugBuySlot4() { BuySlot(4); }

void AHMT_PlayerController::DebugReroll()
{
	if (AHMT_PlayerState* SamplePlayerState = GetPlayerState<AHMT_PlayerState>())
	{
		SamplePlayerState->ServerRerollShop();
	}
}

void AHMT_PlayerController::DebugForceEndPreparation()
{
	if (AHMT_PlayerState* SamplePlayerState = GetPlayerState<AHMT_PlayerState>())
	{
		SamplePlayerState->ServerForceEndPreparation();
	}
}
