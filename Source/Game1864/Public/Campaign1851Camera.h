#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Campaign1851Camera.generated.h"

class UCameraComponent;

/**
 * Strategic map camera: near top-down over all of Denmark, tilting to a three-quarter view when
 * zoomed in. Distances are in km. Yaw 0 = north up. Driven by ACampaign1851PlayerController.
 */
UCLASS()
class GAME1864_API ACampaign1851Camera : public APawn
{
	GENERATED_BODY()

public:
	ACampaign1851Camera();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Camera") float MinDistanceKm = 6.f;
	UPROPERTY(EditAnywhere, Category = "Camera") float MaxDistanceKm = 1000.f;
	UPROPERTY(EditAnywhere, Category = "Camera") float PitchNear = 32.f;
	UPROPERTY(EditAnywhere, Category = "Camera") float PitchFar = 84.f;

	void Init(const FVector& InTarget, float InDistanceKm, const FVector2D& InHalfExtentUnits);
	void SetView(const FVector& InTarget, float InDistanceKm, float InYaw);

	/** Pan in screen-aligned directions; Input -1..1 per axis. */
	void Pan(const FVector2D& Input, float DeltaSeconds);
	void PanByWorld(const FVector& Delta);
	/** Positive = in. Moves the target towards Focus so the point under the cursor stays put. */
	void Zoom(float Notches, const FVector* Focus);
	void Rotate(float Direction, float DeltaSeconds);
	void ResetView();

	float GetDistanceKm() const { return Distance; }
	const FVector& GetTarget() const { return Target; }
	float GetYaw() const { return Yaw; }
	float GetPitch() const;

private:
	void Apply();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	FVector Target = FVector::ZeroVector;
	FVector HomeTarget = FVector::ZeroVector;
	FVector2D HalfExtent = FVector2D(1.0, 1.0);
	float Distance = 960.f;
	float TargetDistance = 960.f;
	float HomeDistance = 960.f;
	float Yaw = 0.f;
};
