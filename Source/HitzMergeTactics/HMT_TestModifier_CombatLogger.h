#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Modifiers/IHMT_ModifierEffect.h"
#include "HMT_TestModifier_CombatLogger.generated.h"

/** Phase 16 test/verification content: logs when OnCombatStart/OnCombatEvent fire, to verify the
 *  resolver-level combat-time hook wiring actually dispatches. Not shipped framework content. */
UCLASS()
class HITZMERGETACTICS_API UHMT_TestModifier_CombatLogger : public UObject, public IHMT_ModifierEffect
{
	GENERATED_BODY()

public:
	virtual void OnActivate_Implementation(APlayerState* Player) override {}
	virtual void OnDeactivate_Implementation(APlayerState* Player) override {}
	virtual void OnCombatStart_Implementation(APlayerState* Player, UHMT_CombatContext* Context) override;
	virtual void OnCombatEvent_Implementation(APlayerState* Player, const FHMT_CombatEvent& Event, UHMT_CombatContext* Context) override;
};
