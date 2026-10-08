#include "StrategyThreatReactionComponent.h"
#include "../AI/StrategyFieldOfficerComponent.h"

#include "StrategyVisibilityComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Visual/StrategyEquipmentVisualComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Units/StrategyUnit.h"
#include "EngineUtils.h"

UStrategyThreatReactionComponent::UStrategyThreatReactionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyThreatReactionComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

void UStrategyThreatReactionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit ||
        OwnerUnit->Echelon != EStrategyEchelon::Company ||
        OwnerUnit->UnitState == EStrategyUnitState::Routed ||
        OwnerUnit->UnitState == EStrategyUnitState::Destroyed)
    {
        return;
    }

    EvaluationAccumulator += DeltaTime;
    if (EvaluationAccumulator < EvaluationIntervalSeconds)
    {
        return;
    }

    const float EvaluationDelta = EvaluationAccumulator;
    EvaluationAccumulator = 0.0f;

    AStrategyUnit* Threat = FindVisibleEnemyCavalry();
    if (Threat)
    {
        NoThreatSeconds = 0.0f;
        EnterSquare();
        return;
    }

    if (bRespondingToCavalry)
    {
        TryLeaveSquare(EvaluationDelta);
    }
}

AStrategyUnit* UStrategyThreatReactionComponent::FindVisibleEnemyCavalry() const
{
    if (!OwnerUnit || !OwnerUnit->VisibilityComponent || !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* BestThreat = nullptr;
    float BestDistanceCm = TNumericLimits<float>::Max();
    for (TActorIterator<AStrategyUnit> ThreatIt(GetWorld()); ThreatIt; ++ThreatIt)
    {
        AStrategyUnit* CavalryThreat = *ThreatIt;
        if (!IsValid(CavalryThreat) || CavalryThreat->Echelon != EStrategyEchelon::Cavalry ||
            !CavalryThreat->IsCombatEffective() || CavalryThreat->Side == EStrategySide::Neutral ||
            CavalryThreat->Side == OwnerUnit->Side)
        {
            continue;
        }
        const FVector ThreatVelocity = CavalryThreat->MovementExecutor
            ? CavalryThreat->MovementExecutor->GetExecutedVelocity()
            : CavalryThreat->GetVelocity();
        const float ThreatSpeed = ThreatVelocity.Size2D();
        if (ThreatSpeed < FMath::Max(1.0f, MinimumCavalryClosingSpeedCmPerSecond)) continue;
        const FVector ApproachDirection = ThreatVelocity.GetSafeNormal2D();

        // Select one living company on the approach ray, independently of which
        // company is evaluating. Stable identity breaks equal-distance ties.
        AStrategyUnit* ApproachedCompany = nullptr;
        float NearestAlongCm = TNumericLimits<float>::Max();
        for (TActorIterator<AStrategyUnit> CompanyIt(GetWorld()); CompanyIt; ++CompanyIt)
        {
            AStrategyUnit* ApproachCompany = *CompanyIt;
            if (!IsValid(ApproachCompany) || ApproachCompany->Echelon != EStrategyEchelon::Company ||
                ApproachCompany->Side != OwnerUnit->Side || !ApproachCompany->IsCombatEffective()) continue;
            FVector ApproachOffset = ApproachCompany->GetActorLocation() - CavalryThreat->GetActorLocation();
            ApproachOffset.Z = 0.0f;
            const float AlongCm = FVector::DotProduct(ApproachOffset, ApproachDirection);
            const float ApproachDot = FVector::DotProduct(ApproachOffset.GetSafeNormal2D(), ApproachDirection);
            if (AlongCm <= 0.0f || ApproachDot < FMath::Clamp(MinimumApproachDot, 0.0f, 1.0f) ||
                (ApproachOffset - ApproachDirection * AlongCm).Size2D() > FMath::Max(0.0f, ApproachCorridorHalfWidthCm)) continue;
            const FString ApproachKey = ApproachCompany->StableUnitId.IsNone()
                ? ApproachCompany->GetPathName() : ApproachCompany->StableUnitId.ToString();
            const FString SelectedKey = ApproachedCompany
                ? (ApproachedCompany->StableUnitId.IsNone() ? ApproachedCompany->GetPathName() : ApproachedCompany->StableUnitId.ToString())
                : FString();
            if (AlongCm < NearestAlongCm ||
                (AlongCm == NearestAlongCm && ApproachKey < SelectedKey))
            {
                NearestAlongCm = AlongCm;
                ApproachedCompany = ApproachCompany;
            }
        }
        if (ApproachedCompany != OwnerUnit) continue;
        const FVector ToCompany = OwnerUnit->GetActorLocation() - CavalryThreat->GetActorLocation();
        const float ClosingSpeed = FVector::DotProduct(ThreatVelocity, ToCompany.GetSafeNormal2D());
        if (ClosingSpeed < FMath::Max(1.0f, MinimumCavalryClosingSpeedCmPerSecond)) continue;
        const float WarningDistance = FMath::Min(FMath::Max(0.0f, CavalryThreatDistanceCm),
            ClosingSpeed * FMath::Max(0.0f, SquareFormationTimeSeconds));
        // Explicit formation orders use a shorter emergency override distance.
        const float EffectiveDistance = WarningDistance *
            (bHasPlayerFormationOrder
                ? FMath::Clamp(PlayerOrderThreatDistanceScale, 0.0f, 1.0f) : 1.0f);
        const float ThreatDistance = ToCompany.Size2D();
        if (ThreatDistance > EffectiveDistance || ThreatDistance >= BestDistanceCm ||
            !OwnerUnit->VisibilityComponent->CanDetectTarget(CavalryThreat, EffectiveDistance)) continue;
        BestDistanceCm = ThreatDistance;
        BestThreat = CavalryThreat;
    }
    return BestThreat;
}

bool UStrategyThreatReactionComponent::ReactToCavalryThreat()
{
    if (!OwnerUnit || OwnerUnit->Echelon != EStrategyEchelon::Company || !OwnerUnit->IsCombatEffective()) return false;
    if (FindVisibleEnemyCavalry())
    {
        NoThreatSeconds = 0.0f;
        EnterSquare();
    }
    return bRespondingToCavalry;
}

void UStrategyThreatReactionComponent::NotifyPlayerFormationOrder(EStrategyFormationType RequestedFormation)
{
    bHasPlayerFormationOrder = true;
    if (bRespondingToCavalry) PreThreatFormation = RequestedFormation;
}

void UStrategyThreatReactionComponent::EnterSquare()
{
    if (!OwnerUnit || !OwnerUnit->FormationComponent)
    {
        return;
    }

    if (OwnerUnit->FieldOfficerComponent) OwnerUnit->FieldOfficerComponent->LeaveAutomaticFireCover();

    if (!bRespondingToCavalry)
    {
        PreThreatFormation = OwnerUnit->FormationComponent->CurrentFormation;
    }

    // An already ordered square belongs to the player/policy, not this reaction.
    if (!bRespondingToCavalry && PreThreatFormation == EStrategyFormationType::Square) return;
    bRespondingToCavalry = true;

    if (OwnerUnit->FormationComponent->CurrentFormation != EStrategyFormationType::Square)
    {
        const bool bBayonetsAlreadyFixed = OwnerUnit->EquipmentVisualComponent &&
            OwnerUnit->EquipmentVisualComponent->bBayonetFixed;
        OwnerUnit->FormationComponent->SetFormation(EStrategyFormationType::Square);
        OwnerUnit->SetUnitState(EStrategyUnitState::Reforming);
        // Formation code gets first ownership; fallback only when it did not fix them.
        if (OwnerUnit->EquipmentVisualComponent &&
            (bBayonetsAlreadyFixed || !OwnerUnit->EquipmentVisualComponent->bBayonetFixed))
        {
            if (!OwnerUnit->EquipmentVisualComponent->bBayonetFixed)
            {
                OwnerUnit->EquipmentVisualComponent->SetBayonetFixed(true);
            }
            bOwnsSquareBayonets = true;
        }
    }
}

void UStrategyThreatReactionComponent::TryLeaveSquare(float DeltaTime)
{
    if (!OwnerUnit || !OwnerUnit->FormationComponent)
    {
        return;
    }

    NoThreatSeconds += DeltaTime;
    if (NoThreatSeconds < SquareReleaseDelaySeconds)
    {
        return;
    }

    if (OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::Square)
    {
        OwnerUnit->FormationComponent->SetFormation(PreThreatFormation);
    }

    if (bOwnsSquareBayonets && OwnerUnit->EquipmentVisualComponent &&
        OwnerUnit->EquipmentVisualComponent->bBayonetFixed &&
        OwnerUnit->FormationComponent->CurrentFormation != EStrategyFormationType::Square)
    {
        OwnerUnit->EquipmentVisualComponent->SetBayonetFixed(false);
    }
    bOwnsSquareBayonets = false;
    bRespondingToCavalry = false;
    NoThreatSeconds = 0.0f;

    // FormationTransition owns the return transition and its Reforming state.
}
