#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TacticalCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * High-angle tactical battle camera. A narrow FOV and steep pitch give the "diorama" read of
 * the reference shot. The pawn itself is moved by ATacticalPlayerController.
 */
UCLASS()
class GAME1864_API ATacticalCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ATacticalCameraPawn();

	virtual void Tick(float DeltaSeconds) override;

	/** Pan in camera-yaw space. Input is -1..1 per axis. */
	void Pan(const FVector2D& Input, float DeltaSeconds);

	/** Positive = zoom in. One wheel notch = 1. */
	void Zoom(float Notches);

	void Rotate(float Direction, float DeltaSeconds);

	/** Current camera distance, in cm. Semantic zoom (manual F29Q) will key off this. */
	float GetZoomDistance() const { return TargetArmLength; }

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (Units = "cm"))
	float MinZoom = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (Units = "cm"))
	float MaxZoom = 60000.f;

	/** Pitch when fully zoomed in / out. Close views flatten slightly so men read as figures. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float PitchNear = -38.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float PitchFar = -65.f;

	/** Pan speed scales with zoom distance so the screen moves at a steady rate. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float PanSpeedPerZoom = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float RotateSpeed = 90.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float ZoomStepFraction = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float ZoomInterpSpeed = 8.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpringArmComponent> Arm;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

private:
	void FollowGround();

	float TargetArmLength = 9000.f;
};
