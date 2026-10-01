#include "StrategySpecialistStateComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Combat/StrategyDetachmentComponent.h"
#include "../AI/StrategyNCOComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../Combat/StrategyFireDrillComponent.h"
#include "../Engineering/StrategyWorkingPartyComponent.h"

UStrategySpecialistStateComponent::UStrategySpecialistStateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategySpecialistStateComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

FStrategySpecialistStateSnapshot
UStrategySpecialistStateComponent::CaptureSnapshot()
{
    FStrategySpecialistStateSnapshot Snapshot;
    if (!OwnerUnit)
    {
        return Snapshot;
    }

    Snapshot.UnitId = OwnerUnit->StableUnitId;
    Snapshot.UnitStrength = OwnerUnit->CurrentStrength;
    Snapshot.Cohesion = OwnerUnit->Cohesion;

    if (OwnerUnit->DetachmentComponent)
    {
        Snapshot.Detachments = OwnerUnit->DetachmentComponent->Detachments;
    }

    if (OwnerUnit->NCOComponent)
    {
        Snapshot.NCOStrength = OwnerUnit->NCOComponent->NCOStrength;
        Snapshot.NCOQuality = OwnerUnit->NCOComponent->NCOQuality;
        Snapshot.bOfficerAvailable = OwnerUnit->NCOComponent->bOfficerAvailable;
    }

    if (OwnerUnit->StanceComponent)
    {
        Snapshot.Stance = OwnerUnit->StanceComponent->Stance;
    }

    if (OwnerUnit->FireDrillComponent)
    {
        Snapshot.LoadingMethod = OwnerUnit->FireDrillComponent->LoadingMethod;
        Snapshot.FireDrillResearchLevel =
            OwnerUnit->FireDrillComponent->ResearchLevel;
        Snapshot.DrillMode = OwnerUnit->FireDrillComponent->DrillMode;
    }

    if (OwnerUnit->WorkingPartyComponent)
    {
        Snapshot.WorkingPartyWorkers = OwnerUnit->WorkingPartyComponent->AssignedWorkers;
        Snapshot.WorkingPartyTask = OwnerUnit->WorkingPartyComponent->ActiveTask;
        Snapshot.CarriedAmmunition = OwnerUnit->WorkingPartyComponent->CarriedAmmunitionRounds;
        Snapshot.WoundedWaiting = OwnerUnit->WorkingPartyComponent->WoundedWaitingCollection;
        Snapshot.WoundedCollected = OwnerUnit->WorkingPartyComponent->WoundedCollected;
    }

    LastSnapshot = Snapshot;
    return Snapshot;
}

bool UStrategySpecialistStateComponent::RestoreSnapshot(
    const FStrategySpecialistStateSnapshot& Snapshot)
{
    if (!OwnerUnit ||
        Snapshot.UnitId != OwnerUnit->StableUnitId ||
        Snapshot.UnitStrength < 0)
    {
        return false;
    }

    OwnerUnit->CurrentStrength = Snapshot.UnitStrength;
    OwnerUnit->Cohesion = FMath::Clamp(Snapshot.Cohesion, 0.0f, 100.0f);

    if (OwnerUnit->DetachmentComponent)
    {
        OwnerUnit->DetachmentComponent->Detachments = Snapshot.Detachments;
        OwnerUnit->DetachmentComponent->ReconcileInvalidDetachments();
    }

    if (OwnerUnit->NCOComponent)
    {
        OwnerUnit->NCOComponent->NCOStrength = FMath::Max(0, Snapshot.NCOStrength);
        OwnerUnit->NCOComponent->NCOQuality =
            FMath::Clamp(Snapshot.NCOQuality, 0.0f, 100.0f);
        OwnerUnit->NCOComponent->bOfficerAvailable = Snapshot.bOfficerAvailable;
    }

    if (OwnerUnit->StanceComponent)
    {
        OwnerUnit->StanceComponent->Stance = Snapshot.Stance;
    }

    if (OwnerUnit->FireDrillComponent)
    {
        OwnerUnit->FireDrillComponent->SetLoadingMethod(
            Snapshot.LoadingMethod);
        OwnerUnit->FireDrillComponent->SetResearchLevel(
            Snapshot.FireDrillResearchLevel);
        OwnerUnit->FireDrillComponent->SetDrillMode(
            Snapshot.DrillMode);
    }

    if (OwnerUnit->WorkingPartyComponent)
    {
        OwnerUnit->WorkingPartyComponent->CancelTask();
        OwnerUnit->WorkingPartyComponent->AssignedWorkers =
            FMath::Max(0, Snapshot.WorkingPartyWorkers);
        OwnerUnit->WorkingPartyComponent->ActiveTask = Snapshot.WorkingPartyTask;
        OwnerUnit->WorkingPartyComponent->CarriedAmmunitionRounds =
            FMath::Max(0, Snapshot.CarriedAmmunition);
        OwnerUnit->WorkingPartyComponent->WoundedWaitingCollection =
            FMath::Max(0, Snapshot.WoundedWaiting);
        OwnerUnit->WorkingPartyComponent->WoundedCollected =
            FMath::Max(0, Snapshot.WoundedCollected);

        OwnerUnit->WorkingPartyComponent->SetComponentTickEnabled(
            Snapshot.WorkingPartyTask != EStrategyWorkingPartyTask::None);
    }

    OwnerUnit->RefreshDebugLabel();
    LastSnapshot = Snapshot;

    FString Reason;
    return ValidateCurrentState(Reason);
}

bool UStrategySpecialistStateComponent::RestoreLastSnapshot()
{
    return RestoreSnapshot(LastSnapshot);
}

bool UStrategySpecialistStateComponent::ValidateCurrentState(
    FString& FailureReason) const
{
    if (!OwnerUnit)
    {
        FailureReason = TEXT("NO_OWNER");
        return false;
    }

    if (HasDetachedStrengthLeak())
    {
        FailureReason = TEXT("DETACHED_STRENGTH_LEAK");
        return false;
    }

    if (HasInvalidNCOState())
    {
        FailureReason = TEXT("INVALID_NCO_STATE");
        return false;
    }

    if (HasInvalidWorkingPartyState())
    {
        FailureReason = TEXT("INVALID_WORKING_PARTY_STATE");
        return false;
    }

    if (OwnerUnit->FireDrillComponent &&
        !OwnerUnit->FireDrillComponent->IsDrillModeUnlocked(
            OwnerUnit->FireDrillComponent->DrillMode))
    {
        FailureReason = TEXT("LOCKED_FIRE_DRILL_MODE");
        return false;
    }

    FailureReason = TEXT("OK");
    return true;
}

int32 UStrategySpecialistStateComponent::BuildDeterministicDigest() const
{
    if (!OwnerUnit)
    {
        return 0;
    }

    uint32 Hash = GetTypeHash(OwnerUnit->StableUnitId);
    Hash = HashCombine(Hash, GetTypeHash(OwnerUnit->CurrentStrength));
    Hash = HashCombine(Hash, GetTypeHash(FMath::RoundToInt(OwnerUnit->Cohesion * 10.0f)));

    if (OwnerUnit->FireDrillComponent)
    {
        Hash = HashCombine(
            Hash,
            GetTypeHash(
                static_cast<uint8>(
                    OwnerUnit->FireDrillComponent->ResearchLevel)));
        Hash = HashCombine(
            Hash,
            GetTypeHash(
                static_cast<uint8>(
                    OwnerUnit->FireDrillComponent->DrillMode)));
    }

    if (OwnerUnit->DetachmentComponent)
    {
        for (const FStrategyDetachmentRecord& Record :
             OwnerUnit->DetachmentComponent->Detachments)
        {
            if (!Record.bActive)
            {
                continue;
            }

            Hash = HashCombine(Hash, GetTypeHash(Record.DetachmentId));
            Hash = HashCombine(Hash, GetTypeHash(Record.CurrentStrength));
            Hash = HashCombine(Hash, GetTypeHash(Record.RemainingAmmoRounds));
        }
    }

    if (OwnerUnit->NCOComponent)
    {
        Hash = HashCombine(Hash, GetTypeHash(OwnerUnit->NCOComponent->NCOStrength));
        Hash = HashCombine(Hash, GetTypeHash(FMath::RoundToInt(OwnerUnit->NCOComponent->NCOQuality * 10.0f)));
    }

    return static_cast<int32>(Hash);
}

void UStrategySpecialistStateComponent::ResetSpecialistState()
{
    if (!OwnerUnit)
    {
        return;
    }

    if (OwnerUnit->DetachmentComponent)
    {
        OwnerUnit->DetachmentComponent->Detachments.Reset();
    }

    if (OwnerUnit->NCOComponent)
    {
        OwnerUnit->NCOComponent->NCOStrength = 12;
        OwnerUnit->NCOComponent->NCOQuality = 55.0f;
        OwnerUnit->NCOComponent->bOfficerAvailable = true;
    }

    if (OwnerUnit->StanceComponent)
    {
        OwnerUnit->StanceComponent->Stance = EStrategyStance::Standing;
    }

    if (OwnerUnit->FireDrillComponent)
    {
        OwnerUnit->FireDrillComponent->SetLoadingMethod(
            EStrategyLoadingMethod::MuzzleLoader);
        OwnerUnit->FireDrillComponent->SetResearchLevel(
            EStrategyFireDrillResearchLevel::FrontRankFire);
        OwnerUnit->FireDrillComponent->SetDrillMode(
            EStrategyFireDrillMode::FrontRank);
    }

    if (OwnerUnit->WorkingPartyComponent)
    {
        OwnerUnit->WorkingPartyComponent->CancelTask();
        OwnerUnit->WorkingPartyComponent->CarriedAmmunitionRounds = 0;
        OwnerUnit->WorkingPartyComponent->WoundedWaitingCollection = 0;
        OwnerUnit->WorkingPartyComponent->WoundedCollected = 0;
    }
}

bool UStrategySpecialistStateComponent::HasDetachedStrengthLeak() const
{
    return OwnerUnit &&
        OwnerUnit->DetachmentComponent &&
        OwnerUnit->DetachmentComponent->GetDetachedStrength() >
            OwnerUnit->CurrentStrength;
}

bool UStrategySpecialistStateComponent::HasInvalidNCOState() const
{
    return OwnerUnit &&
        OwnerUnit->NCOComponent &&
        (OwnerUnit->NCOComponent->NCOStrength < 0 ||
         OwnerUnit->NCOComponent->NCOQuality < 0.0f ||
         OwnerUnit->NCOComponent->NCOQuality > 100.0f);
}

bool UStrategySpecialistStateComponent::HasInvalidWorkingPartyState() const
{
    return OwnerUnit &&
        OwnerUnit->WorkingPartyComponent &&
        (OwnerUnit->WorkingPartyComponent->AssignedWorkers < 0 ||
         OwnerUnit->WorkingPartyComponent->AssignedWorkers >
            OwnerUnit->WorkingPartyComponent->AvailableWorkers ||
         OwnerUnit->WorkingPartyComponent->WoundedWaitingCollection < 0 ||
         OwnerUnit->WorkingPartyComponent->WoundedCollected < 0);
}

FString UStrategySpecialistStateComponent::BuildQAStatusLine() const
{
    FString Reason;
    const bool bValid = ValidateCurrentState(Reason);

    return FString::Printf(
        TEXT("SPECIALIST|%s|Unit=%s|Digest=%d|Detached=%d|NCO=%d|WP=%d"),
        bValid ? TEXT("OK") : *Reason,
        OwnerUnit ? *OwnerUnit->StableUnitId.ToString() : TEXT("NONE"),
        BuildDeterministicDigest(),
        OwnerUnit && OwnerUnit->DetachmentComponent
            ? OwnerUnit->DetachmentComponent->GetDetachedStrength()
            : 0,
        OwnerUnit && OwnerUnit->NCOComponent
            ? OwnerUnit->NCOComponent->NCOStrength
            : 0,
        OwnerUnit && OwnerUnit->WorkingPartyComponent
            ? OwnerUnit->WorkingPartyComponent->AssignedWorkers
            : 0);
}
