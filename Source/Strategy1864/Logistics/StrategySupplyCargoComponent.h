#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategySupplyCargoComponent.generated.h"

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategySupplyCargoComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategySupplyCargoComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply|Cargo")
    int32 SmallArmsRounds = 12000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply|Cargo")
    int32 ArtilleryRounds = 320;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply|Cargo")
    int32 FoodUnits = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply|Cargo")
    int32 MedicalUnits = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply|Cargo")
    FName ArtilleryAmmunitionFamilyTag = TEXT("FIELD_ARTILLERY_GENERIC");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Supply|Cargo", meta=(ClampMin="0.0", ClampMax="100.0"))
    float CargoIntegrity = 100.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Supply|Cargo")
    int32 GetTotalAmmunitionRounds() const
    {
        return FMath::Max(0, SmallArmsRounds) + FMath::Max(0, ArtilleryRounds);
    }

    UFUNCTION(BlueprintCallable, Category="Strategy|Supply|Cargo")
    int32 ConsumeSmallArmsRounds(int32 Requested);

    UFUNCTION(BlueprintCallable, Category="Strategy|Supply|Cargo")
    int32 ConsumeArtilleryRounds(int32 Requested);

    UFUNCTION(BlueprintCallable, Category="Strategy|Supply|Cargo")
    void ApplyCargoLossFraction(float Fraction);
};
