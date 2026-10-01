#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyAITelemetryComponent.generated.h"

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyAITelemetryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyAITelemetryComponent();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|AI Telemetry")
    FString CurrentTask;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|AI Telemetry")
    FString ReasonCode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|AI Telemetry")
    float LastDecisionWorldSeconds = 0.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|AI Telemetry")
    void SetDecision(const FString& Task, const FString& Reason);
};
