#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Stats/HMT_StatTypes.h"
#include "Stats/IHMT_StatProvider.h"
#include "HMT_StatComponent.generated.h"

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_CORE_API UHMT_StatComponent : public UActorComponent, public IHMT_StatProvider
{
	GENERATED_BODY()

public:
	UHMT_StatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetBaseStatValue_Implementation(FGameplayTag StatTag);
	virtual void SetBaseStatValue_Implementation(FGameplayTag StatTag, float Value);
	virtual float GetFinalStatValue_Implementation(FGameplayTag StatTag);
	virtual void ApplyStatModifier_Implementation(FGameplayTag StatTag, EHMT_StatModOp ModOp, float Magnitude, EHMT_ModifierDuration Duration, float TimedDurationSeconds, EHMT_StackingRule StackingRule, FGameplayTag SourceTag);
	virtual void RemoveModifiersBySource_Implementation(FGameplayTag SourceTag);
	virtual TArray<FHMT_StatModifier> GetActiveModifiers_Implementation();
	virtual void RemoveModifiersByDuration_Implementation(EHMT_ModifierDuration Duration);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Stats")
	TArray<FHMT_BaseStat> GetAllBaseStats() const { return BaseStats; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "HitzMergeTactics|Stats")
	TArray<FHMT_BaseStat> BaseStats;

	UPROPERTY(Replicated)
	FHMT_ModifierArray ModifierArray;

private:
	FHMT_BaseStat* FindBaseStat(FGameplayTag StatTag);
	const FHMT_BaseStat* FindBaseStat(FGameplayTag StatTag) const;

	void RemoveModifierByHandle(int32 Handle);

	int32 NextModifierHandle = 0;

	TMap<int32, FTimerHandle> TimedExpiryHandles;
};
