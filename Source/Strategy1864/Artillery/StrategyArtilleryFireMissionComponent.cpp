#include "StrategyArtilleryFireMissionComponent.h"

#include "StrategyArtilleryBatteryUnit.h"
#include "StrategyArtilleryDeploymentComponent.h"
#include "StrategyArtilleryAmmunitionComponent.h"
#include "StrategyArtilleryTraverseComponent.h"
#include "StrategyArtilleryDamageComponent.h"
#include "StrategyArtilleryProjectilePresentationComponent.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "../Combat/StrategyContactComponent.h"
#include "../Combat/StrategyVisibilityComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyDirectionalCoverComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../Combat/StrategySkirmisherComponent.h"
#include "../Engineering/StrategyPositionOccupancyComponent.h"
#include "../Combat/StrategySmokeField.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Logistics/StrategySupplyWagonUnit.h"
#include "../Terrain/StrategyTerrainAwarenessComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

UStrategyArtilleryFireMissionComponent::UStrategyArtilleryFireMissionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyArtilleryFireMissionComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerBattery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());

    const int32 Seed =
        OwnerBattery
        ? static_cast<int32>(GetTypeHash(OwnerBattery->StableUnitId)) ^ 0xA1864
        : GetUniqueID();

    RandomStream.Initialize(Seed);
}

void UStrategyArtilleryFireMissionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerBattery)
    {
        return;
    }

    if (ReloadRemainingSeconds > 0.0f)
    {
        ReloadRemainingSeconds =
            FMath::Max(0.0f, ReloadRemainingSeconds - DeltaTime);
    }

    const bool bManualMissionActive =
        FireMode == EStrategyArtilleryFireMode::ManualTarget &&
        (IsValid(ManualTarget) || bHasManualAreaTarget);

    if (bManualMissionActive)
    {
        MissionElapsedSeconds += DeltaTime;

        if (ShouldStopForMissionLimit())
        {
            CompleteManualMission(false);
            return;
        }
    }

    EvaluationAccumulator += DeltaTime;
    if (EvaluationAccumulator < EvaluationIntervalSeconds)
    {
        return;
    }

    EvaluationAccumulator = 0.0f;

    if (FireMode == EStrategyArtilleryFireMode::HoldFire ||
        ReloadRemainingSeconds > 0.0f ||
        !OwnerBattery->CanFireBattery())
    {
        return;
    }

    if (FireMode == EStrategyArtilleryFireMode::ManualTarget &&
        bHasManualAreaTarget)
    {
        if (OwnerBattery->ArtilleryTraverseComponent &&
            !OwnerBattery->ArtilleryTraverseComponent
                ->IsLocationInsideTraverseArc(ManualAreaTarget))
        {
            OwnerBattery->ArtilleryTraverseComponent
                ->RequestTraverseToward(ManualAreaTarget);
            return;
        }

        if (CanEngageLocation(ManualAreaTarget))
        {
            FireAtLocation(ManualAreaTarget);
        }

        return;
    }

    AStrategyUnit* Target =
        FireMode == EStrategyArtilleryFireMode::ManualTarget
        ? ManualTarget.Get()
        : FindBestAutoTarget();

    if (!IsValid(Target) || !Target->IsCombatEffective())
    {
        if (FireMode == EStrategyArtilleryFireMode::ManualTarget)
        {
            CompleteManualMission(!IsValid(Target));
        }
        return;
    }

    if (FireMode == EStrategyArtilleryFireMode::AutoTarget &&
        bAutoSelectAmmunition)
    {
        SelectBestAmmoForTarget(Target);
    }

    if (OwnerBattery->ArtilleryTraverseComponent &&
        !OwnerBattery->ArtilleryTraverseComponent
            ->IsLocationInsideTraverseArc(Target->GetActorLocation()))
    {
        OwnerBattery->ArtilleryTraverseComponent
            ->RequestTraverseToward(Target->GetActorLocation());
        return;
    }

    if (CanEngageTarget(Target))
    {
        FireAt(Target);
    }
}

bool UStrategyArtilleryFireMissionComponent::SetManualTarget(
    AStrategyUnit* Target,
    EStrategyOrderAuthority Authority)
{
    if (!OwnerBattery ||
        !OwnerBattery->OrderComponent ||
        !IsValid(Target) ||
        Target == OwnerBattery ||
        Target->Side == EStrategySide::Neutral ||
        Target->Side == OwnerBattery->Side)
    {
        return false;
    }

    ManualTarget = Target;
    bHasManualAreaTarget = false;
    FireMode = EStrategyArtilleryFireMode::ManualTarget;
    PreviousNonHoldMode = FireMode;
    ResetMissionCounters();

    FStrategyOrder Order;
    Order.Type = EStrategyOrderType::ArtilleryFireMission;
    Order.TargetLocation = Target->GetActorLocation();
    Order.FacingYaw =
        (Target->GetActorLocation() - OwnerBattery->GetActorLocation())
        .Rotation().Yaw;
    Order.bHasFacing = true;
    Order.Authority = Authority;

    if (!OwnerBattery->OrderComponent->SetOrder(Order))
    {
        ManualTarget = nullptr;
        return false;
    }

    OwnerBattery->OrderComponent->BeginExecution();
    return true;
}

bool UStrategyArtilleryFireMissionComponent::SetManualAreaTarget(
    const FVector& TargetLocation,
    float RadiusCm,
    EStrategyOrderAuthority Authority)
{
    if (!OwnerBattery ||
        !OwnerBattery->OrderComponent ||
        !CanObserveLocation(TargetLocation))
    {
        return false;
    }

    ManualTarget = nullptr;
    bHasManualAreaTarget = true;
    ManualAreaTarget = TargetLocation;
    ManualAreaRadiusCm = FMath::Clamp(RadiusCm, 200.0f, 10000.0f);
    FireMode = EStrategyArtilleryFireMode::ManualTarget;
    PreviousNonHoldMode = FireMode;
    ResetMissionCounters();

    FStrategyOrder Order;
    Order.Type = EStrategyOrderType::ArtilleryFireMission;
    Order.TargetLocation = TargetLocation;
    Order.FacingYaw =
        (TargetLocation - OwnerBattery->GetActorLocation()).Rotation().Yaw;
    Order.bHasFacing = true;
    Order.Authority = Authority;

    if (!OwnerBattery->OrderComponent->SetOrder(Order))
    {
        bHasManualAreaTarget = false;
        return false;
    }

    OwnerBattery->OrderComponent->BeginExecution();
    return true;
}

void UStrategyArtilleryFireMissionComponent::SetAutoTargetEnabled(
    bool bEnabled)
{
    if (!OwnerBattery)
    {
        return;
    }

    if (bEnabled)
    {
        FireMode = EStrategyArtilleryFireMode::AutoTarget;
        PreviousNonHoldMode = FireMode;
        ManualTarget = nullptr;
        bHasManualAreaTarget = false;
        bAutoSelectAmmunition = true;
    }
    else
    {
        FireMode = EStrategyArtilleryFireMode::ManualTarget;
        PreviousNonHoldMode = FireMode;
    }
}

void UStrategyArtilleryFireMissionComponent::SetHoldFire(bool bHold)
{
    if (bHold)
    {
        if (FireMode != EStrategyArtilleryFireMode::HoldFire)
        {
            PreviousNonHoldMode = FireMode;
        }

        FireMode = EStrategyArtilleryFireMode::HoldFire;

        if (OwnerBattery &&
            OwnerBattery->OrderComponent &&
            OwnerBattery->OrderComponent->GetCurrentOrder().Type ==
                EStrategyOrderType::ArtilleryFireMission)
        {
            OwnerBattery->OrderComponent->CompleteExecution();
        }
    }
    else if (FireMode == EStrategyArtilleryFireMode::HoldFire)
    {
        FireMode = PreviousNonHoldMode;

        if (OwnerBattery &&
            OwnerBattery->OrderComponent &&
            OwnerBattery->OrderComponent->GetCurrentOrder().Type ==
                EStrategyOrderType::ArtilleryFireMission)
        {
            OwnerBattery->OrderComponent->BeginExecution();
        }
    }
}

bool UStrategyArtilleryFireMissionComponent::SelectAmmo(
    EStrategyArtilleryAmmoType AmmoType)
{
    bAutoSelectAmmunition = false;

    return OwnerBattery &&
        OwnerBattery->ArtilleryAmmunitionComponent &&
        OwnerBattery->ArtilleryAmmunitionComponent->SelectAmmo(AmmoType);
}

void UStrategyArtilleryFireMissionComponent::SetMissionLimits(
    int32 MaxSalvos,
    float MaxDurationSeconds)
{
    MaxSalvosPerMission = FMath::Max(0, MaxSalvos);
    MaxMissionDurationSeconds = FMath::Max(0.0f, MaxDurationSeconds);
}

void UStrategyArtilleryFireMissionComponent::SetConserveAmmunition(
    bool bConserve,
    float ReserveFraction)
{
    bConserveAmmunition = bConserve;
    MinimumReserveFraction =
        FMath::Clamp(ReserveFraction, 0.0f, 0.95f);
}

float UStrategyArtilleryFireMissionComponent::GetMinimumRangeCm(
    EStrategyArtilleryAmmoType AmmoType) const
{
    if (!OwnerBattery)
    {
        return 0.0f;
    }

    switch (AmmoType)
    {
        case EStrategyArtilleryAmmoType::Canister:
            return 0.0f;
        case EStrategyArtilleryAmmoType::Shrapnel:
            return OwnerBattery->GunProfile.MinimumRangeCm * 1.10f;
        case EStrategyArtilleryAmmoType::Shell:
            return OwnerBattery->GunProfile.MinimumRangeCm * 1.25f;
        case EStrategyArtilleryAmmoType::RoundShot:
        default:
            return OwnerBattery->GunProfile.MinimumRangeCm;
    }
}

float UStrategyArtilleryFireMissionComponent::GetMaximumRangeCm(
    EStrategyArtilleryAmmoType AmmoType) const
{
    if (!OwnerBattery)
    {
        return 0.0f;
    }

    const float MaxRange = OwnerBattery->GunProfile.MaximumRangeCm;

    switch (AmmoType)
    {
        case EStrategyArtilleryAmmoType::Canister:
            return FMath::Min(40000.0f, MaxRange * 0.22f);
        case EStrategyArtilleryAmmoType::Shrapnel:
            return MaxRange * 0.75f;
        case EStrategyArtilleryAmmoType::Shell:
            return MaxRange * 0.95f;
        case EStrategyArtilleryAmmoType::RoundShot:
        default:
            return MaxRange;
    }
}

bool UStrategyArtilleryFireMissionComponent::CanEngageTarget(
    const AStrategyUnit* Target) const
{
    if (!OwnerBattery ||
        !OwnerBattery->CanFireBattery() ||
        FireMode == EStrategyArtilleryFireMode::HoldFire ||
        !IsValid(Target) ||
        !Target->IsCombatEffective() ||
        !OwnerBattery->ArtilleryAmmunitionComponent ||
        OwnerBattery->ArtilleryAmmunitionComponent->GetSelectedRounds() <= 0 ||
        !OwnerBattery->ArtilleryTraverseComponent ||
        !OwnerBattery->ArtilleryTraverseComponent
            ->IsLocationInsideTraverseArc(Target->GetActorLocation()) ||
        !CanObserveTarget(Target))
    {
        return false;
    }

    const int32 TotalRounds =
        OwnerBattery->ArtilleryAmmunitionComponent->GetTotalRounds();

    const int32 MaxRounds =
        FMath::Max(
            1,
            OwnerBattery->ArtilleryAmmunitionComponent->MaximumTotalRounds);

    const float ReserveFraction =
        TargetPriority == EStrategyArtilleryTargetPriority::ConserveAmmo
        ? FMath::Max(MinimumReserveFraction, 0.35f)
        : MinimumReserveFraction;

    if (FireMode == EStrategyArtilleryFireMode::AutoTarget &&
        (bConserveAmmunition ||
         TargetPriority == EStrategyArtilleryTargetPriority::ConserveAmmo) &&
        static_cast<float>(TotalRounds) / static_cast<float>(MaxRounds) <=
            ReserveFraction)
    {
        return false;
    }

    const EStrategyArtilleryAmmoType AmmoType =
        OwnerBattery->ArtilleryAmmunitionComponent->SelectedAmmo;

    const float DistanceCm =
        FVector::Dist2D(
            OwnerBattery->GetActorLocation(),
            Target->GetActorLocation());

    return DistanceCm >= GetMinimumRangeCm(AmmoType) &&
           DistanceCm <= GetMaximumRangeCm(AmmoType);
}

bool UStrategyArtilleryFireMissionComponent::CanEngageLocation(
    const FVector& TargetLocation) const
{
    if (!OwnerBattery ||
        !OwnerBattery->CanFireBattery() ||
        FireMode == EStrategyArtilleryFireMode::HoldFire ||
        !OwnerBattery->ArtilleryAmmunitionComponent ||
        OwnerBattery->ArtilleryAmmunitionComponent->GetSelectedRounds() <= 0 ||
        !OwnerBattery->ArtilleryTraverseComponent ||
        !OwnerBattery->ArtilleryTraverseComponent
            ->IsLocationInsideTraverseArc(TargetLocation) ||
        !CanObserveLocation(TargetLocation))
    {
        return false;
    }

    const EStrategyArtilleryAmmoType AmmoType =
        OwnerBattery->ArtilleryAmmunitionComponent->SelectedAmmo;

    const float DistanceCm =
        FVector::Dist2D(
            OwnerBattery->GetActorLocation(),
            TargetLocation);

    return DistanceCm >= GetMinimumRangeCm(AmmoType) &&
           DistanceCm <= GetMaximumRangeCm(AmmoType);
}

bool UStrategyArtilleryFireMissionComponent::CanObserveTarget(
    const AStrategyUnit* Target) const
{
    if (!OwnerBattery ||
        !IsValid(Target) ||
        !OwnerBattery->ContactComponent ||
        !OwnerBattery->VisibilityComponent)
    {
        return false;
    }

    return OwnerBattery->ContactComponent->HasCurrentContact(Target) &&
        OwnerBattery->VisibilityComponent->HasLineOfSightTo(Target);
}

bool UStrategyArtilleryFireMissionComponent::CanObserveLocation(
    const FVector& TargetLocation) const
{
    return OwnerBattery &&
        OwnerBattery->VisibilityComponent &&
        OwnerBattery->VisibilityComponent->HasLineOfSightToLocation(
            TargetLocation,
            50.0f);
}

bool UStrategyArtilleryFireMissionComponent::HasUsableAmmoForTarget(
    const AStrategyUnit* Target) const
{
    if (!OwnerBattery ||
        !OwnerBattery->ArtilleryAmmunitionComponent ||
        !IsValid(Target))
    {
        return false;
    }

    const float Distance =
        FVector::Dist2D(
            OwnerBattery->GetActorLocation(),
            Target->GetActorLocation());

    const EStrategyArtilleryAmmoType Types[] =
    {
        EStrategyArtilleryAmmoType::Canister,
        EStrategyArtilleryAmmoType::Shrapnel,
        EStrategyArtilleryAmmoType::Shell,
        EStrategyArtilleryAmmoType::RoundShot
    };

    for (EStrategyArtilleryAmmoType Type : Types)
    {
        if (OwnerBattery->ArtilleryAmmunitionComponent->GetRounds(Type) > 0 &&
            Distance >= GetMinimumRangeCm(Type) &&
            Distance <= GetMaximumRangeCm(Type))
        {
            return true;
        }
    }

    return false;
}

bool UStrategyArtilleryFireMissionComponent::SelectBestAmmoForTarget(
    const AStrategyUnit* Target)
{
    if (!OwnerBattery ||
        !OwnerBattery->ArtilleryAmmunitionComponent ||
        !IsValid(Target))
    {
        return false;
    }

    const float Distance =
        FVector::Dist2D(
            OwnerBattery->GetActorLocation(),
            Target->GetActorLocation());

    TArray<EStrategyArtilleryAmmoType> Preference;

    if (Distance <= GetMaximumRangeCm(EStrategyArtilleryAmmoType::Canister) &&
        (Target->Echelon == EStrategyEchelon::Company ||
         Target->Echelon == EStrategyEchelon::Cavalry))
    {
        Preference =
        {
            EStrategyArtilleryAmmoType::Canister,
            EStrategyArtilleryAmmoType::Shrapnel,
            EStrategyArtilleryAmmoType::Shell,
            EStrategyArtilleryAmmoType::RoundShot
        };
    }
    else if (Target->Echelon == EStrategyEchelon::Artillery)
    {
        Preference =
        {
            EStrategyArtilleryAmmoType::Shell,
            EStrategyArtilleryAmmoType::RoundShot,
            EStrategyArtilleryAmmoType::Shrapnel,
            EStrategyArtilleryAmmoType::Canister
        };
    }
    else
    {
        Preference =
        {
            EStrategyArtilleryAmmoType::Shrapnel,
            EStrategyArtilleryAmmoType::Shell,
            EStrategyArtilleryAmmoType::RoundShot,
            EStrategyArtilleryAmmoType::Canister
        };
    }

    for (EStrategyArtilleryAmmoType Type : Preference)
    {
        if (OwnerBattery->ArtilleryAmmunitionComponent->GetRounds(Type) > 0 &&
            Distance >= GetMinimumRangeCm(Type) &&
            Distance <= GetMaximumRangeCm(Type))
        {
            OwnerBattery->ArtilleryAmmunitionComponent->SelectedAmmo = Type;
            return true;
        }
    }

    return false;
}

bool UStrategyArtilleryFireMissionComponent::ShouldStopForMissionLimit() const
{
    return
        (MaxSalvosPerMission > 0 &&
         MissionSalvosFired >= MaxSalvosPerMission) ||
        (MaxMissionDurationSeconds > 0.0f &&
         MissionElapsedSeconds >= MaxMissionDurationSeconds);
}

void UStrategyArtilleryFireMissionComponent::ResetMissionCounters()
{
    MissionSalvosFired = 0;
    MissionElapsedSeconds = 0.0f;
}

AStrategyUnit* UStrategyArtilleryFireMissionComponent::FindBestAutoTarget() const
{
    if (!OwnerBattery || !GetWorld())
    {
        return nullptr;
    }

    AStrategyUnit* Best = nullptr;
    float BestScore = -TNumericLimits<float>::Max();

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;

        if (!IsValid(Candidate) ||
            Candidate == OwnerBattery ||
            Candidate->Side == EStrategySide::Neutral ||
            Candidate->Side == OwnerBattery->Side ||
            !Candidate->IsCombatEffective() ||
            !CanObserveTarget(Candidate))
        {
            continue;
        }

        if (!HasUsableAmmoForTarget(Candidate))
        {
            continue;
        }

        const float Score = CalculateTargetScore(Candidate);
        if (Score > BestScore)
        {
            BestScore = Score;
            Best = Candidate;
        }
    }

    return Best;
}

bool UStrategyArtilleryFireMissionComponent::FireAt(
    AStrategyUnit* Target)
{
    if (!CanEngageTarget(Target) ||
        !OwnerBattery->ArtilleryAmmunitionComponent)
    {
        return false;
    }

    const int32 OperationalGuns =
        OwnerBattery->GetOperationalGunCount();

    const int32 GunsFiring =
        FMath::Min(
            OperationalGuns,
            OwnerBattery->ArtilleryAmmunitionComponent->GetSelectedRounds());

    if (GunsFiring <= 0)
    {
        return false;
    }

    const int32 Consumed =
        OwnerBattery->ArtilleryAmmunitionComponent
            ->ConsumeSelectedRounds(GunsFiring);

    if (Consumed <= 0)
    {
        return false;
    }

    const float DistanceCm =
        FVector::Dist2D(
            OwnerBattery->GetActorLocation(),
            Target->GetActorLocation());

    TArray<FVector> ImpactLocations;
    TArray<uint8> HitFlags;
    TArray<int32> CasualtiesPerProjectile;

    const int32 Casualties =
        ResolveCasualties(
            Target,
            Consumed,
            DistanceCm,
            ImpactLocations,
            HitFlags,
            CasualtiesPerProjectile);

    const EStrategyArtilleryAmmoType AmmoType =
        OwnerBattery->ArtilleryAmmunitionComponent->SelectedAmmo;

    if (AStrategyArtilleryBatteryUnit* TargetBattery =
        Cast<AStrategyArtilleryBatteryUnit>(Target))
    {
        if (TargetBattery->ArtilleryDamageComponent && Casualties > 0)
        {
            const bool bExplosive =
                AmmoType == EStrategyArtilleryAmmoType::Shell ||
                AmmoType == EStrategyArtilleryAmmoType::Shrapnel;

            TargetBattery->ArtilleryDamageComponent
                ->ApplyIncomingHits(Casualties, bExplosive);
        }
    }
    else if (AStrategySupplyWagonUnit* SupplyTarget =
        Cast<AStrategySupplyWagonUnit>(Target))
    {
        if (Casualties > 0)
        {
            SupplyTarget->ApplySupplyDamage(
                FMath::Max(0, FMath::RoundToInt(Casualties * 0.35f)),
                FMath::Max(0, FMath::RoundToInt(Casualties * 0.30f)),
                static_cast<float>(Casualties) * 3.0f,
                FMath::Clamp(
                    static_cast<float>(Casualties) * 0.006f,
                    0.0f,
                    0.35f));
        }
    }
    else if (Casualties > 0)
    {
        Target->ApplyStrengthLoss(Casualties);
    }

    if (Target->CombatComponent)
    {
        Target->CombatComponent->NotifyIncomingVolley(Casualties);
    }

    const float CrewRatio =
        OwnerBattery->GunCount > 0
        ? static_cast<float>(OwnerBattery->GetOperationalGunCount()) /
          static_cast<float>(OwnerBattery->GunCount)
        : 0.0f;

    const float ExperienceFactor =
        FMath::Lerp(
            1.12f,
            0.88f,
            FMath::Clamp(OwnerBattery->Experience / 100.0f, 0.0f, 1.0f));

    const float FatigueFactor =
        FMath::Lerp(
            1.0f,
            1.25f,
            FMath::Clamp(OwnerBattery->Fatigue / 100.0f, 0.0f, 1.0f));

    const float CrewFactor =
        FMath::Lerp(1.35f, 1.0f, FMath::Clamp(CrewRatio, 0.0f, 1.0f));

    ReloadRemainingSeconds =
        OwnerBattery->GunProfile.ReloadSeconds *
        ExperienceFactor *
        FatigueFactor *
        CrewFactor;

    ++MissionSalvosFired;
    OwnerBattery->Fatigue =
        FMath::Clamp(
            OwnerBattery->Fatigue +
            static_cast<float>(Consumed) * FiringFatiguePerGun,
            0.0f,
            100.0f);

    OnArtilleryShotResolved.Broadcast(
        Target,
        AmmoType,
        Consumed,
        Casualties);

    OnArtilleryShotResolvedNative.Broadcast(
        Target,
        AmmoType,
        Consumed,
        Casualties);

    if (OwnerBattery->ProjectilePresentationComponent)
    {
        OwnerBattery->ProjectilePresentationComponent->PresentResolvedSalvo(
            AmmoType,
            Target->GetActorLocation(),
            ImpactLocations,
            HitFlags,
            CasualtiesPerProjectile);
    }

    if (GetWorld())
    {
        FVector Direction =
            Target->GetActorLocation() - OwnerBattery->GetActorLocation();
        Direction.Z = 0.0f;
        Direction = Direction.GetSafeNormal();

        if (AStrategySmokeField* Smoke =
            GetWorld()->SpawnActor<AStrategySmokeField>(
                AStrategySmokeField::StaticClass(),
                OwnerBattery->GetActorLocation() + Direction * 350.0f,
                Direction.Rotation()))
        {
            Smoke->InitialDensity =
                FMath::Clamp(0.32f + Consumed * 0.05f, 0.32f, 0.70f);
            Smoke->RadiusCm =
                FMath::Clamp(1100.0f + Consumed * 140.0f, 1100.0f, 2400.0f);
        }
    }

    return true;
}

int32 UStrategyArtilleryFireMissionComponent::ResolveCasualties(
    AStrategyUnit* Target,
    int32 GunsFired,
    float DistanceCm,
    TArray<FVector>& OutImpactLocations,
    TArray<uint8>& OutHitFlags,
    TArray<int32>& OutCasualtiesPerProjectile)
{
    OutImpactLocations.Reset();
    OutHitFlags.Reset();
    OutCasualtiesPerProjectile.Reset();

    if (!OwnerBattery ||
        !OwnerBattery->ArtilleryAmmunitionComponent ||
        !IsValid(Target))
    {
        return 0;
    }

    const EStrategyArtilleryAmmoType AmmoType =
        OwnerBattery->ArtilleryAmmunitionComponent->SelectedAmmo;

    const float MaxRange =
        FMath::Max(1.0f, GetMaximumRangeCm(AmmoType));

    const float RangeFraction =
        FMath::Clamp(DistanceCm / MaxRange, 0.0f, 1.0f);

    float HitChance =
        GetBaseGunHitChance(AmmoType) *
        FMath::Lerp(1.0f, 0.45f, RangeFraction);

    if (Target->StanceComponent)
    {
        HitChance *= Target->StanceComponent->GetIncomingHitMultiplier();
    }

    if (Target->DirectionalCoverComponent)
    {
        HitChance *=
            Target->DirectionalCoverComponent->CalculateIncomingHitMultiplier(
                OwnerBattery->GetActorLocation());
    }

    if (Target->SkirmisherComponent)
    {
        HitChance *=
            Target->SkirmisherComponent->GetIncomingHitMultiplier();
    }

    if (Target->PositionOccupancyComponent)
    {
        HitChance *=
            Target->PositionOccupancyComponent->GetIncomingHitMultiplier(
                OwnerBattery->GetActorLocation());
    }

    if (Target->TerrainAwarenessComponent)
    {
        HitChance *=
            Target->TerrainAwarenessComponent->GetIncomingHitMultiplierFrom(
                OwnerBattery->GetActorLocation());
    }

    if (OwnerBattery->VisibilityComponent)
    {
        HitChance *=
            OwnerBattery->VisibilityComponent->GetSmokeTransmissionTo(Target);
    }

    HitChance = FMath::Clamp(HitChance, 0.01f, 0.95f);

    const FIntPoint CasualtyRange = GetCasualtyRange(AmmoType);

    const float MissDispersionCm =
        FMath::Lerp(450.0f, 2800.0f, RangeFraction);

    int32 Casualties = 0;

    for (int32 GunIndex = 0; GunIndex < GunsFired; ++GunIndex)
    {
        const bool bHit = RandomStream.FRand() <= HitChance;
        const float Angle = RandomStream.FRandRange(0.0f, 2.0f * PI);
        const float Radius =
            bHit
            ? RandomStream.FRandRange(0.0f, 250.0f)
            : FMath::Sqrt(RandomStream.FRand()) * MissDispersionCm;

        FVector Impact =
            Target->GetActorLocation() +
            FVector(
                FMath::Cos(Angle) * Radius,
                FMath::Sin(Angle) * Radius,
                0.0f);

        Impact =
            UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
                OwnerBattery,
                Impact);

        int32 ProjectileCasualties = 0;

        if (bHit)
        {
            ProjectileCasualties =
                RandomStream.RandRange(
                    CasualtyRange.X,
                    CasualtyRange.Y);

            Casualties += ProjectileCasualties;
        }

        OutImpactLocations.Add(Impact);
        OutHitFlags.Add(bHit ? 1 : 0);
        OutCasualtiesPerProjectile.Add(ProjectileCasualties);
    }

    return Casualties;
}


bool UStrategyArtilleryFireMissionComponent::FireAtLocation(
    const FVector& TargetLocation)
{
    if (!CanEngageLocation(TargetLocation) ||
        !OwnerBattery ||
        !OwnerBattery->ArtilleryAmmunitionComponent ||
        !GetWorld())
    {
        return false;
    }

    const int32 OperationalGuns = OwnerBattery->GetOperationalGunCount();
    const int32 GunsFiring =
        FMath::Min(
            OperationalGuns,
            OwnerBattery->ArtilleryAmmunitionComponent->GetSelectedRounds());

    if (GunsFiring <= 0)
    {
        return false;
    }

    const int32 Consumed =
        OwnerBattery->ArtilleryAmmunitionComponent
            ->ConsumeSelectedRounds(GunsFiring);

    if (Consumed <= 0)
    {
        return false;
    }

    const EStrategyArtilleryAmmoType AmmoType =
        OwnerBattery->ArtilleryAmmunitionComponent->SelectedAmmo;

    const float DistanceCm =
        FVector::Dist2D(
            OwnerBattery->GetActorLocation(),
            TargetLocation);

    const float MaxRange =
        FMath::Max(1.0f, GetMaximumRangeCm(AmmoType));

    const float RangeFraction =
        FMath::Clamp(DistanceCm / MaxRange, 0.0f, 1.0f);

    const float DispersionCm =
        ManualAreaRadiusCm +
        FMath::Lerp(250.0f, 2200.0f, RangeFraction);

    TMap<AStrategyUnit*, int32> CasualtiesByTarget;
    TArray<FVector> ImpactLocations;
    TArray<uint8> HitFlags;
    TArray<int32> CasualtiesPerProjectile;

    for (int32 GunIndex = 0; GunIndex < Consumed; ++GunIndex)
    {
        const float Angle = RandomStream.FRandRange(0.0f, 2.0f * PI);
        const float Radius = FMath::Sqrt(RandomStream.FRand()) * DispersionCm;

        FVector Impact =
            TargetLocation +
            FVector(
                FMath::Cos(Angle) * Radius,
                FMath::Sin(Angle) * Radius,
                0.0f);

        Impact =
            UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
                OwnerBattery,
                Impact);

        ImpactLocations.Add(Impact);

        AStrategyUnit* BestTarget = nullptr;
        float BestDistance = ManualAreaRadiusCm;

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

            const float ImpactDistance =
                FVector::Dist2D(
                    Candidate->GetActorLocation(),
                    Impact);

            if (ImpactDistance <= BestDistance)
            {
                BestDistance = ImpactDistance;
                BestTarget = Candidate;
            }
        }

        if (!IsValid(BestTarget))
        {
            HitFlags.Add(0);
            CasualtiesPerProjectile.Add(0);
            continue;
        }

        float HitChance =
            GetBaseGunHitChance(AmmoType) *
            FMath::Lerp(1.0f, 0.45f, RangeFraction);

        if (RandomStream.FRand() <= HitChance)
        {
            const FIntPoint Range = GetCasualtyRange(AmmoType);
            const int32 ProjectileCasualties =
                RandomStream.RandRange(Range.X, Range.Y);

            CasualtiesByTarget.FindOrAdd(BestTarget) +=
                ProjectileCasualties;

            HitFlags.Add(1);
            CasualtiesPerProjectile.Add(ProjectileCasualties);
        }
        else
        {
            HitFlags.Add(0);
            CasualtiesPerProjectile.Add(0);
        }
    }

    int32 TotalCasualties = 0;

    for (TPair<AStrategyUnit*, int32>& Pair : CasualtiesByTarget)
    {
        AStrategyUnit* Target = Pair.Key;
        const int32 Casualties = Pair.Value;

        if (!IsValid(Target) || Casualties <= 0)
        {
            continue;
        }

        TotalCasualties += Casualties;

        if (AStrategyArtilleryBatteryUnit* TargetBattery =
            Cast<AStrategyArtilleryBatteryUnit>(Target))
        {
            if (TargetBattery->ArtilleryDamageComponent)
            {
                const bool bExplosive =
                    AmmoType == EStrategyArtilleryAmmoType::Shell ||
                    AmmoType == EStrategyArtilleryAmmoType::Shrapnel;

                TargetBattery->ArtilleryDamageComponent
                    ->ApplyIncomingHits(Casualties, bExplosive);
            }
        }
        else if (AStrategySupplyWagonUnit* SupplyTarget =
            Cast<AStrategySupplyWagonUnit>(Target))
        {
            SupplyTarget->ApplySupplyDamage(
                FMath::Max(0, FMath::RoundToInt(Casualties * 0.35f)),
                FMath::Max(0, FMath::RoundToInt(Casualties * 0.30f)),
                static_cast<float>(Casualties) * 3.0f,
                FMath::Clamp(
                    static_cast<float>(Casualties) * 0.006f,
                    0.0f,
                    0.35f));
        }
        else
        {
            Target->ApplyStrengthLoss(Casualties);
        }

        if (Target->CombatComponent)
        {
            Target->CombatComponent->NotifyIncomingVolley(Casualties);
        }
    }

    const float CrewRatio =
        OwnerBattery->GunCount > 0
        ? static_cast<float>(OwnerBattery->GetOperationalGunCount()) /
          static_cast<float>(OwnerBattery->GunCount)
        : 0.0f;

    const float ExperienceFactor =
        FMath::Lerp(
            1.12f,
            0.88f,
            FMath::Clamp(
                OwnerBattery->Experience / 100.0f,
                0.0f,
                1.0f));

    ReloadRemainingSeconds =
        OwnerBattery->GunProfile.ReloadSeconds *
        ExperienceFactor *
        FMath::Lerp(1.35f, 1.0f, FMath::Clamp(CrewRatio, 0.0f, 1.0f)) *
        FMath::Lerp(
            1.0f,
            1.25f,
            FMath::Clamp(OwnerBattery->Fatigue / 100.0f, 0.0f, 1.0f));

    ++MissionSalvosFired;
    OwnerBattery->Fatigue =
        FMath::Clamp(
            OwnerBattery->Fatigue +
            static_cast<float>(Consumed) * FiringFatiguePerGun,
            0.0f,
            100.0f);

    OnArtilleryShotResolved.Broadcast(
        nullptr,
        AmmoType,
        Consumed,
        TotalCasualties);

    OnArtilleryShotResolvedNative.Broadcast(
        nullptr,
        AmmoType,
        Consumed,
        TotalCasualties);

    if (OwnerBattery->ProjectilePresentationComponent)
    {
        OwnerBattery->ProjectilePresentationComponent->PresentResolvedSalvo(
            AmmoType,
            TargetLocation,
            ImpactLocations,
            HitFlags,
            CasualtiesPerProjectile);
    }

    if (GetWorld())
    {
        FVector Direction =
            TargetLocation - OwnerBattery->GetActorLocation();
        Direction.Z = 0.0f;
        Direction = Direction.GetSafeNormal();

        if (AStrategySmokeField* Smoke =
            GetWorld()->SpawnActor<AStrategySmokeField>(
                AStrategySmokeField::StaticClass(),
                OwnerBattery->GetActorLocation() + Direction * 350.0f,
                Direction.Rotation()))
        {
            Smoke->InitialDensity =
                FMath::Clamp(
                    0.32f + Consumed * 0.05f,
                    0.32f,
                    0.70f);

            Smoke->RadiusCm =
                FMath::Clamp(
                    1100.0f + Consumed * 140.0f,
                    1100.0f,
                    2400.0f);
        }
    }

    return true;
}

float UStrategyArtilleryFireMissionComponent::CalculateTargetScore(
    const AStrategyUnit* Target) const
{
    if (!OwnerBattery || !IsValid(Target))
    {
        return -TNumericLimits<float>::Max();
    }

    const float Distance =
        FVector::Dist2D(
            OwnerBattery->GetActorLocation(),
            Target->GetActorLocation());

    float Score =
        static_cast<float>(Target->CurrentStrength) * 0.35f -
        Distance / 1000.0f;

    switch (TargetPriority)
    {
        case EStrategyArtilleryTargetPriority::CounterBattery:
            Score +=
                Target->Echelon == EStrategyEchelon::Artillery
                ? 900.0f
                : -150.0f;
            break;

        case EStrategyArtilleryTargetPriority::Infantry:
            Score +=
                Target->Echelon == EStrategyEchelon::Company
                ? 650.0f
                : 0.0f;
            break;

        case EStrategyArtilleryTargetPriority::ClosestThreat:
            Score -= Distance / 250.0f;
            break;

        case EStrategyArtilleryTargetPriority::ConserveAmmo:
            Score += static_cast<float>(Target->CurrentStrength) * 0.70f;
            Score -= Distance / 500.0f;
            break;

        case EStrategyArtilleryTargetPriority::Balanced:
        default:
            if (Target->Echelon == EStrategyEchelon::Artillery)
            {
                Score += 260.0f;
            }
            else if (Target->Echelon == EStrategyEchelon::Company)
            {
                Score += 100.0f;
            }
            else if (Target->Echelon == EStrategyEchelon::Cavalry)
            {
                Score += 80.0f;
            }
            break;
    }

    if (Target->UnitState == EStrategyUnitState::Routed)
    {
        Score -= 300.0f;
    }

    return Score;
}

float UStrategyArtilleryFireMissionComponent::GetBaseGunHitChance(
    EStrategyArtilleryAmmoType AmmoType) const
{
    switch (AmmoType)
    {
        case EStrategyArtilleryAmmoType::Canister:
            return 0.82f;
        case EStrategyArtilleryAmmoType::Shrapnel:
            return 0.58f;
        case EStrategyArtilleryAmmoType::Shell:
            return 0.48f;
        case EStrategyArtilleryAmmoType::RoundShot:
        default:
            return 0.55f;
    }
}

FIntPoint UStrategyArtilleryFireMissionComponent::GetCasualtyRange(
    EStrategyArtilleryAmmoType AmmoType) const
{
    switch (AmmoType)
    {
        case EStrategyArtilleryAmmoType::Canister:
            return FIntPoint(4, 8);
        case EStrategyArtilleryAmmoType::Shrapnel:
            return FIntPoint(2, 5);
        case EStrategyArtilleryAmmoType::Shell:
            return FIntPoint(1, 4);
        case EStrategyArtilleryAmmoType::RoundShot:
        default:
            return FIntPoint(1, 3);
    }
}

void UStrategyArtilleryFireMissionComponent::CompleteManualMission(
    bool bFailed)
{
    ManualTarget = nullptr;
    bHasManualAreaTarget = false;

    if (!OwnerBattery || !OwnerBattery->OrderComponent)
    {
        return;
    }

    if (bFailed)
    {
        OwnerBattery->OrderComponent->FailExecution();
    }
    else
    {
        OwnerBattery->OrderComponent->CompleteExecution();
    }
}
