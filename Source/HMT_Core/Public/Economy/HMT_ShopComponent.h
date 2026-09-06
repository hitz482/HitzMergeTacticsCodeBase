#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Economy/HMT_MatchRulesAsset.h"
#include "Economy/HMT_ShopPoolComponent.h"
#include "HMT_ShopComponent.generated.h"

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_CORE_API UHMT_ShopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHMT_ShopComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	void InitializeShop(UHMT_MatchRulesAsset* Rules, const TArray<UHMT_UnitDefinition*>& InAvailableUnitPool);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	void RollShop(UHMT_ShopPoolComponent* Pool);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	void SetLocked(bool bInLocked) { bLocked = bInLocked; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	bool IsLocked() const { return bLocked; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	UHMT_UnitDefinition* BuyUnit(int32 SlotIndex, UHMT_ShopPoolComponent* Pool);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	bool SellUnit(UHMT_UnitDefinition* UnitDefinition, int32 StarLevel, UHMT_ShopPoolComponent* Pool);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	bool RerollShop(UHMT_ShopPoolComponent* Pool);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	void ApplyRoundIncome(bool bWonLastRound);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	int32 GetGold() const { return Gold; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	void AddGold(int32 Amount) { Gold += Amount; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	UHMT_MatchRulesAsset* GetMatchRules() const { return MatchRules; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	UHMT_UnitDefinition* GetShopSlot(int32 SlotIndex) const { return ShopSlots.IsValidIndex(SlotIndex) ? ShopSlots[SlotIndex].Get() : nullptr; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	int32 GetShopSlotCount() const { return ShopSlots.Num(); }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Economy")
	int32 GetVisibleShopOccupancy() const { return VisibleShopOccupancy; }

protected:
	UPROPERTY(Replicated)
	TObjectPtr<UHMT_MatchRulesAsset> MatchRules = nullptr;

	UPROPERTY(Replicated)
	int32 Gold = 0;

	UPROPERTY(Replicated)
	int32 CurrentStreak = 0;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<UHMT_UnitDefinition>> ShopSlots;

	UPROPERTY(Replicated)
	int32 VisibleShopOccupancy = 0;

	UPROPERTY(Replicated)
	bool bLocked = false;

	UPROPERTY()
	TArray<TObjectPtr<UHMT_UnitDefinition>> AvailableUnitPool;

private:
	int32 PickWeightedTier(const TArray<float>& TierOdds) const;
};
