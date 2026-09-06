#include "Synergy/HMT_TraitDefinition.h"

int32 UHMT_TraitDefinition::GetHighestMetBreakpointIndex(int32 Count) const
{
	int32 BestIndex = INDEX_NONE;
	int32 BestRequiredCount = -1;

	for (int32 Index = 0; Index < Breakpoints.Num(); ++Index)
	{
		const FHMT_TraitBreakpoint& Breakpoint = Breakpoints[Index];
		if (Count >= Breakpoint.RequiredCount && Breakpoint.RequiredCount > BestRequiredCount)
		{
			BestIndex = Index;
			BestRequiredCount = Breakpoint.RequiredCount;
		}
	}

	return BestIndex;
}
