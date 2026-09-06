#include "Grid/HMT_TileMeshUtils.h"
#include "ProceduralMeshComponent.h"

void HMT_TileMeshUtils::BuildHexTile(UProceduralMeshComponent* PMC, float Radius)
{
	if (!PMC)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FLinearColor> VertexColors;

	Vertices.Add(FVector::ZeroVector);
	UVs.Add(FVector2D(0.5f, 0.5f));

	for (int32 Index = 0; Index < 6; ++Index)
	{
		const float AngleRad = FMath::DegreesToRadians(60.f * Index - 30.f);
		const FVector Corner(Radius * FMath::Cos(AngleRad), Radius * FMath::Sin(AngleRad), 0.f);
		Vertices.Add(Corner);
		UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(AngleRad), 0.5f + 0.5f * FMath::Sin(AngleRad)));
	}

	for (int32 Index = 0; Index < 6; ++Index)
	{
		const int32 Next = (Index + 1) % 6;
		Triangles.Add(0);
		Triangles.Add(1 + Next);
		Triangles.Add(1 + Index);
	}

	Normals.Init(FVector::UpVector, Vertices.Num());
	VertexColors.Init(FLinearColor::White, Vertices.Num());
	Tangents.Init(FProcMeshTangent(1.f, 0.f, 0.f), Vertices.Num());

	PMC->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents,  true);
}

void HMT_TileMeshUtils::BuildQuadTile(UProceduralMeshComponent* PMC, float HalfSize)
{
	if (!PMC)
	{
		return;
	}

	const TArray<FVector> Vertices = {
		FVector(-HalfSize, -HalfSize, 0.f),
		FVector(HalfSize, -HalfSize, 0.f),
		FVector(HalfSize, HalfSize, 0.f),
		FVector(-HalfSize, HalfSize, 0.f),
	};
	const TArray<int32> Triangles = { 0, 2, 1, 0, 3, 2 };
	TArray<FVector> Normals;
	Normals.Init(FVector::UpVector, 4);
	const TArray<FVector2D> UVs = { FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1) };
	TArray<FLinearColor> VertexColors;
	VertexColors.Init(FLinearColor::White, 4);
	TArray<FProcMeshTangent> Tangents;
	Tangents.Init(FProcMeshTangent(1.f, 0.f, 0.f), 4);

	PMC->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents,  true);
}
