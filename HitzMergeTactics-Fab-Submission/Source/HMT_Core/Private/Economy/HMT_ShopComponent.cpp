#include "Economy/HMT_ShopComponent.h"
#include "Net/UnrealNetwork.h"

UHMT_ShopComponent::UHMT_ShopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHMT_ShopComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHMT_ShopComponent, MatchRules);
	DOREPLIFETIME_CONDITION(UHMT_ShopComponent, Gold, COND_OwnerOnly);
	DOREPLIFETIME(UHMT_ShopComponent, CurrentStreak);
	DOREPLIFETIME_CONDITION(UHMT_ShopComponent, ShopSlots, COND_OwnerOnly);
	DOREPLIFETIME(UHMT_ShopComponent, VisibleShopOccupancy);
	DOREPLIFETIME(UHMT_ShopComponent, bLocked);
}

void UHMT_ShopComponent::InitializeShop(UHMT_MatchRulesAsset* Rules, const TArray<UHMT_UnitDefinition*>& InAvailableUnitPool)
{
	if (!Rules)
	{
		return;
	}

	MatchRules = Rules;
	AvailableUnitPool = InAvailableUnitPool;
	Gold = Rules->StartingGold;
	CurrentStreak = 0;
	bLocked = false;
	ShopSlots.Init(nullptr, Rules->ShopSlotCount);
}

int32 UHMT_ShopComponent::PickWeightedTier(const TArray<float>& TierOdds) const
{
	const float Roll = FMath::FRand();
	float Cumulative = 0.f;

	for (int32 Index = 0; Index < TierOdds.Num(); ++Index)
	{
		Cumulative += TierOdds[Index];
		if (Roll <= Cumulative)
		{
			return Index + 1;
		}
	}

	return TierOdds.Num();
}

void UHMT_ShopComponent::RollShop(UHMT_ShopPoolComponent* Pool)
{
	if (!MatchRules || !Pool || bLocked)
	{
		return;
	}

	const TArray<float>& Odds = MatchRules->ShopTierOdds;
	if (Odds.Num() == 0)
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < ShopSlots.Num(); ++SlotIndex)
	{
		UHMT_UnitDefinition* Picked = nullptr;

		for (int32 Attempt = 0; Attempt < 8 && !Picked; ++Attempt)
		{
			const int32 Tier = PickWeightedTier(Odds);

			TArray<UHMT_UnitDefinition*> Candidates;
			for (UHMT_UnitDefinition* Unit : AvailableUnitPool)
			{
				if (Unit && !Unit->bIsBuilding && Unit->CostTier == Tier && Pool->GetRemainingCount(Unit->UnitID) > 0)
				{
					Candidates.Add(Unit);
				}
			}

			if (Candidates.Num() > 0)
			{
				Picked = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
			}
		}

		ShopSlots[SlotIndex] = Picked;
	}

	VisibleShopOccupancy = 0;
	for (const TObjectPtr<UHMT_UnitDefinition>& Slot : ShopSlots)
	{
		if (Slot)
		{
			++VisibleShopOccupancy;
		}
	}
}

UHMT_UnitDefinition* UHMT_ShopComponent::BuyUnit(int32 SlotIndex, UHMT_ShopPoolComponent* Pool)
{
	if (!MatchRules || !Pool || !ShopSlots.IsValidIndex(SlotIndex) || !ShopSlots[SlotIndex])
	{
		return nullptr;
	}

	UHMT_UnitDefinition* Unit = ShopSlots[SlotIndex];
	const int32 TierIndex = Unit->CostTier - 1;
	if (!MatchRules->GoldCostByTier.IsValidIndex(TierIndex))
	{
		return nullptr;
	}

	const int32 Cost = MatchRules->GoldCostByTier[TierIndex];
	if (Gold < Cost || !Pool->TryDecrement(Unit->UnitID))
	{
		return nullptr;
	}

	Gold -= Cost;
	ShopSlots[SlotIndex] = nullptr;
	--VisibleShopOccupancy;
	return Unit;
}

bool UHMT_ShopComponent::SellUnit(UHMT_UnitDefinition* UnitDefinition, int32 StarLevel, UHMT_ShopPoolComponent* Pool)
{
	if (!MatchRules || !Pool || !UnitDefinition)
	{
		return false;
	}

	const int32 TierIndex = UnitDefinition->CostTier - 1;
	const int32 BaseSellValue = MatchRules->GoldCostByTier.IsValidIndex(TierIndex) ? MatchRules->GoldCostByTier[TierIndex] : 0;

	const int32 CopiesToReturn = FMath::Pow(3.f, static_cast<float>(FMath::Max(StarLevel - 1, 0)));
	const int32 SellValue = BaseSellValue * CopiesToReturn;

	Gold += SellValue;
	Pool->Increment(UnitDefinition->UnitID, CopiesToReturn);
	return true;
}

bool UHMT_ShopComponent::RerollShop(UHMT_ShopPoolComponent* Pool)
{
	if (!MatchRules || Gold < MatchRules->RerollCost || bLocked)
	{
		return false;
	}

	Gold -= MatchRules->RerollCost;
	RollShop(Pool);
	return true;
}

void UHMT_ShopComponent::ApplyRoundIncome(bool bWonLastRound)
{
	if (!MatchRules)
	{
		return;
	}

	CurrentStreak = bWonLastRound
		? (CurrentStreak >= 0 ? CurrentStreak + 1 : 1)
		: (CurrentStreak <= 0 ? CurrentStreak - 1 : -1);

	const int32 StreakMagnitude = FMath::Abs(CurrentStreak);
	int32 StreakBonus = 0;
	if (MatchRules->StreakBonusByCount.Num() > 0)
	{
		const int32 ClampedIndex = FMath::Clamp(StreakMagnitude - 1, 0, MatchRules->StreakBonusByCount.Num() - 1);
		StreakBonus = MatchRules->StreakBonusByCount[ClampedIndex];
	}

	const int32 Interest = FMath::Min(Gold / MatchRules->InterestDivisor, MatchRules->InterestCap);
	Gold += MatchRules->IncomePerRound + Interest + StreakBonus;
}
