#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyOfficerProfileComponent.generated.h"

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyOfficerProfileComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyOfficerProfileComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Leadership = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Inspiration = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer", meta=(ClampMin="0.0", ClampMax="100.0"))
    float TacticalSkill = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Initiative = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer", meta=(ClampMin="0.0", ClampMax="100.0"))
    float StaffQuality = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Aggression = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Caution = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Discipline = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Composure = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Experience = 50.0f;

    /** Who he is (the campaign's officer id, a name and rank), when the battle knows him. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer")
    FString OfficerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer")
    FString OfficerName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Officer")
    FString OfficerRank;

    /** What has become of him in the battle: 0 well, 1 wounded, 2 taken prisoner (never killed). */
    UPROPERTY(BlueprintReadWrite, Category="Strategy|Officer")
    int32 Fate = 0;

    /** How much of his ability is left (a wounded officer 0.6, a prisoner's unit has none to lead it 0.4). */
    UPROPERTY(BlueprintReadWrite, Category="Strategy|Officer")
    float Impairment = 1.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Officer")
    float GetCommandEfficiency() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Officer")
    float GetStressReactionMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Officer")
    float GetDecisionStability() const;
};
