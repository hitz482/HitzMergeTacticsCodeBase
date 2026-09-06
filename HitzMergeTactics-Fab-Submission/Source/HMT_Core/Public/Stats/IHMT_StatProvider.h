#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Stats/HMT_StatTypes.h"
#include "IHMT_StatProvider.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class UHMT_StatProvider : public UInterface
{
	GENERATED_BODY()
};

class HMT_CORE_API IHMT_StatProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Stats")
	float GetBaseStatValue(FGameplayTag StatTag);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Stats")
	void SetBaseStatValue(FGameplayTag StatTag, float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Stats")
	float GetFinalStatValue(FGameplayTag StatTag);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Stats")
	void ApplyStatModifier(FGameplayTag StatTag, EHMT_StatModOp ModOp, float Magnitude, EHMT_ModifierDuration Duration, float TimedDurationSeconds, EHMT_StackingRule StackingRule, FGameplayTag SourceTag);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Stats")
	void RemoveModifiersBySource(FGameplayTag SourceTag);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Stats")
	TArray<FHMT_StatModifier> GetActiveModifiers();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Stats")
	void RemoveModifiersByDuration(EHMT_ModifierDuration Duration);
};
