#include "Grid/HMT_SquareGridLayout.h"

TArray<FHMT_GridCoord> UHMT_SquareGridLayout::GetNeighbors_Implementation(FHMT_GridCoord Coord)
{
	return {
		FHMT_GridCoord(Coord.Q, Coord.R + 1),
		FHMT_GridCoord(Coord.Q, Coord.R - 1),
		FHMT_GridCoord(Coord.Q + 1, Coord.R),
		FHMT_GridCoord(Coord.Q - 1, Coord.R),
	};
}

int32 UHMT_SquareGridLayout::GetDistance_Implementation(FHMT_GridCoord A, FHMT_GridCoord B)
{
	return FMath::Abs(A.Q - B.Q) + FMath::Abs(A.R - B.R);
}

FVector UHMT_SquareGridLayout::GridToWorld_Implementation(FHMT_GridCoord Coord)
{
	return FVector(Coord.Q * CellSize, Coord.R * CellSize, 0.f);
}

FHMT_GridCoord UHMT_SquareGridLayout::WorldToGrid_Implementation(FVector WorldLocation)
{
	return FHMT_GridCoord(FMath::RoundToInt(WorldLocation.X / CellSize), FMath::RoundToInt(WorldLocation.Y / CellSize));
}
