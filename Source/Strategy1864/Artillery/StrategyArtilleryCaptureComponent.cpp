#include "StrategyArtilleryCaptureComponent.h"

#include "StrategyArtilleryBatteryUnit.h"
#include "StrategyArtilleryDeploymentComponent.h"
#include "StrategyArtilleryAmmunitionComponent.h"
#include "../Units/StrategyUnit.h"
#include "EngineUtils.h"

UStrategyArtilleryCaptureComponent::UStrategyArtilleryCaptureComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyArtilleryCaptureComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerBattery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
}

void UStrategyArtilleryCaptureComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerBattery)
    {
        return;
    }

    if (bReusePending)
    {
        ReuseRemainingSeconds =
            FMath::Max(0.0f, ReuseRemainingSeconds - DeltaTime);

        if (ReuseRemainingSeconds <= 0.0f)
        {
            bReusePending = false;

            OwnerBattery->OwnershipState =
                EStrategyArtilleryOwnershipState::Operational;

            OwnerBattery->CrewStrength =
                FMath::Max(
                    OwnerBattery->CrewStrength,
                    MinimumQualifiedCrewForReuse);

            OwnerBattery->CurrentStrength =
                FMath::Max(
                    1,
                    OwnerBattery->CrewStrength +
                    OwnerBattery->DriverStrength);

            if (OwnerBattery->DeploymentComponent)
            {
                OwnerBattery->DeploymentComponent->MobilityState =
                    EStrategyArtilleryMobilityState::Deployed;
            }

            OwnerBattery->SetUnitState(EStrategyUnitState::Ready);
            OwnerBattery->RefreshDebugLabel();
        }

        return;
    }

    if (OwnerBattery->OwnershipState !=
        EStrategyArtilleryOwnershipState::Abandoned)
    {
        CaptureProgressSeconds = 0.0f;
        ActiveCaptor = nullptr;
        return;
    }

    if (HasFriendlyProtection())
    {
        CaptureProgressSeconds = 0.0f;
        ActiveCaptor = nullptr;
        return;
    }

    AStrategyUnit* Captor = FindCapturingEnemy();
    if (!IsValid(Captor))
    {
        CaptureProgressSeconds = 0.0f;
        ActiveCaptor = nullptr;
        return;
    }

    if (ActiveCaptor != Captor)
    {
        ActiveCaptor = Captor;
        CaptureProgressSeconds = 0.0f;
    }

    CaptureProgressSeconds += DeltaTime;

    if (CaptureProgressSeconds >= CaptureHoldSeconds)
    {
        CompleteCapture(Captor);
    }
}

bool UStrategyArtilleryCaptureComponent::AttemptReuse(
    int32 AvailableQualifiedCrew,
    bool bHasCompatibleAmmunition)
{
    if (!OwnerBattery ||
        OwnerBattery->OwnershipState !=
            EStrategyArtilleryOwnershipState::Captured ||
        bReusePending ||
        AvailableQualifiedCrew < MinimumQualifiedCrewForReuse ||
        !bHasCompatibleAmmunition ||
        !OwnerBattery->ArtilleryAmmunitionComponent ||
        OwnerBattery->ArtilleryAmmunitionComponent->GetTotalRounds() <= 0 ||
        OwnerBattery->GetPhysicallyAvailableGunCount() <= 0)
    {
        return false;
    }

    bReusePending = true;
    ReuseRemainingSeconds = FMath::Max(0.1f, ReusePreparationSeconds);
    OwnerBattery->SetUnitState(EStrategyUnitState::Reforming);
    return true;
}

AStrategyUnit* UStrategyArtilleryCaptureComponent::FindCapturingEnemy() const
{
    if (!OwnerBattery || !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* Best = nullptr;
    float BestDistance = CaptureRadiusCm;

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;

        if (!IsValid(Candidate) ||
            Candidate == OwnerBattery ||
            Candidate->Side == EStrategySide::Neutral ||
            Candidate->Side == OwnerBattery->Side ||
            !Candidate->IsCombatEffective())
        {
            continue;
        }

        const float Distance =
            FVector::Dist2D(
                OwnerBattery->GetActorLocation(),
                Candidate->GetActorLocation());

        if (Distance <= BestDistance)
        {
            BestDistance = Distance;
            Best = Candidate;
        }
    }

    return Best;
}

bool UStrategyArtilleryCaptureComponent::HasFriendlyProtection() const
{
    if (!OwnerBattery || !GetWorld())
    {
        return false;
    }

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        const AStrategyUnit* Candidate = *It;

        if (!IsValid(Candidate) ||
            Candidate == OwnerBattery ||
            Candidate->Side != OwnerBattery->Side ||
            !Candidate->IsCombatEffective())
        {
            continue;
        }

        if (FVector::Dist2D(
                OwnerBattery->GetActorLocation(),
                Candidate->GetActorLocation()) <= CaptureRadiusCm * 1.5f)
        {
            return true;
        }
    }

    return false;
}

void UStrategyArtilleryCaptureComponent::CompleteCapture(
    AStrategyUnit* Captor)
{
    if (!OwnerBattery || !IsValid(Captor))
    {
        return;
    }

    OwnerBattery->Side = Captor->Side;
    OwnerBattery->bPlayerControllable =
        Captor->Side == EStrategySide::Denmark ||
        Captor->Side == EStrategySide::Allied;

    OwnerBattery->OwnershipState =
        EStrategyArtilleryOwnershipState::Captured;

    OwnerBattery->CrewStrength = 0;
    OwnerBattery->DriverStrength = 0;
    OwnerBattery->CurrentStrength = 0;

    if (OwnerBattery->DeploymentComponent)
    {
        OwnerBattery->DeploymentComponent->MobilityState =
            EStrategyArtilleryMobilityState::Abandoned;
    }

    OwnerBattery->SetUnitState(EStrategyUnitState::Abandoned);
    OwnerBattery->RefreshDebugLabel();

    ActiveCaptor = nullptr;
    CaptureProgressSeconds = 0.0f;
}
