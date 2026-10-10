#include "StrategyContactComponent.h"

#include "StrategyVisibilityComponent.h"
#include "../Units/StrategyUnit.h"
#include "StrategySkirmisherComponent.h"
#include "../Terrain/StrategyTerrainAwarenessComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../AI/StrategyDecisionLog.h"
#include "Engine/World.h"
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
    const float ContactNow = GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds()) : 0.0f;
    for (FStrategyContactRecord& Contact : Contacts)
    {
        Contact.bCurrentlyVisible = false;
        Contact.SecondsSinceSeen += ElapsedSeconds;
        Contact.Confidence = FMath::Clamp(1.0f - Contact.SecondsSinceSeen /
            FMath::Max(0.01f, ForgetAfterSeconds), 0.0f, 1.0f);
        Contact.UncertaintyRadiusCm = Contact.SecondsSinceSeen * FMath::Max(0.0f, UncertaintyGrowthCmPerSecond);
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

        if (!OwnerUnit->VisibilityComponent->CanDetectTarget(Candidate, MaximumAwarenessRangeCm * AwarenessMultiplier))
        {
            continue;
        }

        FStrategyContactRecord* Existing =
            Contacts.FindByPredicate(
                [Candidate](const FStrategyContactRecord& Contact)
                {
                    return Candidate->StableUnitId.IsNone()
                        ? Contact.ObservedUnit.Get() == Candidate
                        : Contact.StableUnitId == Candidate->StableUnitId;
                });

        if (!Existing)
        {
            FStrategyContactRecord NewContact;
            NewContact.StableUnitId = Candidate->StableUnitId;
            const int32 ContactIndex = Contacts.Add(NewContact);
            Existing = &Contacts[ContactIndex];
        }
        Existing->ObservedUnit = Candidate;
        Existing->LastKnownPosition = Candidate->GetActorLocation();
        Existing->LastKnownLocation = Existing->LastKnownPosition;
        Existing->ObservedVelocity = Candidate->MovementExecutor
            ? Candidate->MovementExecutor->GetExecutedVelocity() : Candidate->GetVelocity();
        Existing->Heading = Candidate->GetActorForwardVector().GetSafeNormal2D();
        Existing->LastSeenTime = ContactNow;
        Existing->SecondsSinceSeen = 0.0f;
        Existing->Confidence = 1.0f;
        Existing->UncertaintyRadiusCm = 0.0f;
        Existing->Source = EStrategyContactSource::OwnEyes;
        Existing->bCurrentlyVisible = true;
    }

    Contacts.RemoveAll(
        [this](const FStrategyContactRecord& Contact)
        {
            return Contact.SecondsSinceSeen > FMath::Max(0.0f, ForgetAfterSeconds);
        });

    for (const FStrategyContactRecord& Contact : Contacts)
    {
        STRATEGY1864_DECISION(OwnerUnit, TEXT("Kontakt"), Contact.bCurrentlyVisible ? TEXT("Observation") : TEXT("Hukommelse"),
            TEXT("Lokalt synsbillede; mistet kontakt ældes uden at følge aktøren"),
            FString::Printf(TEXT("contact=%s age=%.2f lastSeen=%.2f confidence=%.3f uncertaintyCm=%.1f position=%s heading=%s source=%d"),
                *Contact.StableUnitId.ToString(), Contact.SecondsSinceSeen, Contact.LastSeenTime, Contact.Confidence,
                Contact.UncertaintyRadiusCm, *Contact.LastKnownPosition.ToCompactString(), *Contact.Heading.ToCompactString(), int32(Contact.Source)),
            TEXT("skjult aktørposition: ikke opdateret; rapport/HQ: transport endnu ikke implementeret"));
    }
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
                return Contact.ObservedUnit.Get() == Target;
            });

    return Found && Found->bCurrentlyVisible && Found->Confidence > 0.0f;
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
