#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Tiles/IHMT_TileEffect.h"
#include "HMT_TestTileEffect_Poison.generated.h"

/** Phase 16 test/verification content: logs when a unit occupies this hazard tile during combat,
 *  to verify the resolver's per-tick tile dispatch actually fires. Not shipped framework content. */
UCLASS()
class HITZMERGETACTICS_API UHMT_TestTileEffect_Poison : public UObject, public IHMT_TileEffect
{
	GENERATED_BODY()

public:
	virtual void OnUnitOnTile_Implementation(AActor* Unit, UHMT_CombatContext* Context) override;
};
