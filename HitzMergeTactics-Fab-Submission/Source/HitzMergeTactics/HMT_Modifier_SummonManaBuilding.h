#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Modifiers/IHMT_ModifierEffect.h"
#include "HMT_Modifier_SummonManaBuilding.generated.h"

class UHMT_UnitDefinition;

/** Grants a building unit (BuildingDefinition, expected bIsBuilding=true) onto the player's bench
 *  once, at match start (assign a UHMT_GameModifierDefinition with TriggerType = OnMatchStart).
 *  The building itself does nothing here — its passive mana generation comes from whatever
 *  AbilityExecutorClass BuildingDefinition is authored with (see UHMT_Ability_ManaAura), fired
 *  through the normal combat "cast cadence" like any other unit's ability. No-ops (logs a warning)
 *  if the bench is already full — buyer content, not core framework, so failing loud beats silently
 *  eating the modifier. */
UCLASS(Blueprintable)
class HITZMERGETACTICS_API UHMT_Modifier_SummonManaBuilding : public UObject, public IHMT_ModifierEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	TObjectPtr<UHMT_UnitDefinition> BuildingDefinition;

	virtual void OnActivate_Implementation(APlayerState* Player) override;
	virtual void OnDeactivate_Implementation(APlayerState* Player) override {}
	virtual void OnCombatStart_Implementation(APlayerState* Player, UHMT_CombatContext* Context) override {}
	virtual void OnCombatEvent_Implementation(APlayerState* Player, const FHMT_CombatEvent& Event, UHMT_CombatContext* Context) override {}
};
