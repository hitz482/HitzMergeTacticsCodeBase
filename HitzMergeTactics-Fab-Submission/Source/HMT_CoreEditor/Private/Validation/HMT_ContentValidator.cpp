#include "Validation/HMT_ContentValidator.h"
#include "Units/HMT_UnitDefinition.h"
#include "Synergy/HMT_TraitDefinition.h"
#include "Grid/HMT_BoardLayoutAsset.h"
#include "Match/HMT_PvERoundDefinition.h"
#include "Ruler/HMT_RulerDefinition.h"
#include "Modifiers/HMT_GameModifierDefinition.h"
#include "Tiles/HMT_TileModifierDefinition.h"
#include "Misc/DataValidation.h"

bool UHMT_ContentValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return InObject
		&& (InObject->IsA<UHMT_UnitDefinition>()
			|| InObject->IsA<UHMT_TraitDefinition>()
			|| InObject->IsA<UHMT_BoardLayoutAsset>()
			|| InObject->IsA<UHMT_PvERoundDefinition>()
			|| InObject->IsA<UHMT_RulerDefinition>()
			|| InObject->IsA<UHMT_GameModifierDefinition>()
			|| InObject->IsA<UHMT_TileModifierDefinition>());
}

EDataValidationResult UHMT_ContentValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	if (const UHMT_UnitDefinition* Unit = Cast<UHMT_UnitDefinition>(InAsset))
	{
		return ValidateUnitDefinition(Unit);
	}
	if (const UHMT_TraitDefinition* Trait = Cast<UHMT_TraitDefinition>(InAsset))
	{
		return ValidateTraitDefinition(Trait);
	}
	if (const UHMT_BoardLayoutAsset* Board = Cast<UHMT_BoardLayoutAsset>(InAsset))
	{
		return ValidateBoardLayout(Board);
	}
	if (const UHMT_PvERoundDefinition* PvERound = Cast<UHMT_PvERoundDefinition>(InAsset))
	{
		return ValidatePvERoundDefinition(PvERound);
	}
	if (const UHMT_RulerDefinition* Ruler = Cast<UHMT_RulerDefinition>(InAsset))
	{
		return ValidateRulerDefinition(Ruler);
	}
	if (const UHMT_GameModifierDefinition* Modifier = Cast<UHMT_GameModifierDefinition>(InAsset))
	{
		return ValidateGameModifierDefinition(Modifier);
	}
	if (const UHMT_TileModifierDefinition* Tile = Cast<UHMT_TileModifierDefinition>(InAsset))
	{
		return ValidateTileModifierDefinition(Tile);
	}

	return EDataValidationResult::NotValidated;
}

EDataValidationResult UHMT_ContentValidator::ValidateUnitDefinition(const UHMT_UnitDefinition* Unit)
{
	bool bHasErrors = false;

	if (Unit->Mesh.IsNull())
	{
		AssetFails(Unit, NSLOCTEXT("HMT", "UnitNoMesh", "Unit has no Mesh assigned."));
		bHasErrors = true;
	}

	if (Unit->BaseStats.Num() == 0)
	{
		AssetFails(Unit, NSLOCTEXT("HMT", "UnitNoStats", "Unit has no BaseStats — it will have zero HP/damage/range/etc."));
		bHasErrors = true;
	}

	if (!Unit->AnimSet.Idle)
	{
		AssetFails(Unit, NSLOCTEXT("HMT", "UnitNoIdleAnim", "Unit's AnimSet.Idle is unset."));
		bHasErrors = true;
	}
	if (!Unit->AnimSet.Attack)
	{
		AssetFails(Unit, NSLOCTEXT("HMT", "UnitNoAttackAnim", "Unit's AnimSet.Attack is unset."));
		bHasErrors = true;
	}
	if (!Unit->AnimSet.Death)
	{
		AssetFails(Unit, NSLOCTEXT("HMT", "UnitNoDeathAnim", "Unit's AnimSet.Death is unset."));
		bHasErrors = true;
	}
	if (!Unit->AnimSet.Move)
	{
		AssetWarning(Unit, NSLOCTEXT("HMT", "UnitNoMoveAnim", "Unit's AnimSet.Move is unset — it will slide rather than animate while repositioning in combat."));
	}

	if (!Unit->TargetingStrategyClass)
	{
		AssetWarning(Unit, NSLOCTEXT("HMT", "UnitNoTargeting", "No TargetingStrategyClass set — the resolver will default to nearest-enemy targeting."));
	}

	if (Unit->AbilityExecutorClass && !Unit->AnimSet.Ability)
	{
		AssetWarning(Unit, NSLOCTEXT("HMT", "UnitAbilityNoAnim", "AbilityExecutorClass is set but AnimSet.Ability is unset."));
	}

	if (!bHasErrors)
	{
		AssetPasses(Unit);
	}

	return bHasErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

EDataValidationResult UHMT_ContentValidator::ValidateTraitDefinition(const UHMT_TraitDefinition* Trait)
{
	bool bHasErrors = false;

	if (Trait->Breakpoints.Num() == 0)
	{
		AssetFails(Trait, NSLOCTEXT("HMT", "TraitNoBreakpoints", "Trait has no Breakpoints — it can never activate."));
		bHasErrors = true;
	}

	for (int32 Index = 0; Index < Trait->Breakpoints.Num(); ++Index)
	{
		const FHMT_TraitBreakpoint& Breakpoint = Trait->Breakpoints[Index];
		if (Breakpoint.StatGrants.Num() == 0 && !Breakpoint.TraitEffectClass)
		{
			AssetWarning(Trait, FText::Format(
				NSLOCTEXT("HMT", "TraitBreakpointNoop", "Breakpoint {0} (RequiredCount={1}) has no StatGrants and no TraitEffectClass — it does nothing."),
				FText::AsNumber(Index), FText::AsNumber(Breakpoint.RequiredCount)));
		}
	}

	if (!bHasErrors)
	{
		AssetPasses(Trait);
	}

	return bHasErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

EDataValidationResult UHMT_ContentValidator::ValidateBoardLayout(const UHMT_BoardLayoutAsset* Board)
{
	bool bHasErrors = false;

	if (Board->Columns <= 0 || Board->Rows <= 0)
	{
		AssetFails(Board, NSLOCTEXT("HMT", "BoardBadDimensions", "Board Columns/Rows must both be greater than zero."));
		bHasErrors = true;
	}

	if (Board->CellSize <= 0.f)
	{
		AssetFails(Board, NSLOCTEXT("HMT", "BoardBadCellSize", "Board CellSize must be greater than zero."));
		bHasErrors = true;
	}

	if (!bHasErrors)
	{
		AssetPasses(Board);
	}

	return bHasErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

EDataValidationResult UHMT_ContentValidator::ValidatePvERoundDefinition(const UHMT_PvERoundDefinition* PvERound)
{
	bool bHasErrors = false;

	if (PvERound->EncounterUnits.Num() == 0)
	{
		AssetFails(PvERound, NSLOCTEXT("HMT", "PvENoUnits", "PvE round has no EncounterUnits — it will resolve as an empty-board instant win."));
		bHasErrors = true;
	}

	for (int32 Index = 0; Index < PvERound->EncounterUnits.Num(); ++Index)
	{
		if (!PvERound->EncounterUnits[Index].UnitDefinition)
		{
			AssetFails(PvERound, FText::Format(
				NSLOCTEXT("HMT", "PvEUnitNoDefinition", "EncounterUnits[{0}] has no UnitDefinition assigned."),
				FText::AsNumber(Index)));
			bHasErrors = true;
		}
	}

	if (!bHasErrors)
	{
		AssetPasses(PvERound);
	}

	return bHasErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

EDataValidationResult UHMT_ContentValidator::ValidateRulerDefinition(const UHMT_RulerDefinition* Ruler)
{
	bool bHasErrors = false;

	if (Ruler->RulerName.IsEmpty())
	{
		AssetFails(Ruler, NSLOCTEXT("HMT", "RulerNoName", "Ruler has no RulerName assigned."));
		bHasErrors = true;
	}

	if (!bHasErrors)
	{
		AssetPasses(Ruler);
	}

	return bHasErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

EDataValidationResult UHMT_ContentValidator::ValidateGameModifierDefinition(const UHMT_GameModifierDefinition* Modifier)
{
	bool bHasErrors = false;

	if (Modifier->ModifierName.IsEmpty())
	{
		AssetFails(Modifier, NSLOCTEXT("HMT", "ModifierNoName", "Game modifier has no ModifierName assigned."));
		bHasErrors = true;
	}

	if (!Modifier->ModifierEffectClass)
	{
		AssetWarning(Modifier, NSLOCTEXT("HMT", "ModifierNoEffectClass", "Game modifier has no ModifierEffectClass — it is purely cosmetic and will have no gameplay effect."));
	}

	if (!bHasErrors)
	{
		AssetPasses(Modifier);
	}

	return bHasErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

EDataValidationResult UHMT_ContentValidator::ValidateTileModifierDefinition(const UHMT_TileModifierDefinition* Tile)
{
	bool bHasErrors = false;

	if (Tile->TileName.IsEmpty())
	{
		AssetFails(Tile, NSLOCTEXT("HMT", "TileNoName", "Tile modifier has no TileName assigned."));
		bHasErrors = true;
	}

	if (!Tile->TileEffectClass)
	{
		AssetWarning(Tile, NSLOCTEXT("HMT", "TileNoEffectClass", "Tile modifier has no TileEffectClass — it is purely a visual marker and will have no gameplay effect."));
	}

	if (!bHasErrors)
	{
		AssetPasses(Tile);
	}

	return bHasErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
