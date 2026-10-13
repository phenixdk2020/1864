#include "StrategySemanticZoomComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

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

    // Test flag: -Strategy1864SemanticZoom=<0 close, 1 medium, 2 operational, 3 strategic, 4 very far> forces the state.
    int32 ForcedState = -1;
    if (FParse::Value(FCommandLine::Get(), TEXT("Strategy1864SemanticZoom="), ForcedState) && ForcedState >= 0 && ForcedState <= 4)
    {
        static const EStrategySemanticZoomState Forced[5] = { EStrategySemanticZoomState::Close, EStrategySemanticZoomState::Medium,
            EStrategySemanticZoomState::Operational, EStrategySemanticZoomState::Strategic, EStrategySemanticZoomState::VeryFar };
        return Forced[ForcedState];
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
