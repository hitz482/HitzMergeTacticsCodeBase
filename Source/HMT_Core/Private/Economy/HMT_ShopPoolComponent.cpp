#include "Economy/HMT_ShopPoolComponent.h"
#include "Net/UnrealNetwork.h"

UHMT_ShopPoolComponent::UHMT_ShopPoolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHMT_ShopPoolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHMT_ShopPoolComponent, Entries);
}

void UHMT_ShopPoolComponent::InitializePool(const UHMT_MatchRulesAsset* MatchRules, const TArray<UHMT_UnitDefinition*>& AllUnits)
{
	if (!MatchRules)
	{
		return;
	}

	Entries.Reset();
	for (const UHMT_UnitDefinition* Unit : AllUnits)
	{
		if (!Unit)
		{
			continue;
		}

		const int32 TierIndex = Unit->CostTier - 1;
		if (!MatchRules->PoolSizeByTier.IsValidIndex(TierIndex))
		{
			continue;
		}

		FHMT_UnitPoolEntry Entry;
		Entry.UnitID = Unit->UnitID;
		Entry.Remaining = MatchRules->PoolSizeByTier[TierIndex];
		Entries.Add(Entry);
	}
}

FHMT_UnitPoolEntry* UHMT_ShopPoolComponent::FindEntry(FGameplayTag UnitID)
{
	return Entries.FindByPredicate([&UnitID](const FHMT_UnitPoolEntry& Entry) { return Entry.UnitID == UnitID; });
}

int32 UHMT_ShopPoolComponent::GetRemainingCount(FGameplayTag UnitID) const
{
	const FHMT_UnitPoolEntry* Entry = Entries.FindByPredicate([&UnitID](const FHMT_UnitPoolEntry& E) { return E.UnitID == UnitID; });
	return Entry ? Entry->Remaining : 0;
}

bool UHMT_ShopPoolComponent::TryDecrement(FGameplayTag UnitID)
{
	FHMT_UnitPoolEntry* Entry = FindEntry(UnitID);
	if (!Entry || Entry->Remaining <= 0)
	{
		return false;
	}

	--Entry->Remaining;
	return true;
}

void UHMT_ShopPoolComponent::Increment(FGameplayTag UnitID, int32 Count)
{
	if (FHMT_UnitPoolEntry* Entry = FindEntry(UnitID))
	{
		Entry->Remaining += Count;
	}
}
