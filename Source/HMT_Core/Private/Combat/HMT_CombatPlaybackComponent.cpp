#include "Combat/HMT_CombatPlaybackComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"

UHMT_CombatPlaybackComponent::UHMT_CombatPlaybackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHMT_CombatPlaybackComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHMT_CombatPlaybackComponent, EventLog);
	DOREPLIFETIME(UHMT_CombatPlaybackComponent, SideASnapshot);
	DOREPLIFETIME(UHMT_CombatPlaybackComponent, SideBSnapshot);
	DOREPLIFETIME(UHMT_CombatPlaybackComponent, bOwnerIsSideA);
	DOREPLIFETIME(UHMT_CombatPlaybackComponent, CombatStartServerTime);
}

void UHMT_CombatPlaybackComponent::BeginPlay()
{
	Super::BeginPlay();
	EventLog.OwningComponent = this;
}

void UHMT_CombatPlaybackComponent::SetCombatLog(const FHMT_CombatEventLog& NewLog, const FHMT_BoardSnapshot& InSideA, const FHMT_BoardSnapshot& InSideB, bool bInOwnerIsSideA)
{
	SideASnapshot = InSideA;
	SideBSnapshot = InSideB;
	bOwnerIsSideA = bInOwnerIsSideA;
	if (const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		CombatStartServerTime = GameState->GetServerWorldTimeSeconds();
	}

	EventLog.Events = NewLog.Events;
	EventLog.MarkArrayDirty();

	for (const FHMT_CombatEvent& Event : EventLog.Events)
	{
		NotifyCombatEventReceived(Event);
	}
}

void UHMT_CombatPlaybackComponent::NotifyCombatEventReceived(const FHMT_CombatEvent& Event)
{
	OnCombatEventReceived(Event);
	OnCombatEvent.Broadcast(Event);
}

float UHMT_CombatPlaybackComponent::GetElapsedSinceCombatStart() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState || CombatStartServerTime <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, GameState->GetServerWorldTimeSeconds() - CombatStartServerTime);
}
