#include "StrategyWorkingPartyComponent.h"
#include "StrategyDefensivePosition.h"
#include "../Units/StrategyUnit.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyFieldworksComponent.h"
#include "../AI/StrategyNCOComponent.h"

UStrategyWorkingPartyComponent::UStrategyWorkingPartyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyWorkingPartyComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

bool UStrategyWorkingPartyComponent::AssignTask(
    EStrategyWorkingPartyTask Task,
    int32 WorkerCount)
{
    if (!OwnerUnit ||
        Task == EStrategyWorkingPartyTask::None ||
        WorkerCount <= 0)
    {
        return false;
    }

    AssignedWorkers =
        FMath::Clamp(WorkerCount, 1, FMath::Max(1, AvailableWorkers));

    ActiveTask = Task;
    TaskProgress = 0.0f;
    SetComponentTickEnabled(true);
    return true;
}

void UStrategyWorkingPartyComponent::CancelTask()
{
    ActiveTask = EStrategyWorkingPartyTask::None;
    AssignedWorkers = 0;
    TaskProgress = 0.0f;
    TargetUnit = nullptr;
    TargetPosition = nullptr;
    SetComponentTickEnabled(false);
}

void UStrategyWorkingPartyComponent::SetTargetUnit(AStrategyUnit* NewTarget)
{
    TargetUnit = NewTarget;
}

void UStrategyWorkingPartyComponent::SetTargetPosition(
    AStrategyDefensivePosition* NewTarget)
{
    TargetPosition = NewTarget;
}

int32 UStrategyWorkingPartyComponent::LoadAmmunition(int32 Rounds)
{
    const int32 Added = FMath::Max(0, Rounds);
    CarriedAmmunitionRounds += Added;
    return Added;
}

int32 UStrategyWorkingPartyComponent::DeliverAmmunition()
{
    if (!IsValid(TargetUnit) ||
        !TargetUnit->CombatComponent ||
        CarriedAmmunitionRounds <= 0)
    {
        return 0;
    }

    const int32 Before = TargetUnit->CombatComponent->AmmunitionRounds;
    TargetUnit->CombatComponent->ResupplyAmmunition(CarriedAmmunitionRounds);
    const int32 Delivered =
        FMath::Max(0, TargetUnit->CombatComponent->AmmunitionRounds - Before);

    CarriedAmmunitionRounds -= Delivered;
    return Delivered;
}

void UStrategyWorkingPartyComponent::AddWoundedForCollection(int32 Wounded)
{
    WoundedWaitingCollection += FMath::Max(0, Wounded);
}

int32 UStrategyWorkingPartyComponent::CollectWounded(int32 Requested)
{
    const int32 Capacity =
        FMath::Max(1, AssignedWorkers) * 2;

    const int32 Collected =
        FMath::Min3(
            FMath::Max(0, Requested),
            WoundedWaitingCollection,
            Capacity);

    WoundedWaitingCollection -= Collected;
    WoundedCollected += Collected;
    return Collected;
}

float UStrategyWorkingPartyComponent::GetWorkRate() const
{
    float Rate =
        FMath::Clamp(
            static_cast<float>(AssignedWorkers) / 12.0f,
            0.10f,
            2.0f);

    if (OwnerUnit && OwnerUnit->NCOComponent)
    {
        Rate *= OwnerUnit->NCOComponent->GetDetachmentControlMultiplier();
    }

    return Rate;
}

bool UStrategyWorkingPartyComponent::HasActiveTask() const
{
    return ActiveTask != EStrategyWorkingPartyTask::None;
}

void UStrategyWorkingPartyComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!HasActiveTask())
    {
        SetComponentTickEnabled(false);
        return;
    }

    TaskProgress =
        FMath::Clamp(
            TaskProgress + DeltaTime * 0.05f * GetWorkRate(),
            0.0f,
            1.0f);

    if (ActiveTask == EStrategyWorkingPartyTask::CarryAmmunition &&
        TaskProgress >= 1.0f)
    {
        DeliverAmmunition();
        CancelTask();
    }
    else if (ActiveTask == EStrategyWorkingPartyTask::StretcherCollection &&
             TaskProgress >= 1.0f)
    {
        CollectWounded(WoundedWaitingCollection);
        CancelTask();
    }
    else if (ActiveTask == EStrategyWorkingPartyTask::DigFieldworks &&
             TaskProgress >= 1.0f)
    {
        if (OwnerUnit && OwnerUnit->FieldworksComponent)
        {
            OwnerUnit->FieldworksComponent->bEngineerUnit = true;
            OwnerUnit->FieldworksComponent->BeginHastyFieldworks();
        }
        CancelTask();
    }
    else if (ActiveTask == EStrategyWorkingPartyTask::BreachObstacle &&
             TaskProgress >= 1.0f)
    {
        if (IsValid(TargetPosition))
        {
            TargetPosition->ApplyStructuralDamage(15.0f * GetWorkRate());
            if (TargetPosition->Condition <= 35.0f)
            {
                TargetPosition->MarkBreached(true);
            }
        }
        CancelTask();
    }
    else if (ActiveTask == EStrategyWorkingPartyTask::RepairPosition &&
             TaskProgress >= 1.0f)
    {
        if (IsValid(TargetPosition))
        {
            TargetPosition->Repair(20.0f * GetWorkRate());
        }
        CancelTask();
    }
}
