#include "StrategySemanticZoomComponent.h"

#include "../Player/StrategyCameraPawn.h"
#include "../Units/StrategyUnit.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"

UStrategySemanticZoomComponent::UStrategySemanticZoomComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategySemanticZoomComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    ApplyZoomState(ResolveZoomState());
}

void UStrategySemanticZoomComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    EvaluationAccumulator += DeltaTime;
    if (EvaluationAccumulator < EvaluationIntervalSeconds)
    {
        return;
    }

    EvaluationAccumulator = 0.0f;
    ApplyZoomState(ResolveZoomState());
}

EStrategySemanticZoomState UStrategySemanticZoomComponent::ResolveZoomState() const
{
    if (!GetWorld())
    {
        return EStrategySemanticZoomState::Close;
    }

    const APlayerController* PC = GetWorld()->GetFirstPlayerController();
    const AStrategyCameraPawn* CameraPawn =
        PC ? Cast<AStrategyCameraPawn>(PC->GetPawn()) : nullptr;

    const float ZoomCm =
        CameraPawn && CameraPawn->SpringArm
        ? CameraPawn->SpringArm->TargetArmLength
        : 0.0f;

    if (ZoomCm >= VeryFarThresholdCm)
    {
        return EStrategySemanticZoomState::VeryFar;
    }

    if (ZoomCm >= StrategicThresholdCm)
    {
        return EStrategySemanticZoomState::Strategic;
    }

    if (ZoomCm >= OperationalThresholdCm)
    {
        return EStrategySemanticZoomState::Operational;
    }

    if (ZoomCm >= MediumThresholdCm)
    {
        return EStrategySemanticZoomState::Medium;
    }

    return EStrategySemanticZoomState::Close;
}

void UStrategySemanticZoomComponent::ApplyZoomState(
    EStrategySemanticZoomState NewState)
{
    if (!OwnerUnit || OwnerUnit->SemanticZoomState == NewState)
    {
        return;
    }

    OwnerUnit->SetSemanticZoomState(NewState);
}
