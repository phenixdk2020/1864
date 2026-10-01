#include "StrategySupplyComponent.h"

#include "../Combat/StrategyCombatComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryAmmunitionComponent.h"
#include "StrategySupplyWagonUnit.h"
#include "StrategySupplyCargoComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "EngineUtils.h"

UStrategySupplyComponent::UStrategySupplyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategySupplyComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

void UStrategySupplyComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit)
    {
        return;
    }

    if (!bActsAsSupplySource)
    {
        float Fraction = 1.0f;
        bool bHasAmmoStore = false;

        if (const AStrategyArtilleryBatteryUnit* Battery =
            Cast<AStrategyArtilleryBatteryUnit>(OwnerUnit))
        {
            if (Battery->ArtilleryAmmunitionComponent &&
                Battery->ArtilleryAmmunitionComponent->MaximumTotalRounds > 0)
            {
                Fraction =
                    static_cast<float>(
                        Battery->ArtilleryAmmunitionComponent->GetTotalRounds()) /
                    static_cast<float>(
                        Battery->ArtilleryAmmunitionComponent->MaximumTotalRounds);
                bHasAmmoStore = true;
            }
        }
        else if (OwnerUnit->CombatComponent &&
                 OwnerUnit->CombatComponent->MaxAmmunitionRounds > 0)
        {
            Fraction =
                static_cast<float>(OwnerUnit->CombatComponent->AmmunitionRounds) /
                static_cast<float>(OwnerUnit->CombatComponent->MaxAmmunitionRounds);
            bHasAmmoStore = true;
        }

        if (bHasAmmoStore)
        {
            if (Fraction <= AutomaticRequestThresholdFraction)
            {
                bRequestingResupply = true;
            }
            else if (Fraction >= 0.90f)
            {
                bRequestingResupply = false;
            }
        }

        return;
    }

    if (StoredAmmunitionRounds <= 0)
    {
        return;
    }

    if (AStrategyUnit* Receiver = FindBestReceiver())
    {
        TransferTo(Receiver, DeltaTime);
    }
}

int32 UStrategySupplyComponent::AddStoredAmmunition(int32 Rounds)
{
    if (Rounds <= 0)
    {
        return 0;
    }

    const int32 Before = StoredAmmunitionRounds;
    StoredAmmunitionRounds =
        FMath::Clamp(
            StoredAmmunitionRounds + Rounds,
            0,
            FMath::Max(1, MaxStoredAmmunitionRounds));

    return StoredAmmunitionRounds - Before;
}

AStrategyUnit* UStrategySupplyComponent::FindBestReceiver() const
{
    if (!OwnerUnit || !GetWorld() || IsTransferStateBlocked(OwnerUnit))
    {
        return nullptr;
    }

    AStrategyUnit* Best = nullptr;
    float BestAmmoFraction = 2.0f;

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;

        if (!CanSupplyReceiver(Candidate))
        {
            continue;
        }

        if (FVector::Dist2D(
                OwnerUnit->GetActorLocation(),
                Candidate->GetActorLocation()) > ResupplyRadiusCm)
        {
            continue;
        }

        const float AmmoFraction =
            GetReceiverAmmoFraction(Candidate);

        if (AmmoFraction < BestAmmoFraction)
        {
            BestAmmoFraction = AmmoFraction;
            Best = Candidate;
        }
    }

    return Best;
}


void UStrategySupplyComponent::TransferTo(
    AStrategyUnit* Receiver,
    float DeltaTime)
{
    if (!OwnerUnit ||
        !IsValid(Receiver) ||
        StoredAmmunitionRounds < 0 ||
        IsTransferStateBlocked(OwnerUnit) ||
        IsTransferStateBlocked(Receiver) ||
        !CanSupplyReceiver(Receiver))
    {
        return;
    }

    int32 Missing = 0;

    if (AStrategyArtilleryBatteryUnit* Battery =
        Cast<AStrategyArtilleryBatteryUnit>(Receiver))
    {
        if (!Battery->ArtilleryAmmunitionComponent)
        {
            return;
        }

        Missing =
            Battery->ArtilleryAmmunitionComponent->GetMissingRounds();
    }
    else
    {
        if (!Receiver->CombatComponent ||
            Receiver->CombatComponent->MaxAmmunitionRounds <= 0)
        {
            return;
        }

        Missing =
            FMath::Max(
                0,
                Receiver->CombatComponent->MaxAmmunitionRounds -
                Receiver->CombatComponent->AmmunitionRounds);
    }

    if (Missing <= 0)
    {
        if (Receiver->SupplyComponent)
        {
            Receiver->SupplyComponent->bRequestingResupply = false;
        }
        return;
    }

    const int32 Available =
        GetSourceAvailableRoundsForReceiver(Receiver);

    if (Available <= 0)
    {
        return;
    }

    const int32 Transfer =
        FMath::Clamp(
            FMath::CeilToInt(
                FMath::Max(1.0f, TransferRoundsPerSecond) * DeltaTime),
            1,
            FMath::Min(Missing, Available));

    const int32 Consumed =
        ConsumeSourceRoundsForReceiver(
            Receiver,
            Transfer);

    if (Consumed <= 0)
    {
        return;
    }

    int32 Added = 0;

    if (AStrategyArtilleryBatteryUnit* Battery =
        Cast<AStrategyArtilleryBatteryUnit>(Receiver))
    {
        Added =
            Battery->ArtilleryAmmunitionComponent
                ->AddCompatibleMixedRounds(Consumed);
    }
    else
    {
        const int32 Before =
            Receiver->CombatComponent->AmmunitionRounds;

        Receiver->CombatComponent->ResupplyAmmunition(Consumed);

        Added =
            Receiver->CombatComponent->AmmunitionRounds - Before;
    }

    if (Added < Consumed)
    {
        // Capacity should normally be pre-clamped by Missing. If state changed
        // in the same tick, return generic-source excess only; physical wagon
        // cargo is intentionally conservative and does not duplicate rounds.
        if (!Cast<AStrategySupplyWagonUnit>(OwnerUnit))
        {
            StoredAmmunitionRounds +=
                FMath::Max(0, Consumed - Added);
        }
    }

    if (GetReceiverAmmoFraction(Receiver) >= 0.90f &&
        Receiver->SupplyComponent)
    {
        Receiver->SupplyComponent->bRequestingResupply = false;
    }
}

float UStrategySupplyComponent::GetReceiverAmmoFraction(
    const AStrategyUnit* Receiver) const
{
    if (!IsValid(Receiver))
    {
        return 1.0f;
    }

    if (const AStrategyArtilleryBatteryUnit* Battery =
        Cast<AStrategyArtilleryBatteryUnit>(Receiver))
    {
        if (!Battery->ArtilleryAmmunitionComponent)
        {
            return 1.0f;
        }

        const int32 MaxAmmo =
            FMath::Max(
                1,
                Battery->ArtilleryAmmunitionComponent
                    ->MaximumTotalRounds);

        return FMath::Clamp(
            static_cast<float>(
                Battery->ArtilleryAmmunitionComponent
                    ->GetTotalRounds()) /
            static_cast<float>(MaxAmmo),
            0.0f,
            1.0f);
    }

    if (!Receiver->CombatComponent ||
        Receiver->CombatComponent->MaxAmmunitionRounds <= 0)
    {
        return 1.0f;
    }

    return FMath::Clamp(
        static_cast<float>(
            Receiver->CombatComponent->AmmunitionRounds) /
        static_cast<float>(
            FMath::Max(
                1,
                Receiver->CombatComponent->MaxAmmunitionRounds)),
        0.0f,
        1.0f);
}

bool UStrategySupplyComponent::CanSupplyReceiver(
    const AStrategyUnit* Receiver) const
{
    if (!OwnerUnit ||
        !IsValid(Receiver) ||
        Receiver == OwnerUnit ||
        Receiver->Side != OwnerUnit->Side ||
        !Receiver->SupplyComponent ||
        !Receiver->SupplyComponent->bRequestingResupply ||
        IsTransferStateBlocked(Receiver))
    {
        return false;
    }

    return GetSourceAvailableRoundsForReceiver(Receiver) > 0;
}

bool UStrategySupplyComponent::IsTransferStateBlocked(
    const AStrategyUnit* Unit) const
{
    if (!IsValid(Unit) ||
        Unit->UnitState == EStrategyUnitState::Routed ||
        Unit->UnitState == EStrategyUnitState::Disabled ||
        Unit->UnitState == EStrategyUnitState::Abandoned ||
        Unit->UnitState == EStrategyUnitState::Destroyed)
    {
        return true;
    }

    if (bPauseTransferWhileMoving &&
        (Unit->UnitState == EStrategyUnitState::Moving ||
         (Unit->MovementExecutor &&
          Unit->MovementExecutor->HasMovementGoal())))
    {
        return true;
    }

    if (bPauseTransferUnderFire &&
        (Unit->UnitState == EStrategyUnitState::UnderFire ||
         Unit->UnitState == EStrategyUnitState::Engaged))
    {
        return true;
    }

    return false;
}

bool UStrategySupplyComponent::IsArtilleryCompatible(
    const AStrategyUnit* Receiver) const
{
    const AStrategyArtilleryBatteryUnit* Battery =
        Cast<AStrategyArtilleryBatteryUnit>(Receiver);

    if (!Battery)
    {
        return true;
    }

    const AStrategySupplyWagonUnit* Wagon =
        Cast<AStrategySupplyWagonUnit>(OwnerUnit);

    if (!Wagon || !Wagon->CargoComponent)
    {
        return true;
    }

    if (!bRequireCompatibleArtilleryFamily)
    {
        return true;
    }

    const FName SourceFamily =
        Wagon->CargoComponent->ArtilleryAmmunitionFamilyTag;

    const FName ReceiverFamily =
        Battery->GunProfile.AmmunitionFamilyTag;

    return SourceFamily.IsNone() ||
           ReceiverFamily.IsNone() ||
           SourceFamily == ReceiverFamily;
}

int32 UStrategySupplyComponent::GetSourceAvailableRoundsForReceiver(
    const AStrategyUnit* Receiver) const
{
    if (!OwnerUnit || !IsValid(Receiver))
    {
        return 0;
    }

    if (const AStrategySupplyWagonUnit* Wagon =
        Cast<AStrategySupplyWagonUnit>(OwnerUnit))
    {
        if (!Wagon->CargoComponent)
        {
            return 0;
        }

        if (Cast<AStrategyArtilleryBatteryUnit>(Receiver))
        {
            return IsArtilleryCompatible(Receiver)
                ? FMath::Max(
                    0,
                    Wagon->CargoComponent->ArtilleryRounds)
                : 0;
        }

        return FMath::Max(
            0,
            Wagon->CargoComponent->SmallArmsRounds);
    }

    return FMath::Max(0, StoredAmmunitionRounds);
}

int32 UStrategySupplyComponent::ConsumeSourceRoundsForReceiver(
    const AStrategyUnit* Receiver,
    int32 RequestedRounds)
{
    if (!OwnerUnit || !IsValid(Receiver) || RequestedRounds <= 0)
    {
        return 0;
    }

    if (AStrategySupplyWagonUnit* Wagon =
        Cast<AStrategySupplyWagonUnit>(OwnerUnit))
    {
        if (!Wagon->CargoComponent)
        {
            return 0;
        }

        if (Cast<AStrategyArtilleryBatteryUnit>(Receiver))
        {
            if (!IsArtilleryCompatible(Receiver))
            {
                return 0;
            }

            return Wagon->CargoComponent
                ->ConsumeArtilleryRounds(RequestedRounds);
        }

        return Wagon->CargoComponent
            ->ConsumeSmallArmsRounds(RequestedRounds);
    }

    const int32 Consumed =
        FMath::Min(
            FMath::Max(0, RequestedRounds),
            FMath::Max(0, StoredAmmunitionRounds));

    StoredAmmunitionRounds -= Consumed;
    return Consumed;
}
