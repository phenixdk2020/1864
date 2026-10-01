#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyArtilleryProjectileTypes.h"
#include "StrategyArtilleryProjectilePresentation.generated.h"

class USceneComponent;

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyArtilleryProjectilePresentation : public AActor
{
    GENERATED_BODY()

public:
    AStrategyArtilleryProjectilePresentation();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Projectile")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Projectile")
    FStrategyArtilleryProjectileSpec Spec;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Projectile")
    TArray<FVector> TrajectoryPoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Projectile")
    bool bImpacted = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Projectile")
    FVector ImpactLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    bool bDrawProjectilePoint = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    bool bDrawTrajectory = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    float ProjectilePointSize = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Projectile")
    float PostImpactLifetimeSeconds = 1.75f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Projectile")
    void InitializePresentation(
        const FStrategyArtilleryProjectileSpec& InSpec,
        const TArray<FVector>& InTrajectoryPoints,
        const FVector& InFinalPoint,
        bool bInDrawTrajectory);

    UFUNCTION(BlueprintPure, Category="Strategy|Projectile")
    bool IsFollowable() const
    {
        return Spec.Style != EStrategyProjectilePresentationStyle::Canister;
    }

private:
    FVector EvaluateTrajectory(float Alpha) const;
    void DrawPresentationDebug();
    void DrawCanisterPresentation();

    float ElapsedSeconds = 0.0f;
    float PostImpactElapsedSeconds = 0.0f;
};
