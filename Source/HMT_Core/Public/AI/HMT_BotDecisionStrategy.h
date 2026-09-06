#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HMT_BotDecisionStrategy.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class UHMT_BotDecisionStrategy : public UInterface
{
	GENERATED_BODY()
};

class HMT_CORE_API IHMT_BotDecisionStrategy
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|AI")
	void TakeTurn(APlayerState* SelfPlayerState, int32 RoundNumber);
};
