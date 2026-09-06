#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Combat/HMT_CombatTypes.h"
#include "IHMT_AbilityExecutor.generated.h"

class UHMT_CombatContext;

UINTERFACE(BlueprintType, MinimalAPI)
class UHMT_AbilityExecutor : public UInterface
{
	GENERATED_BODY()
};

class HMT_CORE_API IHMT_AbilityExecutor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Combat")
	bool CanExecuteAbility(AActor* SelfActor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Combat")
	void ExecuteAbility(AActor* SelfActor, AActor* Target, UHMT_CombatContext* Context);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Combat")
	void OnCombatEvent(AActor* SelfActor, const FHMT_CombatEvent& Event, UHMT_CombatContext* Context);
};
