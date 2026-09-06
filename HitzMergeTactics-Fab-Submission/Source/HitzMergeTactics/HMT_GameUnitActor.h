#pragma once

#include "CoreMinimal.h"
#include "Units/HMT_UnitActor.h"
#include "HMT_GameUnitActor.generated.h"

/**
 * Sample-layer unit actor: hides its mesh on machines that aren't playing as its owning
 * player. The actor itself must replicate everywhere (board occupancy, snapshots, and merge
 * logic all reference it), so this is a purely local visibility policy, mirroring the board
 * tiles: you see your own units during placement; opponents' armies appear only through the
 * combat playback proxies (and later, spectator snapshots). Requires the server to spawn it
 * with Owner = the owning AHMT_PlayerState — Owner replicates, OnRep_Owner re-evaluates.
 */
UCLASS()
class AHMT_GameUnitActor : public AHMT_UnitActor
{
	GENERATED_BODY()

public:
	AHMT_GameUnitActor();

	/** Re-applies the local visibility policy (own units always; other players' units only while
	 *  their PlayerState is locally spectated). Safe to call any time; retries itself on a short
	 *  timer while ownership/local-player identity can't be determined yet (replication order). */
	void RefreshLocalVisibility();

	/** Seconds a board/bench move or swap takes to visually glide to its new spot, instead of
	 *  snapping instantly. Purely cosmetic: BoardComponent/BenchComponent occupancy (the actual
	 *  gameplay state) is already updated the instant the server calls this; only the actor's own
	 *  transform eases toward the target, tick by tick, which then rides normal actor-movement
	 *  replication to remote clients like any other moving actor — no extra netcode needed. */
	UPROPERTY(EditDefaultsOnly, Category = "HMT Sample")
	float MoveSmoothingDuration = 0.25f;

	/** Starts (or redirects, if already mid-move) a smooth glide from the actor's CURRENT location
	 *  to NewLocation over MoveSmoothingDuration. Call this instead of SetActorLocation for
	 *  placement moves/swaps — see AHMT_PlayerState::ServerMoveOrSwapUnit. */
	void MoveSmoothly(const FVector& NewLocation);

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_Owner() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	FTimerHandle VisibilityRetryHandle;

	FVector MoveInterpStart = FVector::ZeroVector;
	FVector MoveInterpTarget = FVector::ZeroVector;
	float MoveInterpElapsed = 0.f;
	bool bInterpolatingMove = false;
};
