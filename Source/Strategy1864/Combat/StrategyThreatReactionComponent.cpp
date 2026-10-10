#include "StrategyThreatReactionComponent.h"
#include "../AI/StrategyFieldOfficerComponent.h"
#include "../AI/StrategyDecisionLog.h"

#include "StrategyContactComponent.h"
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
    STRATEGY1864_DECISION(OwnerUnit, TEXT("Kavalerireaktion"), Threat ? TEXT("Carré") : (bRespondingToCavalry ? TEXT("Afvent/opløs carré") : TEXT("Mission")),
        Threat ? TEXT("Synligt rytteri nærmer sig den valgte enhed inden for varslingstiden") : TEXT("Ingen kvalificeret synlig kavaleritrussel"),
        FString::Printf(TEXT("threat=%s responding=%d noThreatSeconds=%.2f releaseSeconds=%.2f warningMaxCm=%.1f formationSeconds=%.1f playerFormation=%d"),
            Threat ? *Threat->StableUnitId.ToString() : TEXT("-"), bRespondingToCavalry, NoThreatSeconds,
            SquareReleaseDelaySeconds, CavalryThreatDistanceCm, SquareFormationTimeSeconds, bHasPlayerFormationOrder),
        Threat ? TEXT("opløs carré/fortsæt normal formation: afvist mens truslen er aktiv") :
            (bRespondingToCavalry && NoThreatSeconds + EvaluationDelta < SquareReleaseDelaySeconds ?
                TEXT("øjeblikkelig opløsning: afvist af release-hysterese; ny carré: ingen kvalificeret trussel") : TEXT("ny carré: ingen kvalificeret trussel")));
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
    if (!OwnerUnit || !OwnerUnit->ContactComponent || !OwnerUnit->VisibilityComponent || !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* BestThreat = nullptr;
    float BestDistanceCm = TNumericLimits<float>::Max();
    for (const FStrategyContactRecord& CavalryContact : OwnerUnit->ContactComponent->GetKnownContacts())
    {
        if (!CavalryContact.bCurrentlyVisible || CavalryContact.Confidence <= 0.0f) continue;
        AStrategyUnit* CavalryThreat = CavalryContact.ObservedUnit.Get();
        if (!IsValid(CavalryThreat) || CavalryThreat->Echelon != EStrategyEchelon::Cavalry ||
            !CavalryThreat->IsCombatEffective() || CavalryThreat->Side == EStrategySide::Neutral ||
            CavalryThreat->Side == OwnerUnit->Side)
        {
            continue;
        }
        const FVector ThreatVelocity = CavalryContact.ObservedVelocity;
        const float ThreatSpeed = ThreatVelocity.Size2D();
        if (ThreatSpeed < FMath::Max(1.0f, MinimumCavalryClosingSpeedCmPerSecond))
        {
            STRATEGY1864_DECISION(OwnerUnit, TEXT("Kavalerikandidat"), TEXT("Afvis"), TEXT("For lav fart"),
                FString::Printf(TEXT("candidate=%s speed=%.1f minSpeed=%.1f"), *CavalryThreat->StableUnitId.ToString(), ThreatSpeed, MinimumCavalryClosingSpeedCmPerSecond), TEXT("FORM_CARRE"));
            continue;
        }
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
            FVector ApproachOffset = ApproachCompany->GetActorLocation() - CavalryContact.LastKnownPosition;
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
        if (ApproachedCompany != OwnerUnit)
        {
            STRATEGY1864_DECISION(OwnerUnit, TEXT("Kavalerikandidat"), TEXT("Afvis"), TEXT("Angrebskorridoren peger på et andet kompagni eller intet kompagni"),
                FString::Printf(TEXT("candidate=%s selected=%s speed=%.1f minDot=%.2f corridorHalfWidthCm=%.1f"),
                    *CavalryThreat->StableUnitId.ToString(), ApproachedCompany ? *ApproachedCompany->StableUnitId.ToString() : TEXT("-"),
                    ThreatSpeed, MinimumApproachDot, ApproachCorridorHalfWidthCm), TEXT("FORM_CARRE for denne enhed"));
            continue;
        }
        const FVector ToCompany = OwnerUnit->GetActorLocation() - CavalryContact.LastKnownPosition;
        const float ClosingSpeed = FVector::DotProduct(ThreatVelocity, ToCompany.GetSafeNormal2D());
        if (ClosingSpeed < FMath::Max(1.0f, MinimumCavalryClosingSpeedCmPerSecond))
        {
            STRATEGY1864_DECISION(OwnerUnit, TEXT("Kavalerikandidat"), TEXT("Afvis"), TEXT("For lav lukningsfart"),
                FString::Printf(TEXT("candidate=%s closingSpeed=%.1f minSpeed=%.1f"), *CavalryThreat->StableUnitId.ToString(), ClosingSpeed, MinimumCavalryClosingSpeedCmPerSecond), TEXT("FORM_CARRE"));
            continue;
        }
        const float WarningDistance = FMath::Min(FMath::Max(0.0f, CavalryThreatDistanceCm),
            ClosingSpeed * FMath::Max(0.0f, SquareFormationTimeSeconds));
        // Explicit formation orders use a shorter emergency override distance.
        const float EffectiveDistance = WarningDistance *
            (bHasPlayerFormationOrder
                ? FMath::Clamp(PlayerOrderThreatDistanceScale, 0.0f, 1.0f) : 1.0f);
        const float ThreatDistance = ToCompany.Size2D();
        // Keep the existing short-range forest/smoke visibility gate after contact selection.
        if (ThreatDistance > EffectiveDistance || ThreatDistance >= BestDistanceCm ||
            !OwnerUnit->VisibilityComponent->CanDetectTarget(CavalryThreat, EffectiveDistance))
        {
            STRATEGY1864_DECISION(OwnerUnit, TEXT("Kavalerikandidat"), TEXT("Afvis"),
                ThreatDistance > EffectiveDistance ? TEXT("Uden for varsling") :
                    (ThreatDistance >= BestDistanceCm ? TEXT("En nærmere trussel er valgt") : TEXT("Ikke synlig")),
                FString::Printf(TEXT("candidate=%s distanceCm=%.1f warningCm=%.1f closingSpeed=%.1f"),
                    *CavalryThreat->StableUnitId.ToString(), ThreatDistance, EffectiveDistance, ClosingSpeed), TEXT("FORM_CARRE fra denne kandidat"));
            continue;
        }
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
        STRATEGY1864_DECISION(OwnerUnit, TEXT("Kavalerireaktion"), TEXT("Carré"), TEXT("Nødreaktion før automatisk udfoldning"),
            FString::Printf(TEXT("responding=%d"), bRespondingToCavalry), TEXT("automatisk kolonne/linje: kavalerireaktion har forrang"));
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
    if (!bRespondingToCavalry && PreThreatFormation == EStrategyFormationType::Square)
    {
        STRATEGY1864_DECISION(OwnerUnit, TEXT("Kavalerireaktion"), TEXT("Bevar beordret carré"), TEXT("Formation tilhører spiller/policy, ikke denne reaktion"),
            TEXT("preThreatFormation=Square responding=0"), TEXT("gem/opløs reaktionsformation: ikke reaktionens ejerskab"));
        return;
    }
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
    STRATEGY1864_DECISION(OwnerUnit, TEXT("Kavalerireaktion"), TEXT("Genopret tidligere formation"), TEXT("Kavaleritrussel væk længe nok; formationsovergang ejer reformeringen"),
        FString::Printf(TEXT("noThreatSeconds=%.2f releaseSeconds=%.2f restoredFormation=%d"), NoThreatSeconds, SquareReleaseDelaySeconds, int32(PreThreatFormation)),
        TEXT("fortsat reaktions-carré: release opfyldt; ny mission: den eksisterende ordre bevares"));
    NoThreatSeconds = 0.0f;

    // FormationTransition owns the return transition and its Reforming state.
}
