#include "StrategyContactComponent.h"

#include "StrategyVisibilityComponent.h"
#include "../Units/StrategyUnit.h"
#include "StrategySkirmisherComponent.h"
#include "../Terrain/StrategyTerrainAwarenessComponent.h"
#include "EngineUtils.h"

UStrategyContactComponent::UStrategyContactComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyContactComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

void UStrategyContactComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit || !OwnerUnit->VisibilityComponent)
    {
        return;
    }

    ScanAccumulator += DeltaTime;
    if (ScanAccumulator < ScanIntervalSeconds)
    {
        return;
    }

    const float Elapsed = ScanAccumulator;
    ScanAccumulator = 0.0f;
    RefreshContacts(Elapsed);
}

void UStrategyContactComponent::RefreshContacts(float ElapsedSeconds)
{
    for (FStrategyContactRecord& Contact : Contacts)
    {
        Contact.bCurrentlyVisible = false;
        Contact.SecondsSinceSeen += ElapsedSeconds;
    }

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Candidate = *It;

        if (!IsValid(Candidate) ||
            Candidate == OwnerUnit ||
            Candidate->Side == EStrategySide::Neutral ||
            Candidate->Side == OwnerUnit->Side ||
            !Candidate->IsCombatEffective())
        {
            continue;
        }

        const float DistanceCm =
            FVector::Dist2D(
                OwnerUnit->GetActorLocation(),
                Candidate->GetActorLocation());

        const float SkirmisherMultiplier =
            OwnerUnit->SkirmisherComponent
            ? OwnerUnit->SkirmisherComponent->GetAwarenessRangeMultiplier()
            : 1.0f;

        const float TerrainMultiplier =
            OwnerUnit->TerrainAwarenessComponent
            ? OwnerUnit->TerrainAwarenessComponent
                ->GetObservationRangeMultiplierTo(Candidate)
            : 1.0f;

        const float AwarenessMultiplier =
            SkirmisherMultiplier * TerrainMultiplier;

        if (DistanceCm > MaximumAwarenessRangeCm * AwarenessMultiplier ||
            !OwnerUnit->VisibilityComponent->HasLineOfSightTo(Candidate))
        {
            continue;
        }

        FStrategyContactRecord* Existing =
            Contacts.FindByPredicate(
                [Candidate](const FStrategyContactRecord& Contact)
                {
                    return Contact.StableUnitId == Candidate->StableUnitId;
                });

        if (!Existing)
        {
            FStrategyContactRecord NewContact;
            NewContact.StableUnitId = Candidate->StableUnitId;
            NewContact.LastKnownLocation = Candidate->GetActorLocation();
            NewContact.SecondsSinceSeen = 0.0f;
            NewContact.bCurrentlyVisible = true;
            Contacts.Add(NewContact);
        }
        else
        {
            Existing->LastKnownLocation = Candidate->GetActorLocation();
            Existing->SecondsSinceSeen = 0.0f;
            Existing->bCurrentlyVisible = true;
        }
    }

    Contacts.RemoveAll(
        [this](const FStrategyContactRecord& Contact)
        {
            return Contact.SecondsSinceSeen > ForgetAfterSeconds;
        });
}

bool UStrategyContactComponent::HasCurrentContact(
    const AStrategyUnit* Target) const
{
    if (!IsValid(Target))
    {
        return false;
    }

    const FStrategyContactRecord* Found =
        Contacts.FindByPredicate(
            [Target](const FStrategyContactRecord& Contact)
            {
                return Contact.StableUnitId == Target->StableUnitId;
            });

    return Found && Found->bCurrentlyVisible;
}

bool UStrategyContactComponent::GetLastKnownContact(
    FName StableUnitId,
    FStrategyContactRecord& OutRecord) const
{
    const FStrategyContactRecord* Found =
        Contacts.FindByPredicate(
            [StableUnitId](const FStrategyContactRecord& Contact)
            {
                return Contact.StableUnitId == StableUnitId;
            });

    if (!Found)
    {
        return false;
    }

    OutRecord = *Found;
    return true;
}

TArray<FStrategyContactRecord> UStrategyContactComponent::GetKnownContacts() const
{
    return Contacts;
}
