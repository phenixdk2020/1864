#include "StrategyCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Components/InputComponent.h"

AStrategyCameraPawn::AStrategyCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(SceneRoot);
    SpringArm->TargetArmLength = 2600.0f;
    SpringArm->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;
    SpringArm->bInheritPitch = false;
    SpringArm->bInheritRoll = false;
    SpringArm->bInheritYaw = true;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

    MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
    MovementComponent->MaxSpeed = 3000.0f;
    MovementComponent->Acceleration = 8000.0f;
    MovementComponent->Deceleration = 10000.0f;
}

void AStrategyCameraPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bFollowingProjectile)
    {
        return;
    }

    if (ProjectileFollowTarget.IsValid())
    {
        LastProjectileLocation =
            ProjectileFollowTarget->GetActorLocation();

        FVector Desired = GetActorLocation();
        Desired.X = LastProjectileLocation.X;
        Desired.Y = LastProjectileLocation.Y;
        Desired.Z = LastProjectileLocation.Z + 220.0f;

        SetActorLocation(
            FMath::VInterpTo(
                GetActorLocation(),
                Desired,
                DeltaTime,
                ProjectileFollowSmoothing));

        if (SpringArm)
        {
            SpringArm->TargetArmLength =
                FMath::FInterpTo(
                    SpringArm->TargetArmLength,
                    ProjectileFollowArmLength,
                    DeltaTime,
                    ProjectileFollowSmoothing);
        }

        ImpactHoldRemainingSeconds =
            ProjectileImpactHoldSeconds;

        return;
    }

    if (ImpactHoldRemainingSeconds > 0.0f)
    {
        ImpactHoldRemainingSeconds =
            FMath::Max(
                0.0f,
                ImpactHoldRemainingSeconds - DeltaTime);

        FVector Desired = GetActorLocation();
        Desired.X = LastProjectileLocation.X;
        Desired.Y = LastProjectileLocation.Y;

        SetActorLocation(
            FMath::VInterpTo(
                GetActorLocation(),
                Desired,
                DeltaTime,
                ProjectileFollowSmoothing));

        return;
    }

    StopProjectileFollow(true);
}

void AStrategyCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AStrategyCameraPawn::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AStrategyCameraPawn::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("CameraZoom"), this, &AStrategyCameraPawn::ZoomCamera);
    PlayerInputComponent->BindAxis(TEXT("CameraYaw"), this, &AStrategyCameraPawn::RotateCamera);
}

void AStrategyCameraPawn::MoveForward(float Value)
{
    if (!FMath::IsNearlyZero(Value) && bFollowingProjectile)
    {
        StopProjectileFollow(true);
    }

    if (FMath::IsNearlyZero(Value))
    {
        return;
    }

    FVector Direction = GetActorForwardVector();
    Direction.Z = 0.0f;
    Direction.Normalize();
    AddMovementInput(Direction, Value);
}

void AStrategyCameraPawn::MoveRight(float Value)
{
    if (!FMath::IsNearlyZero(Value) && bFollowingProjectile)
    {
        StopProjectileFollow(true);
    }

    if (FMath::IsNearlyZero(Value))
    {
        return;
    }

    FVector Direction = GetActorRightVector();
    Direction.Z = 0.0f;
    Direction.Normalize();
    AddMovementInput(Direction, Value);
}

void AStrategyCameraPawn::ZoomCamera(float Value)
{
    if (!FMath::IsNearlyZero(Value) && bFollowingProjectile)
    {
        StopProjectileFollow(true);
    }

    if (!SpringArm || FMath::IsNearlyZero(Value))
    {
        return;
    }

    SpringArm->TargetArmLength = FMath::Clamp(
        SpringArm->TargetArmLength - (Value * ZoomStep),
        MinZoom,
        MaxZoom);
}

void AStrategyCameraPawn::RotateCamera(float Value)
{
    if (!FMath::IsNearlyZero(Value) && bFollowingProjectile)
    {
        StopProjectileFollow(true);
    }

    if (FMath::IsNearlyZero(Value))
    {
        return;
    }

    AddActorLocalRotation(FRotator(0.0f, Value * RotationSpeedDegrees * GetWorld()->GetDeltaSeconds(), 0.0f));
}


void AStrategyCameraPawn::FocusOnWorldLocation(const FVector& WorldLocation)
{
    FVector NewLocation = GetActorLocation();
    NewLocation.X = WorldLocation.X;
    NewLocation.Y = WorldLocation.Y;
    SetActorLocation(NewLocation);
}


void AStrategyCameraPawn::BeginProjectileFollow(AActor* ProjectileActor)
{
    if (!IsValid(ProjectileActor))
    {
        return;
    }

    if (!bFollowingProjectile)
    {
        PreFollowLocation = GetActorLocation();
        PreFollowArmLength =
            SpringArm ? SpringArm->TargetArmLength : 2600.0f;
    }

    ProjectileFollowTarget = ProjectileActor;
    LastProjectileLocation = ProjectileActor->GetActorLocation();
    ImpactHoldRemainingSeconds = ProjectileImpactHoldSeconds;
    bFollowingProjectile = true;
}

void AStrategyCameraPawn::StopProjectileFollow(
    bool bRestorePreviousView)
{
    ProjectileFollowTarget.Reset();
    ImpactHoldRemainingSeconds = 0.0f;

    if (bFollowingProjectile && bRestorePreviousView)
    {
        SetActorLocation(PreFollowLocation);

        if (SpringArm)
        {
            SpringArm->TargetArmLength = PreFollowArmLength;
        }
    }

    bFollowingProjectile = false;
}
