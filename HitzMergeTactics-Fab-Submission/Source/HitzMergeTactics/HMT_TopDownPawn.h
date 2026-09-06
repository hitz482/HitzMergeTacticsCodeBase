#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HMT_TopDownPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;

/**
 * Tilted-perspective board camera for the sample (the genre-standard mobile-autobattler view:
 * camera behind the player's own side, pitched down at the board, bench at the bottom of the
 * screen). WASD pans in the camera's ground plane. FrameBoard() re-anchors it on a board center —
 * called by AHMT_PlayerState once the board origin replicates (pawn possession happens
 * earlier, at PostLogin, so spawn location is just whatever PlayerStart gave it), and again on
 * spectate target switches. Sample-only — HMT_Core has no opinion on cameras.
 */
UCLASS()
class AHMT_TopDownPawn : public APawn
{
	GENERATED_BODY()

public:
	AHMT_TopDownPawn();

	UPROPERTY(VisibleAnywhere, Category = "HMT Sample")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "HMT Sample")
	TObjectPtr<UFloatingPawnMovement> Movement;

	/** Downward tilt of the view. -90 would be straight down (the old debug view); -55 matches the
	 *  reference tilted look where units have visible bodies and the board reads in perspective. */
	UPROPERTY(EditDefaultsOnly, Category = "HMT Sample")
	float CameraPitchDegrees = -55.f;

	/** Camera distance from the framed board center along the view direction. */
	UPROPERTY(EditDefaultsOnly, Category = "HMT Sample")
	float CameraDistance = 2400.f;

	/** Positions + rotates this pawn so BoardCenter fills the view from the player's side of the
	 *  board (camera sits toward -Y, where the bench row is, looking toward +Y / the enemy side). */
	void FrameBoard(const FVector& BoardCenter);

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void OnForwardPressed() { bMoveForward = true; }
	void OnForwardReleased() { bMoveForward = false; }
	void OnBackPressed() { bMoveBack = true; }
	void OnBackReleased() { bMoveBack = false; }
	void OnLeftPressed() { bMoveLeft = true; }
	void OnLeftReleased() { bMoveLeft = false; }
	void OnRightPressed() { bMoveRight = true; }
	void OnRightReleased() { bMoveRight = false; }

	bool bMoveForward = false;
	bool bMoveBack = false;
	bool bMoveLeft = false;
	bool bMoveRight = false;
};
