#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HMT_ProjectileActor.generated.h"

UCLASS(Blueprintable)
class HMT_CORE_API AHMT_ProjectileActor : public AActor
{
	GENERATED_BODY()

public:
	AHMT_ProjectileActor();

	UFUNCTION(BlueprintCallable, Category = "HitzMergeTactics|Combat")
	void Launch(FVector Start, FVector End, float FlightTime);

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintNativeEvent, Category = "HitzMergeTactics|Combat")
	FVector ComputeLocationAtAlpha(float Alpha) const;
	virtual FVector ComputeLocationAtAlpha_Implementation(float Alpha) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "HitzMergeTactics|Combat")
	void OnImpact();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "HitzMergeTactics|Combat")
	FVector LaunchStart = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "HitzMergeTactics|Combat")
	FVector LaunchEnd = FVector::ZeroVector;

private:
	float FlightDuration = 0.f;
	float FlightElapsed = 0.f;
	bool bInFlight = false;
};
