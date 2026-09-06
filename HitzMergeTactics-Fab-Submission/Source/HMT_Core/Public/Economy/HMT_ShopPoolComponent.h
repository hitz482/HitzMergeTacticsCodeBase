#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Economy/HMT_MatchRulesAsset.h"
#include "Units/HMT_UnitDefinition.h"
#include "HMT_ShopPoolComponent.generated.h"

USTRUCT()
struct FHMT_UnitPoolEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag UnitID;

	UPROPERTY()
	int32 Remaining = 0;
};

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_CORE_API UHMT_ShopPoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHMT_ShopPoolComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	void InitializePool(const UHMT_MatchRulesAsset* MatchRules, const TArray<UHMT_UnitDefinition*>& AllUnits);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	int32 GetRemainingCount(FGameplayTag UnitID) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	bool TryDecrement(FGameplayTag UnitID);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	void Increment(FGameplayTag UnitID, int32 Count);

protected:
	UPROPERTY(Replicated)
	TArray<FHMT_UnitPoolEntry> Entries;

private:
	FHMT_UnitPoolEntry* FindEntry(FGameplayTag UnitID);
};
