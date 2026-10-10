#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyContactComponent.generated.h"

class AStrategyUnit;

UENUM(BlueprintType)
enum class EStrategyContactSource : uint8
{
    OwnEyes,
    Report,
    HQ
};

USTRUCT(BlueprintType)
struct FStrategyContactRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName StableUnitId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FVector LastKnownLocation = FVector::ZeroVector;

    // Canonical snapshot position; LastKnownLocation remains compatible with existing consumers.
    UPROPERTY(BlueprintReadOnly)
    FVector LastKnownPosition = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector Heading = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector ObservedVelocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float LastSeenTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float Confidence = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float UncertaintyRadiusCm = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    EStrategyContactSource Source = EStrategyContactSource::OwnEyes;

    // Resolve only current observations for actor-based tactical actions.
    UPROPERTY(Transient)
    TWeakObjectPtr<AStrategyUnit> ObservedUnit;

    UPROPERTY(BlueprintReadOnly)
    float SecondsSinceSeen = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bCurrentlyVisible = false;
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyContactComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyContactComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Contact")
    float ScanIntervalSeconds = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Contact")
    float MaximumAwarenessRangeCm = 60000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Contact")
    float ForgetAfterSeconds = 120.0f;

    // Linear decay to zero at ForgetAfterSeconds; uncertainty is not a predicted position.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Contact", meta=(ClampMin="0"))
    float UncertaintyGrowthCmPerSecond = 1200.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Contact")
    bool HasCurrentContact(const AStrategyUnit* Target) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Contact")
    bool GetLastKnownContact(FName StableUnitId, FStrategyContactRecord& OutRecord) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Contact")
    TArray<FStrategyContactRecord> GetKnownContacts() const;

private:
    void RefreshContacts(float ElapsedSeconds);

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    UPROPERTY()
    TArray<FStrategyContactRecord> Contacts;

    float ScanAccumulator = 0.0f;
};
