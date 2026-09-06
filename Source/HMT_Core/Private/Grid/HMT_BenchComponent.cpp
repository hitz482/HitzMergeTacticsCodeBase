#include "Grid/HMT_BenchComponent.h"
#include "Net/UnrealNetwork.h"

UHMT_BenchComponent::UHMT_BenchComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHMT_BenchComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UHMT_BenchComponent, Slots, COND_OwnerOnly);
	DOREPLIFETIME(UHMT_BenchComponent, VisibleBenchOccupancy);
}

void UHMT_BenchComponent::InitializeBench(int32 SlotCount)
{
	Slots.Init(nullptr, FMath::Max(SlotCount, 0));
}

bool UHMT_BenchComponent::PlaceInSlot(AActor* Unit, int32 SlotIndex)
{
	if (!Unit || !Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex] != nullptr)
	{
		return false;
	}

	Slots[SlotIndex] = Unit;
	++VisibleBenchOccupancy;
	return true;
}

bool UHMT_BenchComponent::RemoveFromSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex] == nullptr)
	{
		return false;
	}

	Slots[SlotIndex] = nullptr;
	--VisibleBenchOccupancy;
	return true;
}

AActor* UHMT_BenchComponent::GetOccupantAt(int32 SlotIndex) const
{
	return Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex] : nullptr;
}

int32 UHMT_BenchComponent::FindFirstFreeSlot() const
{
	return Slots.IndexOfByPredicate([](const TObjectPtr<AActor>& Occupant) { return Occupant == nullptr; });
}

TArray<AActor*> UHMT_BenchComponent::GetAllOccupants() const
{
	TArray<AActor*> Occupants;
	for (const TObjectPtr<AActor>& Occupant : Slots)
	{
		if (Occupant)
		{
			Occupants.Add(Occupant);
		}
	}
	return Occupants;
}

bool UHMT_BenchComponent::RemoveOccupant(AActor* Unit)
{
	const int32 Index = Slots.IndexOfByPredicate([Unit](const TObjectPtr<AActor>& Occupant) { return Occupant == Unit; });
	if (Index == INDEX_NONE)
	{
		return false;
	}

	Slots[Index] = nullptr;
	--VisibleBenchOccupancy;
	return true;
}
