#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "HMT_UnitAnimSet.generated.h"

USTRUCT(BlueprintType)
struct HMT_CORE_API FHMT_UnitAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Animation")
	TObjectPtr<UAnimMontage> Idle = nullptr;

	/** Whether the Idle montage is a seamless loop. When true, presentation code that shows this unit
	 *  as a static idle (e.g. the spectate ghost projection) plays it once looping and never restarts
	 *  it, so a periodic refresh can't pop it back to frame 0. Leave true for looping idle poses; set
	 *  false for a one-shot idle that should not auto-restart. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Animation")
	bool bIdleLoops = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Animation")
	TObjectPtr<UAnimMontage> Move = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Animation")
	TObjectPtr<UAnimMontage> Attack = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Animation")
	TObjectPtr<UAnimMontage> Ability = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Animation")
	TObjectPtr<UAnimMontage> Death = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Animation")
	TObjectPtr<UAnimMontage> Stagger = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitzMergeTactics|Animation")
	TObjectPtr<UAnimMontage> Victory = nullptr;
};
