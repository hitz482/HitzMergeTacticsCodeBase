#include "Combat/HMT_ProjectileActor.h"

AHMT_ProjectileActor::AHMT_ProjectileActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetReplicates(false);
	SetActorTickEnabled(false);
}

void AHMT_ProjectileActor::Launch(FVector Start, FVector End, float FlightTime)
{
	LaunchStart = Start;
	LaunchEnd = End;
	FlightDuration = FMath::Max(FlightTime, 0.f);
	FlightElapsed = 0.f;
	bInFlight = true;

	SetActorLocation(ComputeLocationAtAlpha(0.f));
	SetActorTickEnabled(true);

	if (FlightDuration <= 0.f)
	{
		Tick(0.f);
	}
}

void AHMT_ProjectileActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bInFlight)
	{
		return;
	}

	FlightElapsed += DeltaSeconds;
	const float Alpha = FlightDuration > 0.f ? FMath::Clamp(FlightElapsed / FlightDuration, 0.f, 1.f) : 1.f;
	SetActorLocation(ComputeLocationAtAlpha(Alpha));

	if (Alpha >= 1.f)
	{
		bInFlight = false;
		SetActorTickEnabled(false);
		OnImpact();
		Destroy();
	}
}

FVector AHMT_ProjectileActor::ComputeLocationAtAlpha_Implementation(float Alpha) const
{
	return FMath::Lerp(LaunchStart, LaunchEnd, Alpha);
}
