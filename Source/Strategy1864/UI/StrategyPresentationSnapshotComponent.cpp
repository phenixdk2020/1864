#include "StrategyPresentationSnapshotComponent.h"

#include "../Combat/StrategyCombatComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyDragoonComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryDeploymentComponent.h"
#include "../Artillery/StrategyArtilleryAmmunitionComponent.h"
#include "../Artillery/StrategyArtilleryFireMissionComponent.h"
#include "../Artillery/StrategyArtilleryProjectilePresentationComponent.h"
#include "../Logistics/StrategySupplyWagonUnit.h"
#include "../Logistics/StrategySupplyCargoComponent.h"
#include "../Terrain/StrategyTerrainAwarenessComponent.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "../Visual/StrategyUniformAppearanceComponent.h"
#include "../Visual/StrategyHumanAnimationStateComponent.h"
#include "../Visual/StrategyEquipmentVisualComponent.h"
#include "../Visual/StrategyHorseAnimationStateComponent.h"
#include "../Artillery/StrategyArtilleryCrewAnimationComponent.h"

UStrategyPresentationSnapshotComponent::UStrategyPresentationSnapshotComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FStrategyUnitPresentationSnapshot
UStrategyPresentationSnapshotComponent::BuildSnapshot() const
{
    FStrategyUnitPresentationSnapshot Snapshot;

    const AStrategyUnit* Unit = Cast<AStrategyUnit>(GetOwner());
    if (!IsValid(Unit))
    {
        return Snapshot;
    }

    Snapshot.StableUnitId = Unit->StableUnitId;
    Snapshot.DisplayName = Unit->DisplayName.ToString();
    Snapshot.NATOEchelon = Unit->GetNATOEchelonSymbol();

    Snapshot.Echelon =
        StaticEnum<EStrategyEchelon>()->GetNameStringByValue(
            static_cast<int64>(Unit->Echelon));

    Snapshot.Side =
        StaticEnum<EStrategySide>()->GetNameStringByValue(
            static_cast<int64>(Unit->Side));

    Snapshot.InitialStrength = Unit->InitialStrength;
    Snapshot.CurrentStrength = Unit->CurrentStrength;
    Snapshot.Losses =
        FMath::Max(0, Unit->InitialStrength - Unit->CurrentStrength);
    Snapshot.Morale = Unit->Morale;
    Snapshot.Cohesion = Unit->Cohesion;
    Snapshot.Fatigue = Unit->Fatigue;

    Snapshot.UnitState =
        StaticEnum<EStrategyUnitState>()->GetNameStringByValue(
            static_cast<int64>(Unit->UnitState));

    Snapshot.AIState =
        Unit->bOfficerAIEnabled
        ? TEXT("ON")
        : TEXT("OFF");

    if (Unit->OrderComponent)
    {
        const FStrategyOrder Order =
            Unit->OrderComponent->GetCurrentOrder();

        Snapshot.OrderType =
            StaticEnum<EStrategyOrderType>()->GetNameStringByValue(
                static_cast<int64>(Order.Type));

        Snapshot.ExecutionState =
            StaticEnum<EStrategyOrderExecutionState>()->GetNameStringByValue(
                static_cast<int64>(
                    Unit->OrderComponent->GetExecutionState()));
    }

    if (Unit->FormationComponent)
    {
        Snapshot.Formation =
            StaticEnum<EStrategyFormationType>()->GetNameStringByValue(
                static_cast<int64>(
                    Unit->FormationComponent->CurrentFormation));
    }

    if (Unit->CommandComponent)
    {
        const AStrategyUnit* OrganicParent =
            Unit->CommandComponent->OrganicParent;

        const AStrategyUnit* CurrentParent =
            Unit->CommandComponent->CurrentCommandParent;

        Snapshot.OrganicParentId =
            IsValid(OrganicParent)
            ? OrganicParent->StableUnitId
            : NAME_None;

        Snapshot.CurrentCommandParentId =
            IsValid(CurrentParent)
            ? CurrentParent->StableUnitId
            : NAME_None;

        Snapshot.bTemporarilyAttached =
            IsValid(CurrentParent) &&
            CurrentParent != OrganicParent;
    }

    if (Unit->CombatComponent)
    {
        Snapshot.AmmunitionRounds =
            Unit->CombatComponent->AmmunitionRounds;
    }

    if (Unit->HumanAnimationStateComponent)
    {
        Snapshot.HumanAnimationAction =
            StaticEnum<EStrategyHumanAnimationAction>()
                ->GetNameStringByValue(
                    static_cast<int64>(
                        Unit->HumanAnimationStateComponent->CurrentAction));
    }

    if (Unit->UniformAppearanceComponent)
    {
        Snapshot.SharedSkeletonId =
            Unit->UniformAppearanceComponent
                ->VisualProfile.SharedSkeletonId;

        Snapshot.SharedAnimationSetId =
            Unit->UniformAppearanceComponent
                ->VisualProfile.SharedAnimationSetId;

        Snapshot.UniformPresetId =
            Unit->UniformAppearanceComponent
                ->BasePreset.PresetId;

        const FStrategyUniformColors ResolvedColors =
            Unit->UniformAppearanceComponent->GetResolvedColors();

        Snapshot.UniformCoatColor = ResolvedColors.Coat;
        Snapshot.UniformTrouserColor = ResolvedColors.Trousers;
        Snapshot.UniformFacingColor = ResolvedColors.Facings;
    }

    if (Unit->EquipmentVisualComponent)
    {
        Snapshot.PrimaryWeaponId =
            Unit->EquipmentVisualComponent->PrimaryWeaponId;
    }

    if (Unit->TerrainAwarenessComponent)
    {
        Snapshot.TerrainGroundZ =
            Unit->TerrainAwarenessComponent->GetGroundZ();

        Snapshot.TerrainLocalSlopeDegrees =
            Unit->TerrainAwarenessComponent->GetLocalSlopeDegrees();

        Snapshot.TerrainFeatureElevationOffset =
            UStrategyTerrainQueryLibrary::GetFeatureElevationOffset(
                Unit,
                Unit->GetActorLocation());
    }

    if (const AStrategyArtilleryBatteryUnit* Battery =
        Cast<AStrategyArtilleryBatteryUnit>(Unit))
    {
        Snapshot.ArtilleryGunCount = Battery->GunCount;
        Snapshot.ArtilleryOperationalGuns =
            Battery->GetOperationalGunCount();
        Snapshot.ArtilleryCrew = Battery->CrewStrength;
        Snapshot.ArtilleryDrivers = Battery->DriverStrength;
        Snapshot.ArtilleryHorses = Battery->HorseStrength;

        if (Battery->CrewAnimationComponent)
        {
            Snapshot.ArtilleryCrewActivity =
                StaticEnum<EStrategyArtilleryCrewActivity>()
                    ->GetNameStringByValue(
                        static_cast<int64>(
                            Battery->CrewAnimationComponent
                                ->CurrentBatteryActivity));

            Snapshot.ArtilleryVisualCrewStations =
                Battery->CrewAnimationComponent
                    ->GetActiveCrewStationCount();
        }

        Snapshot.ArtilleryOwnershipState =
            StaticEnum<EStrategyArtilleryOwnershipState>()
                ->GetNameStringByValue(
                    static_cast<int64>(Battery->OwnershipState));

        if (Battery->DeploymentComponent)
        {
            Snapshot.ArtilleryMobilityState =
                StaticEnum<EStrategyArtilleryMobilityState>()
                    ->GetNameStringByValue(
                        static_cast<int64>(
                            Battery->DeploymentComponent->MobilityState));
        }

        if (Battery->ArtilleryFireMissionComponent)
        {
            Snapshot.ArtilleryFireMode =
                StaticEnum<EStrategyArtilleryFireMode>()
                    ->GetNameStringByValue(
                        static_cast<int64>(
                            Battery->ArtilleryFireMissionComponent->FireMode));
        }

        if (Battery->ProjectilePresentationComponent)
        {
            Snapshot.ActiveProjectilePresentations =
                Battery->ProjectilePresentationComponent
                    ->GetActiveProjectileCount();

            Snapshot.ProjectileHistoryCount =
                Battery->ProjectilePresentationComponent
                    ->RecentHistory.Num();

            if (Battery->ProjectilePresentationComponent
                    ->RecentHistory.Num() > 0)
            {
                const FStrategyArtilleryProjectileHistoryRecord& Last =
                    Battery->ProjectilePresentationComponent
                        ->RecentHistory.Last();

                Snapshot.LastProjectileShotSerial = Last.ShotSerial;
                Snapshot.LastProjectileImpact = Last.ImpactLocation;
            }
        }

        if (Battery->ArtilleryAmmunitionComponent)
        {
            Snapshot.AmmunitionRounds =
                Battery->ArtilleryAmmunitionComponent->GetTotalRounds();

            Snapshot.ArtilleryAmmoType =
                StaticEnum<EStrategyArtilleryAmmoType>()
                    ->GetNameStringByValue(
                        static_cast<int64>(
                            Battery->ArtilleryAmmunitionComponent->SelectedAmmo));
        }
    }

    if (const AStrategySupplyWagonUnit* Wagon =
        Cast<AStrategySupplyWagonUnit>(Unit))
    {
        Snapshot.SupplyDrivers = Wagon->DriverStrength;
        Snapshot.SupplyHorses = Wagon->HorseStrength;
        Snapshot.SupplyWagonCondition = Wagon->WagonCondition;

        Snapshot.SupplyOwnershipState =
            StaticEnum<EStrategySupplyOwnershipState>()
                ->GetNameStringByValue(
                    static_cast<int64>(Wagon->OwnershipState));

        if (Wagon->CargoComponent)
        {
            Snapshot.SupplySmallArmsRounds =
                Wagon->CargoComponent->SmallArmsRounds;

            Snapshot.SupplyArtilleryRounds =
                Wagon->CargoComponent->ArtilleryRounds;
        }
    }

    if (const ACavalryUnit* Cavalry = Cast<ACavalryUnit>(Unit))
    {
        if (Cavalry->HorseAnimationStateComponent)
        {
            Snapshot.HorseGait =
                StaticEnum<EStrategyHorseGait>()
                    ->GetNameStringByValue(
                        static_cast<int64>(
                            Cavalry->HorseAnimationStateComponent
                                ->CurrentGait));
        }

        if (Cavalry->DragoonComponent)
        {
            Snapshot.CavalryRole =
                StaticEnum<EStrategyCavalryRole>()->GetNameStringByValue(
                    static_cast<int64>(
                        Cavalry->DragoonComponent->Role));

            Snapshot.MountedState =
                StaticEnum<EStrategyMountedState>()->GetNameStringByValue(
                    static_cast<int64>(
                        Cavalry->DragoonComponent->MountedState));
        }
    }

    return Snapshot;
}
