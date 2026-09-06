#include "Grid/HMT_HexGridLayout.h"

namespace HMT_OddR
{
	static int32 RowParity(int32 Row)
	{
		return Row & 1;
	}

	static FIntVector ToCube(const FHMT_GridCoord& Coord)
	{
		const int32 X = Coord.Q - (Coord.R - RowParity(Coord.R)) / 2;
		const int32 Z = Coord.R;
		return FIntVector(X, -X - Z, Z);
	}

	static FHMT_GridCoord FromCube(int32 X, int32 Z)
	{
		return FHMT_GridCoord(X + (Z - RowParity(Z)) / 2, Z);
	}

	static const FHMT_GridCoord EvenRowDirections[6] =
	{
		FHMT_GridCoord(+1, 0), FHMT_GridCoord(0, -1), FHMT_GridCoord(-1, -1),
		FHMT_GridCoord(-1, 0), FHMT_GridCoord(-1, +1), FHMT_GridCoord(0, +1),
	};
	static const FHMT_GridCoord OddRowDirections[6] =
	{
		FHMT_GridCoord(+1, 0), FHMT_GridCoord(+1, -1), FHMT_GridCoord(0, -1),
		FHMT_GridCoord(-1, 0), FHMT_GridCoord(0, +1), FHMT_GridCoord(+1, +1),
	};
}

TArray<FHMT_GridCoord> UHMT_HexGridLayout::GetNeighbors_Implementation(FHMT_GridCoord Coord)
{
	const FHMT_GridCoord* Directions = HMT_OddR::RowParity(Coord.R) ? HMT_OddR::OddRowDirections : HMT_OddR::EvenRowDirections;

	TArray<FHMT_GridCoord> Neighbors;
	Neighbors.Reserve(6);
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Neighbors.Add(FHMT_GridCoord(Coord.Q + Directions[Index].Q, Coord.R + Directions[Index].R));
	}
	return Neighbors;
}

int32 UHMT_HexGridLayout::GetDistance_Implementation(FHMT_GridCoord A, FHMT_GridCoord B)
{
	const FIntVector CubeA = HMT_OddR::ToCube(A);
	const FIntVector CubeB = HMT_OddR::ToCube(B);
	return (FMath::Abs(CubeA.X - CubeB.X) + FMath::Abs(CubeA.Y - CubeB.Y) + FMath::Abs(CubeA.Z - CubeB.Z)) / 2;
}

FVector UHMT_HexGridLayout::GridToWorld_Implementation(FHMT_GridCoord Coord)
{
	const float X = CellSize * FMath::Sqrt(3.f) * (Coord.Q + 0.5f * HMT_OddR::RowParity(Coord.R));
	const float Y = CellSize * 1.5f * Coord.R;
	return FVector(X, Y, 0.f);
}

FHMT_GridCoord UHMT_HexGridLayout::WorldToGrid_Implementation(FVector WorldLocation)
{
	const float FracQ = (FMath::Sqrt(3.f) / 3.f * WorldLocation.X - 1.f / 3.f * WorldLocation.Y) / CellSize;
	const float FracR = (2.f / 3.f * WorldLocation.Y) / CellSize;

	const float CubeX = FracQ;
	const float CubeZ = FracR;
	const float CubeY = -CubeX - CubeZ;

	float RX = FMath::RoundToFloat(CubeX);
	float RY = FMath::RoundToFloat(CubeY);
	float RZ = FMath::RoundToFloat(CubeZ);

	const float XDiff = FMath::Abs(RX - CubeX);
	const float YDiff = FMath::Abs(RY - CubeY);
	const float ZDiff = FMath::Abs(RZ - CubeZ);

	if (XDiff > YDiff && XDiff > ZDiff)
	{
		RX = -RY - RZ;
	}
	else if (YDiff > ZDiff)
	{
		RY = -RX - RZ;
	}
	else
	{
		RZ = -RX - RY;
	}

	return HMT_OddR::FromCube(FMath::RoundToInt(RX), FMath::RoundToInt(RZ));
}
