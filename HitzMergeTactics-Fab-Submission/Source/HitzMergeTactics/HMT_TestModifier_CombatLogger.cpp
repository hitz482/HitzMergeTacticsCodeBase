#include "HMT_TestModifier_CombatLogger.h"
#include "GameFramework/PlayerState.h"

void UHMT_TestModifier_CombatLogger::OnCombatStart_Implementation(APlayerState* Player, UHMT_CombatContext* Context)
{
	UE_LOG(LogTemp, Warning, TEXT("[HMT Test] CombatLogger modifier: OnCombatStart fired for %s."), Player ? *Player->GetPlayerName() : TEXT("<null>"));
}

void UHMT_TestModifier_CombatLogger::OnCombatEvent_Implementation(APlayerState* Player, const FHMT_CombatEvent& Event, UHMT_CombatContext* Context)
{
	UE_LOG(LogTemp, Warning, TEXT("[HMT Test] CombatLogger modifier: OnCombatEvent fired for %s — EventType=%s."),
		Player ? *Player->GetPlayerName() : TEXT("<null>"), *UEnum::GetValueAsString(Event.EventType));
}
