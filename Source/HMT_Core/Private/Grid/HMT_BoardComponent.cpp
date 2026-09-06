#include "Grid/HMT_BoardComponent.h"
#include "Grid/HMT_HexGridLayout.h"
#include "Grid/HMT_SquareGridLayout.h"
#include "Tiles/HMT_TileModifierDefinition.h"
#include "Net/UnrealNetwork.h"

UHMT_BoardComponent::UHMT_BoardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHMT_BoardComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHMT_BoardComponent, Cells);
	DOREPLIFETIME(UHMT_BoardComponent, BoardColumns);
	DOREPLIFETIME(UHMT_BoardComponent, BoardRows);
	DOREPLIFETIME(UHMT_BoardComponent, BlockedCells);
	DOREPLIFETIME(UHMT_BoardComponent, Topology);
	DOREPLIFETIME(UHMT_BoardComponent, CellSize);
	DOREPLIFETIME(UHMT_BoardComponent, ActiveTileModifiers);
	DOREPLIFETIME(UHMT_BoardComponent, BonusUnitCapacity);
}

void UHMT_BoardComponent::InitializeBoard(UHMT_BoardLayoutAsset* Layout)
{
	if (!Layout)
	{
		return;
	}

	UObject* NewLayoutObject = nullptr;
	if (Layout->Topology == EHMT_GridTopology::Hex)
	{
		UHMT_HexGridLayout* Hex = NewObject<UHMT_HexGridLayout>(this);
		Hex->CellSize = Layout->CellSize;
		NewLayoutObject = Hex;
	}
	else
	{
		UHMT_SquareGridLayout* Square = NewObject<UHMT_SquareGridLayout>(this);
		Square->CellSize = Layout->CellSize;
		NewLayoutObject = Square;
	}
	GridLayout = TScriptInterface<IHMT_GridLayout>(NewLayoutObject);

	Topology = Layout->Topology;
	CellSize = Layout->CellSize;
	BoardColumns = Layout->Columns;
	BoardRows = Layout->Rows;
	BlockedCells = Layout->BlockedCells;

	Cells.Reset();
	for (int32 Col = 0; Col < BoardColumns; ++Col)
	{
		for (int32 Row = 0; Row < BoardRows; ++Row)
		{
			FHMT_BoardCell Cell;
			Cell.Coord = FHMT_GridCoord(Col, Row);
			Cell.Occupant = nullptr;
			Cells.Add(Cell);
		}
	}
}

int32 UHMT_BoardComponent::FindCellIndex(FHMT_GridCoord Coord) const
{
	return Cells.IndexOfByPredicate([&Coord](const FHMT_BoardCell& Cell) { return Cell.Coord == Coord; });
}

bool UHMT_BoardComponent::IsValidCoord(FHMT_GridCoord Coord) const
{
	if (Coord.Q < 0 || Coord.Q >= BoardColumns || Coord.R < 0 || Coord.R >= BoardRows)
	{
		return false;
	}
	return !BlockedCells.Contains(Coord);
}

bool UHMT_BoardComponent::IsCellOccupied(FHMT_GridCoord Coord) const
{
	return GetOccupantAt(Coord) != nullptr;
}

AActor* UHMT_BoardComponent::GetOccupantAt(FHMT_GridCoord Coord) const
{
	const int32 Index = FindCellIndex(Coord);
	return Index != INDEX_NONE ? Cells[Index].Occupant : nullptr;
}

bool UHMT_BoardComponent::PlaceUnit(AActor* Unit, FHMT_GridCoord Coord)
{
	if (!Unit || !IsValidCoord(Coord) || IsCellOccupied(Coord))
	{
		return false;
	}

	const int32 Index = FindCellIndex(Coord);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	Cells[Index].Occupant = Unit;
	return true;
}

bool UHMT_BoardComponent::RemoveUnitAt(FHMT_GridCoord Coord)
{
	const int32 Index = FindCellIndex(Coord);
	if (Index == INDEX_NONE || Cells[Index].Occupant == nullptr)
	{
		return false;
	}

	Cells[Index].Occupant = nullptr;
	return true;
}

bool UHMT_BoardComponent::MoveOrSwapUnit(FHMT_GridCoord From, FHMT_GridCoord To)
{
	if (From == To || !IsValidCoord(From) || !IsValidCoord(To))
	{
		return false;
	}

	const int32 FromIndex = FindCellIndex(From);
	const int32 ToIndex = FindCellIndex(To);
	if (FromIndex == INDEX_NONE || ToIndex == INDEX_NONE || Cells[FromIndex].Occupant == nullptr)
	{
		return false;
	}

	Swap(Cells[FromIndex].Occupant, Cells[ToIndex].Occupant);
	return true;
}

TArray<AActor*> UHMT_BoardComponent::GetAllOccupants() const
{
	TArray<AActor*> Occupants;
	for (const FHMT_BoardCell& Cell : Cells)
	{
		if (Cell.Occupant)
		{
			Occupants.Add(Cell.Occupant);
		}
	}
	return Occupants;
}

bool UHMT_BoardComponent::RemoveOccupant(AActor* Unit)
{
	const int32 Index = Cells.IndexOfByPredicate([Unit](const FHMT_BoardCell& Cell) { return Cell.Occupant == Unit; });
	if (Index == INDEX_NONE)
	{
		return false;
	}

	Cells[Index].Occupant = nullptr;
	return true;
}

FHMT_GridCoord UHMT_BoardComponent::GetCoordOfOccupant(AActor* Unit, bool& bOutFound) const
{
	const int32 Index = Cells.IndexOfByPredicate([Unit](const FHMT_BoardCell& Cell) { return Cell.Occupant == Unit; });
	bOutFound = Index != INDEX_NONE;
	return bOutFound ? Cells[Index].Coord : FHMT_GridCoord();
}

FHMT_GridCoord UHMT_BoardComponent::GetNextStepToward(FHMT_GridCoord From, FHMT_GridCoord Target) const
{
	if (!GridLayout.GetObject())
	{
		return From;
	}

	const TArray<FHMT_GridCoord> Neighbors = IHMT_GridLayout::Execute_GetNeighbors(GridLayout.GetObject(), From);

	FHMT_GridCoord Best = From;
	int32 BestDistance = IHMT_GridLayout::Execute_GetDistance(GridLayout.GetObject(), From, Target);
	bool bFoundUnoccupied = false;

	for (const FHMT_GridCoord& Candidate : Neighbors)
	{
		if (!IsValidCoord(Candidate) || IsCellOccupied(Candidate))
		{
			continue;
		}

		const int32 CandidateDistance = IHMT_GridLayout::Execute_GetDistance(GridLayout.GetObject(), Candidate, Target);
		if (!bFoundUnoccupied || CandidateDistance < BestDistance)
		{
			Best = Candidate;
			BestDistance = CandidateDistance;
			bFoundUnoccupied = true;
		}
	}

	return Best;
}

FVector UHMT_BoardComponent::GridToWorld(FHMT_GridCoord Coord) const
{
	return GridLayout.GetObject() ? IHMT_GridLayout::Execute_GridToWorld(GridLayout.GetObject(), Coord) : FVector::ZeroVector;
}

FHMT_GridCoord UHMT_BoardComponent::WorldToGrid(FVector WorldLocation) const
{
	return GridLayout.GetObject() ? IHMT_GridLayout::Execute_WorldToGrid(GridLayout.GetObject(), WorldLocation) : FHMT_GridCoord();
}

int32 UHMT_BoardComponent::GetDistance(FHMT_GridCoord A, FHMT_GridCoord B) const
{
	return GridLayout.GetObject() ? IHMT_GridLayout::Execute_GetDistance(GridLayout.GetObject(), A, B) : 0;
}

TArray<FHMT_GridCoord> UHMT_BoardComponent::GetNeighbors(FHMT_GridCoord Coord) const
{
	return GridLayout.GetObject() ? IHMT_GridLayout::Execute_GetNeighbors(GridLayout.GetObject(), Coord) : TArray<FHMT_GridCoord>();
}

void UHMT_BoardComponent::ApplyTileModifier(FHMT_GridCoord Coord, UHMT_TileModifierDefinition* Definition)
{
	ClearTileModifierAt(Coord);

	FHMT_TileModifier NewTile;
	NewTile.Coord = Coord;
	NewTile.Definition = Definition;
	NewTile.TileEffectClass = Definition ? Definition->TileEffectClass : nullptr;
	ActiveTileModifiers.Add(NewTile);
}

void UHMT_BoardComponent::ClearTileModifierAt(FHMT_GridCoord Coord)
{
	ActiveTileModifiers.RemoveAll([Coord](const FHMT_TileModifier& Tile) { return Tile.Coord == Coord; });
}

bool UHMT_BoardComponent::GetTileModifierAt(FHMT_GridCoord Coord, FHMT_TileModifier& OutModifier) const
{
	if (const FHMT_TileModifier* Found = ActiveTileModifiers.FindByPredicate([Coord](const FHMT_TileModifier& Tile) { return Tile.Coord == Coord; }))
	{
		OutModifier = *Found;
		return true;
	}
	return false;
}
