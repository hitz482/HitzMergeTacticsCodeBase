#include "Units/HMT_UnitCollectionComponent.h"
#include "Units/HMT_UnitInstanceComponent.h"
#include "Units/HMT_UnitDefinition.h"
#include "Grid/HMT_BoardComponent.h"
#include "Grid/HMT_BenchComponent.h"

UHMT_UnitCollectionComponent::UHMT_UnitCollectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

TArray<AActor*> UHMT_UnitCollectionComponent::GetAllOwnedUnits() const
{
	TArray<AActor*> Units;

	if (const UHMT_BoardComponent* Board = GetOwner() ? GetOwner()->FindComponentByClass<UHMT_BoardComponent>() : nullptr)
	{
		Units.Append(Board->GetAllOccupants());
	}

	if (const UHMT_BenchComponent* Bench = GetOwner() ? GetOwner()->FindComponentByClass<UHMT_BenchComponent>() : nullptr)
	{
		Units.Append(Bench->GetAllOccupants());
	}

	return Units;
}

int32 UHMT_UnitCollectionComponent::GetOwnedCount(FGameplayTag UnitID) const
{
	int32 Count = 0;
	for (const AActor* Unit : GetAllOwnedUnits())
	{
		const UHMT_UnitInstanceComponent* Instance = Unit->FindComponentByClass<UHMT_UnitInstanceComponent>();
		if (Instance && Instance->GetUnitDefinition() && Instance->GetUnitDefinition()->UnitID == UnitID)
		{
			++Count;
		}
	}
	return Count;
}

int32 UHMT_UnitCollectionComponent::GetCopiesRequiredFor_Implementation(const UHMT_UnitDefinition* Definition, int32 StarLevel)
{
	return CopiesRequiredForMerge;
}

bool UHMT_UnitCollectionComponent::TryMergeOnce()
{
	TMap<TPair<FGameplayTag, int32>, TArray<AActor*>> Grouped;

	for (AActor* Unit : GetAllOwnedUnits())
	{
		UHMT_UnitInstanceComponent* Instance = Unit->FindComponentByClass<UHMT_UnitInstanceComponent>();
		if (!Instance || !Instance->GetUnitDefinition() || Instance->GetUnitDefinition()->bIsBuilding)
		{
			continue;
		}

		const TPair<FGameplayTag, int32> Key(Instance->GetUnitDefinition()->UnitID, Instance->GetStarLevel());
		Grouped.FindOrAdd(Key).Add(Unit);
	}

	for (const auto& Entry : Grouped)
	{
		const int32 StarLevel = Entry.Key.Value;
		const UHMT_UnitInstanceComponent* FirstInstance = Entry.Value[0]->FindComponentByClass<UHMT_UnitInstanceComponent>();
		const int32 RequiredCopies = FMath::Max(GetCopiesRequiredFor(FirstInstance->GetUnitDefinition(), StarLevel), 2);
		if (StarLevel >= MaxStarLevel || Entry.Value.Num() < RequiredCopies)
		{
			continue;
		}

		UHMT_BoardComponent* Board = GetOwner() ? GetOwner()->FindComponentByClass<UHMT_BoardComponent>() : nullptr;
		UHMT_BenchComponent* Bench = GetOwner() ? GetOwner()->FindComponentByClass<UHMT_BenchComponent>() : nullptr;

		AActor* Survivor = Entry.Value[0];
		for (AActor* Candidate : Entry.Value)
		{
			bool bOnBoard = false;
			if (Board)
			{
				Board->GetCoordOfOccupant(Candidate, bOnBoard);
			}
			if (bOnBoard)
			{
				Survivor = Candidate;
				break;
			}
		}

		int32 Destroyed = 0;
		for (AActor* Unit : Entry.Value)
		{
			if (Unit == Survivor || Destroyed >= RequiredCopies - 1)
			{
				continue;
			}

			if (Board)
			{
				Board->RemoveOccupant(Unit);
			}
			if (Bench)
			{
				Bench->RemoveOccupant(Unit);
			}
			Unit->Destroy();
			++Destroyed;
		}

		if (UHMT_UnitInstanceComponent* SurvivorInstance = Survivor->FindComponentByClass<UHMT_UnitInstanceComponent>())
		{
			SurvivorInstance->SetStarLevel(StarLevel + 1);
			OnUnitMerged.Broadcast(Survivor, Entry.Key.Key, StarLevel + 1);
		}

		return true;
	}

	return false;
}

void UHMT_UnitCollectionComponent::RunMergeCascade()
{
	while (TryMergeOnce())
	{
	}
}
