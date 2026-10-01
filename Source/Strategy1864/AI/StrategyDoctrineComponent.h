#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyDoctrineComponent.generated.h"

class UStrategyOfficerProfileComponent;

UENUM(BlueprintType)
enum class EStrategyDoctrine : uint8
{
    Defensive,
    Balanced,
    Offensive
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyDoctrineComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyDoctrineComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Doctrine")
    EStrategyDoctrine Doctrine = EStrategyDoctrine::Balanced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Doctrine", meta=(ClampMin="0.0", ClampMax="100.0"))
    float CommanderOrderAggression = 50.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Doctrine")
    float GetEffectiveAggression(
        const UStrategyOfficerProfileComponent* OfficerProfile) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Doctrine")
    float GetPreferredEngagementRangeFraction(
        const UStrategyOfficerProfileComponent* OfficerProfile) const;
};
