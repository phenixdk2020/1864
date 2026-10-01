#include "StrategyArtilleryAmmunitionComponent.h"

UStrategyArtilleryAmmunitionComponent::UStrategyArtilleryAmmunitionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

int32 UStrategyArtilleryAmmunitionComponent::GetRounds(
    EStrategyArtilleryAmmoType AmmoType) const
{
    switch (AmmoType)
    {
        case EStrategyArtilleryAmmoType::RoundShot:
            return RoundShotRounds;
        case EStrategyArtilleryAmmoType::Shell:
            return ShellRounds;
        case EStrategyArtilleryAmmoType::Shrapnel:
            return ShrapnelRounds;
        case EStrategyArtilleryAmmoType::Canister:
            return CanisterRounds;
        default:
            return 0;
    }
}

int32 UStrategyArtilleryAmmunitionComponent::GetSelectedRounds() const
{
    return GetRounds(SelectedAmmo);
}

int32 UStrategyArtilleryAmmunitionComponent::GetTotalRounds() const
{
    return
        FMath::Max(0, RoundShotRounds) +
        FMath::Max(0, ShellRounds) +
        FMath::Max(0, ShrapnelRounds) +
        FMath::Max(0, CanisterRounds);
}

int32 UStrategyArtilleryAmmunitionComponent::GetMissingRounds() const
{
    return FMath::Max(0, MaximumTotalRounds - GetTotalRounds());
}

bool UStrategyArtilleryAmmunitionComponent::SelectAmmo(
    EStrategyArtilleryAmmoType AmmoType)
{
    SelectedAmmo = AmmoType;
    return GetRounds(SelectedAmmo) > 0;
}

int32 UStrategyArtilleryAmmunitionComponent::ConsumeSelectedRounds(
    int32 RequestedRounds)
{
    const int32 Requested = FMath::Max(0, RequestedRounds);
    int32* Store = nullptr;

    switch (SelectedAmmo)
    {
        case EStrategyArtilleryAmmoType::RoundShot:
            Store = &RoundShotRounds;
            break;
        case EStrategyArtilleryAmmoType::Shell:
            Store = &ShellRounds;
            break;
        case EStrategyArtilleryAmmoType::Shrapnel:
            Store = &ShrapnelRounds;
            break;
        case EStrategyArtilleryAmmoType::Canister:
            Store = &CanisterRounds;
            break;
        default:
            break;
    }

    if (!Store)
    {
        return 0;
    }

    const int32 Consumed = FMath::Min(Requested, FMath::Max(0, *Store));
    *Store -= Consumed;
    return Consumed;
}

int32 UStrategyArtilleryAmmunitionComponent::AddRounds(
    EStrategyArtilleryAmmoType AmmoType,
    int32 Rounds)
{
    const int32 Capacity = GetMissingRounds();
    const int32 Added = FMath::Clamp(Rounds, 0, Capacity);

    switch (AmmoType)
    {
        case EStrategyArtilleryAmmoType::RoundShot:
            RoundShotRounds += Added;
            break;
        case EStrategyArtilleryAmmoType::Shell:
            ShellRounds += Added;
            break;
        case EStrategyArtilleryAmmoType::Shrapnel:
            ShrapnelRounds += Added;
            break;
        case EStrategyArtilleryAmmoType::Canister:
            CanisterRounds += Added;
            break;
        default:
            return 0;
    }

    return Added;
}

int32 UStrategyArtilleryAmmunitionComponent::AddCompatibleMixedRounds(
    int32 Rounds)
{
    int32 Remaining = FMath::Clamp(Rounds, 0, GetMissingRounds());
    int32 AddedTotal = 0;

    const EStrategyArtilleryAmmoType Types[] =
    {
        EStrategyArtilleryAmmoType::Shell,
        EStrategyArtilleryAmmoType::RoundShot,
        EStrategyArtilleryAmmoType::Shrapnel,
        EStrategyArtilleryAmmoType::Canister
    };

    int32 Index = 0;
    while (Remaining > 0)
    {
        const int32 Before = AddedTotal;
        AddedTotal += AddRounds(Types[Index % 4], 1);
        Remaining -= AddedTotal > Before ? 1 : 0;

        if (AddedTotal == Before)
        {
            break;
        }

        ++Index;
    }

    return AddedTotal;
}
