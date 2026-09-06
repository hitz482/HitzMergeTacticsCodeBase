#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HMT_GameState.generated.h"

class UHMT_ShopPoolComponent;
class UHMT_MatchStateComponent;

/** Reference wiring: the two match-wide (not per-player) HMT_Core components. */
UCLASS()
class AHMT_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AHMT_GameState();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HMT Sample")
	TObjectPtr<UHMT_ShopPoolComponent> ShopPoolComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HMT Sample")
	TObjectPtr<UHMT_MatchStateComponent> MatchStateComponent;

	/** UFUNCTION wrapper — the reflection-based Blueprint tooling used to build sample UI (Phase 9)
	 *  can only create nodes for callable functions, not bare property reads on a non-self object. */
	UFUNCTION(BlueprintCallable, Category = "HMT Sample")
	UHMT_MatchStateComponent* GetMatchState() const { return MatchStateComponent; }
};
