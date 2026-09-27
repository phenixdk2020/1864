#include "Campaign1851Camera.h"

#include "Camera/CameraComponent.h"
#include "Campaign1851Map.h"

ACampaign1851Camera::ACampaign1851Camera()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SetRootComponent(Camera);
	Camera->SetFieldOfView(50.f);
	Camera->bConstrainAspectRatio = false;
}

void ACampaign1851Camera::Init(const FVector& InTarget, float InDistanceKm, const FVector2D& InHalfExtentUnits)
{
	// Unreal's field of view is horizontal; 50 deg here frames like Unity's 30 deg vertical on 16:9.
	Camera->SetFieldOfView(50.f);
	Target = HomeTarget = InTarget;
	Distance = TargetDistance = HomeDistance = InDistanceKm;
	HalfExtent = InHalfExtentUnits;
	Apply();
}

void ACampaign1851Camera::SetView(const FVector& InTarget, float InDistanceKm, float InYaw)
{
	Target = InTarget;
	Distance = TargetDistance = FMath::Clamp(InDistanceKm, MinDistanceKm, MaxDistanceKm);
	Yaw = InYaw;
	Apply();
}

float ACampaign1851Camera::GetPitch() const
{
	const float T = FMath::GetMappedRangeValueClamped(FVector2D(MinDistanceKm, MaxDistanceKm), FVector2D(0.f, 1.f), Distance);
	return FMath::Lerp(PitchNear, PitchFar, FMath::Pow(T, 0.55f));
}

void ACampaign1851Camera::Pan(const FVector2D& Input, float DeltaSeconds)
{
	if (Input.IsNearlyZero())
	{
		return;
	}
	// North is -Y in Unreal; yaw turns the view clockwise.
	const FRotator Heading(0.f, -90.f + Yaw, 0.f);
	const FVector Forward = Heading.RotateVector(FVector::ForwardVector);
	const FVector Right = Heading.RotateVector(FVector::RightVector);
	Target += (Forward * Input.Y + Right * Input.X) * Distance * ACampaign1851Map::KmToUnits * 0.8 * DeltaSeconds;
}

void ACampaign1851Camera::PanByWorld(const FVector& Delta)
{
	Target += FVector(Delta.X, Delta.Y, 0.0);
}

void ACampaign1851Camera::Zoom(float Notches, const FVector* Focus)
{
	const float Before = TargetDistance;
	TargetDistance = FMath::Clamp(TargetDistance * FMath::Pow(0.85f, Notches), MinDistanceKm, MaxDistanceKm);
	if (Focus)
	{
		const FVector ToFocus = FVector(Focus->X - Target.X, Focus->Y - Target.Y, 0.0);
		Target += ToFocus * (1.f - TargetDistance / Before);
	}
}

void ACampaign1851Camera::Rotate(float Direction, float DeltaSeconds)
{
	Yaw += Direction * 60.f * DeltaSeconds;
}

void ACampaign1851Camera::ResetView()
{
	Target = HomeTarget;
	TargetDistance = HomeDistance;
	Yaw = 0.f;
}

void ACampaign1851Camera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Distance = FMath::Lerp(Distance, TargetDistance, 1.f - FMath::Exp(-10.f * DeltaSeconds));
	Apply();
}

void ACampaign1851Camera::Apply()
{
	Target.X = FMath::Clamp(Target.X, -HalfExtent.X, HalfExtent.X);
	Target.Y = FMath::Clamp(Target.Y, -HalfExtent.Y, HalfExtent.Y);
	const FRotator Rot(-GetPitch(), -90.f + Yaw, 0.f);
	SetActorLocationAndRotation(Target - Rot.Vector() * Distance * ACampaign1851Map::KmToUnits, Rot);
}
