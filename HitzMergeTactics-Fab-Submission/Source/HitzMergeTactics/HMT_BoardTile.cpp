#include "HMT_BoardTile.h"
#include "Grid/HMT_TileMeshUtils.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// Movable is required for runtime SetStaticMesh/mesh-section rebuilds (Static mobility
	// silently rejects it). BlockAllDynamic so the drag-drop mouse trace (Visibility channel)
	// hits whichever component is currently active. SetCollisionProfileName lives on
	// UPrimitiveComponent, not USceneComponent — both Mesh and ProceduralMesh derive from it
	// (via UMeshComponent), but the plain SceneComponent Root does not.
	void ConfigureTileComponent(UPrimitiveComponent* Component)
	{
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	}
}

AHMT_BoardTile::AHMT_BoardTile()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	ConfigureTileComponent(Mesh);

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	ProceduralMesh->SetupAttachment(Root);
	ConfigureTileComponent(ProceduralMesh);
}

void AHMT_BoardTile::InitializeTileVisual(UStaticMesh* InMesh, bool bIsHex, UMaterialInterface* InBaseMaterial, UMaterialInterface* InHighlightMaterial, float CellSize, float TileVisualScale)
{
	BaseMaterial = InBaseMaterial;
	HighlightMaterial = InHighlightMaterial;

	// Procedural fallback is reliably built at a 100uu footprint (see HMT_TileMeshUtils), so this
	// baseline is only overridden below when a real mesh's measured bounds say otherwise.
	float BaseScale = CellSize / 100.f;

	if (InMesh)
	{
		ProceduralMesh->SetVisibility(false);
		ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		Mesh->SetVisibility(true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetStaticMesh(InMesh);
		if (BaseMaterial)
		{
			Mesh->SetMaterial(0, BaseMaterial);
		}
		else
		{
			// No explicit base material — capture the mesh's own, so un-highlighting can restore it
			// (SetMaterial(0, nullptr) would show the default grid material, not the mesh's own).
			BaseMaterial = InMesh->GetMaterial(0);
		}

		// Measure the mesh's own footprint instead of assuming it was authored at exactly 100uu —
		// a mismatch here is exactly what causes a visible gap (mesh smaller than CellSize) or
		// overlap (larger) once tiled across the board.
		const FVector MeshExtent = InMesh->GetBounds().BoxExtent; // half-extents, local space
		const float MeshFootprint = FMath::Max(MeshExtent.X, MeshExtent.Y) * 2.f;
		if (MeshFootprint > KINDA_SMALL_NUMBER)
		{
			BaseScale = CellSize / MeshFootprint;
		}
	}
	else
	{
		Mesh->SetVisibility(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		ProceduralMesh->SetVisibility(true);
		ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (bIsHex)
		{
			HMT_TileMeshUtils::BuildHexTile(ProceduralMesh);
		}
		else
		{
			HMT_TileMeshUtils::BuildQuadTile(ProceduralMesh);
		}
		if (BaseMaterial)
		{
			ProceduralMesh->SetMaterial(0, BaseMaterial);
		}
	}

	SetActorScale3D(FVector(BaseScale * TileVisualScale));
}

void AHMT_BoardTile::SetHighlighted(bool bHighlighted)
{
	SetVisualState(bHighlighted ? EHMT_SampleTileVisualState::Hover : EHMT_SampleTileVisualState::Base);
}

void AHMT_BoardTile::SetStateMaterials(UMaterialInterface* InValidDrop, UMaterialInterface* InInvalidDrop, UMaterialInterface* InSelected, UMaterialInterface* InOccupied)
{
	ValidDropMaterial = InValidDrop;
	InvalidDropMaterial = InInvalidDrop;
	SelectedMaterial = InSelected;
	OccupiedMaterial = InOccupied;
}

void AHMT_BoardTile::SetVisualState(EHMT_SampleTileVisualState State)
{
	CurrentVisualState = State;

	UMaterialInterface* Material = nullptr;
	switch (State)
	{
	case EHMT_SampleTileVisualState::Hover:       Material = HighlightMaterial; break;
	case EHMT_SampleTileVisualState::ValidDrop:   Material = ValidDropMaterial ? ValidDropMaterial : HighlightMaterial; break;
	case EHMT_SampleTileVisualState::Selected:    Material = SelectedMaterial ? SelectedMaterial : HighlightMaterial; break;
	case EHMT_SampleTileVisualState::InvalidDrop: Material = InvalidDropMaterial ? InvalidDropMaterial : BaseMaterial; break;
	case EHMT_SampleTileVisualState::Occupied:    Material = OccupiedMaterial ? OccupiedMaterial : BaseMaterial; break;
	default:                                      Material = BaseMaterial; break;
	}

	if (!Material)
	{
		return;
	}

	UMeshComponent* Active = Mesh->IsVisible() ? static_cast<UMeshComponent*>(Mesh.Get()) : static_cast<UMeshComponent*>(ProceduralMesh.Get());
	Active->SetMaterial(0, Material);
}
