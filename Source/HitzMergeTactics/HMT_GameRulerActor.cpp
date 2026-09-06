#include "HMT_GameRulerActor.h"
#include "HMT_PlayerState.h"
#include "HMT_PlayerController.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"

AHMT_GameRulerActor::AHMT_GameRulerActor()
{
	// Sample overhead name/HP bar, wired here in C++ (not on a Blueprint CDO) so it can't be lost to
	// a transient reflection edit or a reinstance — same reasoning as AHMT_GameMode's data defaults,
	// and the same FClassFinder pattern AHMT_PlayerState uses for its unit health-bar widget. Buyers
	// can still override OverheadWidgetClass on a Blueprint subclass; a project without this WBP just
	// gets no overhead bar (the base class already treats an unset OverheadWidgetClass as "no bar").
	static ConstructorHelpers::FClassFinder<UUserWidget> OverheadWidgetFinder(TEXT("/Game/HitzMergeTactics/UI/WBP_HMT_RulerOverheadBar"));
	if (OverheadWidgetFinder.Succeeded())
	{
		OverheadWidgetClass = OverheadWidgetFinder.Class;
	}
}

void AHMT_GameRulerActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshLocalVisibility();
}

void AHMT_GameRulerActor::OnRep_Owner()
{
	Super::OnRep_Owner();
	RefreshLocalVisibility();
}

void AHMT_GameRulerActor::RefreshLocalVisibility()
{
	AHMT_PlayerState* OwningPlayer = Cast<AHMT_PlayerState>(GetOwner());
	bool bIsLocal = false;
	if (!OwningPlayer || !OwningPlayer->TryDetermineLocallyViewed(bIsLocal))
	{
		// Owner not replicated yet, or local player identity not established — retry shortly.
		GetWorldTimerManager().SetTimer(VisibilityRetryHandle, this, &AHMT_GameRulerActor::RefreshLocalVisibility, 0.25f, false);
		return;
	}

	// Same "hide while spectating anyone" rule AHMT_GameUnitActor::RefreshLocalVisibility applies to
	// units — spectating replaces the local board's contents with the target's, so even our own
	// Ruler standing beside our own bench would be a stale/confusing presence during that view.
	const AHMT_PlayerController* LocalController = nullptr;
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PC = It->Get();
			if (PC && PC->IsLocalPlayerController())
			{
				LocalController = Cast<AHMT_PlayerController>(PC);
				break;
			}
		}
	}
	const bool bSpectatingSomeone = LocalController && LocalController->GetSpectateTarget() != nullptr;

	SetLocalVisibility(bSpectatingSomeone ? false : bIsLocal);
}
