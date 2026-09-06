#include "Tiles/HMT_TileModifierDefinition.h"

FPrimaryAssetId UHMT_TileModifierDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType("TileModifier"), GetFName());
}
