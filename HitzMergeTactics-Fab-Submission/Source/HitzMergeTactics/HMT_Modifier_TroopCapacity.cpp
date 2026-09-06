#include "HMT_Modifier_TroopCapacity.h"
#include "Grid/HMT_BoardComponent.h"
#include "GameFramework/PlayerState.h"

void UHMT_Modifier_TroopCapacity::OnActivate_Implementation(APlayerState* Player)
{
	UHMT_BoardComponent* Board = Player ? Player->FindComponentByClass<UHMT_BoardComponent>() : nullptr;
	if (!Board)
	{
		return;
	}

	if (Board->GetBonusUnitCapacity() >= MaxBonusCapacity)
	{
		return;
	}

	const int32 Grant = FMath::Min(BonusPerRound, MaxBonusCapacity - Board->GetBonusUnitCapacity());
	Board->GrantBonusUnitCapacity(Grant);
	UE_LOG(LogTemp, Log, TEXT("[HMT Sample] TroopCapacity modifier: %s board capacity now +%d (cap +%d)."),
		*Player->GetPlayerName(), Board->GetBonusUnitCapacity(), MaxBonusCapacity);
}
