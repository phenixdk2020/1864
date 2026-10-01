#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "StrategyCameraPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;
class USceneComponent;
class USpringArmComponent;

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    AStrategyCameraPawn();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Camera")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Camera")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Camera")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Camera")
    TObjectPtr<UFloatingPawnMovement> MovementComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Camera")
    float MinZoom = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Camera")
    float MaxZoom = 60000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Camera")
    float ZoomStep = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Camera")
    float RotationSpeedDegrees = 70.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Camera")
    void FocusOnWorldLocation(const FVector& WorldLocation);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Camera|Projectile")
    float ProjectileFollowArmLength = 850.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Camera|Projectile")
    float ProjectileFollowSmoothing = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Camera|Projectile")
    float ProjectileImpactHoldSeconds = 1.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Camera|Projectile")
    bool bFollowingProjectile = false;

    UFUNCTION(BlueprintCallable, Category="Strategy|Camera|Projectile")
    void BeginProjectileFollow(AActor* ProjectileActor);

    UFUNCTION(BlueprintCallable, Category="Strategy|Camera|Projectile")
    void StopProjectileFollow(bool bRestorePreviousView = true);

    UFUNCTION(BlueprintPure, Category="Strategy|Camera|Projectile")
    AActor* GetProjectileFollowTarget() const
    {
        return ProjectileFollowTarget.Get();
    }

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void ZoomCamera(float Value);
    void RotateCamera(float Value);
    void TiltCamera(float Value);
    void MouseOrbitX(float Value);
    void MouseOrbitY(float Value);
    void BeginCameraPan();
    void EndCameraPan();
    void FocusSelected();
    void PresetOverview();
    void PresetTactical();
    void PresetSoldiers();
    void PresetTopDown();
    void ApplyPreset(float ArmLength, float Pitch);
    bool bPresetTransition = false;
    bool bRightMousePan = false;
    float PresetArmLength = 18000.0f;
    float PresetPitch = -55.0f;

    TWeakObjectPtr<AActor> ProjectileFollowTarget;
    FVector PreFollowLocation = FVector::ZeroVector;
    float PreFollowArmLength = 2600.0f;
    FVector LastProjectileLocation = FVector::ZeroVector;
    float ImpactHoldRemainingSeconds = 0.0f;
};
