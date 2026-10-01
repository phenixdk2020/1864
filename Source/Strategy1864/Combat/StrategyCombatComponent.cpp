#include "StrategyCombatComponent.h"

#include "StrategyFireControlComponent.h"
#include "StrategyFireControlTypes.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "StrategyConditionComponent.h"
#include "StrategyFireDisciplineComponent.h"
#include "StrategyStanceComponent.h"
#include "StrategyDirectionalCoverComponent.h"
#include "StrategySmokeField.h"
#include "StrategyVisibilityComponent.h"
#include "StrategySkirmisherComponent.h"
#include "StrategyDetachmentComponent.h"
#include "StrategyFireDrillComponent.h"
#include "../AI/StrategyNCOComponent.h"
#include "../Engineering/StrategyPositionOccupancyComponent.h"
#include "../AI/StrategyOfficerProfileComponent.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryDamageComponent.h"
#include "../Terrain/StrategyTerrainAwarenessComponent.h"
#include "../Logistics/StrategySupplyWagonUnit.h"
#include "EngineUtils.h"

UStrategyCombatComponent::UStrategyCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerUnit = Cast<AStrategyUnit>(GetOwner());

    const int32 Seed =
        OwnerUnit
        ? static_cast<int32>(GetTypeHash(OwnerUnit->StableUnitId))
        : GetUniqueID();

    RandomStream.Initialize(Seed);
}

void UStrategyCombatComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit || !OwnerUnit->IsCombatEffective())
    {
        return;
    }

    if (UnderFireRemainingSeconds > 0.0f)
    {
        UnderFireRemainingSeconds = FMath::Max(
            0.0f,
            UnderFireRemainingSeconds - DeltaTime);

        const bool bMovementStillPaused =
            OwnerUnit->MovementExecutor &&
            OwnerUnit->MovementExecutor->IsTemporarilyPaused();

        if (UnderFireRemainingSeconds <= 0.0f &&
            !bMovementStillPaused &&
            OwnerUnit->UnitState == EStrategyUnitState::UnderFire)
        {
            OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
        }
    }

    if (ReloadRemainingSeconds > 0.0f)
    {
        ReloadRemainingSeconds = FMath::Max(
            0.0f,
            ReloadRemainingSeconds - DeltaTime);
        return;
    }

    if (AmmunitionRounds <= 0)
    {
        bOutOfAmmo = true;
        return;
    }

    bOutOfAmmo = false;

    if (OwnerUnit->UnitState == EStrategyUnitState::Reforming ||
        (OwnerUnit->FireDisciplineComponent &&
         !OwnerUnit->FireDisciplineComponent->AllowsAutomaticFire()))
    {
        return;
    }

    AStrategyUnit* Target = FindBestTarget();
    if (Target)
    {
        TryFireAt(Target);
    }
}

bool UStrategyCombatComponent::TryFireAt(AStrategyUnit* Target)
{
    if (!OwnerUnit ||
        !OwnerUnit->FireControlComponent ||
        !OwnerUnit->FireControlComponent->CanEngageTarget(Target) ||
        OwnerUnit->UnitState == EStrategyUnitState::Reforming ||
        ReloadRemainingSeconds > 0.0f ||
        AmmunitionRounds <= 0)
    {
        return false;
    }

    const float DistanceCm = FVector::Dist2D(
        OwnerUnit->GetActorLocation(),
        Target->GetActorLocation());

    const float ActiveRangeCm =
        OwnerUnit->FireControlComponent
        ? OwnerUnit->FireControlComponent->GetActiveRangeCm()
        : 1.0f;

    const int32 ParentFiringStrength =
        OwnerUnit->DetachmentComponent
        ? OwnerUnit->DetachmentComponent->GetAvailableParentStrength()
        : FMath::Max(0, OwnerUnit->CurrentStrength);

    int32 ShotCount =
        OwnerUnit->FireDisciplineComponent
        ? OwnerUnit->FireDisciplineComponent->CalculateShotBudget(
            ParentFiringStrength,
            MaxShotsPerVolley,
            AmmunitionRounds,
            DistanceCm,
            ActiveRangeCm)
        : FMath::Min3(
            ParentFiringStrength,
            MaxShotsPerVolley,
            AmmunitionRounds);

    if (OwnerUnit->FireDrillComponent)
    {
        const EStrategyStance CurrentStance =
            OwnerUnit->StanceComponent
            ? OwnerUnit->StanceComponent->Stance
            : EStrategyStance::Standing;

        ShotCount =
            FMath::Clamp(
                FMath::RoundToInt(
                    static_cast<float>(ShotCount) *
                    OwnerUnit->FireDrillComponent
                        ->GetEligibleFiringFraction(CurrentStance)),
                0,
                AmmunitionRounds);
    }

    if (OwnerUnit->SkirmisherComponent)
    {
        ShotCount =
            FMath::Clamp(
                FMath::RoundToInt(
                    static_cast<float>(ShotCount) *
                    OwnerUnit->SkirmisherComponent->GetVolleyDensityMultiplier()),
                0,
                AmmunitionRounds);
    }

    if (ShotCount <= 0)
    {
        return false;
    }

    AmmunitionRounds -= ShotCount;
    bOutOfAmmo = AmmunitionRounds <= 0;

    const int32 Hits = ResolveHits(
        ShotCount,
        DistanceCm,
        Target);

    if (Hits > 0)
    {
        if (AStrategyArtilleryBatteryUnit* BatteryTarget =
            Cast<AStrategyArtilleryBatteryUnit>(Target))
        {
            if (BatteryTarget->ArtilleryDamageComponent)
            {
                BatteryTarget->ArtilleryDamageComponent->ApplyIncomingHits(
                    Hits,
                    false);
            }
        }
        else if (AStrategySupplyWagonUnit* SupplyTarget =
            Cast<AStrategySupplyWagonUnit>(Target))
        {
            const int32 DriverLoss =
                FMath::Max(0, FMath::RoundToInt(Hits * 0.45f));

            const int32 HorseLoss =
                FMath::Max(0, FMath::RoundToInt(Hits * 0.35f));

            SupplyTarget->ApplySupplyDamage(
                DriverLoss,
                HorseLoss,
                static_cast<float>(Hits) * 1.5f,
                FMath::Clamp(
                    static_cast<float>(Hits) * 0.0025f,
                    0.0f,
                    0.12f));
        }
        else
        {
            Target->ApplyStrengthLoss(Hits);
        }
    }

    if (Target->CombatComponent)
    {
        Target->CombatComponent->NotifyIncomingVolley(Hits);
    }

    const float ReloadMultiplier =
        OwnerUnit->FireDisciplineComponent
        ? OwnerUnit->FireDisciplineComponent->GetReloadMultiplier()
        : 1.0f;

    const float StanceReloadMultiplier =
        OwnerUnit->StanceComponent
        ? OwnerUnit->StanceComponent->GetReloadMultiplier()
        : 1.0f;

    const EStrategyStance CurrentStance =
        OwnerUnit->StanceComponent
        ? OwnerUnit->StanceComponent->Stance
        : EStrategyStance::Standing;

    const float FireDrillReloadMultiplier =
        OwnerUnit->FireDrillComponent
        ? OwnerUnit->FireDrillComponent->GetReloadMultiplier(CurrentStance)
        : 1.0f;

    const float NCOReloadMultiplier =
        OwnerUnit->NCOComponent
        ? OwnerUnit->NCOComponent->GetReloadDisciplineMultiplier()
        : 1.0f;

    ReloadRemainingSeconds =
        ReloadSeconds *
        ReloadMultiplier *
        StanceReloadMultiplier *
        FireDrillReloadMultiplier *
        NCOReloadMultiplier;
    OnVolleyResolved.Broadcast(Target, ShotCount, Hits);

    FVector FireDirection =
        Target->GetActorLocation() - OwnerUnit->GetActorLocation();
    FireDirection.Z = 0.0f;
    FireDirection = FireDirection.GetSafeNormal();

    FVector VisualOrigin = OwnerUnit->GetActorLocation();

    if (OwnerUnit->FormationComponent &&
        OwnerUnit->FormationComponent->CurrentFormation ==
            EStrategyFormationType::Square)
    {
        const float HalfExtent =
            FMath::Max(
                300.0f,
                OwnerUnit->FormationComponent->EstimateFrontageCm(
                    FMath::Max(1, OwnerUnit->CurrentStrength)) * 0.5f);

        const FRotator SquareRotation(
            0.0f,
            OwnerUnit->GetActorRotation().Yaw,
            0.0f);

        const FVector Forward = SquareRotation.Vector();
        const FVector Right =
            FRotationMatrix(SquareRotation).GetScaledAxis(EAxis::Y);

        const float ForwardDot =
            FVector::DotProduct(FireDirection, Forward);
        const float RightDot =
            FVector::DotProduct(FireDirection, Right);

        if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
        {
            VisualOrigin +=
                Forward *
                (ForwardDot >= 0.0f ? HalfExtent : -HalfExtent);
        }
        else
        {
            VisualOrigin +=
                Right *
                (RightDot >= 0.0f ? HalfExtent : -HalfExtent);
        }
    }

    OnVolleyVisualEvent.Broadcast(
        VisualOrigin,
        FireDirection,
        ShotCount,
        Hits);

    // Advance only after presentation has received the just-fired rank.
    // This keeps FireByRank deterministic and lets future 1:1 visuals query
    // which rank actually produced the current volley.
    if (OwnerUnit->FireDrillComponent)
    {
        OwnerUnit->FireDrillComponent->AdvanceFireByRankCycle();
    }

    if (GetWorld())
    {
        if (AStrategySmokeField* Smoke =
            GetWorld()->SpawnActor<AStrategySmokeField>(
                AStrategySmokeField::StaticClass(),
                VisualOrigin + FireDirection * 250.0f,
                FireDirection.Rotation()))
        {
            Smoke->InitialDensity =
                FMath::Clamp(
                    0.20f + static_cast<float>(ShotCount) / 500.0f,
                    0.20f,
                    0.65f);

            Smoke->RadiusCm =
                FMath::Clamp(
                    800.0f + static_cast<float>(ShotCount) * 4.0f,
                    800.0f,
                    1800.0f);
        }
    }

    return true;
}

AStrategyUnit* UStrategyCombatComponent::FindBestTarget() const
{
    if (!OwnerUnit || !OwnerUnit->FireControlComponent || !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* BestTarget = nullptr;
    float BestDistanceCm = TNumericLimits<float>::Max();

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;

        if (!IsValid(Candidate) ||
            Candidate == OwnerUnit ||
            !OwnerUnit->FireControlComponent->CanEngageTarget(Candidate))
        {
            continue;
        }

        const float DistanceCm = FVector::Dist2D(
            OwnerUnit->GetActorLocation(),
            Candidate->GetActorLocation());

        if (DistanceCm < BestDistanceCm)
        {
            BestDistanceCm = DistanceCm;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

int32 UStrategyCombatComponent::ResolveHits(
    int32 ShotCount,
    float DistanceCm,
    const AStrategyUnit* Target)
{
    if (!OwnerUnit || !OwnerUnit->FireControlComponent || ShotCount <= 0)
    {
        return 0;
    }

    const float ActiveRangeCm =
        FMath::Max(1.0f, OwnerUnit->FireControlComponent->GetActiveRangeCm());

    const float RangeFactor =
        FMath::Clamp(1.0f - (DistanceCm / ActiveRangeCm) * 0.55f, 0.25f, 1.0f);

    const float ConditionMultiplier =
        OwnerUnit->ConditionComponent
        ? OwnerUnit->ConditionComponent->GetAccuracyMultiplier()
        : 1.0f;

    const float FireDrillAccuracyMultiplier =
        OwnerUnit->FireDrillComponent
        ? OwnerUnit->FireDrillComponent->GetVolleyCoordinationMultiplier()
        : 1.0f;

    const float StanceTargetMultiplier =
        IsValid(Target) && Target->StanceComponent
        ? Target->StanceComponent->GetIncomingHitMultiplier()
        : 1.0f;

    const float CoverMultiplier =
        IsValid(Target) && Target->DirectionalCoverComponent
        ? Target->DirectionalCoverComponent->CalculateIncomingHitMultiplier(
            OwnerUnit->GetActorLocation())
        : 1.0f;

    const float SkirmisherTargetMultiplier =
        IsValid(Target) && Target->SkirmisherComponent
        ? Target->SkirmisherComponent->GetIncomingHitMultiplier()
        : 1.0f;

    const float OccupiedPositionMultiplier =
        IsValid(Target) && Target->PositionOccupancyComponent
        ? Target->PositionOccupancyComponent->GetIncomingHitMultiplier(
            OwnerUnit->GetActorLocation())
        : 1.0f;

    const float TerrainExposureMultiplier =
        IsValid(Target) && Target->TerrainAwarenessComponent
        ? Target->TerrainAwarenessComponent->GetIncomingHitMultiplierFrom(
            OwnerUnit->GetActorLocation())
        : 1.0f;

    const float SmokeTransmission =
        OwnerUnit->VisibilityComponent && IsValid(Target)
        ? OwnerUnit->VisibilityComponent->GetSmokeTransmissionTo(Target)
        : 1.0f;

    const float HitChance =
        FMath::Clamp(
            BaseHitChance *
            RangeFactor *
            ConditionMultiplier *
            FireDrillAccuracyMultiplier *
            StanceTargetMultiplier *
            CoverMultiplier *
            SkirmisherTargetMultiplier *
            OccupiedPositionMultiplier *
            TerrainExposureMultiplier *
            SmokeTransmission,
            0.0f,
            1.0f);

    int32 Hits = 0;

    for (int32 Index = 0; Index < ShotCount; ++Index)
    {
        if (RandomStream.FRand() < HitChance)
        {
            ++Hits;
        }
    }

    return Hits;
}


void UStrategyCombatComponent::NotifyIncomingVolley(int32 Hits)
{
    if (!OwnerUnit || !OwnerUnit->IsCombatEffective())
    {
        return;
    }

    const float StressReactionMultiplier =
        OwnerUnit->OfficerProfileComponent
        ? OwnerUnit->OfficerProfileComponent->GetStressReactionMultiplier()
        : 1.0f;

    const float EffectiveUnderFireDuration =
        UnderFireDurationSeconds * StressReactionMultiplier;

    UnderFireRemainingSeconds = FMath::Max(
        UnderFireRemainingSeconds,
        EffectiveUnderFireDuration);

    const float BaseShock =
        Hits > 0
        ? FMath::Clamp(static_cast<float>(Hits) * 0.35f, 0.5f, 8.0f)
        : 0.25f;

    const float ShockMultiplier =
        OwnerUnit->ConditionComponent
        ? OwnerUnit->ConditionComponent->GetMoraleShockMultiplier()
        : 1.0f;

    const float Shock = BaseShock * ShockMultiplier;

    OwnerUnit->Morale = FMath::Clamp(
        OwnerUnit->Morale - Shock,
        0.0f,
        100.0f);

    OwnerUnit->Cohesion = FMath::Clamp(
        OwnerUnit->Cohesion - Shock * 0.75f,
        0.0f,
        100.0f);

    EvaluateRoutState();

    if (OwnerUnit->UnitState == EStrategyUnitState::Routed)
    {
        return;
    }

    if (OwnerUnit->MovementExecutor &&
        OwnerUnit->MovementExecutor->HasMovementGoal())
    {
        OwnerUnit->MovementExecutor->PauseMovementForSeconds(
            EffectiveUnderFireDuration);
    }
    else
    {
        OwnerUnit->SetUnitState(EStrategyUnitState::UnderFire);
    }
}


void UStrategyCombatComponent::EvaluateRoutState()
{
    if (!OwnerUnit ||
        OwnerUnit->UnitState == EStrategyUnitState::Destroyed ||
        OwnerUnit->UnitState == EStrategyUnitState::Routed)
    {
        return;
    }

    const bool bShouldRout =
        OwnerUnit->Morale <= RoutMoraleThreshold ||
        OwnerUnit->Cohesion <= RoutCohesionThreshold;

    if (!bShouldRout)
    {
        return;
    }

    OwnerUnit->SetUnitState(EStrategyUnitState::Routed);

    if (OwnerUnit->MovementExecutor)
    {
        OwnerUnit->MovementExecutor->StopMovement();
    }

    if (OwnerUnit->FireControlComponent)
    {
        OwnerUnit->FireControlComponent->SetFirePolicy(EStrategyFirePolicy::Hold);
    }

    if (OwnerUnit->OrderComponent &&
        OwnerUnit->OrderComponent->GetCurrentOrder().IsValidOrder())
    {
        OwnerUnit->OrderComponent->FailExecution();
    }
}


void UStrategyCombatComponent::SetDeterministicRandomSeed(int32 Seed)
{
    RandomStream.Initialize(Seed);
}


void UStrategyCombatComponent::ResupplyAmmunition(int32 Rounds)
{
    if (Rounds <= 0)
    {
        return;
    }

    AmmunitionRounds =
        FMath::Clamp(
            AmmunitionRounds + Rounds,
            0,
            FMath::Max(1, MaxAmmunitionRounds));

    bOutOfAmmo = AmmunitionRounds <= 0;
}
