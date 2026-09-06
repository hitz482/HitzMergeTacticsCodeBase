#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "HMT_UnitCollectionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHMT_OnUnitMerged, AActor*, SurvivorUnit, FGameplayTag, UnitID, int32, NewStarLevel);

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_CORE_API UHMT_UnitCollectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHMT_UnitCollectionComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Merge")
	int32 CopiesRequiredForMerge = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitzMergeTactics|Merge")
	int32 MaxStarLevel = 3;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HitzMergeTactics|Merge")
	int32 GetCopiesRequiredFor(const UHMT_UnitDefinition* Definition, int32 StarLevel);
	virtual int32 GetCopiesRequiredFor_Implementation(const UHMT_UnitDefinition* Definition, int32 StarLevel);

	UPROPERTY(BlueprintAssignable, Category = "HitzMergeTactics|Merge")
	FHMT_OnUnitMerged OnUnitMerged;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Merge")
	TArray<AActor*> GetAllOwnedUnits() const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Merge")
	int32 GetOwnedCount(FGameplayTag UnitID) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Merge")
	void RunMergeCascade();

private:
	bool TryMergeOnce();
};
