#include "HMT_TopDownPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/SceneComponent.h"
#include "Components/InputComponent.h"

AHMT_TopDownPawn::AHMT_TopDownPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Perspective (not ortho): the tilted reference view depends on foreshortening — near tiles
	// bigger than far tiles — which orthographic projection by definition can't produce.
	// The camera itself carries no rotation; FrameBoard sets the ACTOR's rotation, which also
	// keeps WASD panning camera-relative for free (Tick uses the actor's own axes).
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->ProjectionMode = ECameraProjectionMode::Perspective;
	Camera->SetFieldOfView(45.f);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = 1500.f;
}

void AHMT_TopDownPawn::FrameBoard(const FVector& BoardCenter)
{
	const float PitchRad = FMath::DegreesToRadians(-CameraPitchDegrees);
	// Back out from the center along the (yaw 90, i.e. looking toward +Y) view direction: toward
	// -Y on the ground, +Z up. The player's own rows and bench are the near/bottom edge on screen.
	const FVector BackOffset(0.f, -CameraDistance * FMath::Cos(PitchRad), CameraDistance * FMath::Sin(PitchRad));
	SetActorLocation(BoardCenter + BackOffset);
	SetActorRotation(FRotator(CameraPitchDegrees, 90.f, 0.f));
}

void AHMT_TopDownPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &AHMT_TopDownPawn::OnForwardPressed);
	PlayerInputComponent->BindKey(EKeys::W, IE_Released, this, &AHMT_TopDownPawn::OnForwardReleased);
	PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &AHMT_TopDownPawn::OnBackPressed);
	PlayerInputComponent->BindKey(EKeys::S, IE_Released, this, &AHMT_TopDownPawn::OnBackReleased);
	PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &AHMT_TopDownPawn::OnLeftPressed);
	PlayerInputComponent->BindKey(EKeys::A, IE_Released, this, &AHMT_TopDownPawn::OnLeftReleased);
	PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &AHMT_TopDownPawn::OnRightPressed);
	PlayerInputComponent->BindKey(EKeys::D, IE_Released, this, &AHMT_TopDownPawn::OnRightReleased);
}

void AHMT_TopDownPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector2D Input(
		(bMoveRight ? 1.f : 0.f) - (bMoveLeft ? 1.f : 0.f),
		(bMoveForward ? 1.f : 0.f) - (bMoveBack ? 1.f : 0.f));
	if (!Input.IsNearlyZero())
	{
		Input.Normalize();

		// Pan in the camera's ground plane (actor axes flattened to Z=0), so W is always "up-screen"
		// regardless of how FrameBoard oriented the view.
		FVector Forward = GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();
		FVector Right = GetActorRightVector();
		Right.Z = 0.f;
		Right.Normalize();

		AddMovementInput(Right, Input.X);
		AddMovementInput(Forward, Input.Y);
	}
}
