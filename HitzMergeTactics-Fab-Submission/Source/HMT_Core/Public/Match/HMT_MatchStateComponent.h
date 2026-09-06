#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Match/HMT_MatchTypes.h"
#include "Match/HMT_PvERoundDefinition.h"
#include "Economy/HMT_MatchRulesAsset.h"
#include "Economy/HMT_ShopPoolComponent.h"
#include "Combat/HMT_CombatTypes.h"
#include "Combat/HMT_CombatResolver.h"
#include "Modifiers/HMT_GameModifierDefinition.h"
#include "HMT_MatchStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHMT_OnRoundStart, int32, RoundNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHMT_OnCombatEnd, APlayerState*, Player, bool, bWon);

USTRUCT()
struct FHMT_ModifierEffectInstanceList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UObject>> Instances;
};

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_CORE_API UHMT_MatchStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHMT_MatchStateComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	void StartMatch(UHMT_MatchRulesAsset* Rules, UHMT_ShopPoolComponent* Pool, const TArray<APlayerState*>& InPlayers, const TArray<UHMT_PvERoundDefinition*>& InPvEPool, const TSet<APlayerState*>& InBotPlayers, const TArray<class UHMT_RulerDefinition*>& InAvailableRulers);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	void ForceEndPreparationPhase();

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	EHMT_MatchPhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	int32 GetRoundNumber() const { return RoundNumber; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	UHMT_MatchRulesAsset* GetMatchRules() const { return MatchRules; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	float GetPhaseTimeRemaining() const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	APlayerState* GetCurrentOpponentFor(APlayerState* Player) const
	{
		const FHMT_PlayerMatchState* Found = Players.FindByPredicate([Player](const FHMT_PlayerMatchState& PlayerState) { return PlayerState.Player == Player; });
		return Found ? Found->CurrentOpponent.Get() : nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	void SetPlayerBotControlled(APlayerState* Player, bool bInIsBotControlled)
	{
		if (FHMT_PlayerMatchState* Found = Players.FindByPredicate([Player](const FHMT_PlayerMatchState& PlayerState) { return PlayerState.Player == Player; }))
		{
			Found->bIsBotControlled = bInIsBotControlled;
		}
	}

	UFUNCTION(BlueprintPure, Category = "HitzMergeTactics|Match")
	bool IsPlayerBotControlled(APlayerState* Player) const
	{
		const FHMT_PlayerMatchState* Found = Players.FindByPredicate([Player](const FHMT_PlayerMatchState& PlayerState) { return PlayerState.Player == Player; });
		return Found && Found->bIsBotControlled;
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	bool GetPlayerMatchState(APlayerState* Player, FHMT_PlayerMatchState& OutState) const
	{
		const FHMT_PlayerMatchState* Found = Players.FindByPredicate([Player](const FHMT_PlayerMatchState& PlayerState) { return PlayerState.Player == Player; });
		if (Found)
		{
			OutState = *Found;
			return true;
		}
		return false;
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	bool GetPlayerBoardSnapshot(APlayerState* Player, FHMT_BoardSnapshot& OutSnapshot) const
	{
		if (const FHMT_BoardSnapshot* Found = LastKnownSnapshot.Find(Player))
		{
			OutSnapshot = *Found;
			return true;
		}
		return false;
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Match")
	bool GetPlayerRuler(APlayerState* Player, class UHMT_RulerDefinition*& OutRuler, int32& OutHP) const
	{
		const FHMT_PlayerMatchState* Found = Players.FindByPredicate([Player](const FHMT_PlayerMatchState& PlayerState) { return PlayerState.Player == Player; });
		if (Found)
		{
			OutRuler = Found->RulerDefinition;
			OutHP = Found->HP;
			return true;
		}
		return false;
	}

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Modifiers")
	TArray<class UHMT_GameModifierDefinition*> GetActiveGameModifiers() const
	{
		TArray<UHMT_GameModifierDefinition*> Result;
		Result.Reserve(ActiveGameModifiers.Num());
		for (UHMT_GameModifierDefinition* Modifier : ActiveGameModifiers)
		{
			Result.Add(Modifier);
		}
		return Result;
	}

	UPROPERTY(BlueprintAssignable, Category = "HitzMergeTactics|Match")
	FHMT_OnRoundStart OnRoundStart;

	UPROPERTY(BlueprintAssignable, Category = "HitzMergeTactics|Match")
	FHMT_OnCombatEnd OnCombatEnd;

protected:
	UPROPERTY(Replicated)
	EHMT_MatchPhase CurrentPhase = EHMT_MatchPhase::Preparation;

	UPROPERTY(Replicated)
	float PhaseTimeRemaining = 0.f;

	UPROPERTY(Replicated)
	float PhaseEndServerTime = 0.f;

	UPROPERTY(Replicated)
	int32 RoundNumber = 0;

	UPROPERTY(Replicated)
	EHMT_RoundType CurrentRoundType = EHMT_RoundType::PvP;

	UPROPERTY(Replicated)
	TArray<FHMT_PlayerMatchState> Players;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<class UHMT_GameModifierDefinition>> ActiveGameModifiers;

	UPROPERTY(Replicated)
	TObjectPtr<UHMT_MatchRulesAsset> MatchRules;

	UPROPERTY()
	TObjectPtr<UHMT_ShopPoolComponent> ShopPool;

	UPROPERTY()
	TObjectPtr<UHMT_CombatResolver> CombatResolver;

	UPROPERTY()
	TArray<TObjectPtr<UHMT_PvERoundDefinition>> PvERoundPool;

	TMap<TWeakObjectPtr<APlayerState>, TMap<TWeakObjectPtr<APlayerState>, int32>> RoundLastPaired;

	UPROPERTY()
	TMap<TObjectPtr<APlayerState>, FHMT_BoardSnapshot> LastKnownSnapshot;

	UPROPERTY()
	TMap<TObjectPtr<APlayerState>, TObjectPtr<UObject>> RulerPassiveInstances;

	UPROPERTY()
	TMap<TObjectPtr<APlayerState>, FHMT_ModifierEffectInstanceList> ModifierEffectInstances;

	FTimerHandle PhaseTimerHandle;

	void BeginPreparationPhase();
	void BeginCombatLockPhase();
	void BeginCombatPhase();
	void BeginResultsPhase();
	void BeginRoundEndPhase();
	void CheckForMatchEnd();

	FHMT_BoardSnapshot BuildSnapshotForPlayer(APlayerState* Player) const;
	FHMT_PlayerMatchState* FindPlayerState(APlayerState* Player);

	void ActivateModifiersForTrigger(EHMT_ModifierTriggerType TriggerType);

private:
	struct FHMT_RoundPairing
	{
		TWeakObjectPtr<APlayerState> PlayerA;
		TWeakObjectPtr<APlayerState> PlayerB;
	};

	TArray<FHMT_RoundPairing> BuildPairings() const;

	float ResolvePairing(const FHMT_RoundPairing& Pairing);

	float ResolvePvERound(APlayerState* Player, UHMT_PvERoundDefinition* Encounter);
};
