#include "Units/HMT_UnitDefinition.h"

TArray<FHMT_BaseStat> UHMT_UnitDefinition::GetScaledStats(int32 StarLevel) const
{
	const FHMT_StarStatOverride* Override = PerStarStatOverrides.FindByPredicate(
		[StarLevel](const FHMT_StarStatOverride& Entry) { return Entry.StarLevel == StarLevel; });

	const float ScaleFactor = FMath::Pow(StarStatMultiplier, static_cast<float>(FMath::Max(StarLevel - 1, 0)));

	TArray<FHMT_BaseStat> Result;
	Result.Reserve(BaseStats.Num());

	for (const FHMT_BaseStat& Base : BaseStats)
	{
		FHMT_BaseStat Scaled;
		Scaled.StatTag = Base.StatTag;

		const FHMT_BaseStat* OverrideStat = Override
			? Override->Stats.FindByPredicate([&Base](const FHMT_BaseStat& Entry) { return Entry.StatTag == Base.StatTag; })
			: nullptr;

		Scaled.Value = OverrideStat ? OverrideStat->Value : Base.Value * ScaleFactor;
		Result.Add(Scaled);
	}

	return Result;
}
