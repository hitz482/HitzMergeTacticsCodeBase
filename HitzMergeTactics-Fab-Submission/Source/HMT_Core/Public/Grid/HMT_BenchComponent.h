#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HMT_BenchComponent.generated.h"

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_CORE_API UHMT_BenchComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHMT_BenchComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Bench")
	void InitializeBench(int32 SlotCount);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Bench")
	bool PlaceInSlot(AActor* Unit, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Bench")
	bool RemoveFromSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Bench")
	AActor* GetOccupantAt(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Bench")
	int32 GetSlotCount() const { return Slots.Num(); }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Bench")
	int32 FindFirstFreeSlot() const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Bench")
	TArray<AActor*> GetAllOccupants() const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Bench")
	bool RemoveOccupant(AActor* Unit);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Bench")
	int32 GetVisibleOccupancy() const { return VisibleBenchOccupancy; }

protected:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> Slots;

	UPROPERTY(Replicated)
	int32 VisibleBenchOccupancy = 0;
};
