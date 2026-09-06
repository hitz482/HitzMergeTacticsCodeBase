#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Combat/HMT_CombatTypes.h"
#include "HMT_CombatResolver.generated.h"

UCLASS(BlueprintType, Blueprintable)
class HMT_CORE_API UHMT_CombatResolver : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	float TickInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	float MaxCombatDuration = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag HealthStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag AttackDamageStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag ArmorStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag RangeStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag MovementSpeedStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag AttackSpeedStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag CritChanceStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag CritDamageStatTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag StunEffectTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag RootEffectTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag SilenceEffectTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat")
	FGameplayTag TauntEffectTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Combat", meta = (Categories = "Stat"))
	FGameplayTag CastTimeStatTag;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat", meta = (WorldContext = "WorldContextObject"))
	FHMT_CombatEventLog ResolveCombat(UObject* WorldContextObject, const FHMT_BoardSnapshot& SideA, const FHMT_BoardSnapshot& SideB);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Combat")
	float ComputeAttackDamage(AActor* Attacker, AActor* Defender, bool& bOutCritical);
	virtual float ComputeAttackDamage_Implementation(AActor* Attacker, AActor* Defender, bool& bOutCritical);
};
