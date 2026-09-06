#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Ruler/HMT_RulerDefinition.h"
#include "HMT_RulerActor.generated.h"

UENUM(BlueprintType)
enum class EHMT_RulerPose : uint8
{
	Idle,
	Victory,
	Defeat
};

UCLASS(Blueprintable)
class HMT_CORE_API AHMT_RulerActor : public AActor
{
	GENERATED_BODY()

public:
	AHMT_RulerActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Ruler")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "HitzMergeTactics|Ruler")
	TSubclassOf<class UUserWidget> OverheadWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Ruler")
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Ruler")
	void InitializeFromDefinition(UHMT_RulerDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Ruler")
	void SetHealth(int32 InCurrentHP, int32 InMaxHP);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Ruler")
	int32 GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Ruler")
	int32 GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Ruler")
	UHMT_RulerDefinition* GetRulerDefinition() const { return ReplicatedRulerDefinition; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Ruler")
	void SetPose(EHMT_RulerPose NewPose);

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Ruler")
	EHMT_RulerPose GetPose() const { return CurrentPose; }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Ruler")
	void SetLocalVisibility(bool bVisible);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_RulerDefinition)
	TObjectPtr<UHMT_RulerDefinition> ReplicatedRulerDefinition;

	UPROPERTY(ReplicatedUsing = OnRep_Pose)
	EHMT_RulerPose CurrentPose = EHMT_RulerPose::Idle;

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	int32 CurrentHP = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	int32 MaxHP = 0;

	UFUNCTION()
	void OnRep_RulerDefinition();

	UFUNCTION()
	void OnRep_Pose();

	UFUNCTION()
	void OnRep_Health();

	void ApplyMeshFromDefinition();
	void PlayPoseAnim();

	void EnsureOverheadWidgetSpawned();
};
