#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Units/HMT_UnitDefinition.h"
#include "HMT_UnitInstanceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHMT_OnStarLevelChanged, int32, OldStarLevel, int32, NewStarLevel);

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_CORE_API UHMT_UnitInstanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHMT_UnitInstanceComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "HitzMergeTactics|Units")
	FHMT_OnStarLevelChanged OnStarLevelChanged;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	void InitializeUnit(UHMT_UnitDefinition* Definition, int32 InStarLevel);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	void SetStarLevel(int32 NewStarLevel);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	UHMT_UnitDefinition* GetUnitDefinition() const { return UnitDefinition; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	int32 GetStarLevel() const { return StarLevel; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Units")
	FGuid GetInstanceId() const { return InstanceId; }

protected:
	UPROPERTY(Replicated)
	TObjectPtr<UHMT_UnitDefinition> UnitDefinition = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_StarLevel)
	int32 StarLevel = 1;

	UPROPERTY(Replicated)
	FGuid InstanceId;

	UFUNCTION()
	void OnRep_StarLevel(int32 OldStarLevel);

private:
	void ApplyScaledStats();
};
