#include "StrategyArtilleryCrewAnimationComponent.h"

#include "StrategyArtilleryBatteryUnit.h"
#include "StrategyArtilleryDeploymentComponent.h"
#include "StrategyArtilleryTraverseComponent.h"
#include "StrategyArtilleryRepairComponent.h"
#include "StrategyArtilleryFireMissionComponent.h"

UStrategyArtilleryCrewAnimationComponent::
UStrategyArtilleryCrewAnimationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyArtilleryCrewAnimationComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerBattery =
        Cast<AStrategyArtilleryBatteryUnit>(GetOwner());

    RebuildCrewStations();

    if (OwnerBattery &&
        OwnerBattery->ArtilleryFireMissionComponent)
    {
        OwnerBattery->ArtilleryFireMissionComponent
            ->OnArtilleryShotResolvedNative.AddUObject(
                this,
                &UStrategyArtilleryCrewAnimationComponent::
                    HandleArtilleryShotResolved);
    }
}

void UStrategyArtilleryCrewAnimationComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    if (OwnerBattery &&
        OwnerBattery->ArtilleryFireMissionComponent)
    {
        OwnerBattery->ArtilleryFireMissionComponent
            ->OnArtilleryShotResolvedNative.RemoveAll(this);
    }

    Super::EndPlay(EndPlayReason);
}

void UStrategyArtilleryCrewAnimationComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction);

    if (!OwnerBattery)
    {
        return;
    }

    if (bReloadDrillActive)
    {
        DrillElapsedSeconds += DeltaTime;
        UpdateReloadDrill();

        if (DrillElapsedSeconds >= DrillDurationSeconds)
        {
            bReloadDrillActive = false;
            DrillElapsedSeconds = 0.0f;
            DrillDurationSeconds = 0.0f;
        }
    }

    if (!bReloadDrillActive)
    {
        UpdateBatteryActivity();
    }
}

void UStrategyArtilleryCrewAnimationComponent::RebuildCrewStations()
{
    CrewStations.Reset();

    if (!OwnerBattery)
    {
        return;
    }

    const int32 StationsPerGun =
        FMath::Clamp(VisualCrewStationsPerGun, 1, 10);

    const EStrategyArtilleryCrewRole RoleOrder[] =
    {
        EStrategyArtilleryCrewRole::Gunner,
        EStrategyArtilleryCrewRole::Sponger,
        EStrategyArtilleryCrewRole::Loader,
        EStrategyArtilleryCrewRole::Rammer,
        EStrategyArtilleryCrewRole::Ammunition,
        EStrategyArtilleryCrewRole::WheelLeft,
        EStrategyArtilleryCrewRole::WheelRight,
        EStrategyArtilleryCrewRole::Reserve,
        EStrategyArtilleryCrewRole::Driver,
        EStrategyArtilleryCrewRole::HorseHandler
    };

    const int32 MaximumVisualCrew =
        FMath::Min(
            OwnerBattery->CrewStrength +
                OwnerBattery->DriverStrength,
            OwnerBattery->GunCount * StationsPerGun);

    int32 Added = 0;

    for (int32 GunIndex = 0;
         GunIndex < OwnerBattery->GunCount &&
         Added < MaximumVisualCrew;
         ++GunIndex)
    {
        for (int32 StationIndex = 0;
             StationIndex < StationsPerGun &&
             Added < MaximumVisualCrew;
             ++StationIndex)
        {
            FStrategyArtilleryCrewStation Station;
            Station.GunIndex = GunIndex;
            Station.StationIndex = StationIndex;
            Station.Role =
                RoleOrder[
                    FMath::Clamp(
                        StationIndex,
                        0,
                        UE_ARRAY_COUNT(RoleOrder) - 1)];
            Station.Activity =
                EStrategyArtilleryCrewActivity::Idle;

            CrewStations.Add(Station);
            ++Added;
        }
    }
}

EStrategyArtilleryCrewActivity
UStrategyArtilleryCrewAnimationComponent::GetActivityForStation(
    int32 StationIndex) const
{
    return CrewStations.IsValidIndex(StationIndex)
        ? CrewStations[StationIndex].Activity
        : EStrategyArtilleryCrewActivity::Idle;
}

void UStrategyArtilleryCrewAnimationComponent::
HandleArtilleryShotResolved(
    AStrategyUnit* Target,
    EStrategyArtilleryAmmoType AmmoType,
    int32 GunsFired,
    int32 Casualties)
{
    if (!OwnerBattery)
    {
        return;
    }

    bReloadDrillActive = true;
    DrillElapsedSeconds = 0.0f;

    DrillDurationSeconds =
        FMath::Max(
            1.0f,
            OwnerBattery->ArtilleryFireMissionComponent
            ? OwnerBattery->ArtilleryFireMissionComponent
                ->ReloadRemainingSeconds
            : OwnerBattery->GunProfile.ReloadSeconds);

    CurrentBatteryActivity =
        EStrategyArtilleryCrewActivity::RecoilReact;

    SetAllStationsActivity(
        EStrategyArtilleryCrewActivity::RecoilReact);
}

void UStrategyArtilleryCrewAnimationComponent::UpdateReloadDrill()
{
    if (!bReloadDrillActive ||
        DrillDurationSeconds <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const float Fraction =
        FMath::Clamp(
            DrillElapsedSeconds / DrillDurationSeconds,
            0.0f,
            1.0f);

    SetReloadPhaseByFraction(Fraction);
}

void UStrategyArtilleryCrewAnimationComponent::SetReloadPhaseByFraction(
    float Fraction)
{
    EStrategyArtilleryCrewActivity Activity =
        EStrategyArtilleryCrewActivity::ReturnToBattery;

    if (Fraction < 0.08f)
    {
        Activity =
            EStrategyArtilleryCrewActivity::RecoilReact;
    }
    else if (Fraction < 0.24f)
    {
        Activity =
            EStrategyArtilleryCrewActivity::Sponge;
    }
    else if (Fraction < 0.38f)
    {
        Activity =
            EStrategyArtilleryCrewActivity::LoadCharge;
    }
    else if (Fraction < 0.50f)
    {
        Activity =
            EStrategyArtilleryCrewActivity::LoadProjectile;
    }
    else if (Fraction < 0.68f)
    {
        Activity =
            EStrategyArtilleryCrewActivity::Ram;
    }
    else if (Fraction < 0.78f)
    {
        Activity =
            EStrategyArtilleryCrewActivity::Prime;
    }
    else if (Fraction < 0.90f)
    {
        Activity =
            EStrategyArtilleryCrewActivity::ClearGun;
    }

    CurrentBatteryActivity = Activity;

    for (FStrategyArtilleryCrewStation& Station : CrewStations)
    {
        switch (Activity)
        {
            case EStrategyArtilleryCrewActivity::Sponge:
                Station.Activity =
                    Station.Role == EStrategyArtilleryCrewRole::Sponger
                    ? Activity
                    : EStrategyArtilleryCrewActivity::Idle;
                break;

            case EStrategyArtilleryCrewActivity::LoadCharge:
            case EStrategyArtilleryCrewActivity::LoadProjectile:
                Station.Activity =
                    Station.Role == EStrategyArtilleryCrewRole::Loader ||
                    Station.Role == EStrategyArtilleryCrewRole::Ammunition
                    ? Activity
                    : EStrategyArtilleryCrewActivity::Idle;
                break;

            case EStrategyArtilleryCrewActivity::Ram:
                Station.Activity =
                    Station.Role == EStrategyArtilleryCrewRole::Rammer
                    ? Activity
                    : EStrategyArtilleryCrewActivity::Idle;
                break;

            case EStrategyArtilleryCrewActivity::Prime:
            case EStrategyArtilleryCrewActivity::ClearGun:
                Station.Activity =
                    Station.Role == EStrategyArtilleryCrewRole::Gunner
                    ? Activity
                    : EStrategyArtilleryCrewActivity::Idle;
                break;

            default:
                Station.Activity = Activity;
                break;
        }
    }
}

void UStrategyArtilleryCrewAnimationComponent::UpdateBatteryActivity()
{
    if (!OwnerBattery)
    {
        return;
    }

    EStrategyArtilleryCrewActivity Activity =
        EStrategyArtilleryCrewActivity::Idle;

    if (OwnerBattery->DeploymentComponent)
    {
        switch (OwnerBattery->DeploymentComponent->MobilityState)
        {
            case EStrategyArtilleryMobilityState::Deploying:
                Activity =
                    EStrategyArtilleryCrewActivity::Unlimber;
                break;

            case EStrategyArtilleryMobilityState::Limbering:
                Activity =
                    EStrategyArtilleryCrewActivity::Limber;
                break;

            case EStrategyArtilleryMobilityState::Manhandling:
                Activity =
                    EStrategyArtilleryCrewActivity::PushGun;
                break;

            default:
                break;
        }
    }

    if (Activity == EStrategyArtilleryCrewActivity::Idle &&
        OwnerBattery->ArtilleryTraverseComponent &&
        OwnerBattery->ArtilleryTraverseComponent->bTraversing)
    {
        const float Delta =
            FMath::FindDeltaAngleDegrees(
                OwnerBattery->GetActorRotation().Yaw,
                OwnerBattery->ArtilleryTraverseComponent
                    ->DesiredFacingYaw);

        Activity =
            Delta < 0.0f
            ? EStrategyArtilleryCrewActivity::TraverseLeft
            : EStrategyArtilleryCrewActivity::TraverseRight;
    }

    if (Activity == EStrategyArtilleryCrewActivity::Idle &&
        OwnerBattery->ArtilleryRepairComponent &&
        OwnerBattery->ArtilleryRepairComponent->bRepairing)
    {
        Activity =
            EStrategyArtilleryCrewActivity::Repair;
    }

    CurrentBatteryActivity = Activity;
    SetAllStationsActivity(Activity);
}

void UStrategyArtilleryCrewAnimationComponent::SetAllStationsActivity(
    EStrategyArtilleryCrewActivity Activity)
{
    for (FStrategyArtilleryCrewStation& Station : CrewStations)
    {
        Station.Activity = Activity;
    }
}
