#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Modifiers/IHMT_ModifierEffect.h"
#include "HMT_Modifier_TroopCapacity.generated.h"

/** Grants the player's board +1 unit capacity at the start of every round, up to MaxBonusCapacity
 *  total. Assign a UHMT_GameModifierDefinition with TriggerType = OnRoundStart to this class.
 *  Deliberately stateless on this UObject (see IHMT_ModifierEffect's class comment) — the running
 *  total lives on UHMT_BoardComponent::BonusUnitCapacity, so this just reads that back each round
 *  to decide whether the cap has already been reached. */
UCLASS(Blueprintable)
class HITZMERGETACTICS_API UHMT_Modifier_TroopCapacity : public UObject, public IHMT_ModifierEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 BonusPerRound = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HMT Sample")
	int32 MaxBonusCapacity = 6;

	virtual void OnActivate_Implementation(APlayerState* Player) override;
	virtual void OnDeactivate_Implementation(APlayerState* Player) override {}
	virtual void OnCombatStart_Implementation(APlayerState* Player, UHMT_CombatContext* Context) override {}
	virtual void OnCombatEvent_Implementation(APlayerState* Player, const FHMT_CombatEvent& Event, UHMT_CombatContext* Context) override {}
};
