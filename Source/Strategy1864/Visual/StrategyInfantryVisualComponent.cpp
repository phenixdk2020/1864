#include "StrategyInfantryVisualComponent.h"
#include "../Player/StrategyBattlePerformance.h"
#include "HAL/IConsoleManager.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Formations/StrategyFormationTransitionComponent.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../Combat/StrategySkirmisherComponent.h"
#include "../AI/StrategyFieldOfficerComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Units/StrategyCompanyUnit.h"
#include "StrategyEquipmentVisualComponent.h"
#include "StrategyHumanAnimationStateComponent.h"
#include "StrategyMuzzleSmokePuff.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
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
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Kismet/GameplayStatics.h"

UStrategyInfantryVisualComponent::UStrategyInfantryVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0f;
    PrimaryComponentTick.bTickEvenWhenPaused = true;

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
    Strategy1864Performance::RegisterFigures(this);
    VisualAnimationTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    if (OwnerCompany) OwnerCompany->OnCasualtyVisualEvent.AddDynamic(this, &UStrategyInfantryVisualComponent::HandleCasualtyVisualEvent);

    if (OwnerCompany &&
        OwnerCompany->CombatComponent)
    {
        OwnerCompany->CombatComponent->OnVolleyVisualEvent.AddDynamic(
            this,
            &UStrategyInfantryVisualComponent::HandleVolleyVisualEvent);
    }

    PrimaryComponentTick.TickInterval = 0.0f;
    if (OwnerCompany && OwnerCompany->MovementExecutor) AddTickPrerequisiteComponent(OwnerCompany->MovementExecutor);
    if (OwnerCompany && OwnerCompany->HumanAnimationStateComponent) AddTickPrerequisiteComponent(OwnerCompany->HumanAnimationStateComponent);
    if (OwnerCompany && OwnerCompany->CombatComponent) AddTickPrerequisiteComponent(OwnerCompany->CombatComponent);

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

    if (OwnerCompany) OwnerCompany->OnCasualtyVisualEvent.RemoveDynamic(this, &UStrategyInfantryVisualComponent::HandleCasualtyVisualEvent);
    for (USkeletalMeshComponent* EndCorpse : CorpseComponents)
    {
        if (!EndCorpse) continue;
        TArray<USceneComponent*> EndChildren;
        EndCorpse->GetChildrenComponents(true, EndChildren);
        for (USceneComponent* EndChild : EndChildren) EndChild->DestroyComponent();
        EndCorpse->DestroyComponent();
    }
    CorpseComponents.Reset();
    CorpseClips.Reset();
    CorpseBirthTimes.Reset();
    CorpseFading.Reset();
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

    UpdateCorpses();

    if (!bEnabled ||
        !OwnerCompany ||
        !LoadedSoldierMesh)
    {
        return;
    }

    VisualAnimationTime += FMath::Max(0.f, DeltaTime);
    if (DeltaTime <= 0.f)
    {
        if (bCrowdMode) { bCrowdDirty = true; UpdateCrowdMode(); }
        DebugSmooth();
        return;
    }

    if (bResumeCrowdPosesNextTick)
    {
        // The switch frame already evaluated the exact VAT phase. Do not advance it twice.
        for (int32 ResumeIndex = 0; ResumeIndex < SoldierComponents.Num(); ++ResumeIndex)
            if (SoldierComponents[ResumeIndex] && SoldierClips.IsValidIndex(ResumeIndex))
                SoldierComponents[ResumeIndex]->bPauseAnims = SoldierClips[ResumeIndex].Rate <= 0.f;
        for (int32 ResumeIndex = 0; ResumeIndex < CorpseComponents.Num(); ++ResumeIndex)
            if (CorpseComponents[ResumeIndex] && CorpseClips.IsValidIndex(ResumeIndex))
                CorpseComponents[ResumeIndex]->bPauseAnims = CorpseClips[ResumeIndex].Rate <= 0.f;
        bResumeCrowdPosesNextTick = false;
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
        bScaleOnlyRebuild = true; // Losses leave holes; surviving identities keep their slots.
        RebuildFormation();
        bScaleOnlyRebuild = false;
        CachedStrength = CurrentStrength;
    }

    const float CoverVisualLateralSpacing = OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierLateralSpacingCm : 0.0f;
    const float CoverVisualRankSpacing = OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierRankSpacingCm : 0.0f;
    const int32 SpecialVisualRanks = OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->RankCount : 0;
    const UStrategySkirmisherComponent* SpecialVisualScreen = OwnerCompany->SkirmisherComponent;
    const int32 SpecialScreenStrength = SpecialVisualScreen &&
        (SpecialVisualScreen->State == EStrategySkirmisherState::Deploying || SpecialVisualScreen->IsDeployed()) ? SpecialVisualScreen->DetachedStrength : 0;
    if (CachedFormationValue != CurrentFormationValue || CachedCoverLateralSpacing != CoverVisualLateralSpacing ||
        CachedCoverRankSpacing != CoverVisualRankSpacing || CachedSpecialRanks != SpecialVisualRanks || CachedScreenStrength != SpecialScreenStrength)
    {
        RebuildFormation();
        CachedFormationValue = CurrentFormationValue;
        CachedCoverLateralSpacing = CoverVisualLateralSpacing;
        CachedCoverRankSpacing = CoverVisualRankSpacing;
        CachedSpecialRanks = SpecialVisualRanks;
        CachedScreenStrength = SpecialScreenStrength;
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
    DebugSmooth();
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

    if (bEnabled && OwnerCompany && LoadedSoldierMesh)
    {
        const int32 ScaleOldCount = SoldierComponents.Num();
        const int32 ScaleDesiredCount = GetDesiredVisualCount();
        // Thin evenly across the existing company; survivors keep position, slot and animation phase.
        TSet<int32> ScaleKeep;
        const bool bScaleColumn = OwnerCompany->FormationComponent &&
            (OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
             OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn);
        const int32 ScaleFiles = bScaleColumn ?
            (OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn ? 2 :
             FMath::Max(1, OwnerCompany->FormationComponent->ColumnWidth)) : 1;
        const int32 ScaleOldRows = FMath::DivideAndRoundUp(ScaleOldCount, ScaleFiles);
        const int32 ScaleNewRows = FMath::DivideAndRoundUp(ScaleDesiredCount, ScaleFiles);
        for (int32 ScaleIndex = 0; ScaleIndex < FMath::Min(ScaleDesiredCount, ScaleOldCount); ++ScaleIndex)
        {
            const int32 ScaleRow = ScaleNewRows <= 1 ? 0 : FMath::FloorToInt(float(ScaleIndex / ScaleFiles) * ScaleOldRows / ScaleNewRows);
            ScaleKeep.Add(FMath::Min(ScaleOldCount - 1, ScaleRow * ScaleFiles + ScaleIndex % ScaleFiles));
        }
        if (ScaleDesiredCount < ScaleOldCount)
        {
            for (int32 ScaleIndex = ScaleOldCount - 1; ScaleIndex >= 0; --ScaleIndex)
            {
                if (ScaleKeep.Contains(ScaleIndex)) continue;
                if (SoldierComponents[ScaleIndex]) SoldierComponents[ScaleIndex]->DestroyComponent();
                if (WeaponComponents.IsValidIndex(ScaleIndex) && WeaponComponents[ScaleIndex]) WeaponComponents[ScaleIndex]->DestroyComponent();
                SoldierComponents.RemoveAt(ScaleIndex);
                WeaponComponents.RemoveAt(ScaleIndex);
                SoldierClips.RemoveAt(ScaleIndex);
                SoldierSettle.RemoveAt(ScaleIndex);
                if (SoldierSlots.IsValidIndex(ScaleIndex)) SoldierSlots.RemoveAt(ScaleIndex);
                SoldierFireAt.RemoveAt(ScaleIndex);
                SoldierBusyUntil.RemoveAt(ScaleIndex);
                SoldierFirePhase.RemoveAt(ScaleIndex);
            }
        }
        EnsureVisualCount(ScaleDesiredCount);
        bScaleOnlyRebuild = true;
        RebuildFormation();
        bScaleOnlyRebuild = false;
        RefreshAnimation(false);
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

    if (Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Warmup"))) EnsureCrowdModel();
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
    if (Count == 0 || Shots <= 0) return;
    const int32 Firing = FMath::Clamp(FMath::DivideAndRoundUp(Shots, FMath::Max(1, VisualScaleDivisor)), 1, Count);
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
            Fire->CanPointBearOn(SoldierComponents[i]->GetComponentLocation(), FRotator(0.0f, OwnerCompany->GetActorRotation().Yaw +
                (SoldierSettle.IsValidIndex(i) ? SoldierSettle[i].SlotYaw : 0.0f), 0.0f).Vector(), Target, Fire->GetActiveRangeCm());
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
    for (const TWeakObjectPtr<AStrategyUnit>& BattleEnemyEntry : Strategy1864Performance::VisualUnits(GetWorld()))
    {
        AStrategyUnit* BattleEnemy = BattleEnemyEntry.Get();
        if (IsValid(BattleEnemy) && BattleEnemy->Side != OwnerCompany->Side && BattleEnemy->Side != EStrategySide::Neutral && BattleEnemy->IsCombatEffective() &&
            Fire->CanEngageTarget(BattleEnemy))
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
    const bool bLine = OwnerCompany->FireControlComponent->IsBattleFormationReady();
    if (bMoving || !bLine)
    {
        return false;
    }
    // An enemy within the long range ahead: stand ready.
    const UStrategyFireControlComponent* Fire = OwnerCompany->FireControlComponent;
    for (const TWeakObjectPtr<AStrategyUnit>& BattleEnemyEntry : Strategy1864Performance::VisualUnits(GetWorld()))
    {
        AStrategyUnit* BattleEnemy = BattleEnemyEntry.Get();
        if (IsValid(BattleEnemy) && BattleEnemy->Side != OwnerCompany->Side && BattleEnemy->Side != EStrategySide::Neutral && BattleEnemy->IsCombatEffective() &&
            Fire->IsLocationInsideFireField(BattleEnemy->GetActorLocation(), Fire->LongRangeCm * 1.3f))
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
        Stance == EStrategyStance::Kneeling ? ReloadKneelingAsset : ReloadStandingAsset).Get();
    UAnimSequence* Rise = Stance == EStrategyStance::Kneeling ? RiseFromLoadAsset.Get() : nullptr;
    UAnimSequence* Ready = ReadyAsset.Get();
    UAnimSequence* AimHold = AimHoldAsset.Get();
    UAnimSequence* Raise = RaiseToAimAsset.Get();
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
                    UAnimSequence* Fire = (Stance == EStrategyStance::Prone ? FireProneAsset : Stance == EStrategyStance::Kneeling ? FireKneelingAsset : FireStandingAsset).Get();
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
    if (!bMuzzleSmoke || !World || !Soldier || !Strategy1864Performance::CanSpawnEffect(World))
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
    if (!OwnerCompany->MovementExecutor || OwnerCompany->MovementExecutor->GetExecutedVelocity().Size2D() < 5.f ||
        !OwnerCompany->FormationComponent ||
        (OwnerCompany->FormationComponent->CurrentFormation != EStrategyFormationType::MarchColumn &&
         OwnerCompany->FormationComponent->CurrentFormation != EStrategyFormationType::DefileColumn)) return;
    NextMarchDust = Now + FMath::FRandRange(0.3f, 0.5f);
    // Only near the camera (a sight for the commander, nothing for the far field).
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (PC->PlayerCameraManager && FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), OwnerCompany->GetActorLocation()) > 40000.0f)
        {
            return;
        }
    }
    const FVector Fwd = OwnerCompany->GetActorForwardVector().GetSafeNormal2D();
    if (SoldierComponents.IsEmpty()) return;
    const USkeletalMeshComponent* DustSoldier = SoldierComponents[FMath::RandRange(0, SoldierComponents.Num() - 1)];
    if (!DustSoldier) return;
    const FVector DustGround = UStrategyTerrainQueryLibrary::ProjectPointToTerrain(this, DustSoldier->GetComponentLocation());
    AStrategyBattleBlast::Spawn(GetWorld(), EStrategyBlastKind::FootstepDust, DustGround + FVector(0.f, 0.f, 8.f), Fwd);
}

void UStrategyInfantryVisualComponent::HandleCasualtyVisualEvent(int32 AppliedLoss, FVector SourceLocation)
{
    if (!bEnabled || !OwnerCompany || AppliedLoss <= 0) return;
    // Compare total represented strength, so fractional losses accumulate across the figure divisor.
    const bool bDirectedFire = !OwnerCompany->CasualtySourceLocation.IsNearlyZero();
    if (bDirectedFire) SourceLocation = OwnerCompany->CasualtySourceLocation;
    const int32 FiguresLost = FMath::Max(0, SoldierComponents.Num() - PendingKillCount - GetDesiredVisualCount());
    FStrategyImpactRegistry::FImpact CasualtyImpact;
    const float CasualtyNow = GetWorld()->GetTimeSeconds();
    if (!bDirectedFire && FStrategyImpactRegistry::Find(GetWorld(), OwnerCompany->GetActorLocation(), 6500.f, CasualtyNow, CasualtyImpact))
        QueueKills(FiguresLost);
    else
        KillSoldiers(FiguresLost, &SourceLocation);
    CachedStrength = OwnerCompany->CurrentStrength;
    UpdateFormationBounds();
}

void UStrategyInfantryVisualComponent::UpdateCorpses()
{
    if (CorpseComponents.IsEmpty() || !GetWorld()) return;
    const float CorpseNow = GetWorld()->GetTimeSeconds();
    for (int32 CorpseIndex = CorpseComponents.Num() - 1; CorpseIndex >= 0; --CorpseIndex)
    {
        USkeletalMeshComponent* FallenMesh = CorpseComponents[CorpseIndex];
        const float CorpseAge = CorpseNow - CorpseBirthTimes[CorpseIndex];
        if (CorpseAge >= 65.f)
        {
            TArray<USceneComponent*> FallenChildren;
            FallenMesh->GetChildrenComponents(true, FallenChildren);
            for (USceneComponent* FallenChild : FallenChildren) FallenChild->DestroyComponent();
            FallenMesh->DestroyComponent();
            CorpseComponents.RemoveAt(CorpseIndex);
            CorpseClips.RemoveAt(CorpseIndex);
            CorpseBirthTimes.RemoveAt(CorpseIndex);
            CorpseFading.RemoveAt(CorpseIndex);
            bCrowdDirty = true;
        }
        else if (CorpseAge >= 60.f)
        {
            // A real translucent fade also works when the original uniform material is opaque.
            if (!CorpseFading[CorpseIndex])
            {
                CorpseFading[CorpseIndex] = true;
                RestoreClip(FallenMesh, CorpseClips[CorpseIndex], VisualAnimationTime);
                FallenMesh->bPauseAnims = true;
                UMaterialInterface* FadeBase = LoadedBattleFadeMaterial;
                for (int32 FadeSlot = 0; FadeSlot < FallenMesh->GetNumMaterials(); ++FadeSlot)
                    if (FadeBase) FallenMesh->SetMaterial(FadeSlot, UMaterialInstanceDynamic::Create(FadeBase, this, TEXT("FadeCorpse")));
                TArray<USceneComponent*> FallenChildren;
                FallenMesh->GetChildrenComponents(true, FallenChildren);
                for (USceneComponent* FallenChild : FallenChildren) FallenChild->SetVisibility(false);
            }
            FallenMesh->SetVisibility(true);
            for (int32 FadeSlot = 0; FadeSlot < FallenMesh->GetNumMaterials(); ++FadeSlot)
                if (UMaterialInstanceDynamic* FadeMaterial = Cast<UMaterialInstanceDynamic>(FallenMesh->GetMaterial(FadeSlot)))
                    FadeMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.22f, 0.25f, 0.28f, 1.f - (CorpseAge - 60.f) / 5.f));
            bCrowdDirty = true;
        }
    }
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
        bScaleOnlyRebuild = true;
        RebuildFormation();
        bScaleOnlyRebuild = false;
    }
}

void UStrategyInfantryVisualComponent::KillSoldiers(int32 Count, const FVector* Near, const FVector* ConeOrigin)
{
    // Remove only the chosen figures; all surviving figure identities and slots remain intact.
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
            i = Nearest;
            float ConeNearestDistance = TNumericLimits<float>::Max();
            for (int32 ConeCandidate : Inside)
            {
                const float ConeDistance = FVector::DistSquared2D(SoldierComponents[ConeCandidate]->GetComponentLocation(), *ConeOrigin);
                if (ConeDistance < ConeNearestDistance) { ConeNearestDistance = ConeDistance; i = ConeCandidate; }
            }
        }
        else if (Near)
        {
            // A shot: nearest the fire source or registered impact; no random reshuffle.
            float Best = TNumericLimits<float>::Max();
            for (int32 c = 0; c < SoldierComponents.Num(); ++c)
            {
                if (!SoldierComponents[c]) { continue; }
                const float D = FVector::Dist2D(SoldierComponents[c]->GetComponentLocation(), *Near);
                if (D < Best) { Best = D; i = c; }
            }
        }
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        if (Soldier)
        {
            Soldier->RemoveTickPrerequisiteComponent(this);
            Soldier->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            const int32 Variant = Near && FMath::FRand() < 0.6f ? 2 : FMath::RandRange(0, 2);
            const TSoftObjectPtr<UAnimSequence>& Death = bMoving && !DeathWalkingAsset.IsNull() && FMath::FRand() < 0.5f ? DeathWalkingAsset
                : Variant == 0 ? DeathAsset : Variant == 1 ? DeathAsset2 : DeathAsset3;
            const float Rate = FMath::FRandRange(0.9f, 1.1f);
            Soldier->SetPlayRate(Rate);
            Soldier->bPauseAnims = false;
            UAnimSequence* FallenAnimation = Death.Get();
            if (!FallenAnimation) FallenAnimation = IdleProneAsset.Get();
            Soldier->PlayAnimation(FallenAnimation, false);
            int32 BattleBodyCount = 0;
            for (const TWeakObjectPtr<AStrategyUnit>& BattleBodyEntry : Strategy1864Performance::VisualUnits(GetWorld()))
                if (const AStrategyCompanyUnit* BattleBodyCompany = Cast<AStrategyCompanyUnit>(BattleBodyEntry.Get()))
                    if (BattleBodyCompany->InfantryVisualComponent) BattleBodyCount += BattleBodyCompany->InfantryVisualComponent->GetCorpseCount();
            const bool bKeepBody = bLeaveCorpses && BattleBodyCount < 150;
            if (bKeepBody) { CorpseComponents.Add(Soldier); CorpseBirthTimes.Add(Now); CorpseFading.Add(false); }
            else
            {
                if (WeaponComponents.IsValidIndex(i) && WeaponComponents[i]) WeaponComponents[i]->DestroyComponent();
                Soldier->DestroyComponent();
            }
            FPlayedClip Fallen;
            Fallen.Clip = FallenAnimation;
            Fallen.Start = VisualAnimationTime;
            Fallen.Rate = Rate;
            Fallen.bLoop = false;
            if (bKeepBody) CorpseClips.Add(Fallen);
            bCrowdDirty = true;
        }
        SoldierComponents.RemoveAt(i);
        if (SoldierSettle.IsValidIndex(i)) { SoldierSettle.RemoveAt(i); }
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
    LoadedBattleFadeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent"));
    const TSoftObjectPtr<UAnimSequence>* BattleAnimationAssets[] = { &IdleStandingAsset, &WalkStandingAsset, &RunStandingAsset, &AimStandingAsset, &FireStandingAsset, &ReloadStandingAsset, &IdleKneelingAsset, &AimKneelingAsset, &FireKneelingAsset, &ReloadKneelingAsset, &IdleProneAsset, &CrawlProneAsset, &FireProneAsset, &ReloadProneAsset, &BayonetChargeAsset, &BayonetThrustAsset, &DeathAsset, &DeathAsset2, &DeathAsset3, &RaiseToAimAsset, &LoadAsset, &RiseFromLoadAsset, &ReadyAsset, &AimHoldAsset, &DeathWalkingAsset };
    LoadedBattleAnimations.Reset();
    for (const auto* BattleAnimationAsset : BattleAnimationAssets)
        if (UAnimSequence* BattleAnimation = BattleAnimationAsset->LoadSynchronous()) LoadedBattleAnimations.Add(BattleAnimation);


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
        if (SoldierSlots.IsValidIndex(LastIndex)) { SoldierSlots.RemoveAt(LastIndex); }
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
        Soldier->bEnableUpdateRateOptimizations = false;
        Soldier->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Soldier->AddTickPrerequisiteComponent(this);
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

}

void UStrategyInfantryVisualComponent::RebuildFormation()
{
    if (!OwnerCompany ||
        !OwnerCompany->FormationComponent ||
        SoldierComponents.Num() == 0)
    {
        return;
    }

    // One contiguous slot per figure; never sample a sparse full-strength layout.
    // Pending casualties remain visible until impact, then ProcessPendingKills rebuilds.
    const int32 RenderedCount = SoldierComponents.Num();
    TArray<FStrategyFormationSlot> DrawnSlots =
        OwnerCompany->FormationComponent->GenerateSoldierSlots(
            FVector::ZeroVector, 0.0f, RenderedCount);

    const UStrategySkirmisherComponent* DrawnScreen = OwnerCompany->SkirmisherComponent;
    if (DrawnScreen && (DrawnScreen->State == EStrategySkirmisherState::Deploying || DrawnScreen->IsDeployed()) &&
        OwnerCompany->FormationComponent->CurrentFormation != EStrategyFormationType::Square && RenderedCount > 1)
    {
        const int32 DrawnScreenCount = FMath::Clamp(FMath::RoundToInt(float(RenderedCount) *
            DrawnScreen->DetachedStrength / FMath::Max(1, OwnerCompany->CurrentStrength)), 1, RenderedCount - 1);
        DrawnSlots = OwnerCompany->FormationComponent->GenerateSoldierSlots(FVector::ZeroVector, 0.f, RenderedCount - DrawnScreenCount);
        const float DrawnScreenSpacing = OwnerCompany->FormationComponent->SoldierLateralSpacingCm *
            (OwnerCompany->FieldOfficerComponent && OwnerCompany->FieldOfficerComponent->IsTakingFireCover() ? 1.f : 3.f);
        // Stable scatter: one loose rank ahead of the main body, without frame-to-frame randomness.
        for (int32 ScreenFigure = 0; ScreenFigure < DrawnScreenCount; ++ScreenFigure)
        {
            FStrategyFormationSlot ScreenSlot;
            ScreenSlot.WorldLocation = FVector(DrawnScreen->ScreenDistanceCm + ((ScreenFigure * 7) % 5 - 2) * 90.f,
                (ScreenFigure - (DrawnScreenCount - 1) * 0.5f) * DrawnScreenSpacing, 0.f);
            ScreenSlot.FacingYaw = 0.f;
            ScreenSlot.SlotIndex = DrawnSlots.Num();
            DrawnSlots.Add(ScreenSlot);
        }
    }

    // The simulated square has a 3 m minimum half-extent. At reduced quality,
    // keep ordinary figure spacing even for a tiny square instead of retaining that floor.
    if (VisualScaleDivisor > 1 && OwnerCompany->FormationComponent->UsesSquareVisualOwnership())
    {
        const int32 DrawnPerSide = FMath::DivideAndRoundUp(RenderedCount, 4);
        const float DrawnHalfExtent = FMath::Max(1, DrawnPerSide - 1) *
            OwnerCompany->FormationComponent->SoldierLateralSpacingCm * 0.5f;
        const float DrawnSquareScale = DrawnHalfExtent / FMath::Max(300.0f,
            (DrawnPerSide - 1) * OwnerCompany->FormationComponent->SoldierLateralSpacingCm * 0.5f);
        for (FStrategyFormationSlot& DrawnSquareSlot : DrawnSlots)
        {
            DrawnSquareSlot.WorldLocation *= DrawnSquareScale;
        }
    }

    if (!bScaleOnlyRebuild) VisualPath.SlotBounds = FBox(ForceInit);
    if (bScaleOnlyRebuild)
    {
        for (const FSettle& ExistingSettle : SoldierSettle)
            if (ExistingSettle.bPlaced) VisualPath.SlotBounds += ExistingSettle.Slot;
    }
    else
        for (const FStrategyFormationSlot& VisualSlot : DrawnSlots) VisualPath.SlotBounds += VisualSlot.WorldLocation;

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

        if (bScaleOnlyRebuild && SoldierSettle.IsValidIndex(VisualIndex) && SoldierSettle[VisualIndex].bPlaced)
            continue;
        int32 DrawnSlotIndex = VisualIndex;
        if (bScaleOnlyRebuild)
        {
            // Added figures choose an unoccupied slot; no survivor is remapped.
            for (int32 DrawnCandidate = 0; DrawnCandidate < DrawnSlots.Num(); ++DrawnCandidate)
            {
                const FVector DrawnCandidatePosition = DrawnSlots[DrawnCandidate].WorldLocation;
                if (!SoldierSettle.ContainsByPredicate([&](const FSettle& ExistingSettle)
                    { return ExistingSettle.bPlaced && ExistingSettle.Slot.Equals(DrawnCandidatePosition, 0.1f); }))
                {
                    DrawnSlotIndex = DrawnCandidate;
                    break;
                }
            }
        }
        const FStrategyFormationSlot& Slot = DrawnSlots[DrawnSlotIndex];

        if (SoldierSettle.Num() != RenderedCount) { SoldierSettle.SetNum(RenderedCount); }
        FSettle& Settle = SoldierSettle[VisualIndex];
        Settle.Slot = Slot.WorldLocation;
        if (bScaleOnlyRebuild)
        {
            while (SoldierSettle.ContainsByPredicate([&](const FSettle& ExistingSettle)
                { return ExistingSettle.bPlaced && ExistingSettle.Slot.Equals(Settle.Slot, 0.1f); }))
                Settle.Slot.X -= FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierRankSpacingCm);
        }
        VisualPath.SlotBounds += Settle.Slot;
        Settle.SlotYaw = Slot.FacingYaw;
        Settle.Goal = Settle.Slot;
        Settle.Yaw = Slot.FacingYaw + SoldierMeshYawOffset;
        if (!Settle.bPlaced)
        {
            // Only newly created figures are placed immediately; existing men always walk.
            float InitialDrawnYaw = 0.f;
            Soldier->SetRelativeLocation(VisualPath.bInitialized ? OwnerCompany->GetActorTransform().InverseTransformPosition(
                VisualPath.Goal(Settle.Slot, false, InitialDrawnYaw)) : Settle.Slot);
            Soldier->SetRelativeRotation(FRotator(0.0f, Slot.FacingYaw + SoldierMeshYawOffset, 0.0f));
            Settle.bPlaced = true;
            Settle.bActive = false;
        }
        else
        {
            // A new formation: he runs to his new place (UpdateSettling).
            Settle.Goal = Settle.Slot;
            Settle.Yaw = Slot.FacingYaw + SoldierMeshYawOffset;
            Settle.bActive = true;
        }
        if (SoldierSlots.Num() != RenderedCount)
        {
            SoldierSlots.SetNum(RenderedCount);
        }
        SoldierSlots[VisualIndex] = Slot.SlotIndex >= 0 ? Slot.SlotIndex : VisualIndex;
    }
    bCrowdDirty = true;
}

bool UStrategyInfantryVisualComponent::GetDrawnFireFront(FVector& OutLeft, FVector& OutRight) const
{
    FBox DrawnFrontBounds(ForceInit);
    if (!OwnerCompany || !GetFormationLocalBounds(DrawnFrontBounds)) return false;
    DrawnFrontBounds = DrawnFrontBounds.ExpandBy(15.0f);
    const FTransform DrawnUnitTransform = OwnerCompany->GetActorTransform();
    OutLeft = DrawnUnitTransform.TransformPosition(FVector(DrawnFrontBounds.Max.X, DrawnFrontBounds.Min.Y, 0.0f));
    OutRight = DrawnUnitTransform.TransformPosition(FVector(DrawnFrontBounds.Max.X, DrawnFrontBounds.Max.Y, 0.0f));
    return true;
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

bool UStrategyInfantryVisualComponent::GetDrawnFormation(FTransform& OutFrame, FBox& OutBounds) const
{
    FVector DrawnCentroid;
    if (!OwnerCompany || !VisualPath.bInitialized || !GetFigureLocalCentroid(DrawnCentroid)) return false;
    OutFrame = FTransform(FRotator(0.f, VisualPath.Facing, 0.f),
        OwnerCompany->GetActorTransform().TransformPosition(DrawnCentroid));
    OutBounds = FBox(ForceInit);
    for (const USkeletalMeshComponent* DrawnSoldier : SoldierComponents)
    {
        if (IsValid(DrawnSoldier))
            OutBounds += OutFrame.InverseTransformPosition(DrawnSoldier->GetComponentLocation());
    }
    // Body/weapon padding, independent of animation evaluation and crowd LOD.
    if (OutBounds.IsValid) OutBounds = OutBounds.ExpandBy(100.f);
    return OutBounds.IsValid != 0;
}

bool UStrategyInfantryVisualComponent::GetDrawnFormationEnds(FVector& OutFront, FVector& OutRear) const
{
    FTransform DrawnFrame;
    FBox DrawnBounds(ForceInit);
    if (!GetDrawnFormation(DrawnFrame, DrawnBounds)) return false;
    OutFront = DrawnFrame.TransformPosition(FVector(DrawnBounds.Max.X, 0.f, 0.f));
    OutRear = DrawnFrame.TransformPosition(FVector(DrawnBounds.Min.X, 0.f, 0.f));
    return true;
}

bool UStrategyInfantryVisualComponent::GetDrawnColourPosition(const FVector& Offset, FVector& OutPosition) const
{
    FTransform ColourFrame;
    FBox ColourBounds(ForceInit);
    if (!GetDrawnFormation(ColourFrame, ColourBounds)) return false;
    const bool bColourColumn = OwnerCompany->FormationComponent &&
        (OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
         OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn);
    const FVector ColourGoal = ColourFrame.TransformPosition(FVector(
        bColourColumn ? ColourBounds.Max.X - 100.f : 0.f, Offset.Y, 0.f));
    double ColourBest = TNumericLimits<double>::Max();
    for (const USkeletalMeshComponent* ColourSoldier : SoldierComponents)
    {
        if (!IsValid(ColourSoldier)) continue;
        const double ColourDistance = FVector::DistSquared2D(ColourGoal, ColourSoldier->GetComponentLocation());
        if (ColourDistance < ColourBest)
        {
            ColourBest = ColourDistance;
            OutPosition = ColourSoldier->GetComponentLocation() + FVector(0.f, 0.f, Offset.Z);
        }
    }
    return ColourBest < TNumericLimits<double>::Max();
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
    const FTransform VisualUnit = OwnerCompany->GetActorTransform();
    const bool bVisualColumn = OwnerCompany->FormationComponent &&
        (OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
         OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn);
    if (!VisualPath.bInitialized) { VisualPath.Reset(VisualUnit); }
    const float ActualMarchSpeed = OwnerCompany->MovementExecutor ? OwnerCompany->MovementExecutor->GetExecutedVelocity().Size2D() : 0.f;
    const float VisualWalkSpeed = FMath::Max(160.f, ActualMarchSpeed);
    VisualPath.Advance(VisualUnit, DeltaTime, bVisualColumn, VisualWalkSpeed);
    const bool bActorMoved = !VisualUnit.Equals(VisualPath.PreviousUnit, 0.001f);
    const bool bVisualTravel = VisualPath.bMovingVisuals || bActorMoved;
    const bool bSpreadCrawling = OwnerCompany->StanceComponent && OwnerCompany->StanceComponent->Stance == EStrategyStance::Prone;
    UAnimSequence* Walk = (bSpreadCrawling ? CrawlProneAsset : WalkStandingAsset).Get();
    bool bAny = false;
    UAnimSequence* Run = bSpreadCrawling ? Walk : RunStandingAsset.Get();
    bool bSettleIdleLoop = true;
    UAnimSequence* SettleIdle = ResolveAnimation(bSettleIdleLoop);
    for (int32 i = 0; i < SoldierComponents.Num() && i < SoldierSettle.Num(); ++i)
    {
        FSettle& Settle = SoldierSettle[i];
        if (bVisualTravel && !bWasVisualTravel) Settle.StartRemaining = Settle.StartDelay;
        if (!bVisualTravel && bWasVisualTravel) Settle.HaltRemaining = Settle.StartDelay;
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        if (Soldier && Settle.bPlaced && (bActorMoved || bVisualTravel || Settle.bActive))
        {
            // Undo inherited actor rotation before walking towards the new visual slot.
            const FVector PreviousWorld = VisualPath.PreviousUnit.TransformPosition(Soldier->GetRelativeLocation());
            Soldier->SetRelativeLocation(VisualUnit.InverseTransformPosition(PreviousWorld));
            FRotator VisualRotation = Soldier->GetRelativeRotation();
            VisualRotation.Yaw += FMath::FindDeltaAngleDegrees(VisualUnit.Rotator().Yaw, VisualPath.PreviousUnit.Rotator().Yaw);
            Soldier->SetRelativeRotation(VisualRotation);
            float GoalYaw = 0.f;
            const bool bColumnScreen = bVisualColumn && OwnerCompany->SkirmisherComponent &&
                (OwnerCompany->SkirmisherComponent->State == EStrategySkirmisherState::Deploying || OwnerCompany->SkirmisherComponent->IsDeployed());
            const bool bScreenFigure = bColumnScreen && Settle.Slot.X > 0.f;
            FVector SpecialPathSlot = Settle.Slot;
            // Screen men walk ahead independently; the column's head remains at its original trail coordinate.
            if (bColumnScreen && !bScreenFigure && VisualPath.Trail.Num() >= 2) SpecialPathSlot.X += VisualPath.SlotBounds.Max.X;
            Settle.Goal = VisualUnit.InverseTransformPosition(VisualPath.Goal(SpecialPathSlot, bVisualColumn && !bScreenFigure, GoalYaw));
            Settle.Yaw = GoalYaw - VisualUnit.Rotator().Yaw + Settle.SlotYaw + SoldierMeshYawOffset;
            Settle.bActive = Settle.bActive || bVisualTravel || FVector::Dist2D(Soldier->GetRelativeLocation(), Settle.Goal) > 1.f;
        }
        if (!Settle.bActive || !Soldier)
        {
            continue;
        }
        Settle.StartRemaining = FMath::Max(0.f, Settle.StartRemaining - DeltaTime);
        Settle.HaltRemaining = FMath::Max(0.f, Settle.HaltRemaining - DeltaTime);
        if (Settle.StartRemaining > 0.f)
        {
            Soldier->bPauseAnims = true;
            if (SoldierClips.IsValidIndex(i) && SoldierClips[i].Clip.IsValid())
                RecordClip(Soldier, SoldierClips[i].Clip.Get(), true,
                    SoldierClips[i].Rate > 0.f ? (VisualAnimationTime - SoldierClips[i].Start) * SoldierClips[i].Rate : SoldierClips[i].HeldPosition, 0.f);
            bAny = true;
            continue;
        }
        FVector Here = Soldier->GetRelativeLocation();
        FVector Delta = Settle.Goal - Here;
        const float Distance = Delta.Size();
        if (Distance < 0.1f && Settle.WorldVelocity.Size() < 1.f && !bVisualTravel)
        {
            Settle.WorldVelocity = FVector::ZeroVector;
            FRotator ArrivedRotation = Soldier->GetRelativeRotation();
            ArrivedRotation.Yaw = FStrategyVisualFormationPath::SmoothFacing(ArrivedRotation.Yaw, Settle.YawVelocity, Settle.Yaw, DeltaTime, 120.f);
            ArrivedRotation.Roll = FMath::FInterpTo(ArrivedRotation.Roll, 0.f, DeltaTime, 8.f);
            Soldier->SetRelativeRotation(ArrivedRotation);
            Settle.bActive = Settle.HaltRemaining > 0.f || FMath::Abs(FMath::FindDeltaAngleDegrees(ArrivedRotation.Yaw, Settle.Yaw)) > 0.1f;
            Soldier->bPauseAnims = true;
            if (Walk && SoldierClips.IsValidIndex(i) &&
                (SoldierClips[i].Clip.Get() == Walk || SoldierClips[i].Clip.Get() == Run))
                RecordClip(Soldier, SoldierClips[i].Clip.Get(), true,
                    SoldierClips[i].Rate > 0.f ? (VisualAnimationTime - SoldierClips[i].Start) * SoldierClips[i].Rate : SoldierClips[i].HeldPosition, 0.f);
            if (!Settle.bActive && !bVisualTravel && SettleIdle)
            {
                if (SoldierClips.IsValidIndex(i) && SoldierClips[i].Clip.Get() != SettleIdle) Soldier->PlayAnimation(SettleIdle, bSettleIdleLoop);
                Soldier->SetPlayRate(1.f);
                Soldier->bPauseAnims = !bAnimateIdle;
                RecordClip(Soldier, SettleIdle, bSettleIdleLoop, 0.f, bAnimateIdle ? 1.f : 0.f);
            }
            bAny |= Settle.bActive;
            bCrowdDirty = true;
            continue;
        }
        bAny = true;
        const bool bSlotReforming = OwnerCompany->FormationTransition && OwnerCompany->FormationTransition->IsReforming();
        // Carry the slot at unit speed; personal pace only affects closing a slot error.
        const float FigureSpeed = ActualMarchSpeed +
            ((bSlotReforming || VisualPath.bTurning) ? RunCmPerSecond : 160.f) * Settle.Pace;
        const float SpecialFigureSpeed = bSpreadCrawling ? FMath::Min(FigureSpeed, VisualWalkSpeed * 0.40f) : FigureSpeed;
        FVector FigureWorld = VisualUnit.TransformPosition(Here);
        const FVector FigureBefore = FigureWorld;
        const FVector FigureTarget = VisualUnit.TransformPosition(Settle.Goal);
        // Integrate presentation velocity every frame, with braking near the slot.
        // Substeps affect no orders, simulation positions, collision or game clocks.
        FStrategyVisualFormationPath::SmoothTravel(FigureWorld, Settle.WorldVelocity, FigureTarget, DeltaTime, SpecialFigureSpeed);
        const FVector Step = FigureWorld - FigureBefore;
        Soldier->SetRelativeLocation(VisualUnit.InverseTransformPosition(FigureWorld));
        // He faces the way he runs while he is far from his place, and turns to the front as he comes in.
        const float RunYaw = VisualUnit.InverseTransformVectorNoScale(Settle.WorldVelocity).Rotation().Yaw + SoldierMeshYawOffset;
        const bool bFigureBackstep = OwnerCompany->OrderComponent &&
            OwnerCompany->OrderComponent->GetCurrentOrder().Type == EStrategyOrderType::Disengage;
        const float Yaw = bFigureBackstep ? Settle.Yaw : (bVisualTravel || Distance > 150.0f ? RunYaw : Settle.Yaw);
        FRotator Rot = Soldier->GetRelativeRotation();
        const float FigureTurn = FMath::FindDeltaAngleDegrees(Rot.Yaw, Yaw);
        Rot.Roll = FMath::FInterpTo(Rot.Roll, FMath::Clamp(FigureTurn * 0.08f, -4.f, 4.f), DeltaTime, 8.f);
        Rot.Yaw = FStrategyVisualFormationPath::SmoothFacing(Rot.Yaw, Settle.YawVelocity, Yaw, DeltaTime, 120.f);
        Soldier->SetRelativeRotation(Rot);
        Soldier->bPauseAnims = false;
        UAnimSequence* TravelClip = !bSlotReforming && Walk ? Walk : Run;
        if (TravelClip && SoldierClips.IsValidIndex(i) && (SoldierClips[i].Clip.Get() != TravelClip))
        {
            Soldier->bPauseAnims = false;
            Soldier->PlayAnimation(TravelClip, true);
            const float FigurePhase = Settle.WalkPhase * TravelClip->GetPlayLength();
            Soldier->SetPosition(FigurePhase, false);
            RecordClip(Soldier, TravelClip, true, FigurePhase, 1.0f);
        }
        if (TravelClip && SoldierClips.IsValidIndex(i))
        {
            const FPlayedClip PreviousClip = SoldierClips[i];
            const float DesiredVisualRate = FMath::Clamp(Step.Size2D() / FMath::Max(0.001f, DeltaTime) / (TravelClip == Run ? RunCmPerSecond : 160.f), 0.01f, 4.f);
            const float VisualRate = FMath::Lerp(PreviousClip.Rate, DesiredVisualRate, 1.f - FMath::Exp(-12.f * DeltaTime));
            const float VisualNow = VisualAnimationTime;
            Soldier->SetPlayRate(VisualRate);
            // Preserve animation phase when changing rate (also for distant VAT figures).
            SoldierClips[i].Start = VisualNow - (PreviousClip.Rate > 0.f ? (VisualNow - PreviousClip.Start) * PreviousClip.Rate : PreviousClip.HeldPosition) / VisualRate;
            SoldierClips[i].Rate = VisualRate;
            bCrowdDataDirty = true;
        }
        bCrowdDirty = true;
    }
    VisualPath.PreviousUnit = VisualUnit;
    bWasSettling = bAny;
    bWasVisualTravel = bVisualTravel;
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

    const bool bAnimationUnchanged = !bForce && LastAnimationAsset == Sequence &&
        bLastAnimationLooping == bLooping && bLastHoldingPose == bHoldPose;
    if (bAnimationUnchanged && !SoldierClips.ContainsByPredicate(
        [](const FPlayedClip& VisualClip) { return !VisualClip.Clip.IsValid(); }))
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
        if (!Soldier || (bAnimationUnchanged && SoldierClips.IsValidIndex(Index) && SoldierClips[Index].Clip.IsValid()) ||
            (SoldierFirePhase.IsValidIndex(Index) && SoldierFirePhase[Index] != 0) || (SoldierSettle.IsValidIndex(Index) && SoldierSettle[Index].bActive))
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
        return IdleStandingAsset.Get();
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
        return Chosen.Get();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Fire)
    {
        bOutLooping = false;

        if (CurrentStance == EStrategyStance::Prone &&
            !FireProneAsset.IsNull())
        {
            return FireProneAsset.Get();
        }

        if (CurrentStance == EStrategyStance::Kneeling &&
            !FireKneelingAsset.IsNull())
        {
            return FireKneelingAsset.Get();
        }

        return FireStandingAsset.Get();
    }

    if (OwnerCompany->CombatComponent &&
        OwnerCompany->CombatComponent->IsReloading())
    {
        bOutLooping = true;

        if (CurrentStance == EStrategyStance::Prone &&
            !ReloadProneAsset.IsNull())
        {
            return ReloadProneAsset.Get();
        }

        if (CurrentStance == EStrategyStance::Kneeling &&
            !ReloadKneelingAsset.IsNull())
        {
            return ReloadKneelingAsset.Get();
        }

        if (!ReloadStandingAsset.IsNull())
        {
            return ReloadStandingAsset.Get();
        }

        return AimStandingAsset.Get();
    }

    if (Action ==
        EStrategyHumanAnimationAction::BayonetCharge)
    {
        return BayonetChargeAsset.Get();
    }

    if (Action ==
        EStrategyHumanAnimationAction::BayonetReady)
    {
        return AimStandingAsset.Get();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Run ||
        Action ==
        EStrategyHumanAnimationAction::RoutedRun)
    {
        return RunStandingAsset.Get();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Walk)
    {
        if (CurrentStance == EStrategyStance::Prone &&
            !CrawlProneAsset.IsNull())
        {
            return CrawlProneAsset.Get();
        }

        return WalkStandingAsset.Get();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Aim)
    {
        if (CurrentStance == EStrategyStance::Prone &&
            !IdleProneAsset.IsNull())
        {
            return IdleProneAsset.Get();
        }

        if (CurrentStance == EStrategyStance::Kneeling &&
            !AimKneelingAsset.IsNull())
        {
            return AimKneelingAsset.Get();
        }

        return AimStandingAsset.Get();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Prone ||
        CurrentStance ==
        EStrategyStance::Prone)
    {
        return IdleProneAsset.Get();
    }

    if (Action ==
        EStrategyHumanAnimationAction::Kneel ||
        CurrentStance ==
        EStrategyStance::Kneeling)
    {
        return IdleKneelingAsset.Get();
    }

    return IdleStandingAsset.Get();
}

void UStrategyInfantryVisualComponent::DestroyVisualComponents()
{
    VisualPath.bInitialized = false;
    VisualPath.Trail.Reset();
    bSmoothSampled = false;
    SmoothMaxCenter = 0.f;
    for (int32 SmoothResetIndex = 0; SmoothResetIndex < 5; ++SmoothResetIndex)
    {
        SmoothSamples[SmoothResetIndex].Reset();
        SmoothMaxMen[SmoothResetIndex] = 0.f;
    }
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
    bResumeCrowdPosesNextTick = false;
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
    const float Now = VisualAnimationTime;
    FPlayedClip& Played = SoldierClips[Index];
    Played.Clip = Clip;
    Played.bLoop = bLoop;
    Played.Rate = Rate;
    const float PlayedLength = Clip ? Clip->GetPlayLength() : 0.f;
    Played.HeldPosition = bLoop && PlayedLength > 0.f ? FMath::Fmod(FMath::Max(0.f, Position), PlayedLength) : Position;
    Played.Start = Rate > 0.0f ? Now - Position / Rate : Now;
    if (bCrowdMode && !bCrowdDirty && CrowdLiving && CrowdModel && Index < CrowdLiving->GetInstanceCount())
    {
        float Data[UStrategyCrowdModel::CustomDataFloats];
        if (MakeCrowdClipData(Played, Data))
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
            if (UAnimSequence* Clip = Asset->Get())
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
    if (Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Warmup")))
    {
        UStaticMesh* BattleOtherRifle = CurrentRifleMesh() == LoadedRifleMesh.Get() ? LoadedRifleBayonetMesh.Get() : LoadedRifleMesh.Get();
        if (BattleOtherRifle) UStrategyCrowdModel::Find(GetWorld(), LoadedSoldierMesh, BattleOtherRifle, Clips, Grip);
    }
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
    if (Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Figures")))
    {
        if (const IConsoleVariable* BattleNearVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Strategy1864.Perf.NearCm")))
        { Far = FMath::Max(0.f, BattleNearVar->GetFloat()); Near = Far * 0.8f; }
    }
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
    const bool bBattleNearAllowed = Strategy1864Performance::AllowNear(this);
    if (!bCrowdMode && (Distance > Far || !bBattleNearAllowed))
    {
        EnterCrowdMode();
    }
    else if (bCrowdMode && Distance < Near && bBattleNearAllowed)
    {
        LeaveCrowdMode();
    }
    if (bCrowdMode)
    {
        // Rebase frozen phases against shader time every frame, including paused clips.
        if (OwnerCompany->CustomTimeDilation != 1.f) bCrowdDirty = true;
        if (SoldierClips.ContainsByPredicate([](const FPlayedClip& CrowdClip) { return CrowdClip.Rate <= 0.f && CrowdClip.HeldPosition > 0.f; })) bCrowdDirty = true;
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
    float Position = Played.Rate > 0.0f ? FMath::Max(0.0f, (Now - Played.Start) * Played.Rate) : Played.HeldPosition;
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
    bResumeCrowdPosesNextTick = true;
    const float Now = VisualAnimationTime;
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
                Soldier->bPauseAnims = true;
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
                Corpse->bPauseAnims = true;
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
        if (!Played || !MakeCrowdClipData(*Played, Out))
        {
            if (!CrowdModel->MakeCustomData(Idle, 0.0f, 0.0f, true, Out))
            {
                FMemory::Memzero(Out, sizeof(float) * UStrategyCrowdModel::CustomDataFloats);
            }
        }
    };
    float Data[UStrategyCrowdModel::CustomDataFloats];

    TArray<FTransform> BattleLocalLiving;
    TArray<FTransform>& Living = Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Hitches")) ? CrowdLivingScratch : BattleLocalLiving;
    Living.Reset();
    for (const USkeletalMeshComponent* Soldier : SoldierComponents)
    {
        Living.Add(Soldier ? Soldier->GetRelativeTransform() : FTransform::Identity);
    }
    if (CrowdLiving->GetInstanceCount() != Living.Num())
    {
        CrowdLiving->ClearInstances();
        CrowdLiving->AddInstances(Living, false, false);
    }
    else if (!Living.IsEmpty()) CrowdLiving->BatchUpdateInstancesTransforms(0, Living, false, false, false);
    for (int32 i = 0; i < Living.Num(); ++i)
    {
        DataFor(SoldierClips.IsValidIndex(i) ? &SoldierClips[i] : nullptr, Data);
        CrowdLiving->SetCustomData(i, TArrayView<const float>(Data, UStrategyCrowdModel::CustomDataFloats), false);
    }

    TArray<FTransform> BattleLocalFallen;
    TArray<FTransform>& Fallen = Strategy1864Performance::Enabled(TEXT("Strategy1864.Perf.Hitches")) ? CrowdFallenScratch : BattleLocalFallen;
    Fallen.Reset();
    for (const USkeletalMeshComponent* Corpse : CorpseComponents)
    {
        FTransform FallenTransform = Corpse ? Corpse->GetComponentTransform() : FTransform::Identity;
        const int32 FallenIndex = Fallen.Num();
        if (CorpseBirthTimes.IsValidIndex(FallenIndex) && GetWorld()->GetTimeSeconds() - CorpseBirthTimes[FallenIndex] >= 60.f)
            FallenTransform.SetScale3D(FVector::ZeroVector);
        Fallen.Add(FallenTransform);
    }
    if (CrowdFallen->GetInstanceCount() != Fallen.Num())
    {
        CrowdFallen->ClearInstances();
        CrowdFallen->AddInstances(Fallen, false, true);
    }
    else if (!Fallen.IsEmpty()) CrowdFallen->BatchUpdateInstancesTransforms(0, Fallen, true, false, false);
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

void UStrategyInfantryVisualComponent::DebugSmooth()
{
    static const bool bSmoothDebug = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugSmooth"));
    if (!bSmoothDebug || !GetWorld()) return;
    // First enabled company in this world; no static actor pointer survives PIE/world changes.
    for (const TWeakObjectPtr<AStrategyUnit>& BattleSmoothEntry : Strategy1864Performance::VisualUnits(GetWorld()))
    {
        const AStrategyCompanyUnit* BattleSmoothCompany = Cast<AStrategyCompanyUnit>(BattleSmoothEntry.Get());
        if (!BattleSmoothCompany) continue;
        UStrategyInfantryVisualComponent* SmoothVisual = BattleSmoothCompany->FindComponentByClass<UStrategyInfantryVisualComponent>();
        if (SmoothVisual && SmoothVisual->bEnabled)
        {
            if (SmoothVisual != this) return;
            break;
        }
    }
    const double SmoothNow = FPlatformTime::Seconds();
    SmoothMaxRealDelta = FMath::Max(SmoothMaxRealDelta, float(FApp::GetDeltaTime()));
    const FVector SmoothDrawnCenter = CentroidFigureCount > 0 ? OwnerCompany->GetActorTransform().TransformPosition(FigureLocalCentroid) : VisualPath.Center;
    if (!bSmoothSampled)
    {
        SmoothPreviousCenter = SmoothDrawnCenter;
        SmoothPreviousAnchor = VisualPath.Center;
        SmoothLogAt = SmoothNow + 1.0;
        bSmoothSampled = true;
    }
    SmoothMaxCenter = FMath::Max(SmoothMaxCenter, float(FVector::Dist(SmoothDrawnCenter, SmoothPreviousCenter)));
    SmoothPreviousCenter = SmoothDrawnCenter;
    SmoothMaxAnchor = FMath::Max(SmoothMaxAnchor, float(FVector::Dist(VisualPath.Center, SmoothPreviousAnchor)));
    SmoothPreviousAnchor = VisualPath.Center;
    for (int32 SmoothIndex = 0; SmoothIndex < 5; ++SmoothIndex)
    {
        USkeletalMeshComponent* SmoothMan = SmoothSamples[SmoothIndex].Get();
        if (!SmoothMan || !SoldierComponents.Contains(SmoothMan))
        {
            SmoothSamples[SmoothIndex].Reset();
            for (USkeletalMeshComponent* SmoothCandidate : SoldierComponents)
            {
                bool bAlreadySampled = false;
                for (const auto& SmoothExisting : SmoothSamples) bAlreadySampled |= SmoothExisting.Get() == SmoothCandidate;
                if (SmoothCandidate && !bAlreadySampled)
                {
                    SmoothMan = SmoothCandidate;
                    SmoothSamples[SmoothIndex] = SmoothMan;
                    SmoothPreviousMen[SmoothIndex] = SmoothMan->GetComponentLocation();
                    break;
                }
            }
        }
        if (!SmoothSamples[SmoothIndex].IsValid()) continue;
        const FVector SmoothPosition = SmoothMan->GetComponentLocation();
        SmoothMaxMen[SmoothIndex] = FMath::Max(SmoothMaxMen[SmoothIndex], float(FVector::Dist(SmoothPosition, SmoothPreviousMen[SmoothIndex])));
        SmoothPreviousMen[SmoothIndex] = SmoothPosition;
    }
    if (SmoothNow >= SmoothLogAt)
    {
        UE_LOG(LogTemp, Display, TEXT("Strategy1864DebugSmooth %s max-frame-cm centre=%.3f anchor=%.3f men=[%.3f,%.3f,%.3f,%.3f,%.3f] crowd=%d max-frame-ms=%.3f dilation=%.3f"),
            *OwnerCompany->StableUnitId.ToString(), SmoothMaxCenter, SmoothMaxAnchor, SmoothMaxMen[0], SmoothMaxMen[1], SmoothMaxMen[2], SmoothMaxMen[3], SmoothMaxMen[4], bCrowdMode, SmoothMaxRealDelta * 1000.f, UGameplayStatics::GetGlobalTimeDilation(this) * OwnerCompany->CustomTimeDilation);
        SmoothMaxCenter = 0.f;
        SmoothMaxAnchor = 0.f;
        SmoothMaxRealDelta = 0.f;
        for (float& SmoothMaximum : SmoothMaxMen) SmoothMaximum = 0.f;
        SmoothLogAt = SmoothNow + 1.0;
    }
}

bool UStrategyInfantryVisualComponent::MakeCrowdClipData(const FPlayedClip& Played, float* Out) const
{
    if (!CrowdModel || !GetWorld()) return false;
    const UAnimSequence* PlayedSequence = Played.Clip.Get();
    float PlayedPosition = Played.Rate > 0.f ? FMath::Max(0.f, (VisualAnimationTime - Played.Start) * Played.Rate) : Played.HeldPosition;
    if (PlayedSequence && Played.bLoop && PlayedSequence->GetPlayLength() > 0.f)
        PlayedPosition = FMath::Fmod(PlayedPosition, PlayedSequence->GetPlayLength());
    const float PlayedWorldRate = Played.Rate * FMath::Max(0.f, OwnerCompany->CustomTimeDilation);
    const float PlayedWorldNow = GetWorld()->GetTimeSeconds();
    const float PlayedWorldStart = PlayedWorldRate > 0.f ? PlayedWorldNow - PlayedPosition / PlayedWorldRate : PlayedWorldNow;
    return CrowdModel->MakeCustomData(PlayedSequence, PlayedWorldStart, PlayedWorldRate, Played.bLoop, Out, PlayedWorldNow, PlayedPosition);
}
