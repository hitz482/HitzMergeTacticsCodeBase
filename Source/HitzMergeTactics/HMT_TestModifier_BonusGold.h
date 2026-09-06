#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Modifiers/IHMT_ModifierEffect.h"
#include "HMT_TestModifier_BonusGold.generated.h"

/** Phase 16 test/verification content: grants +50 gold to a player when this modifier activates.
 *  Not shipped framework content — exists purely to give DA_Modifier_BonusGold something observable
 *  to verify the match-wide modifier system actually fires in PIE. */
UCLASS()
class HITZMERGETACTICS_API UHMT_TestModifier_BonusGold : public UObject, public IHMT_ModifierEffect
{
	GENERATED_BODY()

public:
	virtual void OnActivate_Implementation(APlayerState* Player) override;
	virtual void OnDeactivate_Implementation(APlayerState* Player) override {}
	virtual void OnCombatStart_Implementation(APlayerState* Player, UHMT_CombatContext* Context) override {}
	virtual void OnCombatEvent_Implementation(APlayerState* Player, const FHMT_CombatEvent& Event, UHMT_CombatContext* Context) override {}
};
