#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Combat/HMT_CombatTypes.h"
#include "IHMT_ModifierEffect.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class UHMT_ModifierEffect : public UInterface
{
	GENERATED_BODY()
};

class UHMT_CombatContext;
class APlayerState;

class HMT_CORE_API IHMT_ModifierEffect
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Modifiers")
	void OnActivate(APlayerState* Player);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Modifiers")
	void OnDeactivate(APlayerState* Player);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Modifiers")
	void OnCombatStart(APlayerState* Player, UHMT_CombatContext* Context);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Modifiers")
	void OnCombatEvent(APlayerState* Player, const FHMT_CombatEvent& Event, UHMT_CombatContext* Context);
};
