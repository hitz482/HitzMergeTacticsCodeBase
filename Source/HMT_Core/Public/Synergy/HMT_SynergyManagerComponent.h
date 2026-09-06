#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Synergy/HMT_TraitDefinition.h"
#include "HMT_SynergyManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHMT_OnTraitActivated, FGameplayTag, TraitTag, int32, NewBreakpointIndex);

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_ActiveTraitBreakpoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "HitzMergeTactics|Synergy")
	FGameplayTag TraitTag;

	UPROPERTY(BlueprintReadOnly, Category = "HitzMergeTactics|Synergy")
	int32 ActiveBreakpointIndex = INDEX_NONE;
};

UCLASS(Blueprintable, ClassGroup = (HitzMergeTactics), meta = (BlueprintSpawnableComponent))
class HMT_CORE_API UHMT_SynergyManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHMT_SynergyManagerComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "HitzMergeTactics|Synergy")
	TArray<TObjectPtr<UHMT_TraitDefinition>> RegisteredTraits;

	UPROPERTY(BlueprintAssignable, Category = "HitzMergeTactics|Synergy")
	FHMT_OnTraitActivated OnTraitActivated;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	void RecomputeSynergies();

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	int32 GetActiveBreakpointIndex(FGameplayTag TraitTag) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	TArray<TSubclassOf<UObject>> GetActiveTraitEffectClasses() const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Synergy")
	TArray<UHMT_TraitDefinition*> GetRegisteredTraitsList() const
	{
		TArray<UHMT_TraitDefinition*> Result;
		Result.Reserve(RegisteredTraits.Num());
		for (const TObjectPtr<UHMT_TraitDefinition>& Trait : RegisteredTraits)
		{
			Result.Add(Trait);
		}
		return Result;
	}

protected:
	UPROPERTY(Replicated)
	TArray<FHMT_ActiveTraitBreakpoint> ActiveBreakpoints;
};
