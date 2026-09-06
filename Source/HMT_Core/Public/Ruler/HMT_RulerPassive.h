#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HMT_RulerPassive.generated.h"

struct FHMT_CombatEvent;
class APlayerState;

UINTERFACE(MinimalAPI, Blueprintable)
class UHMT_RulerPassive : public UInterface
{
	GENERATED_BODY()
};

class HMT_CORE_API IHMT_RulerPassive
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ruler Passive")
	void OnMatchStart(APlayerState* PlayerState);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ruler Passive")
	void OnRoundStart(APlayerState* PlayerState, int32 RoundNumber);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ruler Passive")
	void OnCombatEvent(APlayerState* PlayerState, const FHMT_CombatEvent& Event);
};
