#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "HMT_MatchTypes.generated.h"

UENUM(BlueprintType)
enum class EHMT_MatchPhase : uint8
{
	Preparation,
	CombatLock,
	Combat,
	Results,
	RoundEnd,
	MatchEnded
};

UENUM(BlueprintType)
enum class EHMT_RoundType : uint8
{
	PvP,
	PvE
};

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_PlayerMatchState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Match")
	TObjectPtr<APlayerState> Player = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Match")
	int32 HP = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Match")
	bool bEliminated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Match")
	TObjectPtr<APlayerState> CurrentOpponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Match")
	bool bIsBotControlled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Match")
	TObjectPtr<class UHMT_RulerDefinition> RulerDefinition = nullptr;
};
