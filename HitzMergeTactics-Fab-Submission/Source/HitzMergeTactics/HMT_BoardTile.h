#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid/HMT_GridTypes.h"
#include "HMT_BoardTile.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/** Drag-drop visual states a tile can be put in — each maps to a UHMT_BoardLayoutAsset material
 *  with graceful fallbacks (see AHMT_BoardTile::SetVisualState), so buyers author only the
 *  distinctions they care about. */
UENUM()
enum class EHMT_SampleTileVisualState : uint8
{
	Base,
	Hover,
	ValidDrop,
	InvalidDrop,
	Selected,
	Occupied
};

/**
 * Local-only tile actor (board cell or bench slot), skinned from UHMT_BoardLayoutAsset's
 * TileMesh/materials — see HMT_SamplePlayerState's ReplicatedBoardLayout comment for why these
 * are spawned client-locally rather than replicated. Doubles as the mouse-pick target for
 * drag-drop: AHMT_PlayerController line-traces the cursor against these and reads
 * bIsBoardCell/BoardCoord/BenchSlotIndex to know what was hit.
 *
 * Renders via ONE of two components at a time: Mesh (buyer-supplied TileMesh) if set, otherwise
 * ProceduralMesh (a built-in honeycomb/quad shape matching board topology) — see
 * InitializeTileVisual. Only the active one keeps collision enabled.
 */
UCLASS()
class AHMT_BoardTile : public AActor
{
	GENERATED_BODY()

public:
	AHMT_BoardTile();

	UPROPERTY(VisibleAnywhere, Category = "HMT Sample")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "HMT Sample")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "HMT Sample")
	TObjectPtr<UProceduralMeshComponent> ProceduralMesh;

	UPROPERTY()
	bool bIsBoardCell = false;

	/** Valid only when bIsBoardCell is true. */
	UPROPERTY()
	FHMT_GridCoord BoardCoord;

	/** Valid only when bIsBoardCell is false. */
	UPROPERTY()
	int32 BenchSlotIndex = INDEX_NONE;

	/** False for scenery tiles (rendered blocked cells) — drag-drop picking ignores these. */
	UPROPERTY()
	bool bInteractable = true;

	/** Whose board/bench this tile belongs to. Spectating spawns ANOTHER player's tiles on this
	 *  machine, and their coords overlap the local player's own — without this, a drag/sell on a
	 *  spectated tile would silently act on the local board's same coordinate. */
	UPROPERTY()
	TObjectPtr<class APlayerState> OwningPlayer;

	/** Applies mesh + base material + scale; call once right after spawning. InMesh null falls back
	 *  to a procedural hex (bIsHex) or quad shape built at a fixed 100uu size. When InMesh is set,
	 *  its actual local-space bounds are measured and scaled to fill CellSize exactly — buyer-
	 *  supplied meshes aren't guaranteed to be authored at exactly 100uu, and assuming they are
	 *  (the old behavior) produces visible gaps or overlap between tiles whenever they aren't.
	 *  TileVisualScale is an extra multiplier applied on top (e.g. buyers deliberately shrinking
	 *  tiles slightly for a grid-line look). BaseMaterial null = keep the mesh's own material.
	 *  HighlightMaterial null = SetHighlighted becomes a no-op. */
	void InitializeTileVisual(UStaticMesh* InMesh, bool bIsHex, UMaterialInterface* InBaseMaterial, UMaterialInterface* InHighlightMaterial, float CellSize, float TileVisualScale = 1.f);

	/** Swaps between base and highlight material on whichever component is currently active.
	 *  Thin wrapper over SetVisualState(Hover/Base), kept for existing call sites. */
	void SetHighlighted(bool bHighlighted);

	/** Optional per-state materials beyond base/highlight — call once after InitializeTileVisual.
	 *  Any null entry falls back per SetVisualState's chain, so this is safe to skip entirely. */
	void SetStateMaterials(UMaterialInterface* InValidDrop, UMaterialInterface* InInvalidDrop, UMaterialInterface* InSelected, UMaterialInterface* InOccupied);

	/** Applies the material for State to whichever mesh component is active. Fallbacks for
	 *  unauthored states: ValidDrop/Selected -> HighlightMaterial, InvalidDrop/Occupied -> Base —
	 *  so with zero extra materials authored this degrades to exactly the old binary highlight. */
	void SetVisualState(EHMT_SampleTileVisualState State);

	EHMT_SampleTileVisualState GetVisualState() const { return CurrentVisualState; }

private:
	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> HighlightMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ValidDropMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> InvalidDropMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SelectedMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> OccupiedMaterial;

	EHMT_SampleTileVisualState CurrentVisualState = EHMT_SampleTileVisualState::Base;
};
