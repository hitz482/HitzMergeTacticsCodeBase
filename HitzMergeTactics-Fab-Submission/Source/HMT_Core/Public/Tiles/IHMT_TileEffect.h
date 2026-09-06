#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IHMT_TileEffect.generated.h"

class UHMT_CombatContext;

UINTERFACE(MinimalAPI, Blueprintable)
class UHMT_TileEffect : public UInterface
{
	GENERATED_BODY()
};

class HMT_CORE_API IHMT_TileEffect
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Tiles")
	void OnUnitOnTile(AActor* Unit, UHMT_CombatContext* Context);
};
