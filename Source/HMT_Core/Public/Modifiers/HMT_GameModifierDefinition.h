#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HMT_GameModifierDefinition.generated.h"

UENUM(BlueprintType)
enum class EHMT_ModifierTriggerType : uint8
{
	OnMatchStart,
	OnRoundStart,
	OnCombatStart,
	Persistent,
	OnKill
};

UCLASS(BlueprintType, Const)
class HMT_CORE_API UHMT_GameModifierDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier")
	FText ModifierName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier")
	FString Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier")
	TSoftObjectPtr<class UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier")
	EHMT_ModifierTriggerType TriggerType = EHMT_ModifierTriggerType::OnMatchStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier", meta = (MustImplement = "/Script/HMT_Core.HMT_ModifierEffect"))
	TSubclassOf<UObject> ModifierEffectClass;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
