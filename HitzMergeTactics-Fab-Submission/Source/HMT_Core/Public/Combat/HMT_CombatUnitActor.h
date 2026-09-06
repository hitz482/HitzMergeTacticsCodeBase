#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HMT_CombatUnitActor.generated.h"

class UHMT_StatComponent;
class UHMT_UnitInstanceComponent;

UCLASS()
class HMT_CORE_API AHMT_CombatUnitActor : public AActor
{
	GENERATED_BODY()

public:
	AHMT_CombatUnitActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat")
	TObjectPtr<UHMT_StatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Combat")
	TObjectPtr<UHMT_UnitInstanceComponent> UnitInstanceComponent;
};
