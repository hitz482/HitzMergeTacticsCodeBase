#include "HMT_TestModifier_BonusGold.h"
#include "Economy/HMT_ShopComponent.h"
#include "GameFramework/PlayerState.h"

void UHMT_TestModifier_BonusGold::OnActivate_Implementation(APlayerState* Player)
{
	if (!Player)
	{
		return;
	}

	if (UHMT_ShopComponent* Shop = Player->FindComponentByClass<UHMT_ShopComponent>())
	{
		Shop->AddGold(50);
		UE_LOG(LogTemp, Warning, TEXT("[HMT Test] BonusGold modifier activated for %s — +50 gold, new total %d."), *Player->GetPlayerName(), Shop->GetGold());
	}
}
