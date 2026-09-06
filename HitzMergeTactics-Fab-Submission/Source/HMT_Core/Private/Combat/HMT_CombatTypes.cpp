#include "Combat/HMT_CombatTypes.h"
#include "Combat/HMT_CombatPlaybackComponent.h"

void FHMT_CombatEvent::PostReplicatedAdd(const FHMT_CombatEventLog& InArraySerializer)
{
	if (UHMT_CombatPlaybackComponent* Component = InArraySerializer.OwningComponent.Get())
	{
		Component->NotifyCombatEventReceived(*this);
	}
}
