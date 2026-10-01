#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyContactComponent.generated.h"

class AStrategyUnit;

USTRUCT(BlueprintType)
struct FStrategyContactRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName StableUnitId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FVector LastKnownLocation = FVector::ZeroVector;

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
