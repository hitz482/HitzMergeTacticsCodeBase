#include "HMT_AttributeTagMapAsset.h"
#include "UObject/UnrealType.h"

FGameplayAttribute UHMT_AttributeTagMapAsset::ResolveAttribute(FGameplayTag StatTag) const
{
	for (const FHMT_AttributeTagMapping& Mapping : Mappings)
	{
		if (Mapping.StatTag != StatTag || !Mapping.AttributeSetClass)
		{
			continue;
		}

		if (FProperty* Property = FindFProperty<FProperty>(Mapping.AttributeSetClass, Mapping.AttributePropertyName))
		{
			return FGameplayAttribute(Property);
		}
	}

	return FGameplayAttribute();
}
