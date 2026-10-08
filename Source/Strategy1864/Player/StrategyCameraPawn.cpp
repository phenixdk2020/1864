#include "StrategyCameraPawn.h"
#include "StrategyHUD.h"
#include "Misc/ConfigCacheIni.h"
#include "StrategyPlayerController.h"
#include "../Units/StrategyUnit.h"
#include "../Visual/StrategyInfantryVisualComponent.h"
#include "GameFramework/PlayerController.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Components/InputComponent.h"

AStrategyCameraPawn::AStrategyCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;   // the camera moves while the battle is paused

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
    MovementComponent->PrimaryComponentTick.bTickEvenWhenPaused = true;
    MovementComponent->MaxSpeed = 3000.0f;
    MovementComponent->Acceleration = 8000.0f;
    MovementComponent->Deceleration = 10000.0f;
}

namespace
{
    const TCHAR* SettingsSection = TEXT("PROJECT1864.Settings");
    float CachedKeySpeed = -1.0f;
}

float AStrategyCameraPawn::GetKeySpeedFactor()
{
    if (CachedKeySpeed < 0.0f)
    {
        CachedKeySpeed = 5.0f;   // the default: five times the prototype's speed
        if (GConfig)
        {
            GConfig->GetFloat(SettingsSection, TEXT("CameraKeySpeed"), CachedKeySpeed, GGameUserSettingsIni);
        }
        CachedKeySpeed = FMath::Clamp(CachedKeySpeed, 0.5f, 30.0f);
    }
    return CachedKeySpeed;
}

void AStrategyCameraPawn::SetKeySpeedFactor(float Factor)
{
    CachedKeySpeed = FMath::Clamp(Factor, 0.5f, 30.0f);
    if (GConfig)
    {
        GConfig->SetFloat(SettingsSection, TEXT("CameraKeySpeed"), CachedKeySpeed, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }
}

void AStrategyCameraPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // The camera runs in real time whatever the battle's speed (half speed, fast, paused).
    {
        const float Dilation = FMath::Max(0.05f, UGameplayStatics::GetGlobalTimeDilation(this));
        CustomTimeDilation = 1.0f / Dilation;
        DeltaTime = FApp::GetDeltaTime();
    }

    if (MovementComponent)
    {
        const APlayerController* PC = Cast<APlayerController>(GetController());
        const float Factor = GetKeySpeedFactor();
        MovementComponent->MaxSpeed = (PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift)) ? 9000.0f : 3000.0f) * Factor;
        MovementComponent->Acceleration = 8000.0f * Factor;
        MovementComponent->Deceleration = 10000.0f * Factor;
    }
    if (bPresetTransition && SpringArm)
    {
        SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, PresetArmLength, DeltaTime, 7.0f);
        FRotator Rotation = SpringArm->GetRelativeRotation();
        Rotation.Pitch = FMath::FInterpTo(Rotation.Pitch, PresetPitch, DeltaTime, 7.0f);
        SpringArm->SetRelativeRotation(Rotation);
        if (FMath::IsNearlyEqual(SpringArm->TargetArmLength, PresetArmLength, 1.0f) && FMath::IsNearlyEqual(Rotation.Pitch, PresetPitch, 0.1f)) bPresetTransition = false;
    }
    if (!bFollowingProjectile)
    {
        if (bFocusTransition)
        {
            const float FocusAlpha = 1.f - FMath::Exp(-8.f * DeltaTime);
            SetActorLocation(FMath::Lerp(GetActorLocation(), FocusTarget, FocusAlpha));
            if (GetActorLocation().Equals(FocusTarget, 0.1f)) bFocusTransition = false;
        }
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
                1.f,
                1.f - FMath::Exp(-ProjectileFollowSmoothing * DeltaTime)));

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
                1.f,
                1.f - FMath::Exp(-ProjectileFollowSmoothing * DeltaTime)));

        return;
    }

    StopProjectileFollow(true);
}

void AStrategyCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AStrategyCameraPawn::MoveForward).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AStrategyCameraPawn::MoveRight).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAxis(TEXT("CameraZoom"), this, &AStrategyCameraPawn::ZoomCamera).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAxis(TEXT("CameraYaw"), this, &AStrategyCameraPawn::RotateCamera).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAxis(TEXT("CameraTilt"), this, &AStrategyCameraPawn::TiltCamera).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAxis(TEXT("CameraOrbitX"), this, &AStrategyCameraPawn::MouseOrbitX).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAxis(TEXT("CameraOrbitY"), this, &AStrategyCameraPawn::MouseOrbitY).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("CameraPan"), IE_Pressed, this, &AStrategyCameraPawn::BeginCameraPan).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("CameraPan"), IE_Released, this, &AStrategyCameraPawn::EndCameraPan).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("CameraFocus"), IE_Pressed, this, &AStrategyCameraPawn::FocusSelected).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("CameraOverview"), IE_Pressed, this, &AStrategyCameraPawn::PresetOverview).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("CameraTactical"), IE_Pressed, this, &AStrategyCameraPawn::PresetTactical).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("CameraSoldiers"), IE_Pressed, this, &AStrategyCameraPawn::PresetSoldiers).bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("CameraTopDown"), IE_Pressed, this, &AStrategyCameraPawn::PresetTopDown).bExecuteWhenPaused = true;
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
    bFocusTransition = false;

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
    bFocusTransition = false;

    FVector Direction = GetActorRightVector();
    Direction.Z = 0.0f;
    Direction.Normalize();
    AddMovementInput(Direction, Value);
}

void AStrategyCameraPawn::ZoomCamera(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        if (APlayerController* HudPC = Cast<APlayerController>(GetController()))
        {
            float HudMouseX = 0.f, HudMouseY = 0.f;
            if (AStrategyHUD* CommandHUD = Cast<AStrategyHUD>(HudPC->GetHUD()))
            {
                if (HudPC->GetMousePosition(HudMouseX, HudMouseY) && CommandHUD->HandleScroll(FVector2D(HudMouseX, HudMouseY), Value)) return;
            }
        }
    }
    if (!FMath::IsNearlyZero(Value) && bFollowingProjectile)
    {
        StopProjectileFollow(true);
    }

    if (!SpringArm || FMath::IsNearlyZero(Value))
    {
        return;
    }

    bPresetTransition = false;
    const float Step = FMath::Clamp(SpringArm->TargetArmLength * 0.15f, 25.0f, ZoomStep);
    SpringArm->TargetArmLength = FMath::Clamp(
        SpringArm->TargetArmLength - (Value * Step),
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

    AddActorLocalRotation(FRotator(0.0f, Value * RotationSpeedDegrees * FApp::GetDeltaTime(), 0.0f));
}


void AStrategyCameraPawn::FocusOnWorldLocation(const FVector& WorldLocation)
{
    FVector NewLocation = GetActorLocation();
    NewLocation.X = WorldLocation.X;
    NewLocation.Y = WorldLocation.Y;
    NewLocation.Z = WorldLocation.Z + 100.0f;
    FocusTarget = NewLocation;
    bFocusTransition = true;
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

    bFocusTransition = false;
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

void AStrategyCameraPawn::TiltCamera(float Value)
{
    if (!SpringArm || FMath::IsNearlyZero(Value)) return;
    if (bFollowingProjectile) StopProjectileFollow(true);
    bPresetTransition = false;
    FRotator Rotation = SpringArm->GetRelativeRotation();
    Rotation.Pitch = FMath::Clamp(Rotation.Pitch - Value * 45.0f * FApp::GetDeltaTime(), -85.0f, -10.0f);
    SpringArm->SetRelativeRotation(Rotation);
}
void AStrategyCameraPawn::MouseOrbitX(float Value)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || FMath::IsNearlyZero(Value)) return;
    if (bFollowingProjectile) StopProjectileFollow(true);
    const AStrategyPlayerController* Commander = Cast<AStrategyPlayerController>(PC);
    if (Commander && Commander->IsRightMouseCommand()) return;   // the right button is giving an order, not panning
    if (bRightMousePan || PC->IsInputKeyDown(EKeys::RightMouseButton))
    {
        bFocusTransition = false;
        AddActorWorldOffset(-GetActorRightVector() * Value * 35.0f);
    }
    else if (PC->IsInputKeyDown(EKeys::MiddleMouseButton))
    {
        AddActorLocalRotation(FRotator(0.0f, Value * 1.5f, 0.0f));
    }
}
void AStrategyCameraPawn::MouseOrbitY(float Value)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !SpringArm || FMath::IsNearlyZero(Value)) return;
    if (bFollowingProjectile) StopProjectileFollow(true);
    if (const AStrategyPlayerController* Commander = Cast<AStrategyPlayerController>(PC)) { if (Commander->IsRightMouseCommand()) return; }
    if (bRightMousePan || PC->IsInputKeyDown(EKeys::RightMouseButton))
    {
        FVector Forward = GetActorForwardVector();
        Forward.Z = 0.0f;
        bFocusTransition = false;
        AddActorWorldOffset(-Forward.GetSafeNormal() * Value * 35.0f);
    }
    else if (PC->IsInputKeyDown(EKeys::MiddleMouseButton))
    {
        bPresetTransition = false;
        FRotator Rotation = SpringArm->GetRelativeRotation();
        Rotation.Pitch = FMath::Clamp(Rotation.Pitch + Value * 1.5f, -85.0f, -10.0f);
        SpringArm->SetRelativeRotation(Rotation);
    }
}
void AStrategyCameraPawn::BeginCameraPan() { bRightMousePan = true; }
void AStrategyCameraPawn::EndCameraPan() { bRightMousePan = false; }
void AStrategyCameraPawn::FocusSelected()
{
    if (AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetController()))
    {
        const TArray<AStrategyUnit*> Units = PC->GetSelectedUnits();
        FVector Center = FVector::ZeroVector;
        int32 Count = 0;
        for (AStrategyUnit* FocusUnit : Units)
        {
            if (!IsValid(FocusUnit)) continue;
            FVector FocusPoint = FocusUnit->GetActorLocation();
            if (const UStrategyInfantryVisualComponent* FocusVisual = FocusUnit->FindComponentByClass<UStrategyInfantryVisualComponent>())
            {
                FVector FocusCentroid;
                if (FocusVisual->GetFigureLocalCentroid(FocusCentroid)) FocusPoint = FocusUnit->GetActorTransform().TransformPosition(FocusCentroid);
            }
            Center += FocusPoint;
            ++Count;
        }
        if (Count > 0) { if (bFollowingProjectile) StopProjectileFollow(true); FocusOnWorldLocation(Center / Count); }
    }
}
void AStrategyCameraPawn::ApplyPreset(float ArmLength, float Pitch)
{
    if (bFollowingProjectile) StopProjectileFollow(true);
    FocusSelected();
    PresetArmLength = FMath::Clamp(ArmLength, MinZoom, MaxZoom);
    PresetPitch = Pitch;
    bPresetTransition = true;
}
void AStrategyCameraPawn::PresetOverview() { ApplyPreset(18000.0f, -60.0f); }
void AStrategyCameraPawn::PresetTactical() { ApplyPreset(6000.0f, -55.0f); }
void AStrategyCameraPawn::PresetSoldiers() { ApplyPreset(650.0f, -20.0f); }
void AStrategyCameraPawn::PresetTopDown() { ApplyPreset(12000.0f, -85.0f); }
