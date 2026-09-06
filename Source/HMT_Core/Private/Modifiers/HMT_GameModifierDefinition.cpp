#include "Modifiers/HMT_GameModifierDefinition.h"

FPrimaryAssetId UHMT_GameModifierDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType("GameModifier"), GetFName());
}
