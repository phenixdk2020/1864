#include "TacticalCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"

ATacticalCameraPawn::ATacticalCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	Arm->SetupAttachment(Root);
	Arm->bDoCollisionTest = false;
	Arm->bEnableCameraLag = true;
	Arm->CameraLagSpeed = 10.f;
	Arm->TargetArmLength = TargetArmLength;
	Arm->SetRelativeRotation(FRotator(-50.f, 0.f, 0.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Arm);
	Camera->SetFieldOfView(35.f);

	// Mild tilt-shift: focus on the look-at point, soft beyond it.
	Camera->PostProcessSettings.bOverride_DepthOfFieldFstop = true;
	Camera->PostProcessSettings.DepthOfFieldFstop = 4.f;
	Camera->PostProcessSettings.bOverride_DepthOfFieldSensorWidth = true;
	Camera->PostProcessSettings.DepthOfFieldSensorWidth = 36.f;
}

void ATacticalCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, TargetArmLength, DeltaSeconds, ZoomInterpSpeed);

	const float ZoomAlpha = FMath::GetMappedRangeValueClamped(FVector2D(MinZoom, MaxZoom), FVector2D(0.f, 1.f), Arm->TargetArmLength);
	FRotator ArmRotation = Arm->GetRelativeRotation();
	ArmRotation.Pitch = FMath::Lerp(PitchNear, PitchFar, FMath::Sqrt(ZoomAlpha));
	Arm->SetRelativeRotation(ArmRotation);

	Camera->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
	Camera->PostProcessSettings.DepthOfFieldFocalDistance = Arm->TargetArmLength;

	FollowGround();
}

void ATacticalCameraPawn::Pan(const FVector2D& Input, float DeltaSeconds)
{
	if (Input.IsNearlyZero())
	{
		return;
	}
	const FRotator Yaw(0.f, Arm->GetRelativeRotation().Yaw + GetActorRotation().Yaw, 0.f);
	const FVector Forward = Yaw.RotateVector(FVector::ForwardVector);
	const FVector Right = Yaw.RotateVector(FVector::RightVector);
	const float Speed = TargetArmLength * PanSpeedPerZoom;
	AddActorWorldOffset((Forward * Input.Y + Right * Input.X) * Speed * DeltaSeconds);
}

void ATacticalCameraPawn::Zoom(float Notches)
{
	TargetArmLength = FMath::Clamp(TargetArmLength * FMath::Pow(1.f - ZoomStepFraction, Notches), MinZoom, MaxZoom);
}

void ATacticalCameraPawn::Rotate(float Direction, float DeltaSeconds)
{
	FRotator ArmRotation = Arm->GetRelativeRotation();
	ArmRotation.Yaw += Direction * RotateSpeed * DeltaSeconds;
	Arm->SetRelativeRotation(ArmRotation);
}

void ATacticalCameraPawn::FollowGround()
{
	// Keep the pivot on the terrain so hills don't push the camera through the ground.
	FHitResult Hit;
	const FVector Location = GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CameraGround), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Location + FVector(0.f, 0.f, 50000.f), Location - FVector(0.f, 0.f, 50000.f), ECC_WorldStatic, Params))
	{
		SetActorLocation(FVector(Location.X, Location.Y, FMath::FInterpTo(Location.Z, Hit.ImpactPoint.Z, GetWorld()->GetDeltaSeconds(), 6.f)));
	}
}
