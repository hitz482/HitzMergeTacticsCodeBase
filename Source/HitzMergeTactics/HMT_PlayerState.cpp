#include "HMT_PlayerState.h"
#include "HMT_GameState.h"
#include "HMT_CombatPlaybackDriver.h"
#include "HMT_BoardTile.h"
#include "HMT_GameUnitActor.h"
#include "HMT_TopDownPawn.h"
#include "Grid/HMT_BoardComponent.h"
#include "Grid/HMT_BenchComponent.h"
#include "Units/HMT_UnitCollectionComponent.h"
#include "Units/HMT_UnitInstanceComponent.h"
#include "Units/HMT_UnitActor.h"
#include "Units/HMT_UnitDefinition.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Synergy/HMT_SynergyManagerComponent.h"
#include "Economy/HMT_ShopComponent.h"
#include "Economy/HMT_ShopPoolComponent.h"
#include "Combat/HMT_CombatPlaybackComponent.h"
#include "Match/HMT_MatchStateComponent.h"
#include "Ruler/HMT_RulerActor.h"
#include "HMT_GameRulerActor.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Algo/Count.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"

AHMT_PlayerState::AHMT_PlayerState()
{
	BoardComponent = CreateDefaultSubobject<UHMT_BoardComponent>(TEXT("BoardComponent"));
	BenchComponent = CreateDefaultSubobject<UHMT_BenchComponent>(TEXT("BenchComponent"));
	UnitCollectionComponent = CreateDefaultSubobject<UHMT_UnitCollectionComponent>(TEXT("UnitCollectionComponent"));
	SynergyManagerComponent = CreateDefaultSubobject<UHMT_SynergyManagerComponent>(TEXT("SynergyManagerComponent"));
	ShopComponent = CreateDefaultSubobject<UHMT_ShopComponent>(TEXT("ShopComponent"));
	CombatPlaybackComponent = CreateDefaultSubobject<UHMT_CombatPlaybackComponent>(TEXT("CombatPlaybackComponent"));
	CombatPlaybackDriver = CreateDefaultSubobject<UHMT_CombatPlaybackDriver>(TEXT("CombatPlaybackDriver"));

	// Wires the Phase 9 sample health bar widget into combat playback by default — buyers can
	// swap this for their own WBP by changing CombatPlaybackDriver->HealthBarWidgetClass on a
	// Blueprint child of this class instead of editing C++.
	static ConstructorHelpers::FClassFinder<UUserWidget> HealthBarWidgetFinder(TEXT("/Game/HitzMergeTactics/UI/WBP_HMT_HealthBar"));
	if (HealthBarWidgetFinder.Succeeded())
	{
		CombatPlaybackDriver->HealthBarWidgetClass = HealthBarWidgetFinder.Class;
	}
	static ConstructorHelpers::FClassFinder<UUserWidget> CastBarWidgetFinder(TEXT("/Game/HitzMergeTactics/UI/WBP_HMT_CastBar"));
	if (CastBarWidgetFinder.Succeeded())
	{
		CombatPlaybackDriver->CastBarWidgetClass = CastBarWidgetFinder.Class;
	}

	// Prefers a Blueprint subclass (buyer-authored OnStarLevelChanged merge feedback, see
	// AHMT_UnitActor::VisualsByStar/ScaleByStar) if one exists at this path; falls back to the
	// plain C++ class — same degrade-safely pattern as the widget finders above.
	UnitActorClass = AHMT_GameUnitActor::StaticClass();
	static ConstructorHelpers::FClassFinder<AHMT_UnitActor> UnitActorFinder(TEXT("/Game/HitzMergeTactics/Blueprints/BP_HMT_GameUnit"));
	if (UnitActorFinder.Succeeded())
	{
		UnitActorClass = UnitActorFinder.Class;
	}

	RulerActorClass = AHMT_GameRulerActor::StaticClass();

	UnitCollectionComponent->OnUnitMerged.AddDynamic(this, &AHMT_PlayerState::HandleUnitMerged);
}

void AHMT_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHMT_PlayerState, ReplicatedBoardLayout);
	DOREPLIFETIME(AHMT_PlayerState, ReplicatedBoardOrigin);
	DOREPLIFETIME(AHMT_PlayerState, ReplicatedBenchSlotCount);
}

void AHMT_PlayerState::InitializeForMatch(UHMT_BoardLayoutAsset* BoardLayout, int32 BenchSlotCount, UHMT_MatchRulesAsset* MatchRules, const TArray<UHMT_UnitDefinition*>& AvailableUnits, UHMT_ShopPoolComponent* ShopPool, FVector BoardWorldOrigin)
{
	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] InitializeForMatch called for %s. BoardLayout=%s MatchRules=%s"),
		*GetName(), BoardLayout ? TEXT("set") : TEXT("NULL"), MatchRules ? TEXT("set") : TEXT("NULL"));

	if (BoardLayout)
	{
		BoardComponent->InitializeBoard(BoardLayout);

		// Replicating the data (not the tile actors) so every client's OnRep spawns its own local
		// copy — see the header comment on ReplicatedBoardLayout for why actor replication doesn't work here.
		ReplicatedBoardLayout = BoardLayout;
		ReplicatedBoardOrigin = BoardWorldOrigin;
		ReplicatedBenchSlotCount = BenchSlotCount;

		// The server/listen-host never gets its own OnRep for a property it just set locally, so
		// go through the same visuals entry point directly.
		TrySpawnLocalBoardVisuals();
	}
	BenchComponent->InitializeBench(BenchSlotCount);
	if (MatchRules)
	{
		// Merge rules are match-level config (2 = pairs merge) — authored once on the rules asset
		// instead of per-PlayerState component defaults.
		UnitCollectionComponent->CopiesRequiredForMerge = MatchRules->CopiesRequiredForMerge;
		UnitCollectionComponent->MaxStarLevel = MatchRules->MaxStarLevel;

		ShopComponent->InitializeShop(MatchRules, AvailableUnits);

		// InitializeShop only allocates empty slots — nothing rolls them without an explicit call.
		if (ShopPool)
		{
			ShopComponent->RollShop(ShopPool);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] InitializeForMatch: ShopPool is NULL, shop was not rolled — buying will fail."));
		}
	}
}

void AHMT_PlayerState::HandleUnitMerged(AActor* SurvivorUnit, FGameplayTag UnitID, int32 NewStarLevel)
{
	// Merge feedback for players comes from FX/UI (OnUnitMerged is BlueprintAssignable for that);
	// this log is diagnostics only — no on-screen debug text in a shipping-quality sample.
	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] Merged: %s -> %d-star (survivor %s) for %s."),
		*UnitID.ToString(), NewStarLevel, SurvivorUnit ? *SurvivorUnit->GetName() : TEXT("NULL"), *GetPlayerName());
}

void AHMT_PlayerState::OnRep_DebugBoardLayout()
{
	// InitializeBoard only ran on the server (inside the GameMode). The client's BoardComponent
	// therefore has no grid layout, so GridToWorld would return zero for every cell and stack all
	// tiles at the origin — initialize it locally here before spawning this machine's tiles.
	if (ReplicatedBoardLayout)
	{
		BoardComponent->InitializeBoard(ReplicatedBoardLayout);
	}
	// Bench tiles and pawn repositioning piggyback on this same RepNotify (rather than giving
	// ReplicatedBenchSlotCount its own) so there's no second-RepNotify ordering question.
	TrySpawnLocalBoardVisuals();
}

bool AHMT_PlayerState::TryDetermineLocallyViewed(bool& bOutIsLocal) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// A dedicated server has no local players at all — determinable immediately, never local.
	// (Its PlayerControllerList still contains every REMOTE connection's PC, which is why the old
	// GetFirstPlayerController()->PlayerState == this check was wrong there: it "passed" for
	// whichever player happened to be first.)
	if (World->GetNetMode() == NM_DedicatedServer)
	{
		bOutIsLocal = false;
		return true;
	}

	const APlayerController* LocalPC = nullptr;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (PC && PC->IsLocalPlayerController())
		{
			LocalPC = PC;
			break;
		}
	}

	// No local PC yet, or its PlayerState link hasn't replicated — the caller must retry, NOT
	// assume "not local": on clients this PlayerState's own OnReps routinely fire before the
	// PlayerController->PlayerState link exists.
	if (!LocalPC || !LocalPC->PlayerState)
	{
		return false;
	}

	bOutIsLocal = LocalPC->PlayerState == this;
	return true;
}

void AHMT_PlayerState::TrySpawnLocalBoardVisuals()
{
	if (bLocalBoardVisualsSpawned || !ReplicatedBoardLayout)
	{
		return;
	}

	bool bIsLocal = false;
	if (!TryDetermineLocallyViewed(bIsLocal))
	{
		// Local player identity not established yet (early join replication order) — retry shortly.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(LocalVisualsRetryHandle, this, &AHMT_PlayerState::TrySpawnLocalBoardVisuals, 0.25f, false);
		}
		return;
	}

	if (!bIsLocal)
	{
		// Another player's board: stays data-only on this machine. Combat/spectating shows
		// opponents via the snapshot-driven playback proxies, not by rendering every board.
		return;
	}

	bLocalBoardVisualsSpawned = true;
	SpawnDebugBoardTiles(ReplicatedBoardLayout, ReplicatedBoardOrigin, OwnBoardTiles);
	SpawnDebugBenchTiles(ReplicatedBenchSlotCount, ReplicatedBoardOrigin, OwnBoardTiles);
	RepositionLocalPawnAboveBoard(ReplicatedBoardOrigin);

	// Opponent changes every round (unlike the board layout, set once) — poll rather than chase a
	// cross-actor RepNotify (UHMT_MatchStateComponent lives on GameState, not this PlayerState).
	// Fire once immediately so the opponent board (or its placeholder) appears alongside the
	// player's own board instead of waiting out the first 1s tick — SetTimer's bLoop doesn't call
	// the callback until the first interval elapses on its own.
	TryUpdateOpponentBoardVisuals();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(OpponentTilesPollHandle, this, &AHMT_PlayerState::TryUpdateOpponentBoardVisuals, 1.f, true);
	}
}

void AHMT_PlayerState::SpawnDebugBoardTiles(UHMT_BoardLayoutAsset* BoardLayout, FVector BoardWorldOrigin, TArray<TObjectPtr<AHMT_BoardTile>>& OutTiles, UMaterialInterface* TileMaterialOverride)
{
	UWorld* World = GetWorld();
	if (!World || !BoardLayout)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] SpawnDebugBoardTiles: missing World or BoardLayout."));
		return;
	}

	int32 SpawnedCount = 0;
	const bool bIsHex = BoardLayout->Topology == EHMT_GridTopology::Hex;

	for (int32 Column = 0; Column < BoardLayout->Columns; ++Column)
	{
		for (int32 Row = 0; Row < BoardLayout->Rows; ++Row)
		{
			const FHMT_GridCoord Coord(Column, Row);
			const bool bBlocked = BoardLayout->BlockedCells.Contains(Coord);
			// Blocked cells render only if the layout provides a material for them; otherwise skipped.
			if (bBlocked && !BoardLayout->BlockedTileMaterial)
			{
				continue;
			}

			// Small +Z lift so the flat tiles sit just above the level floor instead of
			// z-fighting with / hiding inside it.
			const FVector CellOffset = BoardComponent->GridToWorld(Coord);
			const FVector TileLocation = BoardWorldOrigin + CellOffset + FVector(0.f, 0.f, 5.f);

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AHMT_BoardTile* Tile = World->SpawnActor<AHMT_BoardTile>(AHMT_BoardTile::StaticClass(), TileLocation, FRotator::ZeroRotator, SpawnParams);
			if (!Tile)
			{
				continue;
			}

			// Not replicated: each machine spawns its own local, independent copy of this purely
			// decorative actor via SpawnDebugBoardTiles — see header comment on ReplicatedBoardLayout.
			Tile->bIsBoardCell = true;
			Tile->BoardCoord = Coord;
			Tile->bInteractable = !bBlocked;   // blocked tiles are scenery, not drag-drop targets
			Tile->OwningPlayer = this;
			Tile->InitializeTileVisual(
				BoardLayout->TileMesh,
				bIsHex,
				bBlocked ? BoardLayout->BlockedTileMaterial.Get() : (TileMaterialOverride ? TileMaterialOverride : BoardLayout->TileMaterial.Get()),
				bBlocked ? nullptr : BoardLayout->HighlightMaterial.Get(),
				BoardLayout->CellSize,
				BoardLayout->TileVisualScale);
			if (!bBlocked)
			{
				Tile->SetStateMaterials(
					BoardLayout->ValidDropMaterial.Get(),
					BoardLayout->InvalidDropMaterial.Get(),
					BoardLayout->SelectedTileMaterial.Get(),
					BoardLayout->OccupiedTileMaterial.Get());
			}
			OutTiles.Add(Tile);
			++SpawnedCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] SpawnDebugBoardTiles: spawned %d tiles at origin %s."), SpawnedCount, *BoardWorldOrigin.ToString());
}

void AHMT_PlayerState::TryUpdateOpponentBoardVisuals()
{
	auto ClearOpponentTiles = [this]()
	{
		for (AHMT_BoardTile* Tile : OpponentTiles)
		{
			if (Tile)
			{
				Tile->Destroy();
			}
		}
		OpponentTiles.Reset();
		CachedOpponentForTiles = nullptr;
		bOpponentTilesSpawned = false;
	};

	if (!ReplicatedBoardLayout || !ReplicatedBoardLayout->bShowOpponentTiles)
	{
		if (bOpponentTilesSpawned)
		{
			ClearOpponentTiles();
		}
		return;
	}

	AHMT_GameState* SampleGameState = GetWorld() ? GetWorld()->GetGameState<AHMT_GameState>() : nullptr;
	UHMT_MatchStateComponent* MatchState = SampleGameState ? SampleGameState->GetMatchState() : nullptr;
	APlayerState* NewOpponent = MatchState ? MatchState->GetCurrentOpponentFor(this) : nullptr;

	if (bOpponentTilesSpawned && NewOpponent == CachedOpponentForTiles.Get())
	{
		// No change since the last poll — including both null (still showing the placeholder) and
		// the same opponent as last round (pairing is recency-weighted, no guaranteed rotation).
		return;
	}

	ClearOpponentTiles();

	// A live paired opponent shows THEIR board layout; no live opponent (solo lobby, ghost-paired
	// round, or simply round 1 before any pairing has happened) shows a PLACEHOLDER grid from the
	// local player's own layout instead — the flag's whole point is formation planning against the
	// opposite board half, which shouldn't require a second live player to be useful.
	AHMT_PlayerState* OpponentSample = Cast<AHMT_PlayerState>(NewOpponent);
	if (OpponentSample && !OpponentSample->ReplicatedBoardLayout)
	{
		// Live opponent exists but their layout hasn't replicated yet — retry on the next poll.
		return;
	}
	UHMT_BoardLayoutAsset* LayoutToShow = OpponentSample ? OpponentSample->ReplicatedBoardLayout.Get() : ReplicatedBoardLayout.Get();

	// Synthetic origin: just past the far edge of the local player's own board, offset along +Y —
	// NOT the opponent's real BoardWorldOrigin, which is PlayerIndex-based and could be far away.
	// Depth comes from the grid layout's ACTUAL world row spacing (GridToWorld), not Rows*CellSize:
	// that formula overestimates hex depth (hex rows pack tighter than CellSize) and, with the old
	// +2-cell pad, left square boards separated by a visibly huge gap while hex looked right. Far
	// edge = last row's center + one full CellSize (own tile's far half + opponent tile's near
	// half), then a quarter-cell seam so the two halves still read as separate boards.
	const int32 OwnRows = FMath::Max(1, ReplicatedBoardLayout->Rows);
	const float LastRowCenterY = BoardComponent->GridToWorld(FHMT_GridCoord(0, OwnRows - 1)).Y;
	// Exactly one cell step past the last row: the opponent half reads as a seamless continuation
	// of the same grid (neighbor-square spacing), per explicit user direction — no extra seam.
	const FVector SyntheticOrigin = ReplicatedBoardOrigin
		+ FVector(0.f, LastRowCenterY + ReplicatedBoardLayout->CellSize, 0.f);

	// Called AS the tile owner's own PlayerState so SpawnDebugBoardTiles's OwningPlayer=this lands
	// correctly (a live opponent's tiles are already non-placeable via the OwningPlayer checks) —
	// private access across two instances of the same class is legal C++.
	AHMT_PlayerState* TileOwner = OpponentSample ? OpponentSample : this;
	// OpponentTileMaterial unset falls back to TileMaterial per the asset's own doc comment — the
	// procedural-mesh path in InitializeTileVisual has no fallback of its own (unlike the static-mesh
	// path, which reuses the mesh's authored material), so passing a raw null here left placeholder
	// opponent boards with no material call at all, rendering the engine's default fallback color.
	UMaterialInterface* OpponentMaterial = ReplicatedBoardLayout->OpponentTileMaterial
		? ReplicatedBoardLayout->OpponentTileMaterial.Get()
		: ReplicatedBoardLayout->TileMaterial.Get();
	TileOwner->SpawnDebugBoardTiles(LayoutToShow, SyntheticOrigin, OpponentTiles, OpponentMaterial);

	// Placeholder tiles are owned by the LOCAL player (no opponent exists to own them), which would
	// make them valid drop targets — force every opponent-side tile non-interactable regardless of
	// owner so the opposite half is always read-only scenery.
	for (AHMT_BoardTile* Tile : OpponentTiles)
	{
		if (Tile)
		{
			Tile->bInteractable = false;
		}
	}

	CachedOpponentForTiles = NewOpponent;
	bOpponentTilesSpawned = true;

	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] Opponent-side tiles spawned (%d): %s."),
		OpponentTiles.Num(), OpponentSample ? *OpponentSample->GetPlayerName() : TEXT("placeholder (no live opponent)"));
}

FVector AHMT_PlayerState::GetBenchSlotWorldLocation(int32 SlotIndex) const
{
	const float Spacing = ReplicatedBoardLayout ? ReplicatedBoardLayout->CellSize : 100.f;
	// One row along local +X (same axis GameMode spaces players' boards apart on), offset behind
	// the board along -Y by a fixed two-cell gap so it never overlaps board rows.
	const float RowOffset = ReplicatedBoardLayout ? -(ReplicatedBoardLayout->Rows * Spacing * 0.75f + Spacing * 2.f) : -Spacing * 2.f;
	return ReplicatedBoardOrigin + FVector(SlotIndex * Spacing, RowOffset, 0.f);
}

FVector AHMT_PlayerState::GetRulerWorldLocation() const
{
	// One slot to the left of bench slot 0, same row — reads as "standing next to your bench"
	// rather than a separate presence floating behind it. Reuses GetBenchSlotWorldLocation's own
	// row/spacing math verbatim rather than re-deriving it.
	return GetBenchSlotWorldLocation(-1);
}

void AHMT_PlayerState::EnsureRulerActorSpawned()
{
	const AHMT_GameState* SampleGameState = GetWorld() ? GetWorld()->GetGameState<AHMT_GameState>() : nullptr;
	const UHMT_MatchStateComponent* MatchState = SampleGameState ? SampleGameState->GetMatchState() : nullptr;

	UHMT_RulerDefinition* Ruler = nullptr;
	int32 UnusedHP = 0;
	if (!MatchState || !MatchState->GetPlayerRuler(this, Ruler, UnusedHP) || !Ruler)
	{
		return;
	}

	if (RulerActor)
	{
		RulerActor->SetPose(EHMT_RulerPose::Idle);
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !RulerActorClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	RulerActor = World->SpawnActor<AHMT_RulerActor>(RulerActorClass, FTransform(GetRulerWorldLocation()), SpawnParams);
	if (RulerActor)
	{
		RulerActor->InitializeFromDefinition(Ruler);
	}
}

void AHMT_PlayerState::SpawnDebugBenchTiles(int32 SlotCount, FVector BoardWorldOrigin, TArray<TObjectPtr<AHMT_BoardTile>>& OutTiles)
{
	UWorld* World = GetWorld();
	if (!World || SlotCount <= 0)
	{
		return;
	}

	const float BenchCellSize = ReplicatedBoardLayout ? ReplicatedBoardLayout->CellSize : 100.f;
	const float BenchTileVisualScale = ReplicatedBoardLayout ? ReplicatedBoardLayout->TileVisualScale : 1.f;

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		const FVector TileLocation = GetBenchSlotWorldLocation(SlotIndex) + FVector(0.f, 0.f, 5.f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AHMT_BoardTile* Tile = World->SpawnActor<AHMT_BoardTile>(AHMT_BoardTile::StaticClass(), TileLocation, FRotator::ZeroRotator, SpawnParams);
		if (!Tile)
		{
			continue;
		}

		Tile->bIsBoardCell = false;
		Tile->BenchSlotIndex = SlotIndex;
		Tile->OwningPlayer = this;
		// Bench slots are always quad-shaped scenery, regardless of board topology — the bench
		// has no grid adjacency of its own (see UHMT_BenchComponent), so "hex bench slots" would be
		// a meaningless shape, not a real topology choice.
		Tile->InitializeTileVisual(
			ReplicatedBoardLayout ? ReplicatedBoardLayout->TileMesh.Get() : nullptr,
			/*bIsHex=*/ false,
			ReplicatedBoardLayout ? ReplicatedBoardLayout->TileMaterial.Get() : nullptr,
			ReplicatedBoardLayout ? ReplicatedBoardLayout->HighlightMaterial.Get() : nullptr,
			BenchCellSize,
			BenchTileVisualScale);
		OutTiles.Add(Tile);
	}

	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] SpawnDebugBenchTiles: spawned %d slots at origin %s."), SlotCount, *BoardWorldOrigin.ToString());
}

FVector AHMT_PlayerState::GetBoardCenterWorld() const
{
	if (!ReplicatedBoardLayout)
	{
		return ReplicatedBoardOrigin;
	}

	// Midpoint of the two corner cells, through the same GridToWorld the tiles use — stays correct
	// for any topology/cell size instead of hand-approximating hex spacing.
	const FVector CornerA = BoardComponent->GridToWorld(FHMT_GridCoord(0, 0));
	const FVector CornerB = BoardComponent->GridToWorld(FHMT_GridCoord(ReplicatedBoardLayout->Columns - 1, ReplicatedBoardLayout->Rows - 1));
	return ReplicatedBoardOrigin + (CornerA + CornerB) * 0.5f;
}

void AHMT_PlayerState::RepositionLocalPawnAboveBoard(FVector BoardWorldOrigin) const
{
	const UWorld* World = GetWorld();
	APlayerController* LocalPC = World ? World->GetFirstPlayerController() : nullptr;
	AHMT_TopDownPawn* Pawn = LocalPC ? Cast<AHMT_TopDownPawn>(LocalPC->GetPawn()) : nullptr;
	if (!Pawn)
	{
		return;
	}

	Pawn->FrameBoard(GetBoardCenterWorld());
}

void AHMT_PlayerState::RefreshOwnedUnitVisibility() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AHMT_GameUnitActor> It(World); It; ++It)
	{
		if (It->GetOwner() == this)
		{
			It->RefreshLocalVisibility();
		}
	}
}

void AHMT_PlayerState::RefreshRulerVisibility() const
{
	if (AHMT_GameRulerActor* GameRuler = Cast<AHMT_GameRulerActor>(RulerActor))
	{
		GameRuler->RefreshLocalVisibility();
	}
}

void AHMT_PlayerState::SetLocallySpectated(bool bSpectated)
{
	if (bLocallySpectated == bSpectated)
	{
		return;
	}
	bLocallySpectated = bSpectated;

	if (bSpectated)
	{
		// Board projection is Preparation-only content (formation scouting) — during Combat/
		// other phases this is a no-op and the spectated view comes entirely from
		// UHMT_CombatPlaybackDriver's ActiveSource swap instead (their fight, not their board).
		if (IsPreparationPhaseNow())
		{
			ApplySpectateRemap();
		}
	}
	else
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(SpectateRemapHandle);
		}
		ClearSpectateGhosts();
	}

	RefreshOwnedUnitVisibility();
}

void AHMT_PlayerState::ApplySpectateRemap()
{
	// Self-terminating: also bails (and stops rescheduling) the instant the phase leaves
	// Preparation, as a defense-in-depth backstop alongside the controller's own auto-stop.
	if (!bLocallySpectated || !IsPreparationPhaseNow())
	{
		ClearSpectateGhosts();
		return;
	}

	UWorld* World = GetWorld();
	AHMT_PlayerState* LocalPlayerState = nullptr;
	if (World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (PC && PC->IsLocalPlayerController())
			{
				LocalPlayerState = PC->GetPlayerState<AHMT_PlayerState>();
				break;
			}
		}
	}

	if (LocalPlayerState && LocalPlayerState != this)
	{
		// Rebuild only when the target actually rearranged their board since the last pass — the
		// target can place/move/sell during their own Preparation, but most 0.5s ticks find nothing
		// changed. Tearing the ghosts down and respawning them unconditionally restarted every
		// looping idle from frame 0, which is the spectate flicker. ClearSpectateGhosts empties the
		// signature, so the very first pass (and any post-clear pass) always builds.
		const FString Signature = ComputeSpectateBoardSignature();
		if (Signature != LastSpectateSignature)
		{
			ClearSpectateGhosts();
			SpawnSpectateGhosts(LocalPlayerState);
			LastSpectateSignature = Signature;
		}
	}

	if (World)
	{
		World->GetTimerManager().SetTimer(SpectateRemapHandle, this, &AHMT_PlayerState::ApplySpectateRemap, 0.5f, false);
	}
}

FString AHMT_PlayerState::ComputeSpectateBoardSignature() const
{
	if (!BoardComponent)
	{
		return FString();
	}

	FString Signature;
	for (int32 R = 0; R < BoardComponent->GetRows(); ++R)
	{
		for (int32 Q = 0; Q < BoardComponent->GetColumns(); ++Q)
		{
			const FHMT_GridCoord Coord(Q, R);
			AActor* Occupant = BoardComponent->GetOccupantAt(Coord);
			const UHMT_UnitInstanceComponent* UnitInstance = Occupant ? Occupant->FindComponentByClass<UHMT_UnitInstanceComponent>() : nullptr;
			const UHMT_UnitDefinition* Definition = UnitInstance ? UnitInstance->GetUnitDefinition() : nullptr;
			if (Definition)
			{
				Signature += FString::Printf(TEXT("%d,%d:%s@%d;"), Q, R, *Definition->UnitID.ToString(), UnitInstance->GetStarLevel());
			}
		}
	}
	return Signature;
}

void AHMT_PlayerState::SpawnSpectateGhosts(const AHMT_PlayerState* OriginOwner)
{
	UWorld* World = GetWorld();
	if (!OriginOwner || !BoardComponent || !World)
	{
		return;
	}

	// +Z lift matches ServerBuyAndPlaceUnit's spawn placement above the tiles.
	const FVector UnitLift(0.f, 0.f, 50.f);

	// Board only, deliberately — a spectator scouts placed formation, never bench/future buys.
	for (int32 R = 0; R < BoardComponent->GetRows(); ++R)
	{
		for (int32 Q = 0; Q < BoardComponent->GetColumns(); ++Q)
		{
			const FHMT_GridCoord Coord(Q, R);
			AActor* Occupant = BoardComponent->GetOccupantAt(Coord);
			const UHMT_UnitInstanceComponent* UnitInstance = Occupant ? Occupant->FindComponentByClass<UHMT_UnitInstanceComponent>() : nullptr;
			const UHMT_UnitDefinition* Definition = UnitInstance ? UnitInstance->GetUnitDefinition() : nullptr;
			if (!Definition)
			{
				continue;
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ASkeletalMeshActor* Ghost = World->SpawnActor<ASkeletalMeshActor>(SpawnParams);
			if (!Ghost)
			{
				continue;
			}

			// Local-only cosmetic — must never replicate (same rule as the combat playback
			// driver's proxies, and for the same reason: this is drawing, not gameplay).
			Ghost->SetReplicates(false);
			USkeletalMeshComponent* GhostMesh = Ghost->GetSkeletalMeshComponent();
			GhostMesh->SetMobility(EComponentMobility::Movable);
			// AlwaysTick so the looping idle keeps advancing even at oblique/edge-on camera angles that
			// would otherwise let the default "only if rendered" option freeze it on a single frame.
			GhostMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			if (!Definition->Mesh.IsNull())
			{
				GhostMesh->SetSkeletalMesh(Definition->Mesh.LoadSynchronous());
			}
			// The ghost is a bare skeletal mesh actor (no unit AnimBlueprint), so without this it renders
			// as a frozen bind pose — play the definition's looping Idle montage so the projected board
			// reads as a live army, matching what the spectated player sees on their own screen.
			if (UAnimMontage* IdleAnim = Definition->GetIdleAnim())
			{
				GhostMesh->PlayAnimation(IdleAnim, Definition->AnimSet.bIdleLoops);
			}
			Ghost->SetActorLocation(OriginOwner->ReplicatedBoardOrigin + BoardComponent->GridToWorld(Coord) + UnitLift);
			SpectateGhostProxies.Add(Ghost);
		}
	}

	// Ghost Ruler: the spectated player's Ruler, projected beside the LOCAL bench (same idea as the
	// unit ghosts above) so a spectator sees whose board this is and its Ruler's idle presence. The
	// real Ruler stays hidden at its own far-away board — this is the projected view, not the real
	// actor. Read via the replicated match state so it resolves the same on clients and the host, and
	// so it works for bot players (whose real Ruler is hidden on every machine, having no local owner).
	const AHMT_GameState* SampleGameState = World->GetGameState<AHMT_GameState>();
	const UHMT_MatchStateComponent* MatchState = SampleGameState ? SampleGameState->GetMatchState() : nullptr;
	UHMT_RulerDefinition* RulerDef = nullptr;
	int32 UnusedRulerHP = 0;
	if (MatchState && MatchState->GetPlayerRuler(this, RulerDef, UnusedRulerHP) && RulerDef && !RulerDef->Mesh.IsNull())
	{
		FActorSpawnParameters RulerSpawnParams;
		RulerSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ASkeletalMeshActor* RulerGhost = World->SpawnActor<ASkeletalMeshActor>(RulerSpawnParams))
		{
			RulerGhost->SetReplicates(false);
			USkeletalMeshComponent* GhostMesh = RulerGhost->GetSkeletalMeshComponent();
			GhostMesh->SetMobility(EComponentMobility::Movable);
			GhostMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			GhostMesh->SetSkeletalMesh(RulerDef->Mesh.LoadSynchronous());
			if (RulerDef->AnimSet.Idle)
			{
				GhostMesh->PlayAnimation(RulerDef->AnimSet.Idle, RulerDef->AnimSet.bIdleLoops);
			}
			RulerGhost->SetActorLocation(OriginOwner->GetRulerWorldLocation());
			SpectateGhostProxies.Add(RulerGhost);
		}
	}
}

void AHMT_PlayerState::ClearSpectateGhosts()
{
	for (ASkeletalMeshActor* Ghost : SpectateGhostProxies)
	{
		if (Ghost)
		{
			Ghost->Destroy();
		}
	}
	SpectateGhostProxies.Reset();
	// Invalidate the change-detection signature so the next ApplySpectateRemap rebuilds from scratch
	// (e.g. after stopping/switching spectate, or the GameMode's stale-ghost safety-net clear).
	LastSpectateSignature.Reset();
}

bool AHMT_PlayerState::IsPreparationPhaseNow() const
{
	const AHMT_GameState* SampleGameState = GetWorld() ? GetWorld()->GetGameState<AHMT_GameState>() : nullptr;
	const UHMT_MatchStateComponent* MatchState = SampleGameState ? SampleGameState->GetMatchState() : nullptr;
	return MatchState && MatchState->GetCurrentPhase() == EHMT_MatchPhase::Preparation;
}

bool AHMT_PlayerState::FindFirstFreeBoardCoord(FHMT_GridCoord& OutCoord) const
{
	const UHMT_MatchRulesAsset* MatchRules = ShopComponent->GetMatchRules();
	const int32 MaxBoardUnits = MatchRules && MatchRules->BaseBoardCapacity > 0
		? MatchRules->BaseBoardCapacity + BoardComponent->GetBonusUnitCapacity()
		: TNumericLimits<int32>::Max();
	if (BoardComponent->GetAllOccupants().Num() >= MaxBoardUnits)
	{
		return false;
	}

	for (int32 R = 0; R < BoardComponent->GetRows(); ++R)
	{
		for (int32 Q = 0; Q < BoardComponent->GetColumns(); ++Q)
		{
			const FHMT_GridCoord Candidate(Q, R);
			if (BoardComponent->IsValidCoord(Candidate) && !BoardComponent->IsCellOccupied(Candidate))
			{
				OutCoord = Candidate;
				return true;
			}
		}
	}
	return false;
}

void AHMT_PlayerState::ServerBuyAndPlaceUnit_Implementation(int32 ShopSlotIndex)
{
	AHMT_GameState* SampleGameState = GetWorld() ? GetWorld()->GetGameState<AHMT_GameState>() : nullptr;
	if (!SampleGameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerBuyAndPlaceUnit: no AHMT_GameState."));
		return;
	}

	// Diagnostic: report the exact shop state so a failed buy points at the real cause (empty slot
	// from an unrolled/misconfigured shop, vs. insufficient gold, vs. an empty pool).
	{
		UHMT_UnitDefinition* SlotUnit = ShopComponent->GetShopSlot(ShopSlotIndex);
		UE_LOG(LogTemp, Log, TEXT("[HMT Sample] Buy attempt slot %d/%d: slotUnit=%s, gold=%d"),
			ShopSlotIndex, ShopComponent->GetShopSlotCount(),
			SlotUnit ? *SlotUnit->UnitID.ToString() : TEXT("EMPTY (shop not rolled, or AvailableUnits/ShopTierOdds unset on the data assets)"),
			ShopComponent->GetGold());
		if (SlotUnit)
		{
			UE_LOG(LogTemp, Log, TEXT("[HMT Sample]   -> tier=%d, poolRemaining=%d, tierCost=(check MatchRules->GoldCostByTier index %d)"),
				SlotUnit->CostTier, SampleGameState->ShopPoolComponent->GetRemainingCount(SlotUnit->UnitID), SlotUnit->CostTier - 1);
		}
	}

	// Outside Preparation the board is locked (snapshots are taken, combat may be replaying):
	// purchases go to the BENCH only, and a full bench rejects the buy outright — never onto the
	// battlefield mid-combat, and no board-first fallback.
	const UHMT_MatchStateComponent* MatchState = SampleGameState->GetMatchState();
	const bool bBoardPlacementAllowed = !MatchState || MatchState->GetCurrentPhase() == EHMT_MatchPhase::Preparation;

	// Board-first (genre convention): an empty battlefield tile is preferred over the bench, so a
	// buy immediately contributes to trait counts (see UHMT_SynergyManagerComponent, deployed-only)
	// instead of sitting inert. Falls back to bench only when no board cell is free or the level cap
	// is hit; reject BEFORE spending gold if NEITHER has room — the old flow bought first and lost
	// the unit (and the gold) on a full board.
	FHMT_GridCoord FreeBoardCoord;
	const bool bHasFreeBoardCoord = bBoardPlacementAllowed && FindFirstFreeBoardCoord(FreeBoardCoord);

	const int32 BenchSlot = bHasFreeBoardCoord ? INDEX_NONE : BenchComponent->FindFirstFreeSlot();
	if (!bHasFreeBoardCoord && BenchSlot == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerBuyAndPlaceUnit: %s full — buy rejected, no gold spent."),
			bBoardPlacementAllowed ? TEXT("board and bench both") : TEXT("bench (board locked outside Preparation) is"));
		return;
	}

	UHMT_UnitDefinition* Bought = ShopComponent->BuyUnit(ShopSlotIndex, SampleGameState->ShopPoolComponent);
	if (!Bought)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerBuyAndPlaceUnit: buy failed for slot %d — see the diagnostic line above for which precondition failed."), ShopSlotIndex);
		return;
	}

	UWorld* World = GetWorld();
	const FVector SpawnLocation = bHasFreeBoardCoord
		? ReplicatedBoardOrigin + BoardComponent->GridToWorld(FreeBoardCoord) + FVector(0.f, 0.f, 50.f)
		: GetBenchSlotWorldLocation(BenchSlot) + FVector(0.f, 0.f, 50.f);

	// Owner drives the per-machine visibility policy (see AHMT_GameUnitActor) — without it,
	// every player's placement units render on every client's screen.
	FActorSpawnParameters UnitSpawnParams;
	UnitSpawnParams.Owner = this;
	AHMT_UnitActor* NewUnit = World->SpawnActor<AHMT_UnitActor>(UnitActorClass, FTransform(SpawnLocation), UnitSpawnParams);
	if (!NewUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerBuyAndPlaceUnit: SpawnActor<AHMT_GameUnitActor> failed."));
		return;
	}

	NewUnit->InitializeFromDefinition(Bought, 1);
	if (bHasFreeBoardCoord)
	{
		BoardComponent->PlaceUnit(NewUnit, FreeBoardCoord);
	}
	else
	{
		BenchComponent->PlaceInSlot(NewUnit, BenchSlot);
	}
	// Run the merge cascade even outside Preparation: a mid-combat bench buy that completes a set
	// should still merge right away (the bench isn't part of the locked board that's currently being
	// simulated, so merging there is safe and is what players expect). Synergy breakpoints are only
	// recomputed in Preparation — they're locked for the round at CombatLock, and a bench-side merge
	// changes no deployed-unit trait counts anyway.
	UnitCollectionComponent->RunMergeCascade();
	if (bBoardPlacementAllowed)
	{
		SynergyManagerComponent->RecomputeSynergies();
	}

	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] ServerBuyAndPlaceUnit: %s to %s. Gold remaining: %d."),
		*Bought->UnitID.ToString(),
		bHasFreeBoardCoord ? *FString::Printf(TEXT("board (%d,%d)"), FreeBoardCoord.Q, FreeBoardCoord.R) : *FString::Printf(TEXT("bench slot %d"), BenchSlot),
		ShopComponent->GetGold());
}

bool AHMT_PlayerState::ServerBuyAndPlaceUnit_Validate(int32 ShopSlotIndex)
{
	return ShopSlotIndex >= 0 && ShopSlotIndex < 32;
}

bool AHMT_PlayerState::ServerRerollShop_Validate()
{
	return true;
}

bool AHMT_PlayerState::ServerForceEndPreparation_Validate()
{
	return true;
}

namespace
{
	bool IsPlacementSlotSane(const FHMT_PlacementSlot& Slot)
	{
		if (Slot.bIsBoard)
		{
			return Slot.BoardCoord.Q >= 0 && Slot.BoardCoord.Q < 64 && Slot.BoardCoord.R >= 0 && Slot.BoardCoord.R < 64;
		}
		return Slot.BenchSlot >= 0 && Slot.BenchSlot < 64;
	}
}

bool AHMT_PlayerState::ServerMoveOrSwapUnit_Validate(FHMT_PlacementSlot From, FHMT_PlacementSlot To)
{
	return IsPlacementSlotSane(From) && IsPlacementSlotSane(To);
}

bool AHMT_PlayerState::ServerSellUnitAt_Validate(FHMT_PlacementSlot Slot)
{
	return IsPlacementSlotSane(Slot);
}

bool AHMT_PlayerState::ServerToggleShopLock_Validate()
{
	return true;
}

void AHMT_PlayerState::ServerToggleShopLock_Implementation()
{
	ShopComponent->SetLocked(!ShopComponent->IsLocked());
	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] ServerToggleShopLock: shop now %s."), ShopComponent->IsLocked() ? TEXT("LOCKED") : TEXT("unlocked"));
}

void AHMT_PlayerState::ServerSellUnitAt_Implementation(FHMT_PlacementSlot Slot)
{
	// Preparation-only, same rule as buying (see ServerBuyAndPlaceUnit's bBoardPlacementAllowed
	// gate). Selling mid-combat destroyed the real placement unit out from under a fight already
	// locked into an immutable snapshot at CombatLock — the combat's own proxies kept fighting
	// unaffected, but the player's real board/bench permanently lost the unit early, leaving
	// Results/next Preparation looking at a board that silently doesn't match what just fought.
	if (!IsPreparationPhaseNow())
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerSellUnitAt: rejected — selling is only allowed during Preparation."));
		return;
	}

	AHMT_GameState* SampleGameState = GetWorld() ? GetWorld()->GetGameState<AHMT_GameState>() : nullptr;
	if (!SampleGameState)
	{
		return;
	}

	AActor* Occupant = Slot.bIsBoard ? BoardComponent->GetOccupantAt(Slot.BoardCoord) : BenchComponent->GetOccupantAt(Slot.BenchSlot);
	const UHMT_UnitInstanceComponent* Instance = Occupant ? Occupant->FindComponentByClass<UHMT_UnitInstanceComponent>() : nullptr;
	if (!Instance || !Instance->GetUnitDefinition())
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerSellUnitAt: slot is empty."));
		return;
	}

	if (!ShopComponent->SellUnit(Instance->GetUnitDefinition(), Instance->GetStarLevel(), SampleGameState->ShopPoolComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerSellUnitAt: SellUnit failed."));
		return;
	}

	if (Slot.bIsBoard)
	{
		BoardComponent->RemoveUnitAt(Slot.BoardCoord);
	}
	else
	{
		BenchComponent->RemoveFromSlot(Slot.BenchSlot);
	}
	Occupant->Destroy();
	SynergyManagerComponent->RecomputeSynergies();

	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] ServerSellUnitAt: sold %s (star %d). Gold: %d."),
		*Instance->GetUnitDefinition()->UnitID.ToString(), Instance->GetStarLevel(), ShopComponent->GetGold());
}

void AHMT_PlayerState::ServerSellUnitByActor_Implementation(AActor* Unit)
{
	if (!Unit)
	{
		return;
	}

	for (int32 R = 0; R < BoardComponent->GetRows(); ++R)
	{
		for (int32 Q = 0; Q < BoardComponent->GetColumns(); ++Q)
		{
			const FHMT_GridCoord Coord(Q, R);
			if (BoardComponent->GetOccupantAt(Coord) == Unit)
			{
				FHMT_PlacementSlot Slot;
				Slot.bIsBoard = true;
				Slot.BoardCoord = Coord;
				ServerSellUnitAt_Implementation(Slot);
				return;
			}
		}
	}

	for (int32 SlotIndex = 0; SlotIndex < BenchComponent->GetSlotCount(); ++SlotIndex)
	{
		if (BenchComponent->GetOccupantAt(SlotIndex) == Unit)
		{
			FHMT_PlacementSlot Slot;
			Slot.bIsBoard = false;
			Slot.BenchSlot = SlotIndex;
			ServerSellUnitAt_Implementation(Slot);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerSellUnitByActor: Unit isn't on this player's board or bench."));
}

bool AHMT_PlayerState::ServerSellUnitByActor_Validate(AActor* Unit)
{
	return true;
}

void AHMT_PlayerState::ServerMoveOrSwapUnit_Implementation(FHMT_PlacementSlot From, FHMT_PlacementSlot To)
{
	const bool bSameSlot = From.bIsBoard == To.bIsBoard &&
		(From.bIsBoard ? From.BoardCoord == To.BoardCoord : From.BenchSlot == To.BenchSlot);
	if (bSameSlot)
	{
		return;
	}
	if (To.bIsBoard && !BoardComponent->IsValidCoord(To.BoardCoord))
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerMoveOrSwapUnit: To board coord is invalid/blocked."));
		return;
	}
	// Reject bad bench indices up front — discovering them at PlaceOccupant time, after the unit
	// was already removed from its source slot, would silently drop the unit from the data model.
	if ((!From.bIsBoard && (From.BenchSlot < 0 || From.BenchSlot >= ReplicatedBenchSlotCount)) ||
		(!To.bIsBoard && (To.BenchSlot < 0 || To.BenchSlot >= ReplicatedBenchSlotCount)))
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerMoveOrSwapUnit: bench slot index out of range."));
		return;
	}

	// One small set of helpers covers all four board/bench combinations rather than four near-
	// identical branches — a "slot" is just (bIsBoard, coord-or-index), see FHMT_PlacementSlot.
	auto GetOccupant = [this](const FHMT_PlacementSlot& Slot) -> AActor*
	{
		return Slot.bIsBoard ? BoardComponent->GetOccupantAt(Slot.BoardCoord) : BenchComponent->GetOccupantAt(Slot.BenchSlot);
	};
	auto RemoveOccupant = [this](const FHMT_PlacementSlot& Slot)
	{
		if (Slot.bIsBoard) { BoardComponent->RemoveUnitAt(Slot.BoardCoord); }
		else { BenchComponent->RemoveFromSlot(Slot.BenchSlot); }
	};
	auto PlaceOccupant = [this](AActor* Unit, const FHMT_PlacementSlot& Slot)
	{
		if (Slot.bIsBoard) { BoardComponent->PlaceUnit(Unit, Slot.BoardCoord); }
		else { BenchComponent->PlaceInSlot(Unit, Slot.BenchSlot); }
	};
	auto WorldLocationOf = [this](const FHMT_PlacementSlot& Slot) -> FVector
	{
		const FVector Base = Slot.bIsBoard ? ReplicatedBoardOrigin + BoardComponent->GridToWorld(Slot.BoardCoord) : GetBenchSlotWorldLocation(Slot.BenchSlot);
		return Base + FVector(0.f, 0.f, 50.f);
	};
	// Board/bench occupancy (the actual gameplay state) is separate from actor transform — moving
	// an actor's visual position is purely cosmetic, so it's safe to glide it instead of teleporting.
	auto MoveActorSmoothly = [](AActor* Unit, const FVector& Location)
	{
		if (AHMT_GameUnitActor* SampleUnit = Cast<AHMT_GameUnitActor>(Unit))
		{
			SampleUnit->MoveSmoothly(Location);
		}
		else
		{
			Unit->SetActorLocation(Location);
		}
	};

	AActor* FromUnit = GetOccupant(From);
	if (!FromUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerMoveOrSwapUnit: From slot is empty."));
		return;
	}
	AActor* ToUnit = GetOccupant(To);

	// Buildings are placed once and never move again — reject the whole operation if either end
	// of the move/swap is a building, rather than silently no-opping just that side.
	auto IsBuilding = [](AActor* Unit) -> bool
	{
		const UHMT_UnitInstanceComponent* Instance = Unit ? Unit->FindComponentByClass<UHMT_UnitInstanceComponent>() : nullptr;
		return Instance && Instance->GetUnitDefinition() && Instance->GetUnitDefinition()->bIsBuilding;
	};
	if (IsBuilding(FromUnit) || IsBuilding(ToUnit))
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerMoveOrSwapUnit: buildings can't be moved once placed."));
		return;
	}

	// Board cap only matters for a NET increase in board occupancy — bench->empty-board cell.
	// Board<->board and any swap (To already occupied) leave the count unchanged and are always
	// allowed regardless of capacity.
	if (!From.bIsBoard && To.bIsBoard && !ToUnit)
	{
		const UHMT_MatchRulesAsset* MatchRules = ShopComponent->GetMatchRules();
		const int32 MaxBoardUnits = MatchRules && MatchRules->BaseBoardCapacity > 0
			? MatchRules->BaseBoardCapacity + BoardComponent->GetBonusUnitCapacity()
			: TNumericLimits<int32>::Max();
		// Buildings don't count toward the cap — only non-building occupants compete for board presence.
		const int32 NonBuildingOccupants = Algo::CountIf(BoardComponent->GetAllOccupants(), [&IsBuilding](AActor* Unit) { return !IsBuilding(Unit); });
		if (NonBuildingOccupants >= MaxBoardUnits)
		{
			UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] ServerMoveOrSwapUnit: board is at its capacity cap (%d units) — grow capacity to deploy more."),
				MaxBoardUnits);
			return;
		}
	}

	RemoveOccupant(From);
	RemoveOccupant(To);
	PlaceOccupant(FromUnit, To);
	// The MOVED unit teleports server-side: the owning client hand-carried it to the drop point
	// during the drag, so the single replicated movement update just confirms where it already is —
	// a server-side glide would visibly drag it backward through the lerp on that client. The
	// displaced swap partner wasn't hand-carried by anyone, so it glides.
	FromUnit->SetActorLocation(WorldLocationOf(To));
	if (ToUnit)
	{
		PlaceOccupant(ToUnit, From);
		MoveActorSmoothly(ToUnit, WorldLocationOf(From));
	}

	// Either end of a move/swap can create or break an adjacency/stack that matters for merge and
	// synergy — same "recompute unconditionally after any composition change" rule ServerBuyAndPlaceUnit follows.
	UnitCollectionComponent->RunMergeCascade();
	SynergyManagerComponent->RecomputeSynergies();

	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] ServerMoveOrSwapUnit: %s -> %s%s."),
		From.bIsBoard ? *FString::Printf(TEXT("board(%d,%d)"), From.BoardCoord.Q, From.BoardCoord.R) : *FString::Printf(TEXT("bench[%d]"), From.BenchSlot),
		To.bIsBoard ? *FString::Printf(TEXT("board(%d,%d)"), To.BoardCoord.Q, To.BoardCoord.R) : *FString::Printf(TEXT("bench[%d]"), To.BenchSlot),
		ToUnit ? TEXT(" (swap)") : TEXT(""));
}

void AHMT_PlayerState::ServerRerollShop_Implementation()
{
	AHMT_GameState* SampleGameState = GetWorld() ? GetWorld()->GetGameState<AHMT_GameState>() : nullptr;
	if (!SampleGameState)
	{
		return;
	}

	const bool bSuccess = ShopComponent->RerollShop(SampleGameState->ShopPoolComponent);
	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] ServerRerollShop: %s."), bSuccess ? TEXT("rerolled") : TEXT("failed, insufficient gold"));
}

void AHMT_PlayerState::ServerForceEndPreparation_Implementation()
{
	AHMT_GameState* SampleGameState = GetWorld() ? GetWorld()->GetGameState<AHMT_GameState>() : nullptr;
	if (!SampleGameState)
	{
		return;
	}

	SampleGameState->MatchStateComponent->ForceEndPreparationPhase();
	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] ServerForceEndPreparation: forced preparation phase to end."));
}
