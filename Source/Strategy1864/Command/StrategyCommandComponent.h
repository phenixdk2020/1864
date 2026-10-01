#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyCommandComponent.generated.h"

class AStrategyUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStrategyCommandParentChanged, AStrategyUnit*, OldParent, AStrategyUnit*, NewParent);

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyCommandComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyCommandComponent();

    UPROPERTY(BlueprintAssignable, Category="Strategy|Command")
    FStrategyCommandParentChanged OnCurrentCommandParentChanged;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Strategy|Command")
    TObjectPtr<AStrategyUnit> OrganicParent;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Strategy|Command")
    TObjectPtr<AStrategyUnit> CurrentCommandParent;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Strategy|Command")
    TArray<TObjectPtr<AStrategyUnit>> OrganicSubordinates;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Strategy|Command")
    TArray<TObjectPtr<AStrategyUnit>> CurrentSubordinates;

    UFUNCTION(BlueprintCallable, Category="Strategy|Command")
    void SetOrganicParent(AStrategyUnit* NewParent);

    UFUNCTION(BlueprintCallable, Category="Strategy|Command")
    void SetCurrentCommandParent(AStrategyUnit* NewParent);

    UFUNCTION(BlueprintCallable, Category="Strategy|Command")
    void RestoreOrganicCommandParent();

    UFUNCTION(BlueprintCallable, Category="Strategy|Command")
    void AddOrganicSubordinate(AStrategyUnit* Unit);

    UFUNCTION(BlueprintCallable, Category="Strategy|Command")
    void RemoveOrganicSubordinate(AStrategyUnit* Unit);

    UFUNCTION(BlueprintCallable, Category="Strategy|Command")
    void AddCurrentSubordinate(AStrategyUnit* Unit);

    UFUNCTION(BlueprintCallable, Category="Strategy|Command")
    void RemoveCurrentSubordinate(AStrategyUnit* Unit);

private:
    bool WouldCreateCommandCycle(AStrategyUnit* NewParent) const;
    static void RemoveFromParentCurrentList(AStrategyUnit* Parent, AStrategyUnit* Child);
    static void AddToParentCurrentList(AStrategyUnit* Parent, AStrategyUnit* Child);
};
