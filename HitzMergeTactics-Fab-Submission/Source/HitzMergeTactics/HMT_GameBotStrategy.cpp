#include "HMT_GameBotStrategy.h"
#include "HMT_PlayerState.h"
#include "HMT_GameState.h"
#include "Grid/HMT_BoardComponent.h"
#include "Grid/HMT_BenchComponent.h"
#include "Economy/HMT_ShopComponent.h"
#include "Economy/HMT_ShopPoolComponent.h"
#include "Units/HMT_UnitCollectionComponent.h"
#include "Units/HMT_UnitInstanceComponent.h"
#include "Units/HMT_UnitActor.h"
#include "Synergy/HMT_SynergyManagerComponent.h"

void UHMT_GameBotStrategy::TakeTurn_Implementation(APlayerState* SelfPlayerState, int32 RoundNumber)
{
	AHMT_PlayerState* Bot = Cast<AHMT_PlayerState>(SelfPlayerState);
	if (!Bot || !Bot->GetShop() || !Bot->GetBoard() || !Bot->GetBench())
	{
		return;
	}

	AHMT_GameState* SampleGameState = Bot->GetWorld() ? Bot->GetWorld()->GetGameState<AHMT_GameState>() : nullptr;
	if (!SampleGameState || !SampleGameState->ShopPoolComponent)
	{
		return;
	}

	UHMT_ShopComponent* Shop = Bot->GetShop();
	UHMT_BoardComponent* Board = Bot->GetBoard();

	// Trait-frequency map from what's already on the board — this is what makes buys "trait-aware":
	// a shop unit sharing traits with units already deployed scores far above a same-tier unit that
	// doesn't, so the bot naturally leans into whatever synergy it's already started.
	TMap<FGameplayTag, int32> TraitCounts;
	for (AActor* Occupant : Board->GetAllOccupants())
	{
		const AHMT_UnitActor* UnitActor = Cast<AHMT_UnitActor>(Occupant);
		const UHMT_UnitDefinition* Definition = (UnitActor && UnitActor->UnitInstanceComponent) ? UnitActor->UnitInstanceComponent->GetUnitDefinition() : nullptr;
		if (!Definition)
		{
			continue;
		}
		for (const FGameplayTag& Trait : Definition->GetTraitsArray())
		{
			++TraitCounts.FindOrAdd(Trait);
		}
	}

	// Bounded by shop size: BuyUnit clears a slot on success, so after at most GetShopSlotCount()
	// successful buys the shop is empty and the scoring pass below finds nothing left to buy.
	const int32 MaxAttempts = Shop->GetShopSlotCount();
	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		int32 BestSlot = INDEX_NONE;
		float BestScore = -1.f;
		for (int32 Slot = 0; Slot < Shop->GetShopSlotCount(); ++Slot)
		{
			const UHMT_UnitDefinition* Candidate = Shop->GetShopSlot(Slot);
			if (!Candidate)
			{
				continue;
			}

			float Score = Candidate->GetCostTierValue() * CostTierWeight;
			for (const FGameplayTag& Trait : Candidate->GetTraitsArray())
			{
				if (const int32* Count = TraitCounts.Find(Trait))
				{
					Score += *Count * TraitOverlapWeight;
				}
			}

			if (Score > BestScore)
			{
				BestScore = Score;
				BestSlot = Slot;
			}
		}

		if (BestSlot == INDEX_NONE)
		{
			break; // shop has nothing left to evaluate
		}

		FHMT_GridCoord FreeCoord;
		const bool bHasFreeBoardCoord = Bot->FindFirstFreeBoardCoord(FreeCoord);
		const int32 BenchSlot = bHasFreeBoardCoord ? INDEX_NONE : Bot->GetBench()->FindFirstFreeSlot();
		if (!bHasFreeBoardCoord && BenchSlot == INDEX_NONE)
		{
			break; // board and bench both full — nowhere left to put another unit
		}

		UHMT_UnitDefinition* Bought = Shop->BuyUnit(BestSlot, SampleGameState->ShopPoolComponent);
		if (!Bought)
		{
			// Highest-scored slot turned out unaffordable (or the pool ran dry) — stop rather than
			// re-picking the same failing slot every remaining attempt.
			break;
		}

		const FVector SpawnLocation = bHasFreeBoardCoord
			? Bot->GetBoardWorldOrigin() + Board->GridToWorld(FreeCoord) + FVector(0.f, 0.f, 50.f)
			: Bot->GetBenchSlotWorldLocation(BenchSlot) + FVector(0.f, 0.f, 50.f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Bot;
		AHMT_UnitActor* NewUnit = Bot->GetWorld()->SpawnActor<AHMT_UnitActor>(Bot->UnitActorClass, FTransform(SpawnLocation), SpawnParams);
		if (!NewUnit)
		{
			break;
		}

		NewUnit->InitializeFromDefinition(Bought, 1);
		if (bHasFreeBoardCoord)
		{
			Board->PlaceUnit(NewUnit, FreeCoord);
			for (const FGameplayTag& Trait : Bought->GetTraitsArray())
			{
				++TraitCounts.FindOrAdd(Trait);
			}
		}
		else
		{
			Bot->GetBench()->PlaceInSlot(NewUnit, BenchSlot);
		}

		Bot->GetUnitCollection()->RunMergeCascade();
		Bot->GetSynergyManager()->RecomputeSynergies();
	}
}
