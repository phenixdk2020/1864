#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyArtilleryTypes.h"
#include "StrategyArtilleryAmmunitionComponent.generated.h"

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryAmmunitionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryAmmunitionComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Ammo")
    EStrategyArtilleryAmmoType SelectedAmmo =
        EStrategyArtilleryAmmoType::Shell;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Ammo")
    int32 RoundShotRounds = 120;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Ammo")
    int32 ShellRounds = 120;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Ammo")
    int32 ShrapnelRounds = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Ammo")
    int32 CanisterRounds = 60;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Ammo")
    int32 MaximumTotalRounds = 500;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery|Ammo")
    int32 GetRounds(EStrategyArtilleryAmmoType AmmoType) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery|Ammo")
    int32 GetSelectedRounds() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery|Ammo")
    int32 GetTotalRounds() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery|Ammo")
    int32 GetMissingRounds() const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|Ammo")
    bool SelectAmmo(EStrategyArtilleryAmmoType AmmoType);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|Ammo")
    int32 ConsumeSelectedRounds(int32 RequestedRounds);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|Ammo")
    int32 AddRounds(
        EStrategyArtilleryAmmoType AmmoType,
        int32 Rounds);

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|Ammo")
    int32 AddCompatibleMixedRounds(int32 Rounds);
};
