#include "Ruler/HMT_RulerDefinition.h"

FPrimaryAssetId UHMT_RulerDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType("Ruler"), GetFName());
}
