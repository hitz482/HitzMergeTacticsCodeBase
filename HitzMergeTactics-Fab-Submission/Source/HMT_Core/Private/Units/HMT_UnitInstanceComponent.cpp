#include "Units/HMT_UnitInstanceComponent.h"
#include "Stats/IHMT_StatProvider.h"
#include "Net/UnrealNetwork.h"

UHMT_UnitInstanceComponent::UHMT_UnitInstanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHMT_UnitInstanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHMT_UnitInstanceComponent, UnitDefinition);
	DOREPLIFETIME(UHMT_UnitInstanceComponent, StarLevel);
	DOREPLIFETIME(UHMT_UnitInstanceComponent, InstanceId);
}

void UHMT_UnitInstanceComponent::InitializeUnit(UHMT_UnitDefinition* Definition, int32 InStarLevel)
{
	UnitDefinition = Definition;
	StarLevel = InStarLevel;
	InstanceId = FGuid::NewGuid();
	ApplyScaledStats();
}

void UHMT_UnitInstanceComponent::SetStarLevel(int32 NewStarLevel)
{
	const int32 OldStarLevel = StarLevel;
	StarLevel = NewStarLevel;
	ApplyScaledStats();

	if (OldStarLevel != NewStarLevel)
	{
		OnStarLevelChanged.Broadcast(OldStarLevel, NewStarLevel);
	}
}

void UHMT_UnitInstanceComponent::OnRep_StarLevel(int32 OldStarLevel)
{
	ApplyScaledStats();
	OnStarLevelChanged.Broadcast(OldStarLevel, StarLevel);
}

void UHMT_UnitInstanceComponent::ApplyScaledStats()
{
	if (!UnitDefinition)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UActorComponent* StatProviderComponent = Owner ? Owner->FindComponentByInterface(UHMT_StatProvider::StaticClass()) : nullptr;
	if (!StatProviderComponent)
	{
		return;
	}

	for (const FHMT_BaseStat& Stat : UnitDefinition->GetScaledStats(StarLevel))
	{
		IHMT_StatProvider::Execute_SetBaseStatValue(StatProviderComponent, Stat.StatTag, Stat.Value);
	}
}
