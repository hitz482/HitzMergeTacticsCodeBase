#include "HMT_TestTileEffect_Poison.h"
#include "GameFramework/Actor.h"

void UHMT_TestTileEffect_Poison::OnUnitOnTile_Implementation(AActor* Unit, UHMT_CombatContext* Context)
{
	UE_LOG(LogTemp, Warning, TEXT("[HMT Test] Poison tile: OnUnitOnTile fired for %s."), Unit ? *Unit->GetName() : TEXT("<null>"));
}
