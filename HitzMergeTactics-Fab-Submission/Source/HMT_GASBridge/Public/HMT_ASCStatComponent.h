#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Stats/IHMT_StatProvider.h"
#include "HMT_AttributeTagMapAsset.h"
#include "HMT_ASCStatComponent.generated.h"

class UAbilitySystemComponent;

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_GASBRIDGE_API UHMT_ASCStatComponent : public UActorComponent, public IHMT_StatProvider
{
	GENERATED_BODY()

public:
	UHMT_ASCStatComponent();

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|GASBridge")
	void InitializeBridge(UHMT_AttributeTagMapAsset* InAttributeTagMap);

	virtual float GetBaseStatValue_Implementation(FGameplayTag StatTag) override;
	virtual void SetBaseStatValue_Implementation(FGameplayTag StatTag, float Value) override;
	virtual float GetFinalStatValue_Implementation(FGameplayTag StatTag) override;
	virtual void ApplyStatModifier_Implementation(FGameplayTag StatTag, EHMT_StatModOp ModOp, float Magnitude, EHMT_ModifierDuration Duration, float TimedDurationSeconds, EHMT_StackingRule StackingRule, FGameplayTag SourceTag) override;
	virtual void RemoveModifiersBySource_Implementation(FGameplayTag SourceTag) override;

protected:
	UPROPERTY()
	TObjectPtr<UHMT_AttributeTagMapAsset> AttributeTagMap;

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

private:
	struct FHMT_ModifierKey
	{
		FGameplayTag StatTag;
		FGameplayTag SourceTag;

		bool operator==(const FHMT_ModifierKey& Other) const
		{
			return StatTag == Other.StatTag && SourceTag == Other.SourceTag;
		}

		friend uint32 GetTypeHash(const FHMT_ModifierKey& Key)
		{
			return HashCombine(GetTypeHash(Key.StatTag), GetTypeHash(Key.SourceTag));
		}
	};

	TMap<FHMT_ModifierKey, FActiveGameplayEffectHandle> ActiveHandles;
	TMap<FGameplayTag, TArray<FHMT_ModifierKey>> SourceTagToKeys;
};
