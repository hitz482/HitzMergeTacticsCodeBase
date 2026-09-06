#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;

namespace HMT_TileMeshUtils
{
	HMT_CORE_API void BuildHexTile(UProceduralMeshComponent* PMC, float Radius = 100.f);

	HMT_CORE_API void BuildQuadTile(UProceduralMeshComponent* PMC, float HalfSize = 50.f);
}
