#include "StrategyInfantryVisualComponent.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Units/StrategyCompanyUnit.h"
#include "StrategyEquipmentVisualComponent.h"
#include "StrategyHumanAnimationStateComponent.h"
#include "StrategyMuzzleSmokePuff.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "../Formations/StrategyFormationComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "StrategyCrowdModel.h"
#include "../Combat/StrategyFireDrillComponent.h"
#include "StrategyBattleBlast.h"
#include "StrategyUniformAppearanceComponent.h"

UStrategyInfantryVisualComponent::UStrategyInfantryVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = RefreshIntervalSeconds;

    SoldierMeshAsset = TSoftObjectPtr<USkeletalMesh>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864.SK_DK_Livgarden_1864")));

    RifleMeshAsset = TSoftObjectPtr<UStaticMesh>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Weapons/SM_Rifle_1.SM_Rifle_1")));

    RifleBayonetMeshAsset = TSoftObjectPtr<UStaticMesh>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Weapons/SM_Rifle_Bayonet_1.SM_Rifle_Bayonet_1")));

    IdleStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_Idle.A_Combat_Idle")));

    WalkStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_Walk.A_Combat_Walk")));

    RunStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Running.A_Running")));

    AimStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_Aim.A_Combat_Aim")));

    FireStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_Fire.A_Combat_Fire")));

    ReloadStandingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_ReloadStanding.A_Combat_ReloadStanding")));

    IdleKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Rifle_Kneel_Idle.A_Rifle_Kneel_Idle")));

    AimKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Idle_Crouching_Aiming.A_Idle_Crouching_Aiming")));

    FireKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Fire_Rifle.A_Fire_Rifle")));

    ReloadKneelingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Reload_sitting.A_Reload_sitting")));

    IdleProneAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Prone_Idle.A_Prone_Idle")));

    CrawlProneAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Crawl_Forward.A_Crawl_Forward")));

    FireProneAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Prone_Firing_Rifle.A_Prone_Firing_Rifle")));

    ReloadProneAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Prone_Reloading.A_Prone_Reloading")));

    BayonetChargeAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Charge.A_Charge")));

    BayonetThrustAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Bayonet_Stab.A_Bayonet_Stab")));

    DeathAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_DeathFront.A_Combat_DeathFront")));
    DeathAsset2 = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_DeathHeadshot.A_Combat_DeathHeadshot")));
    DeathAsset3 = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_DeathBack.A_Combat_DeathBack")));
    RaiseToAimAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Rifle_Down_To_Aim.A_Rifle_Down_To_Aim")));
    LoadAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_ReloadStanding.A_Combat_ReloadStanding")));
    RiseFromLoadAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Rifle_Kneel_To_Stand.A_Rifle_Kneel_To_Stand")));
    ReadyAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Rifle_Idle.A_Rifle_Idle")));
    AimHoldAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Rifle_Aiming_Idle.A_Rifle_Aiming_Idle")));
    DeathWalkingAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Danish/Livgarden1864/Animations/A_Walking_To_Dying.A_Walking_To_Dying")));
}

void UStrategyInfantryVisualComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCompany = Cast<AStrategyCompanyUnit>(GetOwner());

    if (OwnerCompany &&
        OwnerCompany->CombatComponent)
    {
        OwnerCompany->CombatComponent->OnVolleyVisualEvent.AddDynamic(
            this,
            &UStrategyInfantryVisualComponent::HandleVolleyVisualEvent);
    }

    PrimaryComponentTick.TickInterval =
        FMath::Max(0.02f, RefreshIntervalSeconds);

    if (bEnabled)
    {
        RefreshVisuals();
    }
}

void UStrategyInfantryVisualComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    if (OwnerCompany &&
        OwnerCompany->CombatComponent)
    {
        OwnerCompany->CombatComponent->OnVolleyVisualEvent.RemoveDynamic(
            this,
            &UStrategyInfantryVisualComponent::HandleVolleyVisualEvent);
    }

    DestroyVisualComponents();

    Super::EndPlay(EndPlayReason);
}

void UStrategyInfantryVisualComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction);

    if (!bEnabled ||
        !OwnerCompany ||
        !LoadedSoldierMesh)
    {
        return;
    }

    const int32 CurrentStrength =
        FMath::Max(0, OwnerCompany->CurrentStrength);

    const uint8 CurrentFormationValue =
        OwnerCompany->FormationComponent
        ? static_cast<uint8>(
            OwnerCompany->FormationComponent->CurrentFormation)
        : 255;

    if (CachedStrength != CurrentStrength)
    {
        const int32 Desired = GetDesiredVisualCount();
        const int32 Standing = SoldierComponents.Num() - PendingKillCount;
        if (bLeaveCorpses && CachedStrength != INDEX_NONE && Desired < Standing)
        {
            QueueKills(Standing - Desired);
        }
        EnsureVisualCount(Desired + PendingKillCount);
        RebuildFormation();
        CachedStrength = CurrentStrength;
    }

    const float CoverVisualLateralSpacing = OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierLateralSpacingCm : 0.0f;
    const float CoverVisualRankSpacing = OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierRankSpacingCm : 0.0f;
    if (CachedFormationValue != CurrentFormationValue || CachedCoverLateralSpacing != CoverVisualLateralSpacing ||
        CachedCoverRankSpacing != CoverVisualRankSpacing)
    {
        RebuildFormation();
        CachedFormationValue = CurrentFormationValue;
        CachedCoverLateralSpacing = CoverVisualLateralSpacing;
        CachedCoverRankSpacing = CoverVisualRankSpacing;
    }

    const bool bBayonetFixed =
        OwnerCompany->EquipmentVisualComponent &&
        OwnerCompany->EquipmentVisualComponent->bBayonetFixed;

    if (bCachedBayonetFixed != bBayonetFixed)
    {
        RefreshWeaponMeshes();
        bCachedBayonetFixed = bBayonetFixed;
    }

    // The men stand for the company: the grey block of the QA view goes (it shows inside a square).
    if (OwnerCompany->QAPlaceholderMesh && OwnerCompany->QAPlaceholderMesh->IsVisible())
    {
        OwnerCompany->QAPlaceholderMesh->SetVisibility(false);
    }
    ProcessPendingKills();
    UpdateSettling(DeltaTime);
    ApplyPendingStance(GetWorld()->GetTimeSeconds());
    MarchDust(GetWorld()->GetTimeSeconds());
    RefreshAnimation(false);
    UpdatePersonalActions();
    UpdateCrowdMode();
    if (!bCrowdMode)
    {
        AlignWeapons();
    }
    UpdateFormationBounds();
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864AnimationAudit")))
    {
        const float Now = GetWorld()->GetTimeSeconds();
        if (FMath::FloorToInt(Now / 10.0f) != FMath::FloorToInt((Now - DeltaTime) / 10.0f))
        {
            int32 Walking = 0, Firing = 0, Loading = 0;
            for (const FPlayedClip& Clip : SoldierClips)
            {
                const FString Name = Clip.Clip.IsValid() ? Clip.Clip->GetName() : FString();
                Walking += Name.Contains(TEXT("Walk")) ? 1 : 0;
                Firing += Name.Contains(TEXT("Fire")) || Name.Contains(TEXT("Firing")) ? 1 : 0;
                Loading += Name.Contains(TEXT("Reload")) ? 1 : 0;
            }
            UE_LOG(LogTemp, Display, TEXT("COMBAT-ANIMATION-AUDIT %s strength=%d walk=%d fire=%d reload=%d corpses=%d"),
                *OwnerCompany->DisplayName.ToString(), CurrentStrength, Walking, Firing, Loading, CorpseComponents.Num());
        }
    }
}

void UStrategyInfantryVisualComponent::SetEnabled(
    bool bNewEnabled)
{
    if (bEnabled == bNewEnabled &&
        (bEnabled ? SoldierComponents.Num() > 0 : true))
    {
        return;
    }

    bEnabled = bNewEnabled;

    if (!bEnabled)
    {
        DestroyVisualComponents();

        if (OwnerCompany &&
            OwnerCompany->QAPlaceholderMesh)
        {
            OwnerCompany->QAPlaceholderMesh->SetVisibility(true, true);
        }

        return;
    }

    bLoadAttempted = false;
    RefreshVisuals();
}

void UStrategyInfantryVisualComponent::SetVisualScaleDivisor(
    int32 NewDivisor)
{
    const int32 NormalizedDivisor =
        NewDivisor <= 1
        ? 1
        : NewDivisor <= 2
            ? 2
            : NewDivisor <= 5
                ? 5
                : 10;

    if (VisualScaleDivisor == NormalizedDivisor)
    {
        return;
    }

    VisualScaleDivisor = NormalizedDivisor;

    if (bEnabled)
    {
        EnsureVisualCount(GetDesiredVisualCount());
        RebuildFormation();
        RefreshAnimation(true);
        UpdateFormationBounds();
    }
}

void UStrategyInfantryVisualComponent::RefreshVisuals()
{
    if (!bEnabled)
    {
        return;
    }

    if (!OwnerCompany)
    {
        OwnerCompany = Cast<AStrategyCompanyUnit>(GetOwner());
    }

    if (!LoadedSoldierMesh)
    {
        bLoadAttempted = false;
    }

    if (!OwnerCompany ||
        !EnsureAssetsLoaded())
    {
        return;
    }

    EnsureVisualCount(GetDesiredVisualCount());
    RebuildFormation();
    RefreshWeaponMeshes();
    RefreshAnimation(true);
    UpdateFormationBounds();

    CachedStrength =
        FMath::Max(0, OwnerCompany->CurrentStrength);

    CachedFormationValue =
        OwnerCompany->FormationComponent
        ? static_cast<uint8>(
            OwnerCompany->FormationComponent->CurrentFormation)
        : 255;

    bCachedBayonetFixed =
        OwnerCompany->EquipmentVisualComponent &&
        OwnerCompany->EquipmentVisualComponent->bBayonetFixed;

    if (OwnerCompany->QAPlaceholderMesh)
    {
        OwnerCompany->QAPlaceholderMesh->SetVisibility(false, true);
    }
}

void UStrategyInfantryVisualComponent::HandleVolleyVisualEvent(
    FVector Origin,
    FVector Direction,
    int32 Shots,
    int32 Hits)
{
    if (!bEnabled ||
        !OwnerCompany ||
        !OwnerCompany->HumanAnimationStateComponent)
    {
        return;
    }

    // Every soldier of the volley fires his own shot, a little apart from the next: raise, fire, smoke.
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const int32 Count = SoldierComponents.Num();
    SoldierFireAt.SetNumZeroed(Count);
    SoldierBusyUntil.SetNumZeroed(Count);
    SoldierFirePhase.SetNumZeroed(Count);
    const int32 Firing = FMath::Clamp(Shots / FMath::Max(1, VisualScaleDivisor), 1, Count);
    // Those who can bear on the target first (their own place, angle and range); the rest only to make up the number.
    TArray<int32> Order, Others;
    const AStrategyUnit* Target = OwnerCompany->CombatComponent ? OwnerCompany->CombatComponent->LastVolleyTarget.Get() : nullptr;
    const UStrategyFireControlComponent* Fire = OwnerCompany->FireControlComponent;
    // The fire drill: only the rank(s) whose turn it is (fire by rank: the one rank; front rank: the first;
    // two-rank: the first two; volley and independent fire: everyone).
    const UStrategyFireDrillComponent* Drill = OwnerCompany->FireDrillComponent;
    for (int32 i = 0; i < Count; ++i)
    {
        if (Drill && SoldierSlots.IsValidIndex(i) && !Drill->IsFormationSlotEligibleToFire(SoldierSlots[i]))
        {
            continue;
        }
        const bool bBears = !Target || !Fire || !SoldierComponents[i] ||
            Fire->CanPointBearOn(SoldierComponents[i]->GetComponentLocation(), OwnerCompany->GetActorForwardVector(), Target, Fire->GetActiveRangeCm());
        (bBears ? Order : Others).Add(i);
    }
    // How the shots spread: a volley goes off together, fire by rank in a ripple, independent fire man by man.
    const EStrategyFireDrillMode Mode = Drill ? Drill->DrillMode : EStrategyFireDrillMode::FrontRank;
    const float Spread = Mode == EStrategyFireDrillMode::Volley ? 0.2f : Mode == EStrategyFireDrillMode::Independent ? 2.5f : 0.6f;
    for (int32 i = Order.Num() - 1; i > 0; --i) { Order.Swap(i, FMath::RandRange(0, i)); }
    for (int32 k = 0; k < Firing && k < Order.Num(); ++k)
    {
        const int32 i = Order[k];
        // Those standing loaded (company animation, ready or aiming) fire; the loading do not.
        if (SoldierFirePhase[i] == 0 || SoldierFirePhase[i] == 5 || SoldierFirePhase[i] == 6)
        {
            SoldierFireAt[i] = Now + FMath::FRandRange(0.0f, Spread);
        }
    }
}

void UStrategyInfantryVisualComponent::PlayOnSoldier(USkeletalMeshComponent* Soldier, UAnimSequence* Sequence, bool bLooping, bool bRandomStart)
{
    if (!Soldier || !Sequence)
    {
        return;
    }
    Soldier->bPauseAnims = false;
    Soldier->PlayAnimation(Sequence, bLooping);
    // Not all in step: each soldier starts somewhere in a looping animation and at his own pace.
    const float Position = bRandomStart ? FMath::FRandRange(0.0f, Sequence->GetPlayLength()) : 0.0f;
    const float Rate = bRandomStart ? FMath::FRandRange(0.9f, 1.1f) : 1.0f;
    Soldier->SetPosition(Position, false);
    Soldier->SetPlayRate(Rate);
    RecordClip(Soldier, Sequence, bLooping, Position, Rate);
}

namespace
{
    // Each soldier's firing cycle (only in the firing line; marching, the company's animation rules).
    enum EFirePhase : uint8 { PhaseNone = 0, PhaseRaise = 1, PhaseFire = 2, PhaseLoad = 3, PhaseRise = 4, PhaseReady = 5, PhaseAim = 6 };
}

bool UStrategyInfantryVisualComponent::IsEnemyInRange() const
{
    const UStrategyFireControlComponent* Fire = OwnerCompany ? OwnerCompany->FireControlComponent.Get() : nullptr;
    if (!Fire || !GetWorld())
    {
        return false;
    }
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It) && It->Side != OwnerCompany->Side && It->Side != EStrategySide::Neutral && It->IsCombatEffective() &&
            Fire->IsLocationInsideFireField(It->GetActorLocation(), Fire->GetActiveRangeCm()))
        {
            return true;
        }
    }
    return false;
}

bool UStrategyInfantryVisualComponent::IsInFiringLine() const
{
    if (!OwnerCompany || !OwnerCompany->FireControlComponent || !GetWorld())
    {
        return false;
    }
    const bool bMoving = OwnerCompany->HumanAnimationStateComponent &&
        (OwnerCompany->HumanAnimationStateComponent->CurrentAction == EStrategyHumanAnimationAction::Walk ||
         OwnerCompany->HumanAnimationStateComponent->CurrentAction == EStrategyHumanAnimationAction::Run);
    const bool bLine = !OwnerCompany->FormationComponent || OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Line;
    if (bMoving || !bLine)
    {
        return false;
    }
    // An enemy within the long range ahead: stand ready.
    const UStrategyFireControlComponent* Fire = OwnerCompany->FireControlComponent;
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It) && It->Side != OwnerCompany->Side && It->Side != EStrategySide::Neutral && It->IsCombatEffective() &&
            Fire->IsLocationInsideFireField(It->GetActorLocation(), Fire->LongRangeCm * 1.3f))
        {
            return true;
        }
    }
    return false;
}

void UStrategyInfantryVisualComponent::UpdatePersonalActions()
{
    UWorld* World = GetWorld();
    if (!World || SoldierFireAt.Num() != SoldierComponents.Num())
    {
        return;
    }
    const float Now = World->GetTimeSeconds();
    const EStrategyStance Stance = OwnerCompany && OwnerCompany->StanceComponent ? OwnerCompany->StanceComponent->Stance : EStrategyStance::Standing;
    const bool bLine = IsInFiringLine();
    const bool bInRange = bLine && IsEnemyInRange();
    // The company's loading time (the combat core's reload: every man reloads together after a volley).
    const float ReloadLeft = OwnerCompany && OwnerCompany->CombatComponent ? OwnerCompany->CombatComponent->ReloadRemainingSeconds : 0.0f;
    UAnimSequence* Load = (Stance == EStrategyStance::Prone ? ReloadProneAsset :
        Stance == EStrategyStance::Kneeling ? ReloadKneelingAsset : ReloadStandingAsset).LoadSynchronous();
    UAnimSequence* Rise = Stance == EStrategyStance::Kneeling ? RiseFromLoadAsset.LoadSynchronous() : nullptr;
    UAnimSequence* Ready = ReadyAsset.LoadSynchronous();
    UAnimSequence* AimHold = AimHoldAsset.LoadSynchronous();
    UAnimSequence* Raise = RaiseToAimAsset.LoadSynchronous();
    const float RiseLength = Rise ? Rise->GetPlayLength() : 0.0f;
    for (int32 i = 0; i < SoldierComponents.Num(); ++i)
    {
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        if (!Soldier || (SoldierSettle.IsValidIndex(i) && SoldierSettle[i].bActive))
        {
            continue;
        }
        uint8& Phase = SoldierFirePhase[i];
        // Out of the firing line (marching, in column): the company's animation; a pending shot is dropped.
        if (!bLine && Phase >= PhaseLoad)
        {
            Phase = PhaseNone;
            bool bLooping = true;
            PlayOnSoldier(Soldier, ResolveAnimation(bLooping), bLooping, true);
            continue;
        }
        // Into the firing line: loaded (the muskets came loaded), so ready.
        if (bLine && Phase == PhaseNone && SoldierFireAt[i] <= 0.0f)
        {
            Phase = PhaseReady;
            PlayOnSoldier(Soldier, Ready, true, true);
            continue;
        }
        // The shot: from aim straight, otherwise raise first.
        if (SoldierFireAt[i] > 0.0f && Now >= SoldierFireAt[i] && (Phase == PhaseNone || Phase == PhaseReady || Phase == PhaseAim))
        {
            SoldierFireAt[i] = 0.0f;
            if (Phase == PhaseAim || !Raise || Stance != EStrategyStance::Standing)
            {
                Phase = PhaseRaise;
                SoldierBusyUntil[i] = Now;
            }
            else
            {
                PlayOnSoldier(Soldier, Raise, false, false);
                Phase = PhaseRaise;
                SoldierBusyUntil[i] = Now + Raise->GetPlayLength() * 0.85f;
            }
            continue;
        }
        switch (Phase)
        {
            case PhaseRaise:
                if (Now >= SoldierBusyUntil[i])
                {
                    UAnimSequence* Fire = (Stance == EStrategyStance::Prone ? FireProneAsset : Stance == EStrategyStance::Kneeling ? FireKneelingAsset : FireStandingAsset).LoadSynchronous();
                    PlayOnSoldier(Soldier, Fire, false, false);
                    Phase = PhaseFire;
                    SoldierBusyUntil[i] = Now + (Fire ? Fire->GetPlayLength() : 0.6f);
                    SpawnMuzzleSmoke(Soldier, i);
                }
                break;
            case PhaseFire:
                if (Now >= SoldierBusyUntil[i])
                {
                    if (bLine && Load)
                    {
                        // One loading cycle fills the actual reload interval. Standing soldiers stay standing.
                        PlayOnSoldier(Soldier, Load, false, false);
                        Phase = PhaseLoad;
                        const float MusketReload = OwnerCompany && OwnerCompany->CombatComponent ? OwnerCompany->CombatComponent->ReloadSeconds : 18.0f;
                        const float Duration = FMath::Max(0.1f, MusketReload * FMath::FRandRange(0.9f, 1.1f));
                        const float Rate = Load->GetPlayLength() / Duration;
                        Soldier->SetPlayRate(Rate);
                        RecordClip(Soldier, Load, false, 0.0f, Rate);
                        SoldierBusyUntil[i] = Now + Duration;
                    }
                    else
                    {
                        Phase = PhaseNone;
                        bool bLooping = true;
                        PlayOnSoldier(Soldier, ResolveAnimation(bLooping), bLooping, true);
                    }
                }
                break;
            case PhaseLoad:
                // Up again when his loading is almost done (and the company is not still reloading as one).
                if (Now >= SoldierBusyUntil[i] - RiseLength - 0.2f && ReloadLeft <= RiseLength + 0.2f)
                {
                    if (Rise)
                    {
                        PlayOnSoldier(Soldier, Rise, false, false);
                        SoldierBusyUntil[i] = Now + RiseLength;
                    }
                    else
                    {
                        SoldierBusyUntil[i] = Now;
                    }
                    Phase = PhaseRise;
                }
                break;
            case PhaseRise:
                if (Now >= SoldierBusyUntil[i])
                {
                    PlayOnSoldier(Soldier, Ready, true, true);
                    Phase = PhaseReady;
                }
                break;
            case PhaseReady:
                // Present arms when the enemy is inside the chosen range.
                if (bInRange && Raise)
                {
                    PlayOnSoldier(Soldier, Raise, false, false);
                    SoldierBusyUntil[i] = Now + Raise->GetPlayLength();
                    Phase = PhaseAim;
                }
                break;
            case PhaseAim:
                if (Now >= SoldierBusyUntil[i] && AimHold && Soldier->GetAnimationMode() == EAnimationMode::AnimationSingleNode &&
                    Soldier->GetSingleNodeInstance() && Soldier->GetSingleNodeInstance()->GetAnimationAsset() != AimHold)
                {
                    PlayOnSoldier(Soldier, AimHold, true, true);
                }
                if (!bInRange)
                {
                    PlayOnSoldier(Soldier, Ready, true, true);   // the enemy went out of range: order arms
                    Phase = PhaseReady;
                }
                break;
            default:
                break;
        }
    }
}

void UStrategyInfantryVisualComponent::SpawnMuzzleSmoke(const USkeletalMeshComponent* Soldier, int32 Index)
{
    UWorld* World = GetWorld();
    if (!bMuzzleSmoke || !World || !Soldier)
    {
        return;
    }
    // At the muzzle: the far end of the rifle, else ahead of the soldier at shoulder height.
    FVector Muzzle = Soldier->GetComponentLocation() + Soldier->GetRightVector() * -70.0f + FVector(0.0f, 0.0f, 150.0f);
    FVector Forward = OwnerCompany ? OwnerCompany->GetActorForwardVector() : FVector::ForwardVector;
    if (WeaponComponents.IsValidIndex(Index) && WeaponComponents[Index] && WeaponComponents[Index]->GetStaticMesh())
    {
        const UStaticMeshComponent* Weapon = WeaponComponents[Index];
        const FBox Box = Weapon->GetStaticMesh()->GetBoundingBox();
        const float Tip = bRifleBarrelAlongNegativeX ? Box.Min.X : Box.Max.X;
        Muzzle = Weapon->GetComponentTransform().TransformPosition(FVector(Tip, 0.0f, 0.0f));
        Forward = Weapon->GetForwardVector() * (bRifleBarrelAlongNegativeX ? -1.0f : 1.0f);
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AStrategyMuzzleSmokePuff* Puff = World->SpawnActor<AStrategyMuzzleSmokePuff>(AStrategyMuzzleSmokePuff::StaticClass(), Muzzle + Forward * 40.0f, FRotator::ZeroRotator, Params))
    {
        Puff->Drift = Forward * 60.0f + FVector(FMath::FRandRange(-15.0f, 15.0f), FMath::FRandRange(-15.0f, 15.0f), 18.0f);
    }
}

void UStrategyInfantryVisualComponent::MarchDust(float Now)
{
    if (Now < NextMarchDust || !OwnerCompany || !OwnerCompany->HumanAnimationStateComponent)
    {
        return;
    }
    const EStrategyHumanAnimationAction Action = OwnerCompany->HumanAnimationStateComponent->CurrentAction;
    if (Action != EStrategyHumanAnimationAction::Walk && Action != EStrategyHumanAnimationAction::Run)
    {
        return;
    }
    NextMarchDust = Now + FMath::FRandRange(0.6f, 1.2f);
    // Only near the camera (a sight for the commander, nothing for the far field).
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (PC->PlayerCameraManager && FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), OwnerCompany->GetActorLocation()) > 40000.0f)
        {
            return;
        }
    }
    const FVector Fwd = OwnerCompany->GetActorForwardVector().GetSafeNormal2D();
    const FVector Right(-Fwd.Y, Fwd.X, 0.0f);
    const FVector At = OwnerCompany->GetActorLocation() - Fwd * 500.0f + Right * FMath::FRandRange(-1200.0f, 1200.0f);
    AStrategyBattleBlast::Spawn(GetWorld(), EStrategyBlastKind::HoofDust, At + FVector(0.0f, 0.0f, 20.0f), Fwd, 3.2f);
}

void UStrategyInfantryVisualComponent::QueueKills(int32 Count)
{
    UWorld* World = GetWorld();
    const float Now = World ? World->GetTimeSeconds() : 0.0f;
    FStrategyImpactRegistry::FImpact Impact;
    if (World && OwnerCompany && FStrategyImpactRegistry::Find(World, OwnerCompany->GetActorLocation(), 6500.0f, Now, Impact))
    {
        if (Impact.LandTime > Now)
        {
            PendingKills.Add({ Count, Impact.Location, Impact.LandTime, Impact.bCone, Impact.Origin });
            PendingKillCount += Count;
            return;
        }
        KillSoldiers(Count, &Impact.Location, Impact.bCone ? &Impact.Origin : nullptr);
        return;
    }
    KillSoldiers(Count);
}

void UStrategyInfantryVisualComponent::ProcessPendingKills()
{
    if (PendingKills.Num() == 0)
    {
        return;
    }
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    bool bAny = false;
    for (int32 p = PendingKills.Num() - 1; p >= 0; --p)
    {
        if (Now >= PendingKills[p].Time || Now < PendingKills[p].Time - 10.0f)
        {
            const FPendingKill Due = PendingKills[p];
            PendingKills.RemoveAt(p);
            PendingKillCount = FMath::Max(0, PendingKillCount - Due.Count);
            KillSoldiers(Due.Count, &Due.Location, Due.bCone ? &Due.Origin : nullptr);
            bAny = true;
        }
    }
    if (bAny)
    {
        RebuildFormation();
    }
}

void UStrategyInfantryVisualComponent::KillSoldiers(int32 Count, const FVector* Near, const FVector* ConeOrigin)
{
    // The hit: random men of the company fall where they stand and stay there (not with the company).
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const bool bMoving = OwnerCompany && OwnerCompany->HumanAnimationStateComponent &&
        (OwnerCompany->HumanAnimationStateComponent->CurrentAction == EStrategyHumanAnimationAction::Walk ||
         OwnerCompany->HumanAnimationStateComponent->CurrentAction == EStrategyHumanAnimationAction::Run);
    for (int32 k = 0; k < Count && SoldierComponents.Num() > 0; ++k)
    {
        int32 i = FMath::RandRange(0, SoldierComponents.Num() - 1);
        if (Near && ConeOrigin)
        {
            // Case shot: anyone in the cone from the gun (about 11 degrees either side of its line) may fall;
            // outside it, no one. If the cone is empty of men, those nearest its line.
            const FVector Axis = (*Near - *ConeOrigin).GetSafeNormal2D();
            TArray<int32> Inside;
            float BestOff = TNumericLimits<float>::Max();
            int32 Nearest = i;
            for (int32 c = 0; c < SoldierComponents.Num(); ++c)
            {
                if (!SoldierComponents[c]) { continue; }
                const FVector To = (SoldierComponents[c]->GetComponentLocation() - *ConeOrigin).GetSafeNormal2D();
                const float Cos = FVector::DotProduct(To, Axis);
                if (Cos > 0.981f) { Inside.Add(c); }
                if (1.0f - Cos < BestOff) { BestOff = 1.0f - Cos; Nearest = c; }
            }
            i = Inside.Num() > 0 ? Inside[FMath::RandRange(0, Inside.Num() - 1)] : Nearest;
        }
        else if (Near)
        {
            // A shot: the men nearest where it struck (a little scattered).
            float Best = TNumericLimits<float>::Max();
            for (int32 c = 0; c < SoldierComponents.Num(); ++c)
            {
                if (!SoldierComponents[c]) { continue; }
                const float D = FVector::Dist2D(SoldierComponents[c]->GetComponentLocation(), *Near) + FMath::FRandRange(0.0f, 250.0f);
                if (D < Best) { Best = D; i = c; }
            }
        }
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        if (Soldier)
        {
            Soldier->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            const int32 Variant = Near && FMath::FRand() < 0.6f ? 2 : FMath::RandRange(0, 2);
            const TSoftObjectPtr<UAnimSequence>& Death = bMoving && !DeathWalkingAsset.IsNull() && FMath::FRand() < 0.5f ? DeathWalkingAsset
                : Variant == 0 ? DeathAsset : Variant == 1 ? DeathAsset2 : DeathAsset3;
            const float Rate = FMath::FRandRange(0.9f, 1.1f);
            Soldier->SetPlayRate(Rate);
            Soldier->bPauseAnims = false;
            Soldier->PlayAnimation(Death.LoadSynchronous(), false);
            CorpseComponents.Add(Soldier);
            FPlayedClip Fallen;
            Fallen.Clip = Death.LoadSynchronous();
            Fallen.Start = Now;
            Fallen.Rate = Rate;
            Fallen.bLoop = false;
            CorpseClips.Add(Fallen);
            bCrowdDirty = true;
        }
        SoldierComponents.RemoveAt(i);
        if (SoldierClips.IsValidIndex(i)) { SoldierClips.RemoveAt(i); }
        if (SoldierSlots.IsValidIndex(i)) { SoldierSlots.RemoveAt(i); }
        if (WeaponComponents.IsValidIndex(i))
        {
            WeaponComponents.RemoveAt(i);   // the rifle stays in the fallen man's hand
        }
        if (SoldierFireAt.IsValidIndex(i)) { SoldierFireAt.RemoveAt(i); }
        if (SoldierBusyUntil.IsValidIndex(i)) { SoldierBusyUntil.RemoveAt(i); }
        if (SoldierFirePhase.IsValidIndex(i)) { SoldierFirePhase.RemoveAt(i); }
    }
}

void UStrategyInfantryVisualComponent::AlignWeapons()
{
    if (!bAlignRifleBetweenHands)
    {
        return;
    }
    for (int32 i = 0; i < SoldierComponents.Num() && i < WeaponComponents.Num(); ++i)
    {
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        UStaticMeshComponent* Weapon = WeaponComponents[i];
        if (!Soldier || !Weapon || !Weapon->GetStaticMesh())
        {
            continue;
        }
        const FVector Right = Soldier->GetSocketLocation(RightHandBoneName);
        const FVector Left = Soldier->GetSocketLocation(LeftHandBoneName);
        FVector Dir = Left - Right;
        if (Dir.Size() < 12.0f)
        {
            Weapon->SetRelativeTransform(WeaponRelativeTransform);   // hands together: the hand's own grip
            continue;
        }
        Dir.Normalize();
        const FVector Barrel = bRifleBarrelAlongNegativeX ? -Dir : Dir;
        const FRotator Rotation = FRotationMatrix::MakeFromXZ(Barrel, Soldier->GetUpVector()).Rotator();
        const FBox Box = Weapon->GetStaticMesh()->GetBoundingBox();
        const float ButtX = bRifleBarrelAlongNegativeX ? Box.Max.X : Box.Min.X;
        const float GripX = ButtX + (Box.Max.X - Box.Min.X) * RifleGripFraction * (bRifleBarrelAlongNegativeX ? -1.0f : 1.0f);
        Weapon->SetWorldLocationAndRotation(Right - Rotation.RotateVector(FVector(GripX, 0.0f, 0.0f)), Rotation);
    }
}

bool UStrategyInfantryVisualComponent::EnsureAssetsLoaded()
{
    if (LoadedSoldierMesh)
    {
        return true;
    }

    if (bLoadAttempted)
    {
        return false;
    }

    bLoadAttempted = true;

    LoadedSoldierMesh = SoldierMeshAsset.LoadSynchronous();
    LoadedRifleMesh = RifleMeshAsset.LoadSynchronous();
    LoadedRifleBayonetMesh = RifleBayonetMeshAsset.LoadSynchronous();

    if (!LoadedSoldierMesh)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("PROJECT 1864: Livgarden visual mesh not imported yet: %s"),
            *SoldierMeshAsset.ToSoftObjectPath().ToString());

        return false;
    }

    return true;
}

int32 UStrategyInfantryVisualComponent::GetDesiredVisualCount() const
{
    if (!OwnerCompany)
    {
        return 0;
    }

    const int32 Strength =
        FMath::Max(0, OwnerCompany->CurrentStrength);

    if (Strength <= 0)
    {
        return 0;
    }

    const int32 Divisor =
        VisualScaleDivisor <= 1
        ? 1
        : VisualScaleDivisor <= 2
            ? 2
            : VisualScaleDivisor <= 5
                ? 5
                : 10;

    int32 Desired =
        FMath::CeilToInt(
            static_cast<float>(Strength) /
            static_cast<float>(Divisor));

    if (MaxVisualSoldiers > 0)
    {
        Desired = FMath::Min(
            Desired,
            MaxVisualSoldiers);
    }

    return Desired;
}

void UStrategyInfantryVisualComponent::EnsureVisualCount(
    int32 DesiredCount)
{
    if (!OwnerCompany ||
        !LoadedSoldierMesh)
    {
        return;
    }

    DesiredCount = FMath::Max(0, DesiredCount);

    while (SoldierComponents.Num() > DesiredCount)
    {
        const int32 LastIndex =
            SoldierComponents.Num() - 1;

        if (WeaponComponents.IsValidIndex(LastIndex) &&
            WeaponComponents[LastIndex])
        {
            WeaponComponents[LastIndex]->DestroyComponent();
        }

        if (SoldierComponents[LastIndex])
        {
            SoldierComponents[LastIndex]->DestroyComponent();
        }

        if (WeaponComponents.IsValidIndex(LastIndex))
        {
            WeaponComponents.RemoveAt(LastIndex);
        }

        SoldierComponents.RemoveAt(LastIndex);
        if (SoldierClips.IsValidIndex(LastIndex)) { SoldierClips.RemoveAt(LastIndex); }
        bCrowdDirty = true;
        if (SoldierFireAt.IsValidIndex(LastIndex)) { SoldierFireAt.RemoveAt(LastIndex); }
        if (SoldierBusyUntil.IsValidIndex(LastIndex)) { SoldierBusyUntil.RemoveAt(LastIndex); }
        if (SoldierFirePhase.IsValidIndex(LastIndex)) { SoldierFirePhase.RemoveAt(LastIndex); }
        if (SoldierSettle.IsValidIndex(LastIndex)) { SoldierSettle.RemoveAt(LastIndex); }
    }

    while (SoldierComponents.Num() < DesiredCount)
    {
        USkeletalMeshComponent* Soldier =
            NewObject<USkeletalMeshComponent>(
                OwnerCompany,
                NAME_None,
                RF_Transient);

        if (!Soldier)
        {
            break;
        }

        Soldier->SetSkeletalMeshAsset(LoadedSoldierMesh);
        Soldier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Soldier->SetGenerateOverlapEvents(false);
        Soldier->SetCastShadow(true);
        // Hundreds of animated soldiers would overrun the ray tracing memory (the campaign's renderer has it on).
        Soldier->bVisibleInRayTracing = false;
        Soldier->SetupAttachment(OwnerCompany->SceneRoot);
        Soldier->RegisterComponent();
        if (OwnerCompany->UniformAppearanceComponent) { OwnerCompany->UniformAppearanceComponent->ApplyAppearanceToMesh(Soldier); }

        UStaticMeshComponent* Weapon =
            NewObject<UStaticMeshComponent>(
                OwnerCompany,
                NAME_None,
                RF_Transient);

        if (Weapon)
        {
            Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Weapon->SetGenerateOverlapEvents(false);
            Weapon->SetCastShadow(false); // the soldier retains the readable near shadow
            Weapon->bVisibleInRayTracing = false;
            Weapon->RegisterComponent();
            Weapon->AttachToComponent(
                Soldier,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                RightHandBoneName);
            Weapon->SetRelativeTransform(WeaponRelativeTransform);
        }

        if (bCrowdMode)
        {
            // A new man in a far company: drawn baked like the rest.
            Soldier->SetVisibility(false);
            Soldier->SetComponentTickEnabled(false);
            if (Weapon)
            {
                Weapon->SetVisibility(false);
            }
        }
        SoldierComponents.Add(Soldier);
        WeaponComponents.Add(Weapon);
        SoldierClips.AddDefaulted();
        bCrowdDirty = true;
        SoldierFireAt.Add(0.0f);
        SoldierBusyUntil.Add(0.0f);
        SoldierFirePhase.Add(0);
        SoldierSettle.AddDefaulted();
    }

    LastAnimationAsset = nullptr;
}

void UStrategyInfantryVisualComponent::RebuildFormation()
{
    if (!OwnerCompany ||
        !OwnerCompany->FormationComponent ||
        SoldierComponents.Num() == 0)
    {
        return;
    }

    const int32 Strength =
        FMath::Max(0, OwnerCompany->CurrentStrength);

    if (Strength <= 0)
    {
        return;
    }

    const TArray<FStrategyFormationSlot> FullSlots =
        OwnerCompany->FormationComponent->GenerateSoldierSlots(
            FVector::ZeroVector,
            0.0f,
            Strength);

    if (FullSlots.Num() == 0)
    {
        return;
    }

    const int32 RenderedCount =
        SoldierComponents.Num();

    for (int32 VisualIndex = 0;
         VisualIndex < RenderedCount;
         ++VisualIndex)
    {
        USkeletalMeshComponent* Soldier =
            SoldierComponents[VisualIndex];

        if (!Soldier)
        {
            continue;
        }

        // A column keeps its full width at a thinned figure scale: whole rows are skipped, not every second man (which kept two of four files).
        const bool bColumnFormation = OwnerCompany && OwnerCompany->FormationComponent &&
            OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn;
        const int32 ColumnFiles = bColumnFormation ? FMath::Max(1, OwnerCompany->FormationComponent->ColumnWidth) : 1;
        const int32 RowStride = VisualScaleDivisor <= 1 ? 1 : VisualScaleDivisor <= 2 ? 2 : VisualScaleDivisor <= 5 ? 5 : 10;
        const int32 FullIndex =
            bColumnFormation && RowStride > 1 && FullSlots.Num() > 0
            ? FMath::Clamp((VisualIndex / ColumnFiles) * ColumnFiles * RowStride + (VisualIndex % ColumnFiles), 0, FullSlots.Num() - 1)
            : RenderedCount <= 1
            ? 0
            : FMath::Clamp(
                FMath::RoundToInt(
                    static_cast<float>(VisualIndex) *
                    static_cast<float>(FullSlots.Num() - 1) /
                    static_cast<float>(RenderedCount - 1)),
                0,
                FullSlots.Num() - 1);

        const FStrategyFormationSlot& Slot =
            FullSlots[FullIndex];

        if (SoldierSettle.Num() != RenderedCount) { SoldierSettle.SetNumZeroed(RenderedCount); }
        FSettle& Settle = SoldierSettle[VisualIndex];
        if (!Settle.bPlaced || FVector::Dist2D(Soldier->GetRelativeLocation(), Slot.WorldLocation) > 4000.0f)
        {
            // The first placing (or a long way off, as at a deployment): at once.
            Soldier->SetRelativeLocation(Slot.WorldLocation);
            Soldier->SetRelativeRotation(FRotator(0.0f, Slot.FacingYaw + SoldierMeshYawOffset, 0.0f));
            Settle.bPlaced = true;
            Settle.bActive = false;
        }
        else
        {
            // A new formation: he runs to his new place (UpdateSettling).
            Settle.Goal = Slot.WorldLocation;
            Settle.Yaw = Slot.FacingYaw + SoldierMeshYawOffset;
            Settle.bActive = true;
        }
        if (SoldierSlots.Num() != RenderedCount)
        {
            SoldierSlots.SetNum(RenderedCount);
        }
        SoldierSlots[VisualIndex] = Slot.SlotIndex >= 0 ? Slot.SlotIndex : FullIndex;
    }
    bCrowdDirty = true;
}

bool UStrategyInfantryVisualComponent::GetFormationLocalBounds(FBox& OutBounds) const
{
    OutBounds = FormationLocalBounds;
    return bEnabled && OutBounds.IsValid != 0;
}

bool UStrategyInfantryVisualComponent::GetFigureLocalCentroid(FVector& OutCentroid) const
{
    OutCentroid = FigureLocalCentroid;
    return bEnabled && LoadedSoldierMesh && CentroidFigureCount > 0;
}

void UStrategyInfantryVisualComponent::UpdateFormationBounds()
{
    FBox& OutBounds = FormationLocalBounds;
    OutBounds = FBox(ForceInit);
    FigureLocalCentroid = FVector::ZeroVector;
    CentroidFigureCount = 0;
    if (!bEnabled || !LoadedSoldierMesh) return;
    for (const USkeletalMeshComponent* Soldier : SoldierComponents)
    {
        if (IsValid(Soldier))
        {
            FigureLocalCentroid += Soldier->GetRelativeLocation();
            ++CentroidFigureCount;
            // Imported reference-pose bounds stay below ground when an animation
            // moves the hips. Use the evaluated pose so kneeling/prone also fit.
            FBox PoseBounds(ForceInit);
            for (const FTransform& Bone : Soldier->GetComponentSpaceTransforms())
            {
                PoseBounds += Bone.GetLocation();
            }
            if (PoseBounds.IsValid)
            {
                OutBounds += PoseBounds.ExpandBy(20.0f).TransformBy(Soldier->GetRelativeTransform());
            }
        }
    }
    if (CentroidFigureCount > 0) { FigureLocalCentroid /= CentroidFigureCount; }
}

void UStrategyInfantryVisualComponent::UpdateSettling(float DeltaTime)
{
    // The men run to their places in a new formation (out into line, in to the column) at a jog, turning the way they run, and take
    // the company's pose when they are there.
    const float RunCmPerSecond = 420.0f;
    bool bAny = false;
    UAnimSequence* Run = RunStandingAsset.LoadSynchronous();
    for (int32 i = 0; i < SoldierComponents.Num() && i < SoldierSettle.Num(); ++i)
    {
        FSettle& Settle = SoldierSettle[i];
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        if (!Settle.bActive || !Soldier)
        {
            continue;
        }
        FVector Here = Soldier->GetRelativeLocation();
        FVector Delta = Settle.Goal - Here;
        Delta.Z = 0.0f;
        const float Distance = Delta.Size();
        if (Distance < 12.0f)
        {
            Soldier->SetRelativeLocation(FVector(Settle.Goal.X, Settle.Goal.Y, Here.Z));
            Soldier->SetRelativeRotation(FRotator(0.0f, Settle.Yaw, 0.0f));
            Settle.bActive = false;
            bCrowdDirty = true;
            continue;
        }
        bAny = true;
        const FVector Step = Delta / Distance * FMath::Min(Distance, RunCmPerSecond * DeltaTime);
        Soldier->SetRelativeLocation(Here + Step);
        // He faces the way he runs while he is far from his place, and turns to the front as he comes in.
        const float RunYaw = Delta.Rotation().Yaw + SoldierMeshYawOffset;
        const float Yaw = Distance > 150.0f ? RunYaw : Settle.Yaw;
        FRotator Rot = Soldier->GetRelativeRotation();
        Rot.Yaw = FMath::FixedTurn(Rot.Yaw, Yaw, 540.0f * DeltaTime);
        Soldier->SetRelativeRotation(Rot);
        if (Run && SoldierClips.IsValidIndex(i) && SoldierClips[i].Clip.Get() != Run)
        {
            Soldier->bPauseAnims = false;
            Soldier->PlayAnimation(Run, true);
            Soldier->SetPlayRate(FMath::FRandRange(0.95f, 1.1f));
            RecordClip(Soldier, Run, true, 0.0f, 1.0f);
        }
        bCrowdDirty = true;
    }
    if (bWasSettling && !bAny)
    {
        // Everybody is in place: back to the company's own animation.
        RefreshAnimation(true);
    }
    bWasSettling = bAny;
}

void UStrategyInfantryVisualComponent::ApplyPendingStance(float Now)
{
    // The men take up a new stance one after the other (kneel, lie down, stand up): a ripple through the company.
    if (!PendingSequence)
    {
        return;
    }
    bool bLeft = false;
    for (int32 i = 0; i < SoldierComponents.Num() && i < SoldierSettle.Num(); ++i)
    {
        FSettle& Settle = SoldierSettle[i];
        if (Settle.SwitchAt < 0.0f)
        {
            continue;
        }
        if (Now < Settle.SwitchAt)
        {
            bLeft = true;
            continue;
        }
        Settle.SwitchAt = -1.0f;
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        if (!Soldier || Settle.bActive || (SoldierFirePhase.IsValidIndex(i) && SoldierFirePhase[i] != 0))
        {
            continue;
        }
        Soldier->bPauseAnims = false;
        Soldier->PlayAnimation(PendingSequence, bPendingLoop);
        const float Position = bPendingLoop && !bPendingHold ? FMath::FRandRange(0.0f, PendingSequence->GetPlayLength()) : 0.0f;
        const float Rate = bPendingLoop && !bPendingHold ? FMath::FRandRange(0.9f, 1.1f) : 1.0f;
        Soldier->SetPosition(Position, false);
        Soldier->SetPlayRate(Rate);
        RecordClip(Soldier, PendingSequence, bPendingLoop, Position, bPendingHold ? 0.0f : Rate);
        if (bPendingHold)
        {
            Soldier->TickAnimation(0.0f, false);
            Soldier->RefreshBoneTransforms();
        }
        Soldier->bPauseAnims = bPendingHold;
    }
    if (!bLeft)
    {
        PendingSequence = nullptr;
    }
}

void UStrategyInfantryVisualComponent::RefreshAnimation(
    bool bForce)
{
    if (SoldierComponents.Num() == 0)
    {
        return;
    }

    bool bLooping = true;
    UAnimSequence* Sequence =
        ResolveAnimation(bLooping);

    if (!Sequence)
    {
        return;
    }

    const bool bHoldPose = !bAnimateIdle &&
        (Sequence == IdleStandingAsset.Get() || Sequence == IdleKneelingAsset.Get() ||
         Sequence == IdleProneAsset.Get() || Sequence == AimStandingAsset.Get() ||
         Sequence == AimKneelingAsset.Get());

    if (!bForce &&
        LastAnimationAsset == Sequence &&
        bLastAnimationLooping == bLooping &&
        bLastHoldingPose == bHoldPose)
    {
        return;
    }

    // A change between standing, kneeling and lying: the men change one after the other, not all in the same frame.
    auto Family = [this](const UAnimSequence* Clip)
    {
        const UAnimSequence* K[] = { IdleKneelingAsset.Get(), AimKneelingAsset.Get(), FireKneelingAsset.Get(), ReloadKneelingAsset.Get() };
        const UAnimSequence* P[] = { IdleProneAsset.Get(), CrawlProneAsset.Get(), FireProneAsset.Get(), ReloadProneAsset.Get() };
        for (const UAnimSequence* A : K) { if (A && A == Clip) { return 1; } }
        for (const UAnimSequence* A : P) { if (A && A == Clip) { return 2; } }
        return 0;
    };
    if (!bForce && LastAnimationAsset && Family(LastAnimationAsset) != Family(Sequence) && SoldierSettle.Num() == SoldierComponents.Num())
    {
        const float Now = GetWorld()->GetTimeSeconds();
        PendingSequence = Sequence;
        bPendingLoop = bLooping;
        bPendingHold = bHoldPose;
        for (FSettle& Settle : SoldierSettle) { Settle.SwitchAt = Now + FMath::FRandRange(0.0f, 1.6f); }
        LastAnimationAsset = Sequence;
        bLastAnimationLooping = bLooping;
        bLastHoldingPose = bHoldPose;
        return;
    }

    for (int32 Index = 0; Index < SoldierComponents.Num(); ++Index)
    {
        USkeletalMeshComponent* Soldier = SoldierComponents[Index];
        if (!Soldier || (SoldierFirePhase.IsValidIndex(Index) && SoldierFirePhase[Index] != 0) || (SoldierSettle.IsValidIndex(Index) && SoldierSettle[Index].bActive))
        {
            continue;
        }

        Soldier->bPauseAnims = false;
        Soldier->PlayAnimation(
            Sequence,
            bLooping);
        const float Position = bLooping && !bHoldPose ? FMath::FRandRange(0.0f, Sequence->GetPlayLength()) : 0.0f;
        const float Rate = bLooping && !bHoldPose ? FMath::FRandRange(0.9f, 1.1f) : 1.0f;
        Soldier->SetPosition(Position, false);
        Soldier->SetPlayRate(Rate);
        RecordClip(Soldier, Sequence, bLooping, Position, bHoldPose ? 0.0f : Rate);
        if (bHoldPose)
        {
            Soldier->TickAnimation(0.0f, false);
            Soldier->RefreshBoneTransforms();
        }
        Soldier->bPauseAnims = bHoldPose;
    }

    LastAnimationAsset = Sequence;
    bLastAnimationLooping = bLooping;
    bLastHoldingPose = bHoldPose;
}

void UStrategyInfantryVisualComponent::RefreshWeaponMeshes()
{
    if (!OwnerCompany)
    {
        return;
    }

    const bool bUseBayonet =
        OwnerCompany->EquipmentVisualComponent &&
        OwnerCompany->EquipmentVisualComponent->bBayonetFixed;

    UStaticMesh* DesiredMesh =
        bUseBayonet &&
        LoadedRifleBayonetMesh
        ? LoadedRifleBayonetMesh
        : LoadedRifleMesh;

    for (UStaticMeshComponent* Weapon :
         WeaponComponents)
    {
        if (Weapon)
        {
            Weapon->SetStaticMesh(DesiredMesh);
            Weapon->SetRelativeTransform(
                WeaponRelativeTransform);
        }
    }
    // The far men's model carries the rifle: another model for the bayonet.
    CrowdModel = nullptr;
    if (bCrowdMode && EnsureCrowdModel())
    {
        bCrowdDirty = true;
    }
}

UAnimSequence* UStrategyInfantryVisualComponent::ResolveAnimation(
    bool& bOutLooping) const
{
    bOutLooping = true;

    if (!OwnerCompany)
    {
        return IdleStandingAsset.LoadSynchronous();
    }

    const EStrategyStance CurrentStance =
        OwnerCompany->StanceComponent
        ? OwnerCompany->StanceComponent->Stance
        : EStrategyStance::Standing;

    const EStrategyHumanAnimationAction Action =
        OwnerCompany->HumanAnimationStateComponent
        ? OwnerCompany->HumanAnimationStateComponent->CurrentAction
        : EStrategyHumanAnimationAction::Idle;

    if (OwnerCompany->UnitState ==
        EStrategyUnitState::Destroyed)
    {
        bOutLooping = false;
        const uint32 Variant = GetTypeHash(OwnerCompany->StableUnitId) % 3u;
        const TSoftObjectPtr<UAnimSequence>& Chosen =
            Variant == 0 ? DeathAsset : (Variant == 1 ? DeathAsset2 : DeathAsset3);
        return Chosen.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Fire)
    {
        bOutLooping = false;

        if (CurrentStance == EStrategyStance::Prone &&
            !FireProneAsset.IsNull())
        {
            return FireProneAsset.LoadSynchronous();
        }

        if (CurrentStance == EStrategyStance::Kneeling &&
            !FireKneelingAsset.IsNull())
        {
            return FireKneelingAsset.LoadSynchronous();
        }

        return FireStandingAsset.LoadSynchronous();
    }

    if (OwnerCompany->CombatComponent &&
        OwnerCompany->CombatComponent->IsReloading())
    {
        bOutLooping = true;

        if (CurrentStance == EStrategyStance::Prone &&
            !ReloadProneAsset.IsNull())
        {
            return ReloadProneAsset.LoadSynchronous();
        }

        if (CurrentStance == EStrategyStance::Kneeling &&
            !ReloadKneelingAsset.IsNull())
        {
            return ReloadKneelingAsset.LoadSynchronous();
        }

        if (!ReloadStandingAsset.IsNull())
        {
            return ReloadStandingAsset.LoadSynchronous();
        }

        return AimStandingAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::BayonetCharge)
    {
        return BayonetChargeAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::BayonetReady)
    {
        return AimStandingAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Run ||
        Action ==
        EStrategyHumanAnimationAction::RoutedRun)
    {
        return RunStandingAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Walk)
    {
        if (CurrentStance == EStrategyStance::Prone &&
            !CrawlProneAsset.IsNull())
        {
            return CrawlProneAsset.LoadSynchronous();
        }

        return WalkStandingAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Aim)
    {
        if (CurrentStance == EStrategyStance::Prone &&
            !IdleProneAsset.IsNull())
        {
            return IdleProneAsset.LoadSynchronous();
        }

        if (CurrentStance == EStrategyStance::Kneeling &&
            !AimKneelingAsset.IsNull())
        {
            return AimKneelingAsset.LoadSynchronous();
        }

        return AimStandingAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Prone ||
        CurrentStance ==
        EStrategyStance::Prone)
    {
        return IdleProneAsset.LoadSynchronous();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Kneel ||
        CurrentStance ==
        EStrategyStance::Kneeling)
    {
        return IdleKneelingAsset.LoadSynchronous();
    }

    return IdleStandingAsset.LoadSynchronous();
}

void UStrategyInfantryVisualComponent::DestroyVisualComponents()
{
    for (UStaticMeshComponent* Weapon :
         WeaponComponents)
    {
        if (Weapon)
        {
            Weapon->DestroyComponent();
        }
    }

    for (USkeletalMeshComponent* Soldier :
         SoldierComponents)
    {
        if (Soldier)
        {
            Soldier->DestroyComponent();
        }
    }

    if (CrowdLiving)
    {
        CrowdLiving->DestroyComponent();
        CrowdLiving = nullptr;
    }
    if (CrowdFallen)
    {
        CrowdFallen->DestroyComponent();
        CrowdFallen = nullptr;
    }
    for (USkeletalMeshComponent* Corpse : CorpseComponents)
    {
        if (Corpse && bCrowdMode)
        {
            Corpse->SetVisibility(true, true);
        }
    }
    CrowdModel = nullptr;
    bCrowdMode = false;
    bCrowdDirty = false;
    bCrowdDataDirty = false;
    SoldierClips.Reset();
    WeaponComponents.Reset();
    SoldierComponents.Reset();
    SoldierFireAt.Reset();
    SoldierBusyUntil.Reset();
    SoldierFirePhase.Reset();

    LoadedSoldierMesh = nullptr;
    LoadedRifleMesh = nullptr;
    LoadedRifleBayonetMesh = nullptr;
    LastAnimationAsset = nullptr;

    CachedStrength = INDEX_NONE;
    CachedFormationValue = 255;
    bCachedBayonetFixed = false;
    bLastAnimationLooping = false;
    bLastHoldingPose = false;
    bLoadAttempted = false;
    FormationLocalBounds = FBox(ForceInit);
    CentroidFigureCount = 0;
}

// ------------------------------------------------------------------ the hybrid: the far men baked

UStaticMesh* UStrategyInfantryVisualComponent::CurrentRifleMesh() const
{
    const bool bBayonet = OwnerCompany && OwnerCompany->EquipmentVisualComponent && OwnerCompany->EquipmentVisualComponent->bBayonetFixed;
    return bBayonet && LoadedRifleBayonetMesh ? LoadedRifleBayonetMesh.Get() : LoadedRifleMesh.Get();
}

void UStrategyInfantryVisualComponent::RecordClip(USkeletalMeshComponent* Soldier, UAnimSequence* Clip, bool bLoop, float Position, float Rate)
{
    const int32 Index = SoldierComponents.IndexOfByKey(Soldier);
    if (Index == INDEX_NONE)
    {
        return;
    }
    if (SoldierClips.Num() != SoldierComponents.Num())
    {
        SoldierClips.SetNum(SoldierComponents.Num());
    }
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    FPlayedClip& Played = SoldierClips[Index];
    Played.Clip = Clip;
    Played.bLoop = bLoop;
    Played.Rate = Rate;
    Played.Start = Rate > 0.0f ? Now - Position / Rate : Now;
    if (bCrowdMode && !bCrowdDirty && CrowdLiving && CrowdModel && Index < CrowdLiving->GetInstanceCount())
    {
        float Data[UStrategyCrowdModel::CustomDataFloats];
        if (CrowdModel->MakeCustomData(Clip, Played.Start, Rate, bLoop, Data))
        {
            CrowdLiving->SetCustomData(Index, TArrayView<const float>(Data, UStrategyCrowdModel::CustomDataFloats), false);
            bCrowdDataDirty = true;
        }
    }
}

bool UStrategyInfantryVisualComponent::EnsureCrowdModel()
{
    if (OwnerCompany && OwnerCompany->UniformAppearanceComponent)
    {
        const FStrategyUniformOverrides& CustomOverrides = OwnerCompany->UniformAppearanceComponent->Overrides;
        if (CustomOverrides.bOverrideCoat || CustomOverrides.bOverrideTrousers || CustomOverrides.bOverrideHeadgearDetail) { return false; }
    }
    if (CrowdModel)
    {
        return true;
    }
    if (bCrowdFailed || !LoadedSoldierMesh || !GetWorld())
    {
        return false;
    }
    // Every clip the men may play, baked once for the soldier mesh and its rifle.
    const TSoftObjectPtr<UAnimSequence>* All[] = {
        &IdleStandingAsset, &WalkStandingAsset, &RunStandingAsset, &AimStandingAsset, &FireStandingAsset, &ReloadStandingAsset,
        &IdleKneelingAsset, &AimKneelingAsset, &FireKneelingAsset, &ReloadKneelingAsset, &IdleProneAsset, &CrawlProneAsset,
        &FireProneAsset, &ReloadProneAsset, &BayonetChargeAsset, &BayonetThrustAsset, &DeathAsset, &DeathAsset2, &DeathAsset3,
        &RaiseToAimAsset, &LoadAsset, &RiseFromLoadAsset, &ReadyAsset, &AimHoldAsset, &DeathWalkingAsset };
    TArray<UAnimSequence*> Clips;
    for (const TSoftObjectPtr<UAnimSequence>* Asset : All)
    {
        if (!Asset->IsNull())
        {
            if (UAnimSequence* Clip = Asset->LoadSynchronous())
            {
                Clips.Add(Clip);
            }
        }
    }
    FStrategyCrowdRifleGrip Grip;
    Grip.RightHand = RightHandBoneName;
    Grip.LeftHand = bAlignRifleBetweenHands ? LeftHandBoneName : NAME_None;
    Grip.HandTransform = WeaponRelativeTransform;
    Grip.GripFraction = RifleGripFraction;
    Grip.bBarrelAlongNegativeX = bRifleBarrelAlongNegativeX;
    CrowdModel = UStrategyCrowdModel::Find(GetWorld(), LoadedSoldierMesh, CurrentRifleMesh(), Clips, Grip);
    if (!CrowdModel)
    {
        bCrowdFailed = true;
        return false;
    }
    for (UInstancedStaticMeshComponent* Crowd : { CrowdLiving.Get(), CrowdFallen.Get() })
    {
        if (Crowd)
        {
            Crowd->SetStaticMesh(CrowdModel->GetMesh());
        }
    }
    return true;
}

void UStrategyInfantryVisualComponent::UpdateCrowdMode()
{
    UWorld* World = GetWorld();
    if (!World || !OwnerCompany || SoldierComponents.Num() + CorpseComponents.Num() == 0)
    {
        return;
    }
    // -Strategy1864CrowdFar=<cm> (the switch distance for tests), -Strategy1864Crowd=0 (always full animation).
    static const float CommandFar = []()
    {
        float Value = -1.0f;
        FParse::Value(FCommandLine::Get(), TEXT("Strategy1864CrowdFar="), Value);
        int32 On = 1;
        if (FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Crowd="), On) && On == 0)
        {
            Value = 0.0f;
        }
        return Value;
    }();
    float Far = CrowdFarCm, Near = CrowdNearCm;
    if (CommandFar >= 0.0f)
    {
        Far = CommandFar;
        Near = CommandFar * 0.8f;
    }
    if (Far <= 0.0f || bCrowdFailed)
    {
        if (bCrowdMode)
        {
            LeaveCrowdMode();
        }
        return;
    }
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager)
    {
        return;
    }
    const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();
    float Distance = FVector::Dist(Camera, OwnerCompany->GetActorLocation());
    if (FormationLocalBounds.IsValid && OwnerCompany->SceneRoot)
    {
        Distance = FMath::Sqrt(FormationLocalBounds.TransformBy(OwnerCompany->SceneRoot->GetComponentTransform()).ComputeSquaredDistanceToPoint(Camera));
    }
    if (!bCrowdMode && Distance > Far)
    {
        EnterCrowdMode();
    }
    else if (bCrowdMode && Distance < Near)
    {
        LeaveCrowdMode();
    }
    if (bCrowdMode)
    {
        if (bCrowdDirty)
        {
            RebuildCrowdInstances();
        }
        else if (bCrowdDataDirty && CrowdLiving)
        {
            CrowdLiving->MarkRenderStateDirty();
            bCrowdDataDirty = false;
        }
    }
}

void UStrategyInfantryVisualComponent::EnterCrowdMode()
{
    if (!OwnerCompany || !EnsureCrowdModel())
    {
        return;
    }
    auto Make = [&](bool bWorldSpace)
    {
        UInstancedStaticMeshComponent* Crowd = NewObject<UInstancedStaticMeshComponent>(OwnerCompany, NAME_None, RF_Transient);
        Crowd->SetStaticMesh(CrowdModel->GetMesh());
        Crowd->SetNumCustomDataFloats(UStrategyCrowdModel::CustomDataFloats);
        Crowd->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Crowd->SetGenerateOverlapEvents(false);
        Crowd->SetCastShadow(false); // far VAT figures: avoid animated VSM invalidation
        Crowd->bVisibleInRayTracing = false;
        Crowd->bAffectDistanceFieldLighting = false;
        if (bWorldSpace)
        {
            // The fallen stay where they fell, whatever the company does.
            Crowd->SetUsingAbsoluteLocation(true);
            Crowd->SetUsingAbsoluteRotation(true);
            Crowd->SetUsingAbsoluteScale(true);
        }
        Crowd->SetupAttachment(OwnerCompany->SceneRoot);
        Crowd->RegisterComponent();
        if (bWorldSpace)
        {
            Crowd->SetWorldTransform(FTransform::Identity);
        }
        return Crowd;
    };
    if (!CrowdLiving)
    {
        CrowdLiving = Make(false);
    }
    if (!CrowdFallen)
    {
        CrowdFallen = Make(true);
    }
    bCrowdMode = true;
    // The skeletal men and their rifles out of sight and not animated (the saving); their clips are kept.
    for (USkeletalMeshComponent* Soldier : SoldierComponents)
    {
        if (Soldier)
        {
            Soldier->SetVisibility(false, true);
            Soldier->SetComponentTickEnabled(false);
        }
    }
    for (USkeletalMeshComponent* Corpse : CorpseComponents)
    {
        if (Corpse)
        {
            Corpse->SetVisibility(false, true);
            Corpse->SetComponentTickEnabled(false);
        }
    }
    CrowdLiving->SetVisibility(true);
    CrowdFallen->SetVisibility(true);
    RebuildCrowdInstances();
}

void UStrategyInfantryVisualComponent::RestoreClip(USkeletalMeshComponent* Soldier, const FPlayedClip& Played, float Now) const
{
    UAnimSequence* Clip = Played.Clip.Get();
    if (!Soldier || !Clip)
    {
        return;
    }
    const float Length = FMath::Max(Clip->GetPlayLength(), 0.01f);
    float Position = Played.Rate > 0.0f ? FMath::Max(0.0f, (Now - Played.Start) * Played.Rate) : 0.0f;
    Position = Played.bLoop ? FMath::Fmod(Position, Length) : FMath::Min(Position, Length);
    Soldier->PlayAnimation(Clip, Played.bLoop);
    Soldier->SetPosition(Position, false);
    Soldier->SetPlayRate(Played.Rate > 0.0f ? Played.Rate : 1.0f);
    Soldier->bPauseAnims = Played.Rate <= 0.0f;
    Soldier->TickAnimation(0.0f, false);
    Soldier->RefreshBoneTransforms();
}

void UStrategyInfantryVisualComponent::LeaveCrowdMode()
{
    bCrowdMode = false;
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    // Back to full animation, each man where his baked clip had got to.
    for (int32 i = 0; i < SoldierComponents.Num(); ++i)
    {
        if (USkeletalMeshComponent* Soldier = SoldierComponents[i])
        {
            Soldier->SetVisibility(true, true);
            Soldier->SetComponentTickEnabled(true);
            if (SoldierClips.IsValidIndex(i))
            {
                RestoreClip(Soldier, SoldierClips[i], Now);
            }
        }
    }
    for (int32 i = 0; i < CorpseComponents.Num(); ++i)
    {
        if (USkeletalMeshComponent* Corpse = CorpseComponents[i])
        {
            Corpse->SetVisibility(true, true);
            Corpse->SetComponentTickEnabled(true);
            if (CorpseClips.IsValidIndex(i))
            {
                RestoreClip(Corpse, CorpseClips[i], Now);
            }
        }
    }
    for (UInstancedStaticMeshComponent* Crowd : { CrowdLiving.Get(), CrowdFallen.Get() })
    {
        if (Crowd)
        {
            Crowd->ClearInstances();
            Crowd->SetVisibility(false);
        }
    }
    bCrowdDirty = false;
    bCrowdDataDirty = false;
}

void UStrategyInfantryVisualComponent::RebuildCrowdInstances()
{
    if (!CrowdLiving || !CrowdFallen || !CrowdModel)
    {
        return;
    }
    UAnimSequence* Idle = IdleStandingAsset.Get();
    auto DataFor = [&](const FPlayedClip* Played, float Out[UStrategyCrowdModel::CustomDataFloats])
    {
        if (!Played || !CrowdModel->MakeCustomData(Played->Clip.Get(), Played->Start, Played->Rate, Played->bLoop, Out))
        {
            if (!CrowdModel->MakeCustomData(Idle, 0.0f, 0.0f, true, Out))
            {
                FMemory::Memzero(Out, sizeof(float) * UStrategyCrowdModel::CustomDataFloats);
            }
        }
    };
    float Data[UStrategyCrowdModel::CustomDataFloats];

    CrowdLiving->ClearInstances();
    TArray<FTransform> Living;
    for (const USkeletalMeshComponent* Soldier : SoldierComponents)
    {
        Living.Add(Soldier ? Soldier->GetRelativeTransform() : FTransform::Identity);
    }
    CrowdLiving->AddInstances(Living, false, false);
    for (int32 i = 0; i < Living.Num(); ++i)
    {
        DataFor(SoldierClips.IsValidIndex(i) ? &SoldierClips[i] : nullptr, Data);
        CrowdLiving->SetCustomData(i, TArrayView<const float>(Data, UStrategyCrowdModel::CustomDataFloats), false);
    }

    CrowdFallen->ClearInstances();
    TArray<FTransform> Fallen;
    for (const USkeletalMeshComponent* Corpse : CorpseComponents)
    {
        Fallen.Add(Corpse ? Corpse->GetComponentTransform() : FTransform::Identity);
    }
    CrowdFallen->AddInstances(Fallen, false, true);
    for (int32 i = 0; i < Fallen.Num(); ++i)
    {
        DataFor(CorpseClips.IsValidIndex(i) ? &CorpseClips[i] : nullptr, Data);
        CrowdFallen->SetCustomData(i, TArrayView<const float>(Data, UStrategyCrowdModel::CustomDataFloats), false);
    }
    CrowdLiving->MarkRenderStateDirty();
    CrowdFallen->MarkRenderStateDirty();
    bCrowdDirty = false;
    bCrowdDataDirty = false;
}
