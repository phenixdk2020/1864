#include "StrategyArtilleryDamageComponent.h"

#include "StrategyArtilleryBatteryUnit.h"

UStrategyArtilleryDamageComponent::UStrategyArtilleryDamageComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyArtilleryDamageComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerBattery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());

    const int32 Seed =
        OwnerBattery
        ? static_cast<int32>(GetTypeHash(OwnerBattery->StableUnitId)) ^ 0xA771
        : GetUniqueID();

    RandomStream.Initialize(Seed);
}

void UStrategyArtilleryDamageComponent::ApplyIncomingHits(
    int32 Hits,
    bool bExplosive)
{
    if (!OwnerBattery || Hits <= 0)
    {
        return;
    }

    int32 PersonnelLoss = 0;
    int32 HorseLoss = 0;
    int32 GunDisabled = 0;
    int32 GunDestroyed = 0;

    for (int32 Index = 0; Index < Hits; ++Index)
    {
        const float Roll = RandomStream.FRand();

        if (bExplosive)
        {
            if (Roll < 0.58f)
            {
                ++PersonnelLoss;
            }
            else if (Roll < 0.76f)
            {
                ++HorseLoss;
            }
            else if (Roll < 0.94f)
            {
                ++GunDisabled;
            }
            else
            {
                ++GunDestroyed;
            }
        }
        else
        {
            if (Roll < 0.78f)
            {
                ++PersonnelLoss;
            }
            else if (Roll < 0.92f)
            {
                ++HorseLoss;
            }
            else
            {
                ++GunDisabled;
            }
        }
    }

    OwnerBattery->ApplyBatteryDamage(
        PersonnelLoss,
        HorseLoss,
        GunDisabled,
        GunDestroyed);
}
