#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Units/HMT_UnitDefinition.h"
#include "HMT_UnitActor.generated.h"

class UHMT_StatComponent;
class UHMT_UnitInstanceComponent;

UCLASS(Blueprintable)
class HMT_CORE_API AHMT_UnitActor : public AActor
{
	GENERATED_BODY()

public:
	AHMT_UnitActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Unit")
	TObjectPtr<UHMT_StatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Unit")
	TObjectPtr<UHMT_UnitInstanceComponent> UnitInstanceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitzMergeTactics|Unit")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Unit")
	void InitializeFromDefinition(UHMT_UnitDefinition* Definition, int32 StarLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "HitzMergeTactics|Unit")
	void OnStarLevelChanged(int32 OldStarLevel, int32 NewStarLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "HitzMergeTactics|Unit")
	void OnUnitDefinitionApplied();

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Unit")
	FTransform GetSocketOrRootTransform(FName SocketName) const;

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Unit")
	FVector GetSocketOrRootLocation(FName SocketName) const { return GetSocketOrRootTransform(SocketName).GetLocation(); }

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Unit")
	FText GetStatSummaryText();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_UnitDefinition)
	TObjectPtr<UHMT_UnitDefinition> ReplicatedUnitDefinition;

	UFUNCTION()
	void OnRep_UnitDefinition();

	void ApplyMeshFromDefinition();

private:
	UFUNCTION()
	void HandleStarLevelChanged(int32 OldStarLevel, int32 NewStarLevel);
};
