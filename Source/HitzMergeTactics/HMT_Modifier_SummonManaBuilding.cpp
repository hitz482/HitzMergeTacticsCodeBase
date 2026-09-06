#include "HMT_Modifier_SummonManaBuilding.h"
#include "HMT_PlayerState.h"
#include "Grid/HMT_BenchComponent.h"
#include "Units/HMT_UnitActor.h"
#include "Units/HMT_UnitDefinition.h"
#include "GameFramework/PlayerState.h"

void UHMT_Modifier_SummonManaBuilding::OnActivate_Implementation(APlayerState* Player)
{
	// UnitActorClass/BenchComponent/GetBenchSlotWorldLocation are sample-project concepts (which
	// concrete AHMT_UnitActor subclass to spawn, bench presentation), not framework ones — this
	// modifier is sample content, same tier as HMT_TestModifier_BonusGold, so casting down to the
	// sample AHMT_PlayerState is the right call rather than trying to stay framework-generic.
	AHMT_PlayerState* SamplePlayerState = Cast<AHMT_PlayerState>(Player);
	if (!SamplePlayerState || !BuildingDefinition || !SamplePlayerState->UnitActorClass || !SamplePlayerState->BenchComponent)
	{
		return;
	}

	const int32 BenchSlot = SamplePlayerState->BenchComponent->FindFirstFreeSlot();
	if (BenchSlot == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] SummonManaBuilding modifier: %s's bench is full — building not granted."), *Player->GetPlayerName());
		return;
	}

	UWorld* World = SamplePlayerState->GetWorld();
	const FVector SpawnLocation = SamplePlayerState->GetBenchSlotWorldLocation(BenchSlot) + FVector(0.f, 0.f, 50.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SamplePlayerState;
	AHMT_UnitActor* NewUnit = World ? World->SpawnActor<AHMT_UnitActor>(SamplePlayerState->UnitActorClass, FTransform(SpawnLocation), SpawnParams) : nullptr;
	if (!NewUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HMT Sample] SummonManaBuilding modifier: SpawnActor<AHMT_UnitActor> failed."));
		return;
	}

	NewUnit->InitializeFromDefinition(BuildingDefinition, 1);
	SamplePlayerState->BenchComponent->PlaceInSlot(NewUnit, BenchSlot);
	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] SummonManaBuilding modifier: granted %s to %s's bench slot %d."),
		*BuildingDefinition->UnitID.ToString(), *Player->GetPlayerName(), BenchSlot);
}
