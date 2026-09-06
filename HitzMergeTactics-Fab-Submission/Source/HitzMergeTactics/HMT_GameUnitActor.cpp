#include "HMT_GameUnitActor.h"
#include "HMT_PlayerState.h"
#include "HMT_PlayerController.h"
#include "Units/HMT_UnitInstanceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"

AHMT_GameUnitActor::AHMT_GameUnitActor()
{
	// Only needs to tick while an interpolated move is in flight — see MoveSmoothly/Tick.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Default (OnlyTickPoseAndRefreshBonesIfRendered) freezes the current pose the instant
	// RefreshLocalVisibility hides this unit (spectating someone else) — on becoming visible again
	// (stop spectating) it stays stuck on whatever frame it froze on instead of resuming Idle. Keeping
	// the pose ticking while hidden costs a handful of skeletal meshes' worth of anim eval, negligible
	// at this match's player/board count, and guarantees it's already correctly animating when shown.
	if (MeshComponent)
	{
		MeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}

void AHMT_GameUnitActor::MoveSmoothly(const FVector& NewLocation)
{
	MoveInterpStart = GetActorLocation();
	MoveInterpTarget = NewLocation;
	MoveInterpElapsed = 0.f;
	bInterpolatingMove = true;
	SetActorTickEnabled(true);
}

void AHMT_GameUnitActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bInterpolatingMove)
	{
		return;
	}

	MoveInterpElapsed += DeltaSeconds;
	const float Alpha = MoveSmoothingDuration > 0.f ? FMath::Clamp(MoveInterpElapsed / MoveSmoothingDuration, 0.f, 1.f) : 1.f;
	SetActorLocation(FMath::Lerp(MoveInterpStart, MoveInterpTarget, Alpha));

	if (Alpha >= 1.f)
	{
		bInterpolatingMove = false;
		SetActorTickEnabled(false);
	}
}

void AHMT_GameUnitActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshLocalVisibility();
}

void AHMT_GameUnitActor::OnRep_Owner()
{
	Super::OnRep_Owner();
	RefreshLocalVisibility();
}

void AHMT_GameUnitActor::RefreshLocalVisibility()
{
	AHMT_PlayerState* OwningPlayer = Cast<AHMT_PlayerState>(GetOwner());
	bool bIsLocal = false;
	if (!OwningPlayer || !OwningPlayer->TryDetermineLocallyViewed(bIsLocal))
	{
		// Owner not replicated yet, or local player identity not established — retry shortly.
		GetWorldTimerManager().SetTimer(VisibilityRetryHandle, this, &AHMT_GameUnitActor::RefreshLocalVisibility, 0.25f, false);
		return;
	}

	// While spectating ANYONE, every real placement unit hides — including the spectated
	// player's own — because visualization is entirely via local-only ghost proxies now
	// (AHMT_PlayerState::SpawnSpectateGhosts), never by touching real units. Moving/relying on
	// real actors for this was the bug: on a listen server those are the actual authoritative
	// actors, not a harmless replicated proxy, and mutating them corrupted real game state.
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

	if (MeshComponent)
	{
		const bool bVisible = bSpectatingSomeone ? false : bIsLocal;
		MeshComponent->SetVisibility(bVisible, true);
	}
}
