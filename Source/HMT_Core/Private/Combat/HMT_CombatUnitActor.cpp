#include "Combat/HMT_CombatUnitActor.h"
#include "Stats/HMT_StatComponent.h"
#include "Units/HMT_UnitInstanceComponent.h"

AHMT_CombatUnitActor::AHMT_CombatUnitActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	StatComponent = CreateDefaultSubobject<UHMT_StatComponent>(TEXT("StatComponent"));
	UnitInstanceComponent = CreateDefaultSubobject<UHMT_UnitInstanceComponent>(TEXT("UnitInstanceComponent"));
}
