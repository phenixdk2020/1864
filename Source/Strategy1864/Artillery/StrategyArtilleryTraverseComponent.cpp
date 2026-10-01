#include "StrategyArtilleryTraverseComponent.h"

#include "StrategyArtilleryBatteryUnit.h"
#include "StrategyArtilleryDeploymentComponent.h"

UStrategyArtilleryTraverseComponent::UStrategyArtilleryTraverseComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyArtilleryTraverseComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerBattery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
}

void UStrategyArtilleryTraverseComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bTraversing ||
        !OwnerBattery ||
        !OwnerBattery->DeploymentComponent ||
        !OwnerBattery->DeploymentComponent->IsDeployed())
    {
        bTraversing = false;
        return;
    }

    const float CurrentYaw = OwnerBattery->GetActorRotation().Yaw;
    const float Remaining =
        FMath::Abs(
            FMath::FindDeltaAngleDegrees(
                CurrentYaw,
                DesiredFacingYaw));

    if (Remaining <= FacingToleranceDegrees)
    {
        FRotator Rotation = OwnerBattery->GetActorRotation();
        Rotation.Yaw = DesiredFacingYaw;
        OwnerBattery->SetActorRotation(Rotation);
        bTraversing = false;
        return;
    }

    FRotator Desired = OwnerBattery->GetActorRotation();
    Desired.Yaw = DesiredFacingYaw;

    OwnerBattery->SetActorRotation(
        FMath::RInterpConstantTo(
            OwnerBattery->GetActorRotation(),
            Desired,
            DeltaTime,
            TraverseSpeedDegreesPerSecond));
}

bool UStrategyArtilleryTraverseComponent::RequestTraverseToward(
    const FVector& WorldLocation)
{
    if (!OwnerBattery ||
        !OwnerBattery->DeploymentComponent ||
        !OwnerBattery->DeploymentComponent->IsDeployed())
    {
        return false;
    }

    FVector ToTarget =
        WorldLocation - OwnerBattery->GetActorLocation();
    ToTarget.Z = 0.0f;

    if (ToTarget.IsNearlyZero())
    {
        return false;
    }

    DesiredFacingYaw = ToTarget.Rotation().Yaw;
    bTraversing = true;
    return true;
}

bool UStrategyArtilleryTraverseComponent::IsLocationInsideTraverseArc(
    const FVector& WorldLocation) const
{
    if (!OwnerBattery)
    {
        return false;
    }

    FVector ToTarget =
        WorldLocation - OwnerBattery->GetActorLocation();
    ToTarget.Z = 0.0f;

    if (ToTarget.IsNearlyZero())
    {
        return true;
    }

    const float TargetYaw = ToTarget.Rotation().Yaw;
    const float Error =
        FMath::Abs(
            FMath::FindDeltaAngleDegrees(
                OwnerBattery->GetActorRotation().Yaw,
                TargetYaw));

    return Error <=
        FMath::Max(
            1.0f,
            OwnerBattery->GunProfile.TraverseHalfAngleDegrees);
}
