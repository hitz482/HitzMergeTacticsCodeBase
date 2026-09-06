#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Combat/HMT_CombatTypes.h"
#include "IHMT_TraitEffect.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class UHMT_TraitEffect : public UInterface
{
	GENERATED_BODY()
};

class UHMT_CombatContext;

class HMT_CORE_API IHMT_TraitEffect
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	void OnActivate(const TArray<AActor*>& AffectedUnits);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	void OnDeactivate(const TArray<AActor*>& AffectedUnits);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	void OnCombatStart(int32 OwningSide, UHMT_CombatContext* Context);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	void OnCombatEvent(const FHMT_CombatEvent& Event, UHMT_CombatContext* Context);
};
