#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "HMT_ContentValidator.generated.h"

UCLASS()
class HMT_COREEDITOR_API UHMT_ContentValidator : public UEditorValidatorBase
{
	GENERATED_BODY()

public:
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

private:
	EDataValidationResult ValidateUnitDefinition(const class UHMT_UnitDefinition* Unit);
	EDataValidationResult ValidateTraitDefinition(const class UHMT_TraitDefinition* Trait);
	EDataValidationResult ValidateBoardLayout(const class UHMT_BoardLayoutAsset* Board);
	EDataValidationResult ValidatePvERoundDefinition(const class UHMT_PvERoundDefinition* PvERound);
	EDataValidationResult ValidateRulerDefinition(const class UHMT_RulerDefinition* Ruler);
	EDataValidationResult ValidateGameModifierDefinition(const class UHMT_GameModifierDefinition* Modifier);
	EDataValidationResult ValidateTileModifierDefinition(const class UHMT_TileModifierDefinition* Tile);
};
