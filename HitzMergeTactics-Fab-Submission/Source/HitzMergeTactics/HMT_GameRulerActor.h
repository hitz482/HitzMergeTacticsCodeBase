#pragma once

#include "CoreMinimal.h"
#include "Ruler/HMT_RulerActor.h"
#include "HMT_GameRulerActor.generated.h"

/**
 * Sample-layer Ruler actor: hides its own real mesh/overhead widget on machines that aren't playing
 * as its owning player, and while the local player is spectating anyone. The spectated player's Ruler
 * is instead shown as a local-only ghost projected next to the viewer's bench (see
 * AHMT_PlayerState::SpawnSpectateGhosts), the same way their board units are projected — the real
 * far-away Ruler stays hidden. Requires the server to spawn it with Owner = the owning
 * AHMT_PlayerState — Owner replicates, OnRep_Owner re-evaluates.
 */
UCLASS()
class AHMT_GameRulerActor : public AHMT_RulerActor
{
	GENERATED_BODY()

public:
	AHMT_GameRulerActor();

	/** Re-applies the local visibility policy (own Ruler only, and only while not spectating).
	 *  Safe to call any time; retries itself on a short timer while ownership/local-player identity
	 *  can't be determined yet (replication order) — mirrors AHMT_GameUnitActor::RefreshLocalVisibility. */
	void RefreshLocalVisibility();

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_Owner() override;

private:
	FTimerHandle VisibilityRetryHandle;
};
