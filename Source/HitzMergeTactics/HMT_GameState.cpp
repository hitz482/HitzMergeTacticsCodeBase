#include "HMT_GameState.h"
#include "Economy/HMT_ShopPoolComponent.h"
#include "Match/HMT_MatchStateComponent.h"

AHMT_GameState::AHMT_GameState()
{
	ShopPoolComponent = CreateDefaultSubobject<UHMT_ShopPoolComponent>(TEXT("ShopPoolComponent"));
	MatchStateComponent = CreateDefaultSubobject<UHMT_MatchStateComponent>(TEXT("MatchStateComponent"));
}
