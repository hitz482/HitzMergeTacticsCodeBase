#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IHMT_TargetingStrategy.generated.h"

class UHMT_CombatContext;

UINTERFACE(BlueprintType, MinimalAPI)
class UHMT_TargetingStrategy : public UInterface
{
	GENERATED_BODY()
};

class HMT_CORE_API IHMT_TargetingStrategy
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Combat")
	AActor* SelectTarget(AActor* SelfActor, const TArray<AActor*>& Candidates, UHMT_CombatContext* Context);
};
