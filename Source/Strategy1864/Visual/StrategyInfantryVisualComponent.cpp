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
#include "Engine/SkinnedAsset.h"
#include "DrawDebugHelpers.h"
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
#include "CoreGlobals.h"
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
    WalkShoulderAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_WalkShoulder.A_Combat_WalkShoulder")));
    ShoulderUpAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_ShoulderUp.A_Combat_ShoulderUp")));
    ShoulderDownAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_ShoulderDown.A_Combat_ShoulderDown")));
    SideStepAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_SideStep.A_Combat_SideStep")));
    SideStepLeftAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_SideStepLeft.A_Combat_SideStepLeft")));
    ColourWalkAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_ColourWalk.A_Combat_ColourWalk")));
    ColourIdleAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_ColourIdle.A_Combat_ColourIdle")));

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

    // The component ticks while paused (the crowd follows the camera) but the men stand still: no marching or re-forming under a pause.
    if (UGameplayStatics::IsGamePaused(this)) { DeltaTime = 0.f; }
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
        const int32 StandingBeforeTickLoss = SoldierComponents.Num();
        if (bLeaveCorpses && CachedStrength != INDEX_NONE && Desired < Standing)
        {
            QueueKills(Standing - Desired);
        }
        // Men who fell at once: the man behind steps up inside his own file (a loss that lands later is handled by ProcessPendingKills).
        if (SoldierComponents.Num() < StandingBeforeTickLoss) { ScheduleStepUp(); }
        EnsureVisualCount(Desired + PendingKillCount);
        bScaleOnlyRebuild = true; // Losses leave holes; surviving identities keep their slots.
        RebuildFormation();
        bScaleOnlyRebuild = false;
        if (CachedStrength != INDEX_NONE && CurrentStrength < CachedStrength)
        {
            if (!bClosingRanksPending) { FirstPendingLossTimeSeconds = GetWorld()->GetTimeSeconds(); }
            bClosingRanksPending = true;
            LastLossTimeSeconds = GetWorld()->GetTimeSeconds();
        }
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
    {
        const bool bColumnNow = OwnerCompany->FormationComponent &&
            (OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
             OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn);
        if (bColumnNow != bShoulderColumn)
        {
            bShoulderColumn = bColumnNow;
            bShoulderRising = bColumnNow;
            ShoulderTransitionStart = GetWorld()->GetTimeSeconds();
            // Safety cap only: UpdateSettling ends the transition when the Up/Down clips have played to their end (length / play rate).
            ShoulderTransitionEnd = ShoulderTransitionStart + 4.0f;
        }
    }
    // Close ranks: two and a half quiet seconds after the last loss, with the company standing still, the survivors re-form into the compact line.
    const float CloseRanksSlowdown = 1.0f + (1.0f - FMath::Clamp(OwnerCompany->Morale, 0.f, 100.f) / 100.f);
    // In a fight the company holds or creeps forward under a standing goal, so only the visible march blocks it; low morale slows the closing (3-10 s as the design paper asks), steady losses close up after 8 s at the latest.
    // The man behind steps up only after a moment (2 s, up to 4 s at low morale), so a hole is seen before it is filled.
    if (bStepUpPending && GetWorld()->GetTimeSeconds() >= StepUpDueSeconds)
    {
        bStepUpPending = false;
        StepUpFiles();
        bScaleOnlyRebuild = true;
        RebuildFormation();
        bScaleOnlyRebuild = false;
    }
    if (bClosingRanksPending && PendingKillCount == 0 && !bFormationPlanPending && !bArrivalReform &&
        (GetWorld()->GetTimeSeconds() - LastLossTimeSeconds > 8.0f * CloseRanksSlowdown || GetWorld()->GetTimeSeconds() - FirstPendingLossTimeSeconds > 20.0f * CloseRanksSlowdown) &&
        !(OwnerCompany->FormationTransition && OwnerCompany->FormationTransition->IsReforming()) && !VisualPath.bMovingVisuals)
    {
        bClosingRanksPending = false;
        // In line: the files close in towards the colours by side-stepping; otherwise (column, square, spread) the old re-form.
        if (!CloseRanksTowardColours())
        {
            bStepUpPending = false;   // the full re-form also fills the files
            RebuildFormation();
        }
    }
    UpdateSettling(DeltaTime);
    UpdateColourBearer(DeltaTime);
    ApplyPendingStance(GetWorld()->GetTimeSeconds());
    MarchDust(GetWorld()->GetTimeSeconds());
    RefreshAnimation(false);
    UpdateStanceTransitions();
    UpdatePersonalActions();
    UpdateCrowdMode();
    AlignWeapons();
    UpdateFormationBounds();
    DebugSmooth();
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864AnimationAudit")))
    {
        const float Now = GetWorld()->GetTimeSeconds();
        if (FMath::FloorToInt(Now / 10.0f) != FMath::FloorToInt((Now - DeltaTime) / 10.0f))
        {
            int32 Walking = 0, Firing = 0, Loading = 0, ShoulderClips = 0;
            for (const FPlayedClip& Clip : SoldierClips)
            {
                const FString Name = Clip.Clip.IsValid() ? Clip.Clip->GetName() : FString();
                Walking += Name.Contains(TEXT("Walk")) ? 1 : 0;
                ShoulderClips += Name.Contains(TEXT("Shoulder")) ? 1 : 0;
                Firing += Name.Contains(TEXT("Fire")) || Name.Contains(TEXT("Firing")) ? 1 : 0;
                Loading += Name.Contains(TEXT("Reload")) ? 1 : 0;
            }
            UE_LOG(LogTemp, Display, TEXT("COMBAT-ANIMATION-AUDIT %s strength=%d walk=%d fire=%d reload=%d corpses=%d shoulderCol=%d shoulderClips=%d formation=%d"),
                *OwnerCompany->DisplayName.ToString(), CurrentStrength, Walking, Firing, Loading, CorpseComponents.Num(), bShoulderColumn ? 1 : 0, ShoulderClips,
                OwnerCompany->FormationComponent ? int32(OwnerCompany->FormationComponent->CurrentFormation) : -1);
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

    int32 CrowdCommandEnabled = 1;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Crowd="), CrowdCommandEnabled);
    if (CrowdFarCm > 0.f && CrowdCommandEnabled != 0)
    {
        EnsureCrowdModel();
        if (CrowdModel && !bCrowdMode) EnterCrowdMode();
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
    const float Spread = Mode == EStrategyFireDrillMode::Volley ? 0.15f : Mode == EStrategyFireDrillMode::Independent ? 2.5f : 0.35f;
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
    UAnimSequence* Ready = (Stance == EStrategyStance::Prone ? IdleProneAsset : Stance == EStrategyStance::Kneeling ? IdleKneelingAsset : ReadyAsset).Get();
    UAnimSequence* AimHold = (Stance == EStrategyStance::Prone ? IdleProneAsset : Stance == EStrategyStance::Kneeling ? AimKneelingAsset : AimHoldAsset).Get();
    UAnimSequence* Raise = Stance == EStrategyStance::Standing ? RaiseToAimAsset.Get() : nullptr;
    const float RiseLength = Rise ? Rise->GetPlayLength() : 0.0f;
    for (int32 i = 0; i < SoldierComponents.Num(); ++i)
    {
        USkeletalMeshComponent* Soldier = SoldierComponents[i];
        if (!Soldier || (SoldierSettle.IsValidIndex(i) && SoldierSettle[i].bActive))
        {
            continue;
        }
        if (SoldierSettle.IsValidIndex(i) && SoldierSettle[i].StanceEnd > VisualAnimationTime) continue;
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
                // The men are loaded: they bring the musket up briskly (twice the clip's speed) so the first volley is not slow in coming.
                const float RaiseRate = 2.0f;
                PlayOnSoldier(Soldier, Raise, false, false);
                Soldier->SetPlayRate(RaiseRate);
                RecordClip(Soldier, Raise, false, 0.0f, RaiseRate);
                Phase = PhaseRaise;
                SoldierBusyUntil[i] = Now + Raise->GetPlayLength() * 0.85f / RaiseRate;
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
                if (Now >= SoldierBusyUntil[i] && AimHold &&
                    (!SoldierClips.IsValidIndex(i) || SoldierClips[i].Clip.Get() != AimHold))
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
    if (bCrowdMode && CrowdModel && SoldierClips.IsValidIndex(Index) && CurrentRifleMesh())
    {
        const FPlayedClip& CrowdShotClip = SoldierClips[Index];
        const float CrowdShotPosition = CrowdShotClip.Rate > 0.f ? FMath::Max(0.f, (VisualAnimationTime - CrowdShotClip.Start) * CrowdShotClip.Rate) : CrowdShotClip.HeldPosition;
        FTransform CrowdRiflePose;
        if (CrowdModel->RiflePose(CrowdShotClip.Clip.Get(), CrowdShotPosition, CrowdShotClip.bLoop, CrowdRiflePose))
        {
            const FTransform CrowdRifleWorld = CrowdRiflePose * Soldier->GetComponentTransform();
            const FBox CrowdRifleBox = CurrentRifleMesh()->GetBoundingBox();
            Muzzle = CrowdRifleWorld.TransformPosition(FVector(bRifleBarrelAlongNegativeX ? CrowdRifleBox.Min.X : CrowdRifleBox.Max.X, 0.f, 0.f));
            Forward = CrowdRifleWorld.GetRotation().GetForwardVector() * (bRifleBarrelAlongNegativeX ? -1.f : 1.f);
        }
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
    const int32 StandingBeforeCasualty = SoldierComponents.Num();
    if (!bDirectedFire && FStrategyImpactRegistry::Find(GetWorld(), OwnerCompany->GetActorLocation(), 6500.f, CasualtyNow, CasualtyImpact))
        QueueKills(FiguresLost);
    else
        KillSoldiers(FiguresLost, &SourceLocation);
    CachedStrength = OwnerCompany->CurrentStrength;
    // Local replacement first: the man directly behind steps up inside his own file, no sideways move yet.
    if (SoldierComponents.Num() < StandingBeforeCasualty)
    {
        ScheduleStepUp();
        bScaleOnlyRebuild = true;
        RebuildFormation();
        bScaleOnlyRebuild = false;
    }
    // CachedStrength is set above, so the tick never sees this drop: the delayed closing of ranks is armed here (8-40 s, slower at low morale).
    if (!bClosingRanksPending) { FirstPendingLossTimeSeconds = CasualtyNow; }
    bClosingRanksPending = true;
    LastLossTimeSeconds = CasualtyNow;
    UpdateFormationBounds();
}

// The fallen stay on the field for the whole battle (the figure budget caps how many bodies are kept).
static constexpr float kCorpseLifeSeconds = 100000.f;

void UStrategyInfantryVisualComponent::UpdateCorpses()
{
    if (CorpseComponents.IsEmpty() || !GetWorld()) return;
    const float CorpseNow = GetWorld()->GetTimeSeconds();
    for (int32 CorpseIndex = CorpseComponents.Num() - 1; CorpseIndex >= 0; --CorpseIndex)
    {
        USkeletalMeshComponent* FallenMesh = CorpseComponents[CorpseIndex];
        const float CorpseAge = CorpseNow - CorpseBirthTimes[CorpseIndex];
        if (CorpseAge >= kCorpseLifeSeconds)
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
        else if (CorpseAge >= kCorpseLifeSeconds - 5.f)
        {
            // A real translucent fade also works when the original uniform material is opaque.
            if (bCrowdMode) { bCrowdDirty = true; continue; }
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
                    FadeMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.22f, 0.25f, 0.28f, 1.f - (CorpseAge - (kCorpseLifeSeconds - 5.f)) / 5.f));
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

void UStrategyInfantryVisualComponent::ScheduleStepUp()
{
    const float StepUpNow = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    const float StepUpSlowdown = OwnerCompany ? 1.0f + (1.0f - FMath::Clamp(OwnerCompany->Morale, 0.f, 100.f) / 100.f) : 1.0f;
    // The first loss sets the moment; further losses before it do not push it back (the men behind are already moving up).
    if (!bStepUpPending) { StepUpDueSeconds = StepUpNow + 2.0f * StepUpSlowdown; }
    bStepUpPending = true;
}

bool UStrategyInfantryVisualComponent::IsClosingRanks() const
{
    // A marching column closes its ranks only when it halts, so a pending close does not count as forming while it marches.
    const bool bClosingColumn = OwnerCompany && OwnerCompany->FormationComponent &&
        OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn;
    return bClosingRanksStep || (bClosingRanksPending && !bClosingColumn);
}

void UStrategyInfantryVisualComponent::StepUpColumn()
{
    // March column (user 2026-10-22: "if the first or second man of the column falls, the rear man closes up"): in every lane the
    // men behind a hole move up one place each, the nearest first, while the column walks on; the tail gets shorter. Only for the
    // snake, whose men know their row and lane; not while it is still forming (the ramp sets the places itself).
    if (!OwnerCompany || !OwnerCompany->FormationComponent || bSnakeRamp || DrillActive ||
        OwnerCompany->FormationComponent->CurrentFormation != EStrategyFormationType::MarchColumn ||
        SoldierSettle.Num() != SoldierComponents.Num()) return;
    bool bStepColumnKnown = false;
    for (const FSettle& StepColumnMan : SoldierSettle) bStepColumnKnown |= StepColumnMan.SnakeRow > 0;
    if (!bStepColumnKnown) return;
    const float StepColumnRow = SnakeRowSpacing > 0.f ? SnakeRowSpacing : OwnerCompany->FormationComponent->SoldierLateralSpacingCm;
    int32 StepColumnMovers = 0;
    for (int32 StepColumnLane = 0; StepColumnLane < 3; ++StepColumnLane)
    {
        TArray<int32> StepColumnFile;
        for (int32 StepColumnIndex = 0; StepColumnIndex < SoldierSettle.Num(); ++StepColumnIndex)
            if (SoldierSettle[StepColumnIndex].bPlaced && SoldierSettle[StepColumnIndex].SnakeLane == StepColumnLane) StepColumnFile.Add(StepColumnIndex);
        StepColumnFile.Sort([this](int32 A, int32 B) { return SoldierSettle[A].SnakeRow < SoldierSettle[B].SnakeRow; });
        int32 StepColumnHoles = 0;
        for (int32 StepColumnK = 0; StepColumnK < StepColumnFile.Num(); ++StepColumnK)
        {
            FSettle& StepColumnMan = SoldierSettle[StepColumnFile[StepColumnK]];
            if (StepColumnMan.SnakeRow == StepColumnK) continue;
            ++StepColumnHoles;
            const FVector StepColumnFrom = StepColumnMan.Slot;
            StepColumnMan.SnakeRow = StepColumnK;
            StepColumnMan.CloseFrom = StepColumnFrom;
            StepColumnMan.CloseTo = FVector(-StepColumnK * StepColumnRow, StepColumnFrom.Y, StepColumnFrom.Z);
            // The man right behind the hole goes first, those behind him follow a step later each (at most 2 s).
            StepColumnMan.CloseStart = VisualAnimationTime + FMath::Min(2.f, 0.3f * StepColumnHoles);
            StepColumnMan.CloseDuration = FMath::Max(0.8f, FVector::Dist2D(StepColumnMan.CloseTo, StepColumnMan.CloseFrom) / 70.f);
            StepColumnMan.bActive = true;
            ++StepColumnMovers;
        }
    }
    if (StepColumnMovers > 0)
    {
        bClosingRanksStep = true;
        UE_LOG(LogTemp, Display, TEXT("Strategy1864StepUp %s column: %d men close up in their lanes"), *OwnerCompany->StableUnitId.ToString(), StepColumnMovers);
    }
}

void UStrategyInfantryVisualComponent::StepUpFiles()
{
    if (OwnerCompany && OwnerCompany->FormationComponent && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn)
    {
        StepUpColumn();
        return;
    }
    if (!OwnerCompany || !OwnerCompany->FormationComponent ||
        OwnerCompany->FormationComponent->CurrentFormation != EStrategyFormationType::Line ||
        SoldierSettle.Num() != SoldierComponents.Num() || SoldierSlots.Num() != SoldierComponents.Num())
    {
        return;
    }
    const float RankSpacing = FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierRankSpacingCm);
    const float FileTolerance = FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierLateralSpacingCm * 0.4f);
    const int32 Ranks = FMath::Max(1, OwnerCompany->FormationComponent->RankCount);
    TArray<int32> Done;
    int32 StepUpMovers = 0;
    // A man still side-stepping or stepping up counts where he is going, not where he passes (2026-10-22 test: new losses in the
    // middle of a closing put two men of neighbouring files into one file, and one of them stayed 3 cm from his neighbour).
    const auto StepUpPlace = [this](int32 StepUpMan) { const FSettle& StepUpSettle = SoldierSettle[StepUpMan]; return StepUpSettle.CloseStart >= 0.f ? StepUpSettle.CloseTo : StepUpSettle.Slot; };
    for (int32 i = 0; i < SoldierSettle.Num(); ++i)
    {
        if (Done.Contains(i) || !SoldierSettle[i].bPlaced || StepUpPlace(i).X > 0.5f) { continue; }   // skirmisher screen men (slot ahead of the front) are no part of a file
        TArray<int32> File;   // everyone in this man's file, front to back
        for (int32 j = 0; j < SoldierSettle.Num(); ++j)
            if (SoldierSettle[j].bPlaced && StepUpPlace(j).X <= 0.5f && FMath::Abs(StepUpPlace(j).Y - StepUpPlace(i).Y) <= FileTolerance) { File.Add(j); Done.Add(j); }
        File.Sort([&](int32 A, int32 B) { return StepUpPlace(A).X > StepUpPlace(B).X; });
        // A file never holds more men than there are ranks: the extra men (two files read as one) keep their places.
        if (File.Num() > Ranks) File.SetNum(Ranks);
        for (int32 k = 0; k < File.Num(); ++k)
        {
            FSettle& Settle = SoldierSettle[File[k]];
            const FVector StepUpTarget = StepUpPlace(File[k]);
            const float NewX = -float(k) * RankSpacing;
            if (FMath::Abs(StepUpTarget.X - NewX) < 1.f) { continue; }
            // He walks up into the gap, front kept (user 2026-10-21: no man vanishes from his place and appears elsewhere). The
            // rank nearest the gap goes first; same machinery as closing ranks (UpdateCloseRanks moves the slot).
            Settle.CloseFrom = Settle.Slot;
            Settle.CloseTo = FVector(NewX, StepUpTarget.Y, Settle.Slot.Z);
            Settle.CloseStart = VisualAnimationTime + 0.25f * k;
            Settle.CloseDuration = FMath::Max(0.8f, FVector::Dist2D(Settle.CloseTo, Settle.CloseFrom) / 70.f);
            Settle.Goal = Settle.CloseTo;
            Settle.bActive = true;
            bClosingRanksStep = true;
            ++StepUpMovers;
            SoldierSlots[File[k]] = (SoldierSlots[File[k]] / Ranks) * Ranks + FMath::Min(k, Ranks - 1);
        }
    }
    if (StepUpMovers > 0)
        UE_LOG(LogTemp, Display, TEXT("Strategy1864StepUp %s %d men walk up into the gaps"), *OwnerCompany->StableUnitId.ToString(), StepUpMovers);
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
            if (bClosingRanksPending) { LastLossTimeSeconds = Now; }   // the closing of ranks counts from the moment the men fall
        }
    }
    if (bAny)
    {
        ScheduleStepUp();
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
        // The colour bearer is one man of the company: he falls as often as any other (front-rank fire hits him more).
        if (bColourBearerAlive && ColourBearer && !(Near && ConeOrigin) && FMath::RandRange(0, SoldierComponents.Num()) == 0)
        {
            KillColourBearer();
            continue;
        }
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
        // Rifle fire: the men fall where the bullets arrive, on the side facing the shooters (in line the front rank from ahead, the rear
        // rank from behind, the outer files from a flank; in square mostly the face turned to the shooters). Without a known direction:
        // random. The men behind step up (StepUpFiles), the line closes later.
        if (OwnerCompany && !OwnerCompany->CasualtySourceLocation.IsNearlyZero() && !(Near && ConeOrigin))
        {
            const FVector ShotLocal = OwnerCompany->GetActorTransform().InverseTransformPosition(OwnerCompany->CasualtySourceLocation);
            const float ShotAngle = FMath::Abs(FMath::RadiansToDegrees(FMath::Atan2(ShotLocal.Y, ShotLocal.X)));   // 0 = straight ahead
            // Which men, counted from the side that is shot at, from any direction (user 2026-10-21; reference
            // "Kompagni_1864_3Geledder_Historisk_Tab_Reform": 7/2/0 of 9). Each man's depth is measured along the line of fire from the
            // man nearest the shooters; the men fall in layers of that depth: mostly the first layer, now and then the second, very
            // rarely the third. From ahead the layers are the ranks, from behind the rear rank first, from a flank the outer files,
            // obliquely the corner facing the shooters. Weights 0.78/0.20/0.02 are an estimate.
            const FVector HitTowardShooters = FVector(ShotLocal.X, ShotLocal.Y, 0.f).GetSafeNormal();
            const float HitLateral = FMath::Max(1.f, OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierLateralSpacingCm : 60.f);
            const float HitRankSpacing = FMath::Max(1.f, OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierRankSpacingCm : 90.f);
            // One layer = the spacing of the men along the line of fire (rank spacing head-on, file spacing from a flank).
            const float HitLayer = FMath::Max(30.f, FMath::Abs(HitTowardShooters.X) * HitRankSpacing + FMath::Abs(HitTowardShooters.Y) * HitLateral);
            const float HitRoll = FMath::FRand();
            const int32 HitWantedLayer = HitRoll < 0.78f ? 0 : HitRoll < 0.98f ? 1 : 2;
            float HitNearest = -TNumericLimits<float>::Max();
            for (const USkeletalMeshComponent* HitStanding : SoldierComponents)
                if (HitStanding) HitNearest = FMath::Max(HitNearest, float(FVector::DotProduct(HitStanding->GetRelativeLocation(), HitTowardShooters)));
            TArray<int32> Candidates;
            TArray<int32> FirstLayerCandidates;
            for (int32 c = 0; c < SoldierComponents.Num(); ++c)
            {
                if (!SoldierComponents[c]) continue;
                const float HitDepth = HitNearest - float(FVector::DotProduct(SoldierComponents[c]->GetRelativeLocation(), HitTowardShooters));
                if (HitDepth <= HitLayer * 0.6f) FirstLayerCandidates.Add(c);
                if (FMath::Abs(HitDepth - HitWantedLayer * HitLayer) <= HitLayer * 0.6f) Candidates.Add(c);
            }
            if (Candidates.IsEmpty()) Candidates = MoveTemp(FirstLayerCandidates);   // that layer is gone: the one facing the shooters
            if (!Candidates.IsEmpty()) i = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
            if (SoldierComponents[i])
            {
                const int32 HitLayerOfMan = FMath::Clamp(FMath::RoundToInt((HitNearest - float(FVector::DotProduct(SoldierComponents[i]->GetRelativeLocation(), HitTowardShooters))) / HitLayer), 0, 2);
                const int32 HitSector = ShotAngle <= 30.f ? 0 : ShotAngle >= 150.f ? 2 : 1;   // ahead, oblique/flank, behind
                ++HitLayerTally[HitSector][HitLayerOfMan];
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
            const int32 BattleBodyCount = UStrategyCrowdSubsystem::CountFallen(GetWorld());
            // Baked (instanced) figures keep far more bodies than skeletal ones: a body is one instance in a fixed death pose.
            const bool bKeepBody = bLeaveCorpses && BattleBodyCount < (bCrowdMode ? 1000 : 150);
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
            if (CrowdNearFigures.Remove(Soldier))
                if (UStrategyCrowdSubsystem* CrowdDeathSub = GetWorld()->GetSubsystem<UStrategyCrowdSubsystem>())
                    CrowdDeathSub->NearGeometryCount = FMath::Max(0, CrowdDeathSub->NearGeometryCount - 1);
            if (bCrowdMode && bKeepBody)
            {
                Soldier->SetVisibility(false, true);
                Soldier->SetComponentTickEnabled(false);
                Soldier->SetSkeletalMeshAsset(nullptr);
                if (WeaponComponents.IsValidIndex(i) && WeaponComponents[i]) WeaponComponents[i]->DestroyComponent();
            }
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
        if (!Soldier || !Weapon || !Weapon->GetStaticMesh() || (bCrowdMode && !Soldier->IsVisible()))
        {
            continue;
        }
        // The palms' centres, as in the baked crowd (StrategyCrowdModel.cpp): the rifle lies in the grip, not at the wrists.
        const FVector Right = PalmLocation(Soldier, RightHandBoneName);
        const FVector Left = PalmLocation(Soldier, LeftHandBoneName);
        // Side-stepping (closing ranks), as in the bake: the rifle from the right hand up over the right shoulder, not from hand to
        // hand, which held it out sideways through the next men (2026-10-22 video at 6 s).
        const UAnimSequence* RifleClip = SoldierClips.IsValidIndex(i) ? SoldierClips[i].Clip.Get() : nullptr;
        if (RifleClip && (RifleClip == SideStepAsset.Get() || RifleClip == SideStepLeftAsset.Get()) && Weapon->GetStaticMesh())
        {
            const FName RifleRightShoulder(*RightHandBoneName.ToString().Replace(TEXT("Hand"), TEXT("Shoulder")));
            const FName RifleLeftShoulder(*LeftHandBoneName.ToString().Replace(TEXT("Hand"), TEXT("Shoulder")));
            if (Soldier->GetBoneIndex(RifleRightShoulder) != INDEX_NONE && Soldier->GetBoneIndex(RifleLeftShoulder) != INDEX_NONE)
            {
                const FVector RifleShoulderAt = Soldier->GetSocketLocation(RifleRightShoulder);
                const FVector RifleSide = (RifleShoulderAt - Soldier->GetSocketLocation(RifleLeftShoulder)).GetSafeNormal();
                const FVector RifleForward = FVector::CrossProduct(RifleSide, FVector::UpVector).GetSafeNormal();
                const FVector RifleContact = RifleShoulderAt + RifleForward * 11.7f + FVector::UpVector * 5.0f + RifleSide * 10.0f;
                FVector RifleBarrel = (RifleContact - Right).GetSafeNormal();
                if (RifleBarrel.IsNearlyZero()) RifleBarrel = FVector::UpVector;
                const FRotator RifleShoulderRotation = FRotationMatrix::MakeFromXZ(bRifleBarrelAlongNegativeX ? -RifleBarrel : RifleBarrel, RifleSide).Rotator();
                const FBox RifleShoulderBox = Weapon->GetStaticMesh()->GetBoundingBox();
                const float RifleShoulderButt = bRifleBarrelAlongNegativeX ? RifleShoulderBox.Max.X : RifleShoulderBox.Min.X;
                const float RifleShoulderGrip = RifleShoulderButt + (RifleShoulderBox.Max.X - RifleShoulderBox.Min.X) * 0.03f * (bRifleBarrelAlongNegativeX ? -1.0f : 1.0f);
                Weapon->SetWorldLocationAndRotation(Right - RifleShoulderRotation.RotateVector(FVector(RifleShoulderGrip, 0.0f, 0.0f)), RifleShoulderRotation);
                continue;
            }
        }
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
    const TSoftObjectPtr<UAnimSequence>* BattleAnimationAssets[] = { &IdleStandingAsset, &WalkStandingAsset, &RunStandingAsset, &AimStandingAsset, &FireStandingAsset, &ReloadStandingAsset, &IdleKneelingAsset, &AimKneelingAsset, &FireKneelingAsset, &ReloadKneelingAsset, &IdleProneAsset, &CrawlProneAsset, &FireProneAsset, &ReloadProneAsset, &BayonetChargeAsset, &BayonetThrustAsset, &DeathAsset, &DeathAsset2, &DeathAsset3, &RaiseToAimAsset, &LoadAsset, &RiseFromLoadAsset, &ReadyAsset, &AimHoldAsset, &DeathWalkingAsset, &WalkShoulderAsset, &ShoulderUpAsset, &ShoulderDownAsset, &SideStepAsset, &SideStepLeftAsset, &ColourWalkAsset, &ColourIdleAsset };
    CrowdStanceClips.Reset();
    for (const TCHAR* CrowdStanceName : { TEXT("A_Rifle_Stand_To_Kneel"), TEXT("A_Rifle_Kneel_To_Stand"), TEXT("A_Rifle_Kneel_To_Prone"), TEXT("A_Rifle_Prone_To_Kneel") })
    {
        const FString CrowdStancePath = FString(TEXT("/Game/Units/Danish/Livgarden1864/Animations/")) + CrowdStanceName + TEXT(".") + CrowdStanceName;
        CrowdStanceClips.Add(LoadObject<UAnimSequence>(nullptr, *CrowdStancePath));
    }
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
        FMath::Max(0, OwnerCompany->CurrentStrength - (bColourBearerAlive && ColourBearer ? 1 : 0));

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

        if (!bCrowdMode) Soldier->SetSkeletalMeshAsset(LoadedSoldierMesh);
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

        UStaticMeshComponent* Weapon = bCrowdMode ? nullptr : NewObject<UStaticMeshComponent>(OwnerCompany, NAME_None, RF_Transient);

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
    if (OwnerCompany && OwnerCompany->FormationComponent && FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugFormation")))
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s rebuild cached=%d now=%d scale-only=%d path-init=%d figures=%d"),
            *OwnerCompany->StableUnitId.ToString(), int32(CachedFormationValue), int32(OwnerCompany->FormationComponent->CurrentFormation),
            bScaleOnlyRebuild ? 1 : 0, VisualPath.bInitialized ? 1 : 0, SoldierComponents.Num());
    DrillRebuildAborted = DrillActive;
    if (DrillActive) AbortColumnDrill(TEXT("tab eller ombygning af figurerne"));
    if (!OwnerCompany ||
        !OwnerCompany->FormationComponent ||
        SoldierComponents.Num() == 0)
    {
        return;
    }

    // One contiguous slot per figure; never sample a sparse full-strength layout.
    // Pending casualties remain visible until impact, then ProcessPendingKills rebuilds.
    if (!bScaleOnlyRebuild)
    {
        bArrivalReform = false;
        bArrivalFrontReached = false;
        bFormationPlanPending = false;
        for (FSettle& ReformCancelled : SoldierSettle) ReformCancelled.ReformDeadline = -1.f;
    }
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
            if (bArrivalReform)
            {
                Settle.ReformTarget = ArrivalFormationOrigin + FRotator(0.f, ArrivalFacing, 0.f).RotateVector(Settle.Slot);
                Settle.ReformDeadline = VisualAnimationTime + FMath::Max(1.f, FormationChangeDeadlineSeconds);
                Settle.bActive = true;
                bFormationPlanPending = true;
            }
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
    if (!bScaleOnlyRebuild && VisualPath.bInitialized &&
        OwnerCompany->FormationComponent->CurrentFormation != EStrategyFormationType::Square &&
        !(OwnerCompany->StanceComponent && OwnerCompany->StanceComponent->Stance == EStrategyStance::Prone) &&
        !(DrawnScreen && (DrawnScreen->State == EStrategySkirmisherState::Deploying || DrawnScreen->IsDeployed())))
    {
        bArrivalReform = false;
        if (DrillRebuildAborted) UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s not used: figures rebuilt during a drill"), *OwnerCompany->StableUnitId.ToString());
        if (DrillRebuildAborted || !PlanColumnDrill())
            PlanOrderlyReform(false, FTransform(FRotator(0.f, VisualPath.Facing, 0.f), VisualPath.Center));
    }
    else if (!bScaleOnlyRebuild &&
        ((CachedFormationValue == uint8(EStrategyFormationType::Line) && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn) ||
         (CachedFormationValue == uint8(EStrategyFormationType::MarchColumn) && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Line)))
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s not used: no orderly reform (visual path initialised=%d, prone or screen out)"),
            *OwnerCompany->StableUnitId.ToString(), VisualPath.bInitialized ? 1 : 0);
    if (DrillRebuildAborted && !DrillActive)
        PlanOrderlyReform(false, OwnerCompany->GetActorTransform());
    DrillRebuildAborted = false;
    if (!bScaleOnlyRebuild && OwnerCompany->FormationComponent->CurrentFormation != EStrategyFormationType::MarchColumn) SnakeRowSpacing = 0.f;
    bCrowdDirty = true;
}

bool UStrategyInfantryVisualComponent::GetDrawnFireFront(FVector& OutLeft, FVector& OutRight) const
{
    FBox DrawnFrontBounds(ForceInit);
    if (!OwnerCompany || !GetFormationLocalBounds(DrawnFrontBounds)) return false;
    if (bArrivalReform)
    {
        FTransform ArrivalDrawnFrame;
        if (!GetDrawnFormation(ArrivalDrawnFrame, DrawnFrontBounds)) return false;
        OutLeft = ArrivalDrawnFrame.TransformPosition(FVector(DrawnFrontBounds.Max.X, DrawnFrontBounds.Min.Y, 0.f));
        OutRight = ArrivalDrawnFrame.TransformPosition(FVector(DrawnFrontBounds.Max.X, DrawnFrontBounds.Max.Y, 0.f));
        return true;
    }
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

void UStrategyInfantryVisualComponent::PlanOrderlyReform(bool bDestination, const FTransform& ReformFrame)
{
    DrillActive = false;
    DrillPath.Reset();
    // Once per change, never a per-frame assignment/path search. Preserve figure identity and clips.
    if (bDestination) ArrivalFormationOrigin = ReformFrame.GetLocation();
    TArray<int32> ReformMen;
    TArray<FVector> ReformSlots;
    TArray<FVector> ReformStarts;
    TArray<float> ReformSlotYaws;
    TArray<FVector> ReformEndpoints;
    TArray<int32> ReformSlotIds;
    ReformStarts.SetNum(SoldierSettle.Num());
    for (int32 ReformIndex = 0; ReformIndex < SoldierSettle.Num(); ++ReformIndex)
    {
        if (!SoldierComponents.IsValidIndex(ReformIndex) || !SoldierComponents[ReformIndex]) continue;
        ReformMen.Add(ReformIndex);
        ReformSlots.Add(SoldierSettle[ReformIndex].Slot);
        ReformStarts[ReformIndex] = VisualPath.PreviousUnit.TransformPosition(SoldierComponents[ReformIndex]->GetRelativeLocation());
        ReformSlotYaws.Add(SoldierSettle[ReformIndex].SlotYaw);
        ReformEndpoints.Add(ReformFrame.TransformPosition(SoldierSettle[ReformIndex].Slot));
        ReformSlotIds.Add(SoldierSlots[ReformIndex]);
    }
    const bool bReformColumn = OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
        OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn;
    const FTransform ReformSourceFrame(FRotator(0.f, VisualPath.Facing, 0.f), VisualPath.Center);
    const float ReformFileSpacing = FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierLateralSpacingCm);
    const float ReformRankSpacing = FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierRankSpacingCm);
    // Depart by files (left wing first, front rank first); deploy from the column head.
    ReformMen.Sort([&](int32 ReformA, int32 ReformB)
    {
        const FVector ReformLocalA = ReformSourceFrame.InverseTransformPosition(ReformStarts[ReformA]);
        const FVector ReformLocalB = ReformSourceFrame.InverseTransformPosition(ReformStarts[ReformB]);
        // Ignore small residual personal offsets when identifying the old file/rank.
        const int32 ReformFileA = FMath::RoundToInt32(ReformLocalA.Y / ReformFileSpacing);
        const int32 ReformFileB = FMath::RoundToInt32(ReformLocalB.Y / ReformFileSpacing);
        const int32 ReformRankA = FMath::RoundToInt32(-ReformLocalA.X / ReformRankSpacing);
        const int32 ReformRankB = FMath::RoundToInt32(-ReformLocalB.X / ReformRankSpacing);
        if (!bDestination)
        {
            // v2: monotone in the lateral direction (left to right, front first), so every file slides out sideways in its own band
            // and no two paths cross (a plain rank-major order made the two streams cross like an X).
            if (ReformFileA != ReformFileB) return ReformFileA < ReformFileB;
            if (ReformRankA != ReformRankB) return ReformRankA < ReformRankB;
        }
        else
        {
            if (ReformRankA != ReformRankB) return ReformRankA < ReformRankB;
            if (ReformFileA != ReformFileB) return ReformFileA < ReformFileB;
        }
        return ReformA < ReformB;
    });
    TArray<int32> ReformTargets;
    for (int32 ReformIndex = 0; ReformIndex < ReformSlots.Num(); ++ReformIndex) ReformTargets.Add(ReformIndex);
    ReformTargets.Sort([&](int32 ReformA, int32 ReformB)
    {
        const FVector& ReformAOffset = ReformSlots[ReformA];
        const FVector& ReformBOffset = ReformSlots[ReformB];
        if (!bDestination)
            return ReformAOffset.Y != ReformBOffset.Y ? ReformAOffset.Y < ReformBOffset.Y : ReformAOffset.X > ReformBOffset.X;
        // At a destination the men are taken rank by rank (front first, left to right), so the places must be too: every man
        // keeps his rank and file. File-major places gave the front rank the places of the first 21 files in all three ranks,
        // and the rear rank walked through the others and came in ten seconds late (2026-10-22 test).
        return !FMath::IsNearlyEqual(ReformAOffset.X, ReformBOffset.X, 1.f) ? ReformAOffset.X > ReformBOffset.X : ReformAOffset.Y < ReformBOffset.Y;
    });
    // Uncross straight segments by exchanging endpoints. Every exchange strictly shortens total travel.
    // Collinear paths also exchange when the shorter pairing removes overtaking.
    FormationCrossingsAvoided = 0;
    bool bReformExchanged = true;
    while (bReformExchanged)
    {
        bReformExchanged = false;
        for (int32 ReformA = 0; ReformA < ReformMen.Num(); ++ReformA)
        for (int32 ReformB = ReformA + 1; ReformB < ReformMen.Num(); ++ReformB)
        {
            const FVector ReformStartA = ReformStarts[ReformMen[ReformA]];
            const FVector ReformStartB = ReformStarts[ReformMen[ReformB]];
            const FVector ReformEndA = ReformEndpoints[ReformTargets[ReformA]];
            const FVector ReformEndB = ReformEndpoints[ReformTargets[ReformB]];
            const auto ReformSide = [](const FVector& ReformP, const FVector& ReformQ, const FVector& ReformR)
            { return (ReformQ.X - ReformP.X) * (ReformR.Y - ReformP.Y) - (ReformQ.Y - ReformP.Y) * (ReformR.X - ReformP.X); };
            const bool bReformIntersects = ReformSide(ReformStartA, ReformEndA, ReformStartB) * ReformSide(ReformStartA, ReformEndA, ReformEndB) <= 0. &&
                ReformSide(ReformStartB, ReformEndB, ReformStartA) * ReformSide(ReformStartB, ReformEndB, ReformEndA) <= 0.;
            const double ReformOldLength = FVector::Dist2D(ReformStartA, ReformEndA) + FVector::Dist2D(ReformStartB, ReformEndB);
            const double ReformNewLength = FVector::Dist2D(ReformStartA, ReformEndB) + FVector::Dist2D(ReformStartB, ReformEndA);
            if (bReformIntersects && ReformNewLength + 0.01 < ReformOldLength)
            {
                ReformTargets.Swap(ReformA, ReformB);
                ++FormationCrossingsAvoided;
                bReformExchanged = true;
            }
        }
    }
    // Arrival includes the remaining march; do not force every long path to finish simultaneously at seven seconds.
    float ReformBudget = FMath::Max(1.f, FormationChangeDeadlineSeconds);
    if (bDestination)
    {
        for (int32 ReformOrder = 0; ReformOrder < ReformMen.Num(); ++ReformOrder)
            ReformBudget = FMath::Max(ReformBudget, float(FVector::Dist2D(ReformStarts[ReformMen[ReformOrder]],
                ReformEndpoints[ReformTargets[ReformOrder]])) / 160.f + FormationChangeDeadlineSeconds);
    }
    // A linear morph: everybody starts at once and walks a straight line to his place at a constant speed, so that all arrive
    // together. The shape in between is the mix of the old and the new formation (no stream of late starters running past the
    // others, no S-curve); the time is set by the longest path at a brisk walk (about 3.8 m/s at most).
    if (!bDestination)
    {
        float ReformLongest = 0.f;
        for (int32 ReformOrder = 0; ReformOrder < ReformMen.Num(); ++ReformOrder)
            ReformLongest = FMath::Max(ReformLongest, float(FVector::Dist2D(ReformStarts[ReformMen[ReformOrder]], ReformFrame.TransformPosition(ReformSlots[ReformTargets[ReformOrder]]))));
        // Twice as brisk as before: the last men to take their place walk at about 5 m/s; forming a column is quicker still (the march waits for it).
        ReformBudget = FMath::Clamp(ReformLongest / 500.f, 3.f, bReformColumn ? 6.f : 9.f);
        if (bReformColumn)
        {
            // Gathering into column is walked, not run: the pace follows the march (one and a half times the unit's own march speed,
            // never below a plain march and never above what the walk clip can follow at its 4x rate limit), so the feet keep the ground
            // speed. The march is held until the last man is in place.
            const float ReformMarchNominal = OwnerCompany->MovementExecutor ? OwnerCompany->MovementExecutor->MoveSpeedCmPerSecond : 150.f;
            const float ReformWalkPace = FMath::Clamp(ReformMarchNominal * 1.5f, FMath::Max(160.f, ReformMarchNominal), 300.f);
            ReformBudget = FMath::Clamp(ReformLongest / ReformWalkPace, 3.f, 10.f);
            if (OwnerCompany->FormationTransition) OwnerCompany->FormationTransition->ExtendReformSeconds(ReformBudget + 0.3f);
        }
    }
    FormationPlanStarted = VisualAnimationTime;
    FormationLongestPath = 0.f;
    bFormationPlanPending = !ReformMen.IsEmpty();
    for (int32 ReformOrder = 0; ReformOrder < ReformMen.Num(); ++ReformOrder)
    {
        const int32 ReformMan = ReformMen[ReformOrder];
        FSettle& ReformSettle = SoldierSettle[ReformMan];
        ReformSettle.Slot = ReformSlots[ReformTargets[ReformOrder]];
        SoldierSlots[ReformMan] = ReformSlotIds[ReformTargets[ReformOrder]];
        ReformSettle.SlotYaw = ReformSlotYaws[ReformTargets[ReformOrder]];
        ReformSettle.ReformTarget = ReformFrame.TransformPosition(ReformSettle.Slot);
        const FVector ReformStart = ReformStarts[ReformMan];
        FormationLongestPath = FMath::Max(FormationLongestPath, float(FVector::Dist2D(ReformStart, ReformSettle.ReformTarget)));
        const int32 ReformOldColumnWidth = CachedFormationValue == uint8(EStrategyFormationType::DefileColumn) ?
            2 : OwnerCompany->FormationComponent->ColumnWidth;
        const int32 ReformWaveWidth = FMath::Max(1, bReformColumn ? OwnerCompany->FormationComponent->RankCount : ReformOldColumnWidth);
        ReformSettle.StartDelay = bDestination ? FMath::Min((ReformOrder / ReformWaveWidth) * 0.12f + (ReformOrder % ReformWaveWidth) * 0.04f, FormationChangeDeadlineSeconds * 0.65f) : 0.f;
        ReformSettle.StartRemaining = bDestination ? 0.f : 0.f;
        ReformSettle.HaltRemaining = 0.f;
        ReformSettle.ReformDeadline = VisualAnimationTime + ReformBudget;
        ReformSettle.ReformStartLocal = ReformFrame.InverseTransformPosition(ReformStart);
        ReformSettle.ReformStartTime = VisualAnimationTime;
        ReformSettle.ReformDuration = ReformBudget;
        ReformSettle.WorldVelocity = FVector::ZeroVector;
        ReformSettle.bActive = true;
    }
}

void UStrategyInfantryVisualComponent::KeepReformSpacing(float DeltaTime)
{
    // Small local presentation correction, not navigation/collision in the simulation.
    // Spatial buckets bound neighbour work; no all-pairs search or terrain trace in the frame loop.
    const float ReformClearance = FMath::Min(60.f, FMath::Min(
        OwnerCompany->FormationComponent->SoldierLateralSpacingCm,
        OwnerCompany->FormationComponent->SoldierRankSpacingCm) * 0.8f);
    if (ReformClearance <= 0.f) return;
    const FTransform ReformUnit = OwnerCompany->GetActorTransform();
    FormationStepPositions.SetNum(SoldierComponents.Num());
    FormationSpacingNext.SetNum(SoldierComponents.Num());
    for (int32 ReformMan = 0; ReformMan < SoldierComponents.Num(); ++ReformMan)
        if (SoldierComponents[ReformMan])
            FormationStepPositions[ReformMan] = ReformUnit.TransformPosition(SoldierComponents[ReformMan]->GetRelativeLocation());
    for (int32 ReformPass = 0; ReformPass < 4; ++ReformPass)
    {
        FormationSpacingGrid.Reset();
        for (int32 ReformMan = 0; ReformMan < SoldierComponents.Num(); ++ReformMan)
        {
            if (!SoldierComponents[ReformMan]) continue;
            const FVector& ReformPosition = FormationStepPositions[ReformMan];
            int32& ReformCellHead = FormationSpacingGrid.FindOrAdd(FIntPoint(FMath::FloorToInt32(ReformPosition.X / ReformClearance),
                FMath::FloorToInt32(ReformPosition.Y / ReformClearance)), INDEX_NONE);
            FormationSpacingNext[ReformMan] = ReformCellHead;
            ReformCellHead = ReformMan;
        }
        for (int32 ReformMan = 0; ReformMan < SoldierComponents.Num(); ++ReformMan)
        {
            if (!SoldierComponents[ReformMan]) continue;
            const FVector ReformPosition = FormationStepPositions[ReformMan];
            const FIntPoint ReformCell(FMath::FloorToInt32(ReformPosition.X / ReformClearance), FMath::FloorToInt32(ReformPosition.Y / ReformClearance));
            for (int32 ReformCellX = -1; ReformCellX <= 1; ++ReformCellX)
            for (int32 ReformCellY = -1; ReformCellY <= 1; ++ReformCellY)
            {
                const int32* ReformNeighbours = FormationSpacingGrid.Find(FIntPoint(ReformCell.X + ReformCellX, ReformCell.Y + ReformCellY));
                if (!ReformNeighbours) continue;
                for (int32 ReformOther = *ReformNeighbours; ReformOther != INDEX_NONE; ReformOther = FormationSpacingNext[ReformOther])
                {
                    if (ReformOther <= ReformMan) continue;
                    FVector ReformAway = FormationStepPositions[ReformMan] - FormationStepPositions[ReformOther];
                    ReformAway.Z = 0.f;
                    const float ReformDistance = ReformAway.Size2D();
                    if (ReformDistance >= ReformClearance) continue;
                    // A man within 1.2 m of his own place is no longer pushed (otherwise two close slots keep pushing each other and
                    // the men walk on the spot for ever); only men still on their way give way.
                    const bool bReformMoveA = SoldierSettle[ReformMan].bActive && SoldierSettle[ReformMan].StartRemaining <= 0.f &&
                        FVector::Dist2D(FormationStepPositions[ReformMan], ReformUnit.TransformPosition(SoldierSettle[ReformMan].Goal)) > 120.f;
                    const bool bReformMoveB = SoldierSettle[ReformOther].bActive && SoldierSettle[ReformOther].StartRemaining <= 0.f &&
                        FVector::Dist2D(FormationStepPositions[ReformOther], ReformUnit.TransformPosition(SoldierSettle[ReformOther].Goal)) > 120.f;
                    if (!bReformMoveA && !bReformMoveB) continue; // Halted men and waiting files hold their ground.
                    // A man on his way steps in between men who already stand (files are 75 cm apart); only men on the move keep apart
                    // from each other. Pushed back by a rank that stood in his way, he walked on the spot until the time ran out and then
                    // jumped 10 m into his place (2026-10-22 test).
                    if (bReformMoveA != bReformMoveB) continue;
                    if (ReformDistance <= 0.01f)
                    {
                        ReformAway = FormationStepStarts[ReformMan] - FormationStepStarts[ReformOther];
                        ReformAway.Z = 0.f;
                        if (ReformAway.IsNearlyZero()) ReformAway = FVector(0.f, 1.f, 0.f);
                    }
                    const FVector ReformCorrection = ReformAway.GetSafeNormal() * (ReformClearance - ReformDistance);
                    const float ReformShareA = bReformMoveA ? (bReformMoveB ? 0.5f : 1.f) : 0.f;
                    FormationStepPositions[ReformMan] += ReformCorrection * ReformShareA;
                    FormationStepPositions[ReformOther] -= ReformCorrection * (1.f - ReformShareA);
                }
            }
        }
    }
    for (int32 ReformMan = 0; ReformMan < SoldierComponents.Num(); ++ReformMan)
    {
        if (!SoldierComponents[ReformMan]) continue;
        SoldierComponents[ReformMan]->SetRelativeLocation(ReformUnit.InverseTransformPosition(FormationStepPositions[ReformMan]));
        SoldierSettle[ReformMan].WorldVelocity = (FormationStepPositions[ReformMan] - FormationStepStarts[ReformMan]) / FMath::Max(0.001f, DeltaTime);
    }
}

void UStrategyInfantryVisualComponent::UpdateSettling(float DeltaTime)
{
    if (DeltaTime <= 0.f) return;
    if (UpdateColumnDrill(DeltaTime)) return;
    // The men run to their places in a new formation (out into line, in to the column) at a jog, turning the way they run, and take
    // the company's pose when they are there.
    const float RunCmPerSecond = 420.0f;
    // Blender world-space low-foot backward travel / stance time; armature scale 0.01, soldier scale 1.
    // SourceAssets/InfantryCombat/walk_speed_measurement.json; both cycles are 41/30 seconds.
    constexpr float InfantryWalkClipCmPerSecond = 76.99816f;
    // v9 shoulder clips (Mixamo walk, 2026-10-18): Tools/Animation/measure_infantry_walk_speed.py -> walk_speed_measurement.json.
    constexpr float InfantryShoulderWalkClipCmPerSecond = 137.70097f;
    const FTransform VisualUnit = OwnerCompany->GetActorTransform();
    const bool bVisualColumn = OwnerCompany->FormationComponent &&
        (OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
         OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn);
    if (!VisualPath.bInitialized) { VisualPath.Reset(VisualUnit); }
    const float ActualMarchSpeed = OwnerCompany->MovementExecutor ? OwnerCompany->MovementExecutor->GetExecutedVelocity().Size2D() : 0.f;
    const float VisualWalkSpeed = FMath::Max(1.f, ActualMarchSpeed);
    const FStrategyOrder ArrivalOrder = OwnerCompany->OrderComponent ? OwnerCompany->OrderComponent->GetCurrentOrder() : FStrategyOrder();
    if (bArrivalReform && (ArrivalOrder.OrderSerial != ArrivalOrderSerial ||
        !ArrivalOrder.TargetLocation.Equals(ArrivalDestination, 1.f) ||
        FMath::Abs(FMath::FindDeltaAngleDegrees(ArrivalFacing, ArrivalOrder.FacingYaw)) > 0.1f ||
        (OwnerCompany->MovementExecutor && OwnerCompany->MovementExecutor->IsTemporarilyPaused())))
    {
        bArrivalReform = false;
        bArrivalFrontReached = false;
        bFormationPlanPending = false;
        VisualPath.Center = VisualUnit.GetLocation();
        VisualPath.CenterVelocity = FVector::ZeroVector;
        for (FSettle& ArrivalCancelled : SoldierSettle) ArrivalCancelled.ReformDeadline = -1.f;
    }
    const bool bArrivalFinalLeg = OwnerCompany->MovementExecutor && !OwnerCompany->MovementExecutor->IsTemporarilyPaused() &&
        OwnerCompany->MovementExecutor->HasMovementGoal() &&
        OwnerCompany->OrderComponent && OwnerCompany->OrderComponent->IsPhysicallyExecuting() &&
        ArrivalOrder.NextWaypointIndex >= ArrivalOrder.Waypoints.Num() - 1 &&
        OwnerCompany->MovementExecutor->GetMovementGoal().Equals(ArrivalOrder.TargetLocation, 1.f);
    const bool bArrivalScreen = OwnerCompany->SkirmisherComponent &&
        (OwnerCompany->SkirmisherComponent->State == EStrategySkirmisherState::Deploying || OwnerCompany->SkirmisherComponent->IsDeployed());
    if (!bArrivalReform && !bVisualColumn && !bArrivalScreen && !(OwnerCompany->StanceComponent && OwnerCompany->StanceComponent->Stance == EStrategyStance::Prone) && bArrivalFinalLeg &&
        ArrivalOrder.bHasFacing && !ArrivalOrder.bKeepFacing &&
        FVector::Dist2D(VisualPath.Center, ArrivalOrder.TargetLocation) <= 2500.f &&
        FMath::Abs(FMath::FindDeltaAngleDegrees(VisualPath.Facing, ArrivalOrder.FacingYaw)) >= 20.f &&
        OwnerCompany->FormationComponent->CurrentFormation != EStrategyFormationType::Square)
    {
        bArrivalReform = true;
        bArrivalFrontReached = false;
        ArrivalOrderSerial = ArrivalOrder.OrderSerial;
        ArrivalDestination = ArrivalOrder.TargetLocation;
        ArrivalFacing = ArrivalOrder.FacingYaw;
        ArrivalTravelSpeed = VisualWalkSpeed;
        const float ArrivalHalfDepth = VisualPath.SlotBounds.IsValid ? VisualPath.SlotBounds.GetSize().X * 0.5f : 0.f;
        // The destination preview is centred on the order goal; generated infantry slots are front-anchored.
        const FVector ArrivalSlotOrigin = ArrivalDestination + FRotator(0.f, ArrivalFacing, 0.f).Vector() * ArrivalHalfDepth;
        PlanOrderlyReform(true, FTransform(FRotator(0.f, ArrivalFacing, 0.f), ArrivalSlotOrigin));
    }
    {
        int32 RigidSetting = 1;
        if (FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Rigid="), RigidSetting)) bRigidCompany = RigidSetting != 0;
    }
    // A march column is a snake (each man turns where the head turned), even in a rigid company; the line stays rigid.
    const bool bSnakeColumn = bVisualColumn && bRigidCompany && FStrategyColumnDrill::Enabled() &&
        OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn;
    const float SnakeMarchSpeed = OwnerCompany->MovementExecutor ? OwnerCompany->MovementExecutor->MoveSpeedCmPerSecond : 150.f;
    VisualPath.HeadSpeedCap = bSnakeColumn ? FMath::Max(120.f, SnakeMarchSpeed * 1.15f) : 0.f;
    VisualPath.HeadTurnRadiusCm = bSnakeColumn ? FStrategyColumnDrill::Radius(OwnerCompany->FormationComponent->SoldierRankSpacingCm, 0.f) : 0.f;
    if (bSnakeColumn) UpdateSnakeRamp();
    if (bClosingRanksStep) UpdateCloseRanks();
    if (bSnakeColumn && FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugFormation")) && VisualAnimationTime >= DrillLogAt)
    {
        DrillLogAt = VisualAnimationTime + 1.f;
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s snake actor=%s actor-yaw=%.1f head=%s head-yaw=%.1f goal=%s has-goal=%d paused=%d"),
            *OwnerCompany->StableUnitId.ToString(), *VisualUnit.GetLocation().ToString(), VisualUnit.Rotator().Yaw,
            *VisualPath.HeadPos.ToString(), VisualPath.HeadYaw, OwnerCompany->MovementExecutor ? *OwnerCompany->MovementExecutor->GetMovementGoal().ToString() : TEXT("-"),
            OwnerCompany->MovementExecutor && OwnerCompany->MovementExecutor->HasMovementGoal() ? 1 : 0,
            OwnerCompany->MovementExecutor && OwnerCompany->MovementExecutor->IsTemporarilyPaused() ? 1 : 0);
    }
    if (!bArrivalReform && !(bSnakeColumn && VisualAnimationTime < SnakeHoldUntil))
        VisualPath.Advance(VisualUnit, DeltaTime, bVisualColumn, VisualWalkSpeed, bRigidCompany && !bSnakeColumn);
    else
    {
        // Drawn bounds/flag/HQ keep reading the living figure transforms in the final-facing frame.
        VisualPath.Facing = ArrivalFacing;
        VisualPath.bMovingVisuals = false;
        VisualPath.bTurning = false;
    }
    const bool bActorMoved = !VisualUnit.Equals(VisualPath.PreviousUnit, 0.001f);
    const bool bVisualTravel = !bArrivalReform && (VisualPath.bMovingVisuals || bActorMoved);
    const bool bSpreadCrawling = OwnerCompany->StanceComponent && OwnerCompany->StanceComponent->Stance == EStrategyStance::Prone;
    UAnimSequence* Walk = (bSpreadCrawling ? CrawlProneAsset : WalkStandingAsset).Get();
    // The column marches with the rifle on the shoulder (the travel clip is chosen here, not by ResolveAnimation): up/down in one cycle at the change.
    const bool bShoulderClips = !bSpreadCrawling && !WalkShoulderAsset.IsNull() && !ShoulderUpAsset.IsNull() && !ShoulderDownAsset.IsNull();
    if (bShoulderClips)
    {
        if (ShoulderTransitionEnd > GetWorld()->GetTimeSeconds()) { Walk = (bShoulderRising ? ShoulderUpAsset : ShoulderDownAsset).Get(); }
        else if (bShoulderColumn) { Walk = WalkShoulderAsset.Get(); }
    }
    const auto IsWalkFamily = [this](const UAnimSequence* Clip)
    {
        return Clip && (Clip == WalkStandingAsset.Get() || Clip == WalkShoulderAsset.Get() || Clip == ShoulderUpAsset.Get() || Clip == ShoulderDownAsset.Get());
    };
    if (bFormationPlanPending)
    {
        FormationStepStarts.SetNum(SoldierComponents.Num());
        for (int32 ReformMan = 0; ReformMan < SoldierComponents.Num(); ++ReformMan)
            if (SoldierComponents[ReformMan])
                FormationStepStarts[ReformMan] = VisualPath.PreviousUnit.TransformPosition(SoldierComponents[ReformMan]->GetRelativeLocation());
    }
    bool bAny = false;
    float ArrivalLiveFrontX = 0.f;
    bool bArrivalHasLiveFront = false;
    if (bArrivalReform)
    {
        for (int32 ArrivalMan = 0; ArrivalMan < SoldierSettle.Num(); ++ArrivalMan)
        {
            if (!SoldierComponents.IsValidIndex(ArrivalMan) || !SoldierComponents[ArrivalMan]) continue;
            ArrivalLiveFrontX = bArrivalHasLiveFront ? FMath::Max(ArrivalLiveFrontX, float(SoldierSettle[ArrivalMan].Slot.X)) :
                float(SoldierSettle[ArrivalMan].Slot.X);
            bArrivalHasLiveFront = true;
        }
    }
    UAnimSequence* Run = bSpreadCrawling ? Walk : RunStandingAsset.Get();
    bool bSettleIdleLoop = true;
    float ShoulderSlowestFraction = 1.f;
    bool bShoulderAnyPlaying = false;
    UAnimSequence* SettleIdle = bArrivalReform ? IdleStandingAsset.Get() : ResolveAnimation(bSettleIdleLoop);
    for (int32 i = 0; i < SoldierComponents.Num() && i < SoldierSettle.Num(); ++i)
    {
        FSettle& Settle = SoldierSettle[i];
        if (bVisualTravel && !bWasVisualTravel && Settle.ReformDeadline < 0.f) Settle.StartRemaining = 0.f;
        if (!bVisualTravel && bWasVisualTravel) Settle.HaltRemaining = 0.f;
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
            Settle.Goal = VisualUnit.InverseTransformPosition(bArrivalReform ? Settle.ReformTarget :
                VisualPath.Goal(SpecialPathSlot, bVisualColumn && !bScreenFigure, GoalYaw));
            if (bArrivalReform) GoalYaw = ArrivalFacing;
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
                RecordClip(Soldier, SoldierClips[i].Clip.Get(), SoldierClips[i].bLoop,
                    SoldierClips[i].Rate > 0.f ? (VisualAnimationTime - SoldierClips[i].Start) * SoldierClips[i].Rate : SoldierClips[i].HeldPosition, 0.f);
            bAny = true;
            continue;
        }
        FVector Here = Soldier->GetRelativeLocation();
        FVector Delta = Settle.Goal - Here;
        const float Distance = Delta.Size();
        if (Settle.ReformDeadline >= 0.f && Distance <= 0.1f)
            Settle.ReformDeadline = -1.f;
        if (Distance <= 0.1f && !bVisualTravel)
        {
            Settle.ReformDeadline = -1.f;
            if (bArrivalReform && bArrivalHasLiveFront && Settle.Slot.X >= ArrivalLiveFrontX - 1.f)
                bArrivalFrontReached = true;
            Settle.WorldVelocity = FVector::ZeroVector;
            FRotator ArrivedRotation = Soldier->GetRelativeRotation();
            ArrivedRotation.Yaw = FStrategyVisualFormationPath::SmoothFacing(ArrivedRotation.Yaw, Settle.YawVelocity, Settle.Yaw, DeltaTime, 120.f);
            ArrivedRotation.Roll = FMath::FInterpTo(ArrivedRotation.Roll, 0.f, DeltaTime, 8.f);
            Soldier->SetRelativeRotation(ArrivedRotation);
            Settle.bActive = Settle.HaltRemaining > 0.f || FMath::Abs(FMath::FindDeltaAngleDegrees(ArrivedRotation.Yaw, Settle.Yaw)) > 0.1f;
            Soldier->bPauseAnims = true;
            if (Walk && SoldierClips.IsValidIndex(i) &&
                (IsWalkFamily(SoldierClips[i].Clip.Get()) || SoldierClips[i].Clip.Get() == Walk || SoldierClips[i].Clip.Get() == Run))
                RecordClip(Soldier, SoldierClips[i].Clip.Get(), SoldierClips[i].bLoop,
                    SoldierClips[i].Rate > 0.f ? (VisualAnimationTime - SoldierClips[i].Start) * SoldierClips[i].Rate : SoldierClips[i].HeldPosition, 0.f);
            UAnimSequence* SnakeShoulderClip = ShoulderUpAsset.Get();
            if (bSnakeColumn && VisualAnimationTime < SnakeHoldUntil && SnakeShoulderClip && SoldierClips.IsValidIndex(i))
            {
                // Shoulder arms on the spot before marching off: the Up clip once, from the hold's start.
                const float SnakeShoulderAt = FMath::Clamp(VisualAnimationTime - SnakeHoldStart, 0.f, SnakeShoulderClip->GetPlayLength());
                if (SoldierClips[i].Clip.Get() != SnakeShoulderClip)
                {
                    Soldier->PlayAnimation(SnakeShoulderClip, false);
                    Soldier->SetPosition(SnakeShoulderAt, false);
                    Soldier->SetPlayRate(1.f);
                    RecordClip(Soldier, SnakeShoulderClip, false, SnakeShoulderAt, 1.f);
                }
                Soldier->bPauseAnims = false;
                bAny = true;
                bCrowdDirty = true;
                continue;
            }
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
        const float FigureSpeed = (bArrivalReform ? 0.f : ActualMarchSpeed) +
            ((bSlotReforming || VisualPath.bTurning) ? RunCmPerSecond : 160.f) * Settle.Pace;
        const float SpecialFigureSpeed = bSpreadCrawling ? FMath::Min(FigureSpeed, VisualWalkSpeed * 0.40f) : FigureSpeed;
        FVector FigureWorld = VisualUnit.TransformPosition(Here);
        const FVector FigureBefore = FigureWorld;
        const FVector FigureTarget = VisualUnit.TransformPosition(Settle.Goal);
        // Integrate presentation velocity every frame, with braking near the slot.
        // Substeps affect no orders, simulation positions, collision or game clocks.
        float MorphAlpha = -1.f;
        if (!bSpreadCrawling && bRigidCompany && !bArrivalReform && Settle.ReformDeadline >= 0.f && Settle.ReformDuration > 0.f)
        {
            // v2: the change of formation is a straight glide from the old slot to the new one in the unit's own frame, so the
            // whole company morphs as one body while it marches (no men chasing a moving slot through world space).
            const float Alpha = FMath::Clamp((VisualAnimationTime - Settle.ReformStartTime) / Settle.ReformDuration, 0.f, 1.f);
            MorphAlpha = Alpha;
            const float Eased = Alpha * Alpha * (3.f - 2.f * Alpha);
            const FVector Local = FMath::Lerp(Settle.ReformStartLocal, Settle.Goal, Eased);
            FigureWorld = VisualUnit.TransformPosition(Local);
            Settle.WorldVelocity = DeltaTime > 0.f ? (FigureWorld - FigureBefore) / DeltaTime : FVector::ZeroVector;
            if (Alpha >= 1.f) Settle.ReformDeadline = -1.f;
        }
        else if (!bSpreadCrawling && (Settle.ReformDeadline >= 0.f || bArrivalReform))
        {
            const float ReformTimeLeft = FMath::Max(DeltaTime, Settle.ReformDeadline - VisualAnimationTime);
            const float ReformBaseSpeed = bArrivalReform ? FMath::Max(160.f, ArrivalTravelSpeed * Settle.Pace) : ActualMarchSpeed + RunCmPerSecond * Settle.Pace;
            const float ReformEase = Distance < 10.f ? 120.f * Distance / 10.f :
                (Distance < 30.f ? FMath::Max(120.f, ReformBaseSpeed * Distance / 30.f) : ReformBaseSpeed);
            // Not at a destination: the constant speed of the linear morph (Distance / time left), so that everybody arrives together.
            const float ReformSpeed = bArrivalReform ? FMath::Max(ReformEase, Distance / FMath::Max(0.001f, ReformTimeLeft))
                : FMath::Min(Distance / FMath::Max(0.001f, ReformTimeLeft), 700.f);
            const float ReformStep = FMath::Min(Distance, ReformSpeed * DeltaTime);
            FigureWorld += (FigureTarget - FigureWorld).GetSafeNormal() * ReformStep;
            Settle.WorldVelocity = ReformStep >= Distance ? FVector::ZeroVector :
                (FigureWorld - FigureBefore) / FMath::Max(0.001f, DeltaTime);
        }
        else if (bRigidCompany && !bFormationPlanPending && !bSpreadCrawling)
        {
            // Rigid company: the man stands exactly in his slot of the unit's frame (the slot moves and turns with the unit).
            Settle.WorldVelocity = DeltaTime > 0.f ? (FigureTarget - FigureWorld) / DeltaTime : FVector::ZeroVector;
            FigureWorld = FigureTarget;
        }
        else FStrategyVisualFormationPath::SmoothTravel(FigureWorld, Settle.WorldVelocity, FigureTarget, DeltaTime, SpecialFigureSpeed);
        const FVector Step = FigureWorld - FigureBefore;
        Soldier->SetRelativeLocation(VisualUnit.InverseTransformPosition(FigureWorld));
        // He faces the way he runs while he is far from his place, and turns to the front as he comes in.
        const float RunYaw = VisualUnit.InverseTransformVectorNoScale(Settle.WorldVelocity).Rotation().Yaw + SoldierMeshYawOffset;
        const bool bFigureBackstep = OwnerCompany->OrderComponent &&
            OwnerCompany->OrderComponent->GetCurrentOrder().Type == EStrategyOrderType::Disengage;
        // Changing formation: the man walks facing the way he goes (never sideways or backwards), and turns to the front over the last
        // fifth of the way.
        const bool bMorphWalking = bRigidCompany && !bArrivalReform && !bFigureBackstep && MorphAlpha >= 0.f && MorphAlpha < 0.8f &&
            Settle.WorldVelocity.SizeSquared2D() > 60.f * 60.f;
        const float Yaw = bMorphWalking ? RunYaw : bFigureBackstep || Step.Size() >= Distance - 0.1f || (bRigidCompany && !bArrivalReform) ? Settle.Yaw :
            (bVisualTravel || Distance > (bArrivalReform ? 0.1f : 30.f) ? RunYaw : Settle.Yaw);
        FRotator Rot = Soldier->GetRelativeRotation();
        const float FigureTurn = FMath::FindDeltaAngleDegrees(Rot.Yaw, Yaw);
        Rot.Roll = FMath::FInterpTo(Rot.Roll, FMath::Clamp(FigureTurn * 0.08f, -4.f, 4.f), DeltaTime, 8.f);
        Rot.Yaw = FStrategyVisualFormationPath::SmoothFacing(Rot.Yaw, Settle.YawVelocity, Yaw, DeltaTime, 120.f);
        Soldier->SetRelativeRotation(Rot);
        Soldier->bPauseAnims = false;
        UAnimSequence* TravelClip = (!bSlotReforming || bArrivalReform || bRigidCompany) && Walk ? Walk : Run;
        // Closing ranks: a side-step with the front kept, to the right or (mirrored clip) to the left.
        const bool bCloseStepping = Settle.CloseStart >= 0.f && VisualAnimationTime >= Settle.CloseStart;
        const bool bCloseSideways = FMath::Abs(Settle.CloseTo.Y - Settle.CloseFrom.Y) > FMath::Abs(Settle.CloseTo.X - Settle.CloseFrom.X);
        if (bCloseStepping && bCloseSideways && SideStepAsset.Get())
            TravelClip = Settle.CloseTo.Y < Settle.CloseFrom.Y && SideStepLeftAsset.Get() ? SideStepLeftAsset.Get() : SideStepAsset.Get();
        {
            // Past the walk clip's comfortable rate (about 3x) the man runs: the Run clip at its own foot speed (420 cm/s), so double
            // time and a charge are not a shuffle at 4x. Hysteresis keeps a man from flipping between the two clips.
            const float TravelGroundSpeed = Step.Size2D() / FMath::Max(0.001f, DeltaTime);
            const bool bTravelWasRun = Run && Run != Walk && SoldierClips.IsValidIndex(i) && SoldierClips[i].Clip.Get() == Run;
            if (Run && Run != Walk && !bSpreadCrawling && TravelGroundSpeed > (bTravelWasRun ? 210.f : 250.f)) TravelClip = Run;
        }
        if (TravelClip && SoldierClips.IsValidIndex(i) && (SoldierClips[i].Clip.Get() != TravelClip))
        {
            Soldier->bPauseAnims = false;
            // Rifle up/down play once from their first frame (the arm movement lives in clip time); everything else loops.
            const bool bShoulderOnceClip = TravelClip == ShoulderUpAsset.Get() || TravelClip == ShoulderDownAsset.Get();
            Soldier->PlayAnimation(TravelClip, !bShoulderOnceClip);
            float FigurePhase = Settle.WalkPhase * TravelClip->GetPlayLength();
            if (bShoulderOnceClip) FigurePhase = 0.f;
            else if (IsWalkFamily(SoldierClips[i].Clip.Get()) && IsWalkFamily(TravelClip) && SoldierClips[i].Rate > 0.f && SoldierClips[i].Clip->GetPlayLength() > 0.f)
            {
                // Swapping between the walk variants (rifle up/down): keep the stride phase, only the arms change.
                const float PreviousPosition = FMath::Fmod((VisualAnimationTime - SoldierClips[i].Start) * SoldierClips[i].Rate, SoldierClips[i].Clip->GetPlayLength());
                FigurePhase = FMath::Clamp(PreviousPosition, 0.f, FMath::Max(0.f, TravelClip->GetPlayLength() - 0.001f));
            }
            Soldier->SetPosition(FigurePhase, false);
            RecordClip(Soldier, TravelClip, !bShoulderOnceClip, FigurePhase, 1.0f);
        }
        if (bCloseStepping && FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugFormation")) && VisualAnimationTime >= CloseDiagNext)
        {
            CloseDiagNext = VisualAnimationTime + 0.5f;
            UE_LOG(LogTemp, Display, TEXT("Strategy1864CloseDiag %s man %d clip=%s yaw-off-front=%.0f speed=%.0f rate=%.2f from=%s to=%s mode=%s"),
                *OwnerCompany->StableUnitId.ToString(), i, TravelClip ? *TravelClip->GetName() : TEXT("-"), FMath::FindDeltaAngleDegrees(Settle.Yaw, Rot.Yaw),
                Step.Size2D() / FMath::Max(0.001f, DeltaTime), SoldierClips.IsValidIndex(i) ? SoldierClips[i].Rate : -1.f,
                *Settle.CloseFrom.ToString(), *Settle.CloseTo.ToString(),
                MorphAlpha >= 0.f ? TEXT("morph") : Settle.ReformDeadline >= 0.f || bArrivalReform ? TEXT("reform") : (bRigidCompany && !bFormationPlanPending) ? TEXT("rigid") : TEXT("smooth"));
        }
        if (TravelClip && SoldierClips.IsValidIndex(i))
        {
            const FPlayedClip PreviousClip = SoldierClips[i];
            // Shoulder transitions share WalkShoulder's leg cycle. Keep crawl/run calibration unchanged.
            const bool bShoulderTravel = TravelClip == WalkShoulderAsset.Get() || TravelClip == ShoulderUpAsset.Get() || TravelClip == ShoulderDownAsset.Get();
            // Side-step: one 30 cm step per 1.17 s cycle, in place (measured on A_Combat_SideStep.fbx, 2026-10-18).
            const bool bSideStepTravel = TravelClip && (TravelClip == SideStepAsset.Get() || TravelClip == SideStepLeftAsset.Get());
            const float InfantryTravelClipSpeed = bSideStepTravel ? 26.f : TravelClip == Run ? RunCmPerSecond :
                IsWalkFamily(TravelClip) ? (bShoulderTravel ? InfantryShoulderWalkClipCmPerSecond : InfantryWalkClipCmPerSecond) : 160.f;
            const float DesiredVisualRate = FMath::Clamp(Step.Size2D() / FMath::Max(0.001f, DeltaTime) / InfantryTravelClipSpeed, 0.01f, 4.f);
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
    // Every man counts, also those still waiting to start, frozen or inactive this frame (they were skipped above), so one finished man
    // cannot end the rifle up/down for the others.
    for (const FPlayedClip& ShoulderPlayed : SoldierClips)
    {
        if (!ShoulderPlayed.Clip.IsValid() || (ShoulderPlayed.Clip.Get() != ShoulderUpAsset.Get() && ShoulderPlayed.Clip.Get() != ShoulderDownAsset.Get())) continue;
        const float ShoulderPosition = ShoulderPlayed.Rate > 0.f ? FMath::Max(0.f, (VisualAnimationTime - ShoulderPlayed.Start) * ShoulderPlayed.Rate) : ShoulderPlayed.HeldPosition;
        ShoulderSlowestFraction = FMath::Min(ShoulderSlowestFraction, ShoulderPosition / FMath::Max(0.01f, ShoulderPlayed.Clip->GetPlayLength()));
        bShoulderAnyPlaying = true;
    }
    if (bShoulderClips && ShoulderTransitionEnd > GetWorld()->GetTimeSeconds())
    {
        // Ends when the clips are played out at their own play rate (length / rate), not after a fixed wall time; if nobody plays them in
        // this path (they are driven by ResolveAnimation at rate 1) the 1.37 s clip length applies.
        if (bShoulderAnyPlaying ? ShoulderSlowestFraction >= 1.f : GetWorld()->GetTimeSeconds() - ShoulderTransitionStart >= 1.37f)
        {
            ShoulderTransitionEnd = GetWorld()->GetTimeSeconds();
            bCrowdDirty = true;
        }
    }
    if (bFormationPlanPending && FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugFormation")) && VisualAnimationTime - FormationPlanStarted > 4.f &&
        FMath::FloorToInt((VisualAnimationTime - FormationPlanStarted) / 3.f) != FMath::FloorToInt((VisualAnimationTime - FormationPlanStarted - DeltaTime) / 3.f))
    {
        // A reform that runs long: who is still on the way, and how far from the place.
        int32 Unplaced = 0; float Worst = 0.f; int32 Waiting = 0;
        int32 StuckSample = INDEX_NONE;
        for (int32 i = 0; i < SoldierSettle.Num(); ++i)
        {
            const FSettle& Man = SoldierSettle[i];
            if (!SoldierComponents.IsValidIndex(i) || !SoldierComponents[i]) continue;
            const float Gap = FVector::Dist2D(SoldierComponents[i]->GetRelativeLocation(), Man.Goal);
            if (Gap > 30.f) { ++Unplaced; Worst = FMath::Max(Worst, Gap); }
            if (Man.StartRemaining > 0.f || Man.HaltRemaining > 0.f) ++Waiting;
            // A man still away from his place although he has had time to walk there: what holds him.
            const float PlaceGap = FVector::Dist2D(SoldierComponents[i]->GetComponentLocation(), FormationPlace(i, false));
            if (PlaceGap > 150.f && PlaceGap < 600.f && StuckSample == INDEX_NONE) StuckSample = i;
        }
        if (SoldierSettle.IsValidIndex(StuckSample))
        {
            const FSettle& Stuck = SoldierSettle[StuckSample];
            UE_LOG(LogTemp, Display, TEXT("Strategy1864DebugFormation %s stuck man %d slot=%s goal-gap=%.0f place-gap=%.0f active=%d deadline-in=%.1f start=%.1f halt=%.1f velocity=%.0f place=%s at=%s"),
                *OwnerCompany->StableUnitId.ToString(), StuckSample, *Stuck.Slot.ToString(),
                FVector::Dist2D(SoldierComponents[StuckSample]->GetRelativeLocation(), Stuck.Goal),
                FVector::Dist2D(SoldierComponents[StuckSample]->GetComponentLocation(), FormationPlace(StuckSample, false)),
                Stuck.bActive ? 1 : 0, Stuck.ReformDeadline >= 0.f ? Stuck.ReformDeadline - VisualAnimationTime : -1.f,
                Stuck.StartRemaining, Stuck.HaltRemaining, Stuck.WorldVelocity.Size2D(), *FormationPlace(StuckSample, false).ToString(),
                *SoldierComponents[StuckSample]->GetComponentLocation().ToString());
        }
        UE_LOG(LogTemp, Display, TEXT("Strategy1864DebugFormation %s still-reforming t=%.1f unplaced=%d worst-gap-cm=%.0f waiting=%d arrival=%d front-reached=%d"),
            *OwnerCompany->StableUnitId.ToString(), VisualAnimationTime - FormationPlanStarted, Unplaced, Worst, Waiting, bArrivalReform ? 1 : 0, bArrivalFrontReached ? 1 : 0);
    }
    if (bFormationPlanPending && !bSpreadCrawling) KeepReformSpacing(DeltaTime);
    bool bReformStillWalking = false;
    if (bFormationPlanPending)
        for (const FSettle& ReformSettle : SoldierSettle)
            bReformStillWalking |= ReformSettle.ReformDeadline >= 0.f ||
                (!bVisualTravel && ReformSettle.bActive);
    if (bFormationPlanPending && !bReformStillWalking)
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugFormation")))
            UE_LOG(LogTemp, Display, TEXT("Strategy1864DebugFormation %s complete-seconds=%.3f longest-path-cm=%.1f crossing-pairs-avoided=%d arrival=%d"),
                *OwnerCompany->StableUnitId.ToString(), VisualAnimationTime - FormationPlanStarted,
                FormationLongestPath, FormationCrossingsAvoided, bArrivalReform ? 1 : 0);
        bFormationPlanPending = false;
    }
    {
        // After AFBRYD (backing out step by step) the company gathers: an orderly re-placement into its slots.
        const bool bDisengagingNow = OwnerCompany->OrderComponent &&
            OwnerCompany->OrderComponent->GetCurrentOrder().Type == EStrategyOrderType::Disengage;
        if (bWasDisengaging && !bDisengagingNow && !bFormationPlanPending && !bArrivalReform && !bSpreadCrawling)
            PlanOrderlyReform(false, FTransform(FRotator(0.f, VisualPath.Facing, 0.f), VisualPath.Center));
        bWasDisengaging = bDisengagingNow;
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
        if (bPendingHold && Soldier->GetSkeletalMeshAsset())
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
        if (!Soldier || (SoldierSettle.IsValidIndex(Index) && SoldierSettle[Index].StanceEnd > VisualAnimationTime) || (bAnimationUnchanged && SoldierClips.IsValidIndex(Index) && SoldierClips[Index].Clip.IsValid()) ||
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
        if (bHoldPose && Soldier->GetSkeletalMeshAsset())
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
    bCrowdFailed = false;
    if (bCrowdMode)
    {
        if (EnsureCrowdModel()) bCrowdDirty = true;
        else LeaveCrowdMode();
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

        // The column marches with the rifle on the shoulder; one cycle takes it up (into column) or down (into line).
        if (!WalkShoulderAsset.IsNull() && !ShoulderUpAsset.IsNull() && !ShoulderDownAsset.IsNull())
        {
            if (ShoulderTransitionEnd > GetWorld()->GetTimeSeconds())
            {
                bOutLooping = false;
                return bShoulderRising ? ShoulderUpAsset.Get() : ShoulderDownAsset.Get();
            }
            if (bShoulderColumn)
            {
                return WalkShoulderAsset.Get();
            }
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
    DrillActive = false;
    DrillPath.Reset();
    if (OwnerCompany && OwnerCompany->MovementExecutor) OwnerCompany->MovementExecutor->ClearColumnDrillApproach();
    VisualPath.bInitialized = false;
    VisualPath.Trail.Reset();
    bArrivalReform = false;
    bArrivalFrontReached = false;
    bFormationPlanPending = false;
    FormationSpacingGrid.Reset();
    FormationStepStarts.Reset();
    FormationStepPositions.Reset();
    FormationSpacingNext.Reset();
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
    for (int32 CrowdCorpseIndex = 0; CrowdCorpseIndex < CorpseComponents.Num(); ++CrowdCorpseIndex)
    {
        USkeletalMeshComponent* Corpse = CorpseComponents[CrowdCorpseIndex];
        if (Corpse && bCrowdMode)
        {
            Corpse->SetSkeletalMeshAsset(LoadedSoldierMesh);
            if (CorpseClips.IsValidIndex(CrowdCorpseIndex)) RestoreClip(Corpse, CorpseClips[CrowdCorpseIndex], VisualAnimationTime);
            CreateWeaponComponent(Corpse);
            Corpse->SetVisibility(true, true);
        }
    }
    for (FStrategyCrowdAuxGroup& CrowdBand : CrowdLODBands)
        if (CrowdBand.Instances) CrowdBand.Instances->DestroyComponent();
    CrowdLODBands.Reset();
    CrowdFigureBands.Reset();
    CrowdLivingIds.Reset();
    CrowdFallenIds.Reset();
    CrowdFreeLivingIds.Reset();
    CrowdFreeFallenIds.Reset();
    CrowdNearFigures.Reset();
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
    // A clip change must update the figure's current LOD component, not its obsolete LOD0 slot.
    if (bCrowdMode) bCrowdDirty = true;
}

bool UStrategyInfantryVisualComponent::EnsureCrowdModel()
{
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
        &RaiseToAimAsset, &LoadAsset, &RiseFromLoadAsset, &ReadyAsset, &AimHoldAsset, &DeathWalkingAsset, &SideStepAsset, &SideStepLeftAsset };
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
    for (UAnimSequence* CrowdStanceClip : CrowdStanceClips) if (CrowdStanceClip) Clips.AddUnique(CrowdStanceClip);
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
            for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < CrowdModel->GetNumMaterials(); ++CrowdMaterialIndex)
                Crowd->SetMaterial(CrowdMaterialIndex, CrowdModel->GetMaterial(CrowdMaterialIndex));
        }
    }
    for (int32 CrowdBandIndex = 0; CrowdBandIndex < CrowdLODBands.Num(); ++CrowdBandIndex)
    {
        UInstancedStaticMeshComponent* CrowdBand = CrowdLODBands[CrowdBandIndex].Instances;
        if (!CrowdBand) continue;
        CrowdBand->SetStaticMesh(CrowdModel->GetMesh(1 + CrowdBandIndex / 2));
        for (int32 CrowdSlot = 0; CrowdSlot < CrowdModel->GetNumMaterials(); ++CrowdSlot)
            CrowdBand->SetMaterial(CrowdSlot, CrowdModel->GetMaterial(CrowdSlot));
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
    float Far = CrowdFarCm;
    if (CommandFar >= 0.0f) { Far = CommandFar; CrowdFarCm = Far; }
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
    if (!bCrowdMode) EnterCrowdMode();
    if (bCrowdMode) UpdateNearestFigures(Camera);
    if (bCrowdMode)
    {
        float CrowdColoursNow[UStrategyCrowdModel::CustomDataFloats] = {};
        WriteCrowdColours(CrowdColoursNow);
        for (int32 CrowdColourIndex = 4; CrowdColourIndex < 17; ++CrowdColourIndex)
        {
            if (CrowdColoursNow[CrowdColourIndex] != CrowdLastColours[CrowdColourIndex - 4]) bCrowdDirty = true;
            CrowdLastColours[CrowdColourIndex - 4] = CrowdColoursNow[CrowdColourIndex];
        }
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
        for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < CrowdModel->GetNumMaterials(); ++CrowdMaterialIndex)
            Crowd->SetMaterial(CrowdMaterialIndex, CrowdModel->GetMaterial(CrowdMaterialIndex));
        Crowd->SetNumCustomDataFloats(UStrategyCrowdModel::CustomDataFloats);
        Crowd->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Crowd->SetGenerateOverlapEvents(false);
        Crowd->SetCullDistances(0, 100000); // 1 km per-instance rendering limit; gameplay remains active
        Crowd->SetCastShadow(false); // far VAT figures: avoid animated VSM invalidation
        Crowd->SetCastContactShadow(false);
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
    if (CrowdLODBands.IsEmpty())
    {
        CrowdLODBands.SetNum(8);
        for (int32 CrowdBandIndex = 0; CrowdBandIndex < CrowdLODBands.Num(); ++CrowdBandIndex)
        {
            CrowdLODBands[CrowdBandIndex].Instances = Make(CrowdBandIndex % 2 != 0);
            CrowdLODBands[CrowdBandIndex].Instances->SetStaticMesh(CrowdModel->GetMesh(1 + CrowdBandIndex / 2));
            CrowdLODBands[CrowdBandIndex].Instances->SetCastShadow(CrowdBandIndex >= 6);
        }
    }
    // LOD0 is strictly below 15 m; the extra LOD1 pair covers the remaining shadow range.
    CrowdLiving->SetCastShadow(true);
    CrowdFallen->SetCastShadow(true);
    for (FStrategyCrowdAuxGroup& CrowdBand : CrowdLODBands) CrowdBand.Instances->SetVisibility(true);
    bCrowdMode = true;
    // The skeletal men and their rifles out of sight and not animated (the saving); their clips are kept.
    for (USkeletalMeshComponent* Soldier : SoldierComponents)
    {
        if (Soldier)
        {
            Soldier->SetVisibility(false, true);
            Soldier->SetComponentTickEnabled(false);
            Soldier->SetSkeletalMeshAsset(nullptr);
        }
    }
    for (TObjectPtr<UStaticMeshComponent>& CrowdWeapon : WeaponComponents)
    {
        if (CrowdWeapon) CrowdWeapon->DestroyComponent();
        CrowdWeapon = nullptr;
    }
    for (USkeletalMeshComponent* Corpse : CorpseComponents)
    {
        if (Corpse)
        {
            Corpse->SetVisibility(false, true);
            Corpse->SetComponentTickEnabled(false);
            Corpse->SetSkeletalMeshAsset(nullptr);
            const auto CrowdCorpseChildren = Corpse->GetAttachChildren();
            for (USceneComponent* CrowdChild : CrowdCorpseChildren) if (UStaticMeshComponent* CrowdWeapon = Cast<UStaticMeshComponent>(CrowdChild)) CrowdWeapon->DestroyComponent();
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
    for (FStrategyCrowdAuxGroup& CrowdBand : CrowdLODBands)
    {
        if (CrowdBand.Instances) { CrowdBand.Instances->ClearInstances(); CrowdBand.Instances->SetVisibility(false); }
        CrowdBand.Slots.Reset();
        CrowdBand.FreeSlots.Reset();
    }
    CrowdFigureBands.Reset();
    CrowdNearFigures.Reset();
    CrowdLivingIds.Reset();
    CrowdFallenIds.Reset();
    CrowdFreeLivingIds.Reset();
    CrowdFreeFallenIds.Reset();
    bResumeCrowdPosesNextTick = true;
    const float Now = VisualAnimationTime;
    // Back to full animation, each man where his baked clip had got to.
    for (int32 i = 0; i < SoldierComponents.Num(); ++i)
    {
        if (USkeletalMeshComponent* Soldier = SoldierComponents[i])
        {
            Soldier->SetSkeletalMeshAsset(LoadedSoldierMesh);
            if (OwnerCompany->UniformAppearanceComponent) OwnerCompany->UniformAppearanceComponent->ApplyAppearanceToMesh(Soldier);
            EnsureWeaponComponent(i);
            for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < Soldier->GetNumMaterials(); ++CrowdMaterialIndex)
                if (UMaterialInstanceDynamic* CrowdMID = Cast<UMaterialInstanceDynamic>(Soldier->GetMaterial(CrowdMaterialIndex))) CrowdMID->SetScalarParameterValue(TEXT("CrowdOpacity"), 1.f);
            if (SoldierSettle.IsValidIndex(i)) SoldierSettle[i].NearBlend = 0.f;
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
            Corpse->SetSkeletalMeshAsset(LoadedSoldierMesh);
            CreateWeaponComponent(Corpse);
            if (OwnerCompany->UniformAppearanceComponent) OwnerCompany->UniformAppearanceComponent->ApplyAppearanceToMesh(Corpse);
            for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < Corpse->GetNumMaterials(); ++CrowdMaterialIndex)
                if (UMaterialInstanceDynamic* CrowdMID = Cast<UMaterialInstanceDynamic>(Corpse->GetMaterial(CrowdMaterialIndex))) CrowdMID->SetScalarParameterValue(TEXT("CrowdOpacity"), 1.f);
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
    const APlayerController* CrowdLODPC = GetWorld()->GetFirstPlayerController();
    const FVector CrowdLODCamera = CrowdLODPC && CrowdLODPC->PlayerCameraManager ? CrowdLODPC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
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

    // Persistent instance slots are keyed by figure identity. Casualties leave holes, never move survivors.
    auto CrowdSync = [&](UInstancedStaticMeshComponent* CrowdInstances,
                         const TArray<TObjectPtr<USkeletalMeshComponent>>& CrowdFigures,
                         const TArray<FPlayedClip>& CrowdClips,
                         TMap<TWeakObjectPtr<USkeletalMeshComponent>, int32>& CrowdIds, TArray<int32>& CrowdFreeIds, bool bCrowdWorld, int32 CrowdBand)
    {
        TSet<TWeakObjectPtr<USkeletalMeshComponent>> CrowdPresent;
        for (USkeletalMeshComponent* CrowdFigure : CrowdFigures) if (CrowdFigure && ResolveCrowdBand(CrowdFigure) == CrowdBand) CrowdPresent.Add(CrowdFigure);
        FTransform CrowdHidden = FTransform::Identity;
        CrowdHidden.SetScale3D(FVector::ZeroVector);
        for (auto CrowdIterator = CrowdIds.CreateIterator(); CrowdIterator; ++CrowdIterator)
        {
            if (!CrowdPresent.Contains(CrowdIterator.Key()))
            {
                CrowdInstances->UpdateInstanceTransform(CrowdIterator.Value(), CrowdHidden, bCrowdWorld, false, false);
                CrowdFreeIds.Add(CrowdIterator.Value());
                CrowdIterator.RemoveCurrent();
            }
        }
        for (int32 CrowdFigureIndex = 0; CrowdFigureIndex < CrowdFigures.Num(); ++CrowdFigureIndex)
        {
            USkeletalMeshComponent* CrowdFigure = CrowdFigures[CrowdFigureIndex];
            if (!CrowdFigure || !CrowdPresent.Contains(CrowdFigure)) continue;
            FTransform CrowdPose = bCrowdWorld ? CrowdFigure->GetComponentTransform() : CrowdFigure->GetRelativeTransform();
            if ((!bCrowdWorld && SoldierSettle.IsValidIndex(CrowdFigureIndex) && SoldierSettle[CrowdFigureIndex].NearBlend >= 1.f) ||
                (bCrowdWorld && CorpseBirthTimes.IsValidIndex(CrowdFigureIndex) && GetWorld()->GetTimeSeconds() - CorpseBirthTimes[CrowdFigureIndex] >= kCorpseLifeSeconds))
                CrowdPose.SetScale3D(FVector::ZeroVector);
            int32* CrowdExistingId = CrowdIds.Find(CrowdFigure);
            const int32 CrowdId = CrowdExistingId ? *CrowdExistingId : !CrowdFreeIds.IsEmpty() ? CrowdFreeIds.Pop(EAllowShrinking::No) : CrowdInstances->AddInstance(CrowdPose, bCrowdWorld);
            if (!CrowdExistingId) CrowdIds.Add(CrowdFigure, CrowdId);
            CrowdInstances->UpdateInstanceTransform(CrowdId, CrowdPose, bCrowdWorld, false, false);
            DataFor(CrowdClips.IsValidIndex(CrowdFigureIndex) ? &CrowdClips[CrowdFigureIndex] : nullptr, Data);
            WriteCrowdColours(Data);
            const float CrowdDistance = FVector::Distance(CrowdFigure->GetComponentLocation(), CrowdLODCamera);
            Data[17] = CrowdDistance > 3000.f ? 1.f : 0.f;
            Data[18] = CrowdDistance > 6000.f && Data[3] > 0.f ? 1.f : 0.f; // only looping far clips step at baked FPS
            Data[16] = bCrowdWorld ? (CorpseBirthTimes.IsValidIndex(CrowdFigureIndex) ? FMath::Clamp((kCorpseLifeSeconds - (GetWorld()->GetTimeSeconds() - CorpseBirthTimes[CrowdFigureIndex])) / 5.f, 0.f, 1.f) : 1.f)
                : (SoldierSettle.IsValidIndex(CrowdFigureIndex) ? 1.f - SoldierSettle[CrowdFigureIndex].NearBlend : 1.f);
            CrowdInstances->SetCustomData(CrowdId, TArrayView<const float>(Data, UStrategyCrowdModel::CustomDataFloats), false);
        }
    };
    CrowdSync(CrowdLiving, SoldierComponents, SoldierClips, CrowdLivingIds, CrowdFreeLivingIds, false, 0);
    CrowdSync(CrowdFallen, CorpseComponents, CorpseClips, CrowdFallenIds, CrowdFreeFallenIds, true, 0);
    for (int32 CrowdBandIndex = 0; CrowdBandIndex < CrowdLODBands.Num(); ++CrowdBandIndex)
    {
        FStrategyCrowdAuxGroup& CrowdBand = CrowdLODBands[CrowdBandIndex];
        const bool bCrowdBodies = CrowdBandIndex % 2 != 0;
        CrowdSync(CrowdBand.Instances, bCrowdBodies ? CorpseComponents : SoldierComponents,
            bCrowdBodies ? CorpseClips : SoldierClips, CrowdBand.Slots, CrowdBand.FreeSlots, bCrowdBodies, 1 + CrowdBandIndex / 2);
        CrowdBand.Instances->MarkRenderStateDirty();
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
    const bool CrowdHasClip = CrowdModel->MakeCustomData(PlayedSequence, PlayedWorldStart, PlayedWorldRate, Played.bLoop, Out, PlayedWorldNow, PlayedPosition);
    WriteCrowdColours(Out);
    return CrowdHasClip;
}

void UStrategyInfantryVisualComponent::WriteCrowdColours(float* Out) const
{
    for (int32 CrowdDataIndex = 4; CrowdDataIndex < UStrategyCrowdModel::CustomDataFloats; ++CrowdDataIndex) Out[CrowdDataIndex] = 0.f;
    Out[16] = 1.f;
    if (!OwnerCompany || !OwnerCompany->UniformAppearanceComponent) return;
    const UStrategyUniformAppearanceComponent* CrowdAppearance = OwnerCompany->UniformAppearanceComponent;
    const FStrategyUniformColors CrowdColours = CrowdAppearance->GetResolvedColors();
    const FLinearColor CrowdParts[] = { CrowdColours.Coat, CrowdColours.Trousers, CrowdColours.HeadgearDetail };
    for (int32 CrowdPart = 0; CrowdPart < 3; ++CrowdPart)
    {
        Out[4 + CrowdPart * 3] = CrowdParts[CrowdPart].R;
        Out[5 + CrowdPart * 3] = CrowdParts[CrowdPart].G;
        Out[6 + CrowdPart * 3] = CrowdParts[CrowdPart].B;
    }
    const bool CrowdPaletteAllowed = CrowdAppearance->CanOverrideHistoricalPalette();
    Out[13] = CrowdPaletteAllowed && CrowdAppearance->Overrides.bOverrideCoat ? 1.f : 0.f;
    Out[14] = CrowdPaletteAllowed && CrowdAppearance->Overrides.bOverrideTrousers ? 1.f : 0.f;
    Out[15] = CrowdPaletteAllowed && CrowdAppearance->Overrides.bOverrideHeadgearDetail ? 1.f : 0.f;
}

void UStrategyInfantryVisualComponent::SelectNearestFigures(UWorld* World, const FVector& Camera, int32 CrowdBudget)
{
    if (!World) return;
    UStrategyCrowdSubsystem* CrowdSub = World->GetSubsystem<UStrategyCrowdSubsystem>();
    if (!CrowdSub) return;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864CrowdNearest="), CrowdBudget);
    if (CrowdSub->NearSelectionFrame != GFrameCounter)
    {
        CrowdSub->NearSelectionFrame = GFrameCounter;
        CrowdSub->NearBudget = FMath::Max(0, CrowdBudget);
        CrowdSub->SweepAuxiliary();
        CrowdSub->NearFigures.Reset();
        CrowdSub->NearGeometryCount = 0;
        TArray<USkeletalMeshComponent*> CrowdCandidates;
        for (const TWeakObjectPtr<AStrategyUnit>& CrowdEntry : Strategy1864Performance::VisualUnits(World))
        {
            const AStrategyCompanyUnit* CrowdCompany = Cast<AStrategyCompanyUnit>(CrowdEntry.Get());
            const UStrategyInfantryVisualComponent* CrowdVisual = CrowdCompany ? CrowdCompany->InfantryVisualComponent.Get() : nullptr;
            if (!CrowdVisual || !CrowdVisual->bEnabled) continue;
            CrowdSub->NearGeometryCount += CrowdVisual->CrowdNearFigures.Num();
            for (USkeletalMeshComponent* CrowdSoldier : CrowdVisual->SoldierComponents)
            {
                const float CrowdRadius = CrowdVisual->CrowdNearFigures.Contains(CrowdSoldier) ? CrowdVisual->CrowdFarCm : FMath::Min(CrowdVisual->CrowdNearCm, CrowdVisual->CrowdFarCm);
                if (CrowdVisual->CrowdModel && CrowdSoldier && FVector::DistSquared(Camera, CrowdSoldier->GetComponentLocation()) < FMath::Square(CrowdRadius)) CrowdCandidates.Add(CrowdSoldier);
            }
        }
        for (auto CrowdAuxIterator = CrowdSub->AuxiliaryFigures.CreateIterator(); CrowdAuxIterator; ++CrowdAuxIterator)
        {
            USkeletalMeshComponent* CrowdAuxFigure = CrowdAuxIterator.Key().Get();
            if (!CrowdAuxFigure || !CrowdAuxFigure->IsRegistered()) { CrowdAuxIterator.RemoveCurrent(); continue; }
            if (CrowdAuxIterator.Value().NearBlend > 0.f) ++CrowdSub->NearGeometryCount;
            const float CrowdAuxRadius = CrowdAuxIterator.Value().NearBlend > 0.f ? 2000.f : 1800.f;
            if (CrowdAuxIterator.Value().bBaked && CrowdAuxIterator.Value().bLiving && FVector::DistSquared(Camera, CrowdAuxFigure->GetComponentLocation()) < FMath::Square(CrowdAuxRadius))
                CrowdCandidates.Add(CrowdAuxFigure);
        }
        CrowdCandidates.Sort([&Camera](const USkeletalMeshComponent& CrowdA, const USkeletalMeshComponent& CrowdB)
        {
            return FVector::DistSquared(Camera, CrowdA.GetComponentLocation()) < FVector::DistSquared(Camera, CrowdB.GetComponentLocation());
        });
        for (int32 CrowdIndex = 0; CrowdIndex < FMath::Min(FMath::Max(0, CrowdBudget), CrowdCandidates.Num()); ++CrowdIndex)
            CrowdSub->NearFigures.Add(CrowdCandidates[CrowdIndex]);
    }
}

int32 UStrategyInfantryVisualComponent::ResolveCrowdBand(USkeletalMeshComponent* Figure) const
{
    if (!Figure || !GetWorld()) return 3;
    const APlayerController* CrowdPC = GetWorld()->GetFirstPlayerController();
    if (!CrowdPC || !CrowdPC->PlayerCameraManager) return 3;
    const float CrowdDistance = FVector::Distance(CrowdPC->PlayerCameraManager->GetCameraLocation(), Figure->GetComponentLocation());
    int32 CrowdBand = UStrategyCrowdModel::DistanceBand(CrowdDistance);
    // LOD0 is reserved for the global nearest N (including complementary handoff).
    const UStrategyCrowdSubsystem* CrowdLODSub = GetWorld()->GetSubsystem<UStrategyCrowdSubsystem>();
    if (CrowdBand == 0 && !CrowdNearFigures.Contains(Figure) && (!CrowdLODSub || !CrowdLODSub->NearFigures.Contains(Figure))) CrowdBand = 1;
    if (CrowdBand == 1 && CrowdDistance < 1200.f) CrowdBand = 4;
    return CrowdBand;
}

void UStrategyInfantryVisualComponent::UpdateNearestFigures(const FVector& Camera)
{
    UStrategyCrowdSubsystem* CrowdSub = GetWorld()->GetSubsystem<UStrategyCrowdSubsystem>();
    if (!CrowdSub) return;
    SelectNearestFigures(GetWorld(), Camera, CrowdNearestCount);
    const int32 CrowdBudget = CrowdSub->NearBudget;
    for (USkeletalMeshComponent* CrowdFigure : CorpseComponents)
        if (CrowdFigure)
        {
            const int32 CrowdBodyQuality = ResolveCrowdBand(CrowdFigure) + (FVector::DistSquared(Camera, CrowdFigure->GetComponentLocation()) > FMath::Square(3000.f) ? 8 : 0);
            if (CrowdFigureBands.FindOrAdd(CrowdFigure, -1) != CrowdBodyQuality)
            { CrowdFigureBands.Add(CrowdFigure, CrowdBodyQuality); bCrowdDirty = true; }
        }
    for (int32 CrowdIndex = 0; CrowdIndex < SoldierComponents.Num(); ++CrowdIndex)
    {
        USkeletalMeshComponent* CrowdSoldier = SoldierComponents[CrowdIndex];
        if (!CrowdSoldier) continue;
        if (!SoldierSettle.IsValidIndex(CrowdIndex)) continue;
        const bool CrowdWasNear = CrowdNearFigures.Contains(CrowdSoldier);
        // Default is genuinely all-VAT: the nearest N receive VAT LOD0, using the same texture/normal shader.
        // Keep the old skeletal handoff available only for explicit visual comparison.
        static const bool bCrowdSkeletalNear = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864CrowdSkeletalNear"));
        bool CrowdTargetNear = bCrowdSkeletalNear && CrowdSub->NearFigures.Contains(CrowdSoldier);
        // Demotions release their slot after fading. Delay promotions to keep the hard global N budget.
        if (!CrowdWasNear && CrowdSub->NearGeometryCount >= FMath::Max(0, CrowdBudget)) CrowdTargetNear = false;
        FSettle& CrowdSettle = SoldierSettle[CrowdIndex];
        const float CrowdOldBlend = CrowdSettle.NearBlend;
        CrowdSettle.NearBlend = FMath::FInterpConstantTo(CrowdOldBlend, CrowdTargetNear ? 1.f : 0.f, GetWorld()->GetDeltaSeconds(), 5.f);
        const bool CrowdIsNear = CrowdTargetNear || CrowdSettle.NearBlend > 0.f;
        if (CrowdWasNear != CrowdIsNear)
        {
            if (CrowdIsNear)
            {
                CrowdNearFigures.Add(CrowdSoldier);
                ++CrowdSub->NearGeometryCount;
                CrowdSoldier->SetSkeletalMeshAsset(LoadedSoldierMesh);
                // Reset overrides retained while the source mesh was null: near figures use their own texture/normal material.
                for (int32 CrowdSlot = 0; CrowdSlot < LoadedSoldierMesh->GetMaterials().Num(); ++CrowdSlot)
                    CrowdSoldier->SetMaterial(CrowdSlot, LoadedSoldierMesh->GetMaterials()[CrowdSlot].MaterialInterface);
                if (OwnerCompany->UniformAppearanceComponent) OwnerCompany->UniformAppearanceComponent->ApplyAppearanceToMesh(CrowdSoldier);
                for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < CrowdSoldier->GetNumMaterials(); ++CrowdMaterialIndex)
                    if (!Cast<UMaterialInstanceDynamic>(CrowdSoldier->GetMaterial(CrowdMaterialIndex))) CrowdSoldier->CreateAndSetMaterialInstanceDynamic(CrowdMaterialIndex);
                EnsureWeaponComponent(CrowdIndex);
            }
            else
            {
                CrowdNearFigures.Remove(CrowdSoldier);
                --CrowdSub->NearGeometryCount;
            }
            CrowdSoldier->SetVisibility(CrowdIsNear, true);
            bCrowdDirty = true;
        }
        if (CrowdOldBlend != CrowdSettle.NearBlend) bCrowdDirty = true;
        // Both representations share the clock and complementary masked pixel coverage during 0.2s handoff.
        if (CrowdIsNear && SoldierClips.IsValidIndex(CrowdIndex))
        {
            RestoreClip(CrowdSoldier, SoldierClips[CrowdIndex], VisualAnimationTime);
            for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < CrowdSoldier->GetNumMaterials(); ++CrowdMaterialIndex)
                if (UMaterialInstanceDynamic* CrowdMID = Cast<UMaterialInstanceDynamic>(CrowdSoldier->GetMaterial(CrowdMaterialIndex)))
                    CrowdMID->SetScalarParameterValue(TEXT("CrowdOpacity"), CrowdSettle.NearBlend);
            if (WeaponComponents.IsValidIndex(CrowdIndex) && WeaponComponents[CrowdIndex])
                for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < WeaponComponents[CrowdIndex]->GetNumMaterials(); ++CrowdMaterialIndex)
                    if (UMaterialInstanceDynamic* CrowdMID = Cast<UMaterialInstanceDynamic>(WeaponComponents[CrowdIndex]->GetMaterial(CrowdMaterialIndex)))
                        CrowdMID->SetScalarParameterValue(TEXT("CrowdOpacity"), CrowdSettle.NearBlend);
        }
        const int32 CrowdCurrentBand = ResolveCrowdBand(CrowdSoldier) + (FVector::DistSquared(Camera, CrowdSoldier->GetComponentLocation()) > FMath::Square(3000.f) ? 8 : 0);
        if (CrowdFigureBands.FindOrAdd(CrowdSoldier, -1) != CrowdCurrentBand)
        { CrowdFigureBands.Add(CrowdSoldier, CrowdCurrentBand); bCrowdDirty = true; }
        CrowdSoldier->SetCastContactShadow(false); // dense formation contact shadows can blacken the closest uniforms
        CrowdSoldier->SetCastShadow(CrowdIsNear && FVector::DistSquared(Camera, CrowdSoldier->GetComponentLocation()) < FMath::Square(2500.f));
        CrowdSoldier->SetComponentTickEnabled(false);
        if (!CrowdIsNear)
        {
            if (CrowdSoldier->GetSkeletalMeshAsset()) CrowdSoldier->SetSkeletalMeshAsset(nullptr);
            if (WeaponComponents.IsValidIndex(CrowdIndex) && WeaponComponents[CrowdIndex])
            {
                WeaponComponents[CrowdIndex]->DestroyComponent();
                WeaponComponents[CrowdIndex] = nullptr;
            }
        }
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugGpu")) && FPlatformTime::Seconds() >= CrowdBandLogAt)
    {
        int32 CrowdCounts[4] = {};
        int32 CrowdFallenCounts[4] = {};
        int32 CrowdCasters = 0;
        int32 CrowdVATCasters = 0;
        for (int32 CrowdBodyIndex = 0; CrowdBodyIndex < CorpseComponents.Num(); ++CrowdBodyIndex)
        {
            USkeletalMeshComponent* CrowdBody = CorpseComponents[CrowdBodyIndex];
            if (CrowdBody)
            {
                const int32 CrowdBodyBand = ResolveCrowdBand(CrowdBody);
                ++CrowdFallenCounts[CrowdBodyBand == 4 ? 1 : CrowdBodyBand];
                if ((CrowdBodyBand == 0 || CrowdBodyBand == 4) &&
                    (!CorpseBirthTimes.IsValidIndex(CrowdBodyIndex) || GetWorld()->GetTimeSeconds() - CorpseBirthTimes[CrowdBodyIndex] < 65.f)) ++CrowdVATCasters;
            }
        }
        for (int32 CrowdFigureIndex = 0; CrowdFigureIndex < SoldierComponents.Num(); ++CrowdFigureIndex)
        {
            USkeletalMeshComponent* CrowdFigure = SoldierComponents[CrowdFigureIndex];
            if (CrowdFigure)
            {
                const int32 CrowdFigureBand = ResolveCrowdBand(CrowdFigure);
                ++CrowdCounts[CrowdFigureBand == 4 ? 1 : CrowdFigureBand];
                if ((CrowdFigureBand == 0 || CrowdFigureBand == 4) && SoldierSettle.IsValidIndex(CrowdFigureIndex) && SoldierSettle[CrowdFigureIndex].NearBlend < 1.f) ++CrowdVATCasters;
                if (CrowdNearFigures.Contains(CrowdFigure) && FVector::DistSquared(Camera, CrowdFigure->GetComponentLocation()) < FMath::Square(2500.f)) ++CrowdCasters;
            }
        }
        UE_LOG(LogTemp, Display, TEXT("PROJECT1864-CROWD: %s living bands=[%d,%d,%d,%d] fallen bands=[%d,%d,%d,%d] skeletal-shadow-casters=%d VAT-shadow-casters=%d"),
            *OwnerCompany->StableUnitId.ToString(), CrowdCounts[0], CrowdCounts[1], CrowdCounts[2], CrowdCounts[3],
            CrowdFallenCounts[0], CrowdFallenCounts[1], CrowdFallenCounts[2], CrowdFallenCounts[3], CrowdCasters, CrowdVATCasters);
        CrowdBandLogAt = FPlatformTime::Seconds() + 1.0;
    }
}

void UStrategyInfantryVisualComponent::EnsureWeaponComponent(int32 Index)
{
    if (!SoldierComponents.IsValidIndex(Index) || !WeaponComponents.IsValidIndex(Index) || WeaponComponents[Index]) return;
    WeaponComponents[Index] = CreateWeaponComponent(SoldierComponents[Index]);
}

UStaticMeshComponent* UStrategyInfantryVisualComponent::CreateWeaponComponent(USkeletalMeshComponent* Soldier)
{
    if (!Soldier || !OwnerCompany) return nullptr;
    UStaticMeshComponent* CrowdWeapon = NewObject<UStaticMeshComponent>(OwnerCompany, NAME_None, RF_Transient);
    CrowdWeapon->SetStaticMesh(CurrentRifleMesh());
    for (int32 CrowdMaterialIndex = 0; CrowdMaterialIndex < CrowdWeapon->GetNumMaterials(); ++CrowdMaterialIndex)
        CrowdWeapon->CreateAndSetMaterialInstanceDynamic(CrowdMaterialIndex);
    CrowdWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CrowdWeapon->SetGenerateOverlapEvents(false);
    CrowdWeapon->SetCastShadow(false);
    CrowdWeapon->bVisibleInRayTracing = false;
    CrowdWeapon->RegisterComponent();
    CrowdWeapon->AttachToComponent(Soldier, FAttachmentTransformRules::SnapToTargetNotIncludingScale, RightHandBoneName);
    CrowdWeapon->SetRelativeTransform(WeaponRelativeTransform);
    return CrowdWeapon;
}

void UStrategyInfantryVisualComponent::UpdateStanceTransitions()
{
    if (!OwnerCompany || !OwnerCompany->StanceComponent) return;
    const uint8 CrowdTargetStance = static_cast<uint8>(OwnerCompany->StanceComponent->Stance);
    for (int32 CrowdFigureIndex = 0; CrowdFigureIndex < SoldierSettle.Num() && CrowdFigureIndex < SoldierComponents.Num(); ++CrowdFigureIndex)
    {
        FSettle& CrowdSettle = SoldierSettle[CrowdFigureIndex];
        if (CrowdSettle.bActive || CrowdSettle.SwitchAt >= 0.f || CrowdSettle.StanceEnd > VisualAnimationTime) continue;
        if (CrowdSettle.VisualStance == CrowdTargetStance)
        {
            if (CrowdSettle.StanceEnd >= 0.f)
            {
                CrowdSettle.StanceEnd = -1.f;
                bool CrowdLoop = true;
                UAnimSequence* CrowdIdle = ResolveAnimation(CrowdLoop);
                PlayOnSoldier(SoldierComponents[CrowdFigureIndex], CrowdIdle, CrowdLoop, CrowdLoop);
            }
            continue;
        }
        // Standing=0, prone=1, kneeling=2. Standing/prone changes pass through kneeling.
        int32 CrowdTransitionIndex = 0;
        uint8 CrowdNextStance = 2;
        if (CrowdSettle.VisualStance == 1) CrowdTransitionIndex = 3;
        else if (CrowdSettle.VisualStance == 2)
        {
            CrowdNextStance = CrowdTargetStance;
            CrowdTransitionIndex = CrowdTargetStance == 0 ? 1 : 2;
        }
        UAnimSequence* CrowdTransition = CrowdStanceClips.IsValidIndex(CrowdTransitionIndex) ? CrowdStanceClips[CrowdTransitionIndex].Get() : nullptr;
        CrowdSettle.VisualStance = CrowdNextStance;
        if (CrowdTransition)
        {
            PlayOnSoldier(SoldierComponents[CrowdFigureIndex], CrowdTransition, false, false);
            CrowdSettle.StanceEnd = VisualAnimationTime + CrowdTransition->GetPlayLength();
        }
        else CrowdSettle.StanceEnd = VisualAnimationTime;
    }
}


bool UStrategyInfantryVisualComponent::PlanColumnDrill()
{
    // Every refusal of a line <-> column change is logged with its reason (the old morph runs instead).
    const auto DrillReject = [this](const TCHAR* DrillWhy) -> bool
    {
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s not used: %s (formation %d -> %d, figures %d)"),
            OwnerCompany ? *OwnerCompany->StableUnitId.ToString() : TEXT("?"), DrillWhy, int32(CachedFormationValue),
            OwnerCompany && OwnerCompany->FormationComponent ? int32(OwnerCompany->FormationComponent->CurrentFormation) : -1,
            SoldierComponents.Num());
        return false;
    };
    if (!OwnerCompany || !OwnerCompany->MovementExecutor || !OwnerCompany->FormationComponent || !OwnerCompany->OrderComponent)
        return DrillReject(TEXT("missing component"));
    const EStrategyFormationType DrillNew = OwnerCompany->FormationComponent->CurrentFormation;
    const bool DrillEntering = CachedFormationValue == uint8(EStrategyFormationType::Line) && DrillNew == EStrategyFormationType::MarchColumn;
    const bool DrillLeaving = CachedFormationValue == uint8(EStrategyFormationType::MarchColumn) && DrillNew == EStrategyFormationType::Line;
    if (!DrillEntering && !DrillLeaving) return false;   // not a line <-> column change: nothing to log
    if (!FStrategyColumnDrill::Enabled()) return DrillReject(TEXT("-Strategy1864Drill=0"));
    int32 DrillRigidSetting = 1;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Rigid="), DrillRigidSetting);
    if (!bRigidCompany || DrillRigidSetting == 0) return DrillReject(TEXT("not a rigid company"));
    if (!VisualPath.bInitialized) return DrillReject(TEXT("visual path not initialised yet"));
    if (OwnerCompany->FormationComponent->RankCount != 3 || OwnerCompany->FormationComponent->ColumnWidth != 3)
        return DrillReject(TEXT("not 3 ranks / 3 abreast"));
    if (SoldierComponents.IsEmpty() || SoldierComponents.Num() != SoldierSettle.Num())
        return DrillReject(TEXT("figures not ready"));
    if (OwnerCompany->StanceComponent && OwnerCompany->StanceComponent->Stance == EStrategyStance::Prone)
        return DrillReject(TEXT("lying down"));
    if (OwnerCompany->SkirmisherComponent && (OwnerCompany->SkirmisherComponent->IsDeployed() ||
        OwnerCompany->SkirmisherComponent->State == EStrategySkirmisherState::Deploying))
        return DrillReject(TEXT("skirmish screen out"));
    const float DrillFileSpacing = OwnerCompany->FormationComponent->SoldierLateralSpacingCm;
    const float DrillRankSpacing = OwnerCompany->FormationComponent->SoldierRankSpacingCm;
    if (DrillFileSpacing <= 0.f || DrillRankSpacing <= 0.f) return DrillReject(TEXT("zero spacing"));
    int32 DrillRows = (SoldierComponents.Num() + 2) / 3;
    if (DrillEntering)
    {
        // The line as it stands: losses and the colour bearer (out of the grid) leave holes, so count its files from the
        // slots (symmetric about the centre), not from the head count.
        float DrillHalfSpan = 0.f;
        for (const FSettle& DrillSpanMan : SoldierSettle) DrillHalfSpan = FMath::Max(DrillHalfSpan, FMath::Abs(DrillSpanMan.Slot.Y));
        DrillRows = FMath::Max(DrillRows, FMath::RoundToInt(2.f * DrillHalfSpan / DrillFileSpacing) + 1);
    }
    const float DrillWidth = (DrillRows - 1) * DrillFileSpacing;
    const FStrategyOrder DrillCurrentOrder = OwnerCompany->OrderComponent->GetCurrentOrder();
    const FVector DrillOldFacing = FRotator(0.f, VisualPath.Facing, 0.f).Vector();
    DrillRadius = FStrategyColumnDrill::Radius(DrillRankSpacing, DrillFileSpacing);
    if (DrillEntering)
    {
        // Line -> column: right (or left) face, and the column files off from the flank towards the march goal as a snake.
        FVector DrillMarch = DrillOldFacing;
        if (OwnerCompany->MovementExecutor->HasMovementGoal())
        {
            DrillMarch = (OwnerCompany->MovementExecutor->GetMovementGoal() - VisualPath.Center).GetSafeNormal2D();
            if (DrillMarch.IsNearlyZero()) DrillMarch = DrillOldFacing;
        }
        // Always "right face" (user 2026-10-20): the column files off from the right flank, the colours at its head.
        const float DrillEnterFlank = 1.f;
        if (!SeedColumnSnake(DrillOldFacing, FStrategyColumnDrill::Right(DrillOldFacing) * DrillEnterFlank, DrillEnterFlank, DrillWidth, DrillRows))
            return DrillReject(TEXT("the line is not in its file/rank grid (snake not seeded)"));
        return true;
    }
    // Column -> line: the men keep following the trail the column has walked; the head leaves it, curves onto the
    // line's middle-rank axis and walks to the far flank. Every man halts at his place, then all face the front.
    if (VisualPath.Trail.Num() < 2) return DrillReject(TEXT("no column trail to follow"));
    FVector DrillLineCentre = FVector::ZeroVector;
    float DrillLineYaw = 0.f;
    const bool DrillAtArrival = OwnerCompany->MovementExecutor->GetColumnDrillArrival(DrillLineCentre, DrillLineYaw);
    const FStrategyVisualFormationPath::FSample DrillHead = VisualPath.Trail.Last();
    const FVector DrillHeadDir = FRotator(0.f, DrillHead.Yaw, 0.f).Vector();
    const FVector DrillFront = DrillAtArrival ? FRotator(0.f, DrillLineYaw, 0.f).Vector() : DrillHeadDir;
    // The head always ends on the right flank and the men face left into line (user's sketch 2026-10-20).
    const float DrillFlank = 1.f;
    const FVector DrillAxis = FStrategyColumnDrill::Right(DrillFront) * DrillFlank;
    const float DrillTrailBase = VisualPath.Trail[0].Distance;
    DrillPath.Reset();
    for (const FStrategyVisualFormationPath::FSample& DrillTrailSample : VisualPath.Trail)
        DrillPath.Add({DrillTrailSample.Position, DrillTrailSample.Yaw, DrillTrailSample.Distance - DrillTrailBase});
    const float DrillHeadS = DrillPath.Last().Distance;
    FVector DrillNear;
    if (DrillAtArrival)
    {
        DrillNear = DrillLineCentre - DrillAxis * (DrillWidth * 0.5f);
        DrillNear.Z = DrillHead.Position.Z;
        FStrategyColumnDrill::AppendHermite(DrillPath, DrillHead.Position, DrillHeadDir, DrillNear, DrillAxis);
    }
    else
    {
        const TArray<FStrategyColumnDrill::FSample> DrillTurnPath =
            FStrategyColumnDrill::BuildPath(DrillHead.Position, DrillHeadDir, 0.f, DrillRadius, DrillAxis, 0.f);
        for (int32 DrillTurnIndex = 1; DrillTurnIndex < DrillTurnPath.Num(); ++DrillTurnIndex)
            if (DrillTurnPath[DrillTurnIndex].Distance > 0.f)
                DrillPath.Add({DrillTurnPath[DrillTurnIndex].Position, DrillTurnPath[DrillTurnIndex].Yaw, DrillHeadS + DrillTurnPath[DrillTurnIndex].Distance});
        DrillNear = DrillPath.Last().Position;
    }
    const float DrillAxisStart = DrillPath.Last().Distance;
    FStrategyColumnDrill::AppendStraight(DrillPath, DrillNear, DrillAxis, DrillWidth);
    const float DrillFar = DrillPath.Last().Distance;
    // Lanes widen from the column's file spacing to the rank spacing over the last three metres before the axis.
    DrillStraightA = FMath::Max(DrillHeadS, DrillAxisStart - 300.f);
    DrillArcEnd = DrillAxisStart;
    DrillFinalFrame = FTransform(DrillFront.Rotation(), DrillNear + DrillAxis * (DrillWidth * 0.5f) + DrillFront * DrillRankSpacing);
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugFormation")))
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s to-line plan arrival=%d head=%s head-yaw=%.1f axis-yaw=%.1f trail-cm=%.0f path-cm=%.0f line=%s front=%.1f"),
            *OwnerCompany->StableUnitId.ToString(), DrillAtArrival ? 1 : 0, *DrillHead.Position.ToString(), DrillHead.Yaw,
            DrillAxis.Rotation().Yaw, DrillHeadS, DrillFar, *DrillFinalFrame.GetLocation().ToString(), DrillFront.Rotation().Yaw);
    // Validate every assignment before modifying any man. An interrupted/partial layout uses the morph.
    TArray<FSettle> DrillPlanned = SoldierSettle;
    TArray<int32> DrillSlots;
    TSet<int32> DrillUsed;
    const float DrillColumnRowSpacing = SnakeRowSpacing > 0.f ? SnakeRowSpacing : DrillRankSpacing;
    // A snake column knows every man's row and lane already: they decide his place, not where he happens to stand (in a bend
    // the lanes press together and two men were read into one place, so the drill was refused: 2026-10-22 test).
    bool bDrillSnakeKnown = false;
    for (const FSettle& DrillSnakeMan : SoldierSettle) bDrillSnakeKnown |= DrillSnakeMan.SnakeRow > 0;
    for (int32 DrillIndex = 0; DrillIndex < SoldierComponents.Num(); ++DrillIndex)
    {
        if (!SoldierComponents[DrillIndex] || !SoldierSettle[DrillIndex].bPlaced) return DrillReject(TEXT("a figure is not placed yet"));
        const FVector DrillWorld = VisualPath.PreviousUnit.TransformPosition(SoldierComponents[DrillIndex]->GetRelativeLocation());
        // His place on the walked trail: the nearest point of the trail part of the path.
        double DrillBest = TNumericLimits<double>::Max();
        float DrillS0 = 0.f;
        float DrillOffset = 0.f;
        for (int32 DrillSeg = 0; DrillSeg + 1 < DrillPath.Num() && DrillPath[DrillSeg].Distance < DrillHeadS; ++DrillSeg)
        {
            const FVector DrillP0 = DrillPath[DrillSeg].Position;
            FVector DrillEdge = DrillPath[DrillSeg + 1].Position - DrillP0;
            DrillEdge.Z = 0.f;
            const double DrillEdgeLength2 = DrillEdge.SizeSquared2D();
            const double DrillT = DrillEdgeLength2 > 0.01 ?
                FMath::Clamp(double(FVector::DotProduct(DrillWorld - DrillP0, DrillEdge)) / DrillEdgeLength2, 0.0, 1.0) : 0.0;
            const FVector DrillOn = DrillP0 + DrillEdge * DrillT;
            const double DrillGap = FVector::Dist2D(DrillWorld, DrillOn);
            if (DrillGap < DrillBest)
            {
                DrillBest = DrillGap;
                DrillS0 = DrillPath[DrillSeg].Distance + float(DrillT) * (DrillPath[DrillSeg + 1].Distance - DrillPath[DrillSeg].Distance);
                const FVector DrillSide = DrillEdgeLength2 > 0.01 ? FStrategyColumnDrill::Right(DrillEdge.GetSafeNormal2D()) :
                    FStrategyColumnDrill::Right(FRotator(0.f, DrillPath[DrillSeg].Yaw, 0.f).Vector());
                DrillOffset = FVector::DotProduct(DrillWorld - DrillOn, DrillSide);
            }
        }
        if (DrillBest > DrillFileSpacing * 2.5) return DrillReject(TEXT("a man is not on the column trail"));
        const int32 DrillRow = bDrillSnakeKnown ? FMath::Clamp(SoldierSettle[DrillIndex].SnakeRow, 0, DrillRows - 1) :
            FMath::Clamp(FMath::RoundToInt((DrillHeadS - DrillS0) / DrillColumnRowSpacing), 0, DrillRows - 1);
        const int32 DrillLane = bDrillSnakeKnown ? FMath::Clamp(SoldierSettle[DrillIndex].SnakeLane, 0, 2) :
            FMath::Clamp(FMath::RoundToInt(DrillOffset / DrillFileSpacing) + 1, 0, 2);
        const int32 DrillRank = DrillFlank > 0.f ? DrillLane : 2 - DrillLane;
        const int32 DrillFile = DrillFlank > 0.f ? DrillRows - 1 - DrillRow : DrillRow;
        int32 DrillSlotId = DrillFile * 3 + DrillRank;
        if (DrillUsed.Contains(DrillSlotId))
        {
            // The nearest free place (as the snake's seeding does) rather than refusing the drill.
            int32 DrillFree = INDEX_NONE;
            int32 DrillFreeCost = TNumericLimits<int32>::Max();
            for (int32 DrillTry = 0; DrillTry < DrillRows * 3; ++DrillTry)
            {
                if (DrillUsed.Contains(DrillTry)) continue;
                const int32 DrillTryCost = FMath::Abs(DrillTry / 3 - DrillFile) * 3 + FMath::Abs(DrillTry % 3 - DrillRank);
                if (DrillTryCost < DrillFreeCost) { DrillFreeCost = DrillTryCost; DrillFree = DrillTry; }
            }
            if (DrillFree == INDEX_NONE) return DrillReject(TEXT("slot assignment clash (no free place on the trail)"));
            DrillSlotId = DrillFree;
        }
        DrillUsed.Add(DrillSlotId);
        // Row, lane, file and rank of the place he actually got (a clash moves him to the nearest free one).
        const int32 DrillPlaceFile = DrillSlotId / 3, DrillPlaceRank = DrillSlotId % 3;
        const int32 DrillPlaceRow = DrillFlank > 0.f ? DrillRows - 1 - DrillPlaceFile : DrillPlaceFile;
        const int32 DrillPlaceLane = DrillFlank > 0.f ? DrillPlaceRank : 2 - DrillPlaceRank;
        FSettle& DrillMan = DrillPlanned[DrillIndex];
        DrillMan.DrillS0 = DrillS0;
        DrillMan.DrillLateral = DrillOffset;
        DrillMan.DrillFinalLateral = (DrillPlaceLane - 1) * DrillRankSpacing;
        DrillMan.DrillLaneShift = 0.f;
        DrillMan.DrillDelta = (DrillFar - DrillPlaceRow * DrillFileSpacing) - DrillS0;
        if (DrillMan.DrillDelta <= 0.f) return DrillReject(TEXT("non-positive walk distance"));
        DrillMan.DrillStopped = false;
        DrillMan.DrillStopTime = -1.f;
        DrillMan.Slot = FVector(-DrillPlaceRank * DrillRankSpacing, (DrillPlaceFile - (DrillRows - 1) * 0.5f) * DrillFileSpacing, 0.f);
        DrillMan.SlotYaw = 0.f;
        DrillMan.ReformDeadline = -1.f;
        DrillMan.bActive = true;
        DrillMan.WorldVelocity = FVector::ZeroVector;
        DrillSlots.Add(DrillSlotId);
    }
    SoldierSettle = MoveTemp(DrillPlanned);
    SoldierSlots = MoveTemp(DrillSlots);
    DrillToColumn = DrillEntering;
    DrillArrival = DrillAtArrival;
    DrillOrder = DrillCurrentOrder;
    DrillOrderSerial = DrillCurrentOrder.OrderSerial;
    DrillStrength = OwnerCompany->CurrentStrength;
    DrillPlannedLateralSpacing = DrillFileSpacing;
    DrillPlannedRankSpacing = DrillRankSpacing;
    DrillFormation = uint8(DrillNew);
    DrillStarted = VisualAnimationTime;
    DrillLogAt = DrillStarted;
    DrillSpeed = FMath::Clamp(OwnerCompany->MovementExecutor->MoveSpeedCmPerSecond, 120.f, 160.f);
    DrillMarchDuration = 0.f;
    for (const FSettle& DrillMan : SoldierSettle) DrillMarchDuration = FMath::Max(DrillMarchDuration, DrillMan.DrillDelta / DrillSpeed);
    // Into line: the last man halts, brings his rifle down (ShoulderDown) and faces the front.
    DrillDuration = DrillMarchDuration + FMath::Max(0.9f, ShoulderDownAsset.Get() ? ShoulderDownAsset.Get()->GetPlayLength() : 0.f);
    DrillActive = true;
    OwnerCompany->MovementExecutor->ClearColumnDrillApproach();
    OwnerCompany->MovementExecutor->ClearColumnDrillArrival();
    bArrivalReform = false;
    bFormationPlanPending = true;
    FormationPlanStarted = DrillStarted;
    FormationLongestPath = DrillPath.Last().Distance;
    if (OwnerCompany->FormationTransition) OwnerCompany->FormationTransition->ExtendReformSeconds(DrillDuration + 0.25f);
    OwnerCompany->MovementExecutor->PauseMovementForSeconds(DrillDuration + 0.25f);
    // Always logged (once per change): the start of the drill, in the output log and the company's unit log.
    UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s start kind=%s flank=%s path-cm=%.1f expected-seconds=%.3f final=(%s yaw=%.2f)"),
        *OwnerCompany->StableUnitId.ToString(),
        DrillEntering ? TEXT("to-column") : DrillAtArrival ? TEXT("to-line-arrival") : TEXT("to-line-free"),
        DrillFlank > 0.f ? TEXT("right") : TEXT("left"), DrillPath.Last().Distance, DrillDuration,
        *DrillFinalFrame.GetLocation().ToString(), DrillFinalFrame.Rotator().Yaw);
    OwnerCompany->AddUnitLog(TEXT("Formation"),
        FString::Printf(TEXT("eksercits %s, %s fløj, ca. %.0f s"),
            DrillEntering ? TEXT("linje -> kolonne") : DrillAtArrival ? TEXT("kolonne -> linje ved målet") : TEXT("kolonne -> linje foran"),
            DrillFlank > 0.f ? TEXT("højre") : TEXT("venstre"), DrillDuration),
        FString::Printf(TEXT("fælles bue, %.0f m sti"), DrillPath.Last().Distance / 100.f), 0.0f);
    return true;
}

void UStrategyInfantryVisualComponent::AbortColumnDrill(const TCHAR* DrillWhy)
{
    if (!DrillActive) return;
    UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s abort after %.2f s: %s (falls back to the morph)"),
        OwnerCompany ? *OwnerCompany->StableUnitId.ToString() : TEXT("?"), VisualAnimationTime - DrillStarted, DrillWhy);
    if (OwnerCompany) OwnerCompany->AddUnitLog(TEXT("Formation"), TEXT("eksercits afbrudt"), DrillWhy, 0.0f);
    DrillActive = false;
    DrillPath.Reset();
    bArrivalReform = false;
    if (OwnerCompany && OwnerCompany->MovementExecutor) OwnerCompany->MovementExecutor->ClearColumnDrillApproach();
    // PlanOrderlyReform reads these actual world positions, never the abandoned final frame.
    PlanOrderlyReform(false, OwnerCompany->GetActorTransform());
    float DrillFallbackSeconds = 0.f;
    for (const FSettle& DrillMan : SoldierSettle) DrillFallbackSeconds = FMath::Max(DrillFallbackSeconds, DrillMan.ReformDuration);
    if (uint8(OwnerCompany->FormationComponent->CurrentFormation) != DrillFormation && OwnerCompany->FormationTransition)
        DrillFallbackSeconds = FMath::Max(DrillFallbackSeconds, OwnerCompany->FormationTransition->ReformRemainingSeconds);
    if (OwnerCompany->MovementExecutor) OwnerCompany->MovementExecutor->FinishFormationDrillHold(DrillFallbackSeconds);
}

void UStrategyInfantryVisualComponent::LogColumnDrillDistance(bool DrillComplete)
{
    // The completion is always logged; the running distance (four times a second) only with -Strategy1864DebugFormation.
    if (!DrillComplete && !FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugFormation"))) return;
    if (!DrillComplete && VisualAnimationTime < DrillLogAt) return;
    DrillLogAt = VisualAnimationTime + 0.25f;
    double DrillMinimum = TNumericLimits<double>::Max();
    for (int32 DrillA = 0; DrillA < SoldierComponents.Num(); ++DrillA)
    for (int32 DrillB = DrillA + 1; DrillB < SoldierComponents.Num(); ++DrillB)
        if (SoldierComponents[DrillA] && SoldierComponents[DrillB])
            DrillMinimum = FMath::Min(DrillMinimum, FVector::Dist2D(SoldierComponents[DrillA]->GetComponentLocation(), SoldierComponents[DrillB]->GetComponentLocation()));
    UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s %s elapsed=%.3f min-2d-cm=%.3f"),
        *OwnerCompany->StableUnitId.ToString(), DrillComplete ? TEXT("complete") : TEXT("distance"), VisualAnimationTime - DrillStarted, DrillMinimum);
    if (DrillComplete)
        OwnerCompany->AddUnitLog(TEXT("Formation"), FString::Printf(TEXT("eksercits færdig efter %.1f s"), VisualAnimationTime - DrillStarted),
            FString::Printf(TEXT("mindste afstand mellem to mænd %.0f cm"), DrillMinimum), 0.0f);
}

bool UStrategyInfantryVisualComponent::UpdateColumnDrill(float DrillDt)
{
    if (!DrillActive) return false;
    const FStrategyOrder DrillCurrent = OwnerCompany->OrderComponent ? OwnerCompany->OrderComponent->GetCurrentOrder() : FStrategyOrder();
    // At the destination the finished march order is replaced by a hold: that does not stop the deployment, a new march does.
    const bool DrillOrderMatters = !DrillArrival || OwnerCompany->MovementExecutor->HasMovementGoal();
    // The same order given again (an officer re-issues "hold here" with a new serial) is no new order: the drill goes on
    // (2026-10-22 test: a drill into line was aborted after 0.15 s by exactly that, and the men ran 50 m in 9 s).
    if (DrillCurrent.OrderSerial != DrillOrderSerial && DrillCurrent.Type == DrillOrder.Type &&
        DrillCurrent.TargetLocation.Equals(DrillOrder.TargetLocation, 0.1f) && DrillCurrent.bHasFacing == DrillOrder.bHasFacing &&
        DrillCurrent.bKeepFacing == DrillOrder.bKeepFacing && DrillCurrent.NextWaypointIndex == DrillOrder.NextWaypointIndex &&
        DrillCurrent.Waypoints == DrillOrder.Waypoints && FMath::Abs(FMath::FindDeltaAngleDegrees(DrillCurrent.FacingYaw, DrillOrder.FacingYaw)) <= 0.1f)
    {
        DrillOrderSerial = DrillCurrent.OrderSerial;
        DrillOrder = DrillCurrent;
    }
    if ((DrillOrderMatters && (DrillCurrent.OrderSerial != DrillOrderSerial || DrillCurrent.Type != DrillOrder.Type ||
        !DrillCurrent.TargetLocation.Equals(DrillOrder.TargetLocation, 0.1f) || DrillCurrent.bHasFacing != DrillOrder.bHasFacing ||
        DrillCurrent.bKeepFacing != DrillOrder.bKeepFacing || DrillCurrent.NextWaypointIndex != DrillOrder.NextWaypointIndex ||
        DrillCurrent.Waypoints != DrillOrder.Waypoints ||
        FMath::Abs(FMath::FindDeltaAngleDegrees(DrillCurrent.FacingYaw, DrillOrder.FacingYaw)) > 0.1f)) ||
        OwnerCompany->CurrentStrength != DrillStrength || SoldierSettle.Num() != SoldierComponents.Num() ||
        uint8(OwnerCompany->FormationComponent->CurrentFormation) != DrillFormation ||
        OwnerCompany->FormationComponent->RankCount != 3 || OwnerCompany->FormationComponent->ColumnWidth != 3 ||
        OwnerCompany->FormationComponent->SoldierLateralSpacingCm != DrillPlannedLateralSpacing ||
        OwnerCompany->FormationComponent->SoldierRankSpacingCm != DrillPlannedRankSpacing ||
        OwnerCompany->MovementExecutor->IsHoldingForFire() ||
        (OwnerCompany->StanceComponent && OwnerCompany->StanceComponent->Stance == EStrategyStance::Prone) ||
        (OwnerCompany->SkirmisherComponent && (OwnerCompany->SkirmisherComponent->IsDeployed() ||
            OwnerCompany->SkirmisherComponent->State == EStrategySkirmisherState::Deploying)))
    {
        const bool DrillOrderChanged = DrillOrderMatters && (DrillCurrent.OrderSerial != DrillOrderSerial || DrillCurrent.Type != DrillOrder.Type ||
            !DrillCurrent.TargetLocation.Equals(DrillOrder.TargetLocation, 0.1f) || DrillCurrent.Waypoints != DrillOrder.Waypoints ||
            DrillCurrent.NextWaypointIndex != DrillOrder.NextWaypointIndex);
        if (DrillOrderChanged)
            UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s order changed under the drill: serial %d->%d type %d->%d target %s->%s authority %d"),
                *OwnerCompany->StableUnitId.ToString(), DrillOrderSerial, DrillCurrent.OrderSerial, int32(DrillOrder.Type), int32(DrillCurrent.Type),
                *DrillOrder.TargetLocation.ToString(), *DrillCurrent.TargetLocation.ToString(), int32(DrillCurrent.Authority));
        AbortColumnDrill(DrillOrderChanged ? TEXT("ny eller ændret ordre") :
            OwnerCompany->CurrentStrength != DrillStrength ? TEXT("tab") :
            uint8(OwnerCompany->FormationComponent->CurrentFormation) != DrillFormation ? TEXT("nyt formationsskifte") :
            OwnerCompany->MovementExecutor->IsHoldingForFire() ? TEXT("holder for at skyde") : TEXT("stilling, skærm eller ændret front"));
        return false;
    }
    const FTransform DrillUnit = OwnerCompany->GetActorTransform();
    const float DrillElapsed = VisualAnimationTime - DrillStarted;
    const float DrillTravel = FMath::Max(0.f, DrillElapsed - (DrillToColumn ? 0.9f : 0.f)) * DrillSpeed;
    bool DrillFacingComplete = true;
    for (int32 DrillIndex = 0; DrillIndex < SoldierComponents.Num(); ++DrillIndex)
    {
        USkeletalMeshComponent* DrillSoldier = SoldierComponents[DrillIndex];
        if (!DrillSoldier) { AbortColumnDrill(TEXT("en figur mangler")); return false; }
        FSettle& DrillMan = SoldierSettle[DrillIndex];
        const float DrillS = DrillMan.DrillS0 + FMath::Min(DrillTravel, DrillMan.DrillDelta);
        const float DrillSpacingAlpha = FMath::Clamp((DrillS - DrillStraightA) /
            FMath::Max(300.f, DrillArcEnd - DrillStraightA), 0.f, 1.f);
        const float DrillInitialSpacing = DrillToColumn ? DrillPlannedRankSpacing : DrillPlannedLateralSpacing;
        const float DrillFinalSpacing = DrillToColumn ? DrillPlannedLateralSpacing : DrillPlannedRankSpacing;
        const float DrillLaneSpacing = FMath::Lerp(DrillInitialSpacing, DrillFinalSpacing, DrillSpacingAlpha);
        // Finish a short rear group's lane shift before the first full lane starts
        // (s0 = file/rank spacing). Full and short curves can merge, never cross.
        const float DrillTailAlpha = FMath::Clamp(DrillS / (0.75f * DrillFinalSpacing), 0.f, 1.f);
        const float DrillLateral = FMath::Lerp(DrillMan.DrillLateral, DrillMan.DrillFinalLateral, DrillSpacingAlpha) +
            DrillMan.DrillLaneShift * DrillLaneSpacing * DrillTailAlpha;
        float DrillYaw = 0.f;
        const FVector DrillWorld = FStrategyColumnDrill::Sample(DrillPath, DrillS, DrillLateral, DrillYaw);
        // The man's own ground speed this frame (inner/outer lane, lane widening): it drives his stride, so feet do not slide.
        const FVector DrillWasWorld = VisualPath.PreviousUnit.TransformPosition(DrillSoldier->GetRelativeLocation());
        const float DrillGroundSpeed = DrillDt > 0.f ? float(FVector::Dist2D(DrillWasWorld, DrillWorld)) / DrillDt : 0.f;
        const bool DrillMoving = DrillGroundSpeed > 8.f;
        const bool DrillWasStopped = DrillMan.DrillStopped;
        DrillMan.DrillStopped = DrillTravel >= DrillMan.DrillDelta;
        if (DrillMan.DrillStopped && !DrillWasStopped) DrillMan.DrillStopTime = VisualAnimationTime;
        if (!DrillToColumn && DrillElapsed >= DrillMarchDuration) DrillYaw = DrillFinalFrame.Rotator().Yaw;
        FRotator DrillRotation = DrillSoldier->GetRelativeRotation();
        DrillRotation.Yaw += FMath::FindDeltaAngleDegrees(DrillUnit.Rotator().Yaw, VisualPath.PreviousUnit.Rotator().Yaw);
        DrillRotation.Yaw = FStrategyVisualFormationPath::SmoothFacing(DrillRotation.Yaw, DrillMan.YawVelocity,
            DrillYaw - DrillUnit.Rotator().Yaw + SoldierMeshYawOffset, DrillDt, 120.f);
        DrillRotation.Roll = 0.f;
        DrillSoldier->SetRelativeLocation(DrillUnit.InverseTransformPosition(DrillWorld));
        DrillSoldier->SetRelativeRotation(DrillRotation);
        DrillFacingComplete &= FMath::Abs(FMath::FindDeltaAngleDegrees(DrillRotation.Yaw,
            DrillYaw - DrillUnit.Rotator().Yaw + SoldierMeshYawOffset)) < 0.1f;
        // Into line, the column keeps the rifle on the shoulder while it files along the axis; each man brings it down to the
        // ready stance (ShoulderDown, standing) when he halts on his place.
        UAnimSequence* DrillDownClip = ShoulderDownAsset.Get();
        const bool DrillLowering = !DrillToColumn && !DrillMoving && DrillMan.DrillStopped && DrillDownClip &&
            VisualAnimationTime - DrillMan.DrillStopTime < DrillDownClip->GetPlayLength();
        UAnimSequence* DrillClip = DrillMoving ? ((DrillToColumn ? (bShoulderColumn ? WalkShoulderAsset : WalkStandingAsset) :
            (WalkShoulderAsset.Get() ? WalkShoulderAsset : WalkStandingAsset)).Get()) :
            (DrillLowering ? DrillDownClip : IdleStandingAsset.Get());
        if (!DrillClip && DrillMoving) DrillClip = WalkStandingAsset.Get();
        if (DrillLowering && SoldierClips.IsValidIndex(DrillIndex))
        {
            const float DrillDownAt = FMath::Clamp(VisualAnimationTime - DrillMan.DrillStopTime, 0.f, DrillDownClip->GetPlayLength());
            if (SoldierClips[DrillIndex].Clip.Get() != DrillDownClip)
            {
                DrillSoldier->PlayAnimation(DrillDownClip, false);
                DrillSoldier->SetPosition(DrillDownAt, false);
                DrillSoldier->SetPlayRate(1.f);
                RecordClip(DrillSoldier, DrillDownClip, false, DrillDownAt, 1.f);
            }
            DrillSoldier->bPauseAnims = false;
            continue;
        }
        if (DrillClip && SoldierClips.IsValidIndex(DrillIndex))
        {
            const float DrillClipSpeed = DrillClip == WalkShoulderAsset.Get() ? 137.70097f : 76.99816f;
            const float DrillRate = DrillMoving ? FMath::Clamp(DrillGroundSpeed / DrillClipSpeed, 0.2f, 3.f) : (bAnimateIdle ? 1.f : 0.f);
            const FPlayedClip& DrillPlayed = SoldierClips[DrillIndex];
            const float DrillPlayedLength = DrillPlayed.Clip.IsValid() ? DrillPlayed.Clip->GetPlayLength() : 0.f;
            // Keep the stride phase on a rate change (no restart on the same foot for everybody); start at the man's own phase.
            const float DrillPhase = DrillPlayed.Clip.Get() == DrillClip && DrillPlayedLength > 0.f ?
                FMath::Fmod(FMath::Max(0.f, (VisualAnimationTime - DrillPlayed.Start) * DrillPlayed.Rate), DrillPlayedLength) :
                DrillMan.WalkPhase * DrillClip->GetPlayLength();
            if (DrillPlayed.Clip.Get() != DrillClip)
            {
                DrillSoldier->PlayAnimation(DrillClip, true);
                DrillSoldier->SetPosition(DrillPhase, false);
                DrillSoldier->SetPlayRate(DrillRate);
                RecordClip(DrillSoldier, DrillClip, true, DrillPhase, DrillRate);
            }
            else if (FMath::Abs(DrillPlayed.Rate - DrillRate) > 0.02f)
            {
                DrillSoldier->SetPlayRate(DrillRate);
                RecordClip(DrillSoldier, DrillClip, true, DrillPhase, DrillRate);
            }
            DrillSoldier->bPauseAnims = DrillRate == 0.f;
        }
    }
    bCrowdDirty = true;
    bCrowdDataDirty = true;
    VisualPath.PreviousUnit = DrillUnit;
    VisualPath.bMovingVisuals = false;
    LogColumnDrillDistance(false);
    if (DrillElapsed >= DrillDuration && DrillFacingComplete)
    {
        LogColumnDrillDistance(true);
        // Serial and order were checked above, immediately before the only simulation relocation.
        OwnerCompany->MovementExecutor->RelocateAfterFormationDrill(DrillFinalFrame);
        OwnerCompany->MovementExecutor->FinishFormationDrillHold(0.f);
        if (!DrillToColumn) OwnerCompany->MovementExecutor->ClearColumnDrillApproach();
        VisualPath.Reset(OwnerCompany->GetActorTransform());
        VisualPath.SlotBounds = FBox(ForceInit);
        for (int32 DrillIndex = 0; DrillIndex < SoldierComponents.Num(); ++DrillIndex)
        {
            FSettle& DrillMan = SoldierSettle[DrillIndex];
            DrillMan.Goal = DrillMan.Slot;
            DrillMan.bActive = false;
            DrillMan.ReformDeadline = -1.f;
            DrillMan.WorldVelocity = FVector::ZeroVector;
            SoldierComponents[DrillIndex]->SetRelativeLocation(DrillMan.Slot);
            SoldierComponents[DrillIndex]->SetRelativeRotation(FRotator(0.f, SoldierMeshYawOffset, 0.f));
            VisualPath.SlotBounds += DrillMan.Slot;
        }
        DrillActive = false;
        DrillPath.Reset();
        bFormationPlanPending = false;
        bWasVisualTravel = false;
        // A deployment during a march (the line button): the march goes on from the line's new place, on a fresh route.
        if (OwnerCompany->MovementExecutor->HasMovementGoal()) OwnerCompany->MovementExecutor->RestartCurrentOrderExecution();
    }
    else if (DrillElapsed > DrillDuration - 0.25f)
    {
        // Retain the hold while the bounded-rate facing spring finishes after a hitch.
        OwnerCompany->MovementExecutor->PauseMovementForSeconds(0.5f);
        if (OwnerCompany->FormationTransition) OwnerCompany->FormationTransition->ExtendReformSeconds(0.5f);
    }
    return true;
}


bool UStrategyInfantryVisualComponent::SeedColumnSnake(const FVector& SnakeFront, const FVector& SnakeAxis, float SnakeFlank,
    float SnakeWidth, int32 SnakeRows)
{
    // Line -> column (reference drill): every man faces along the line on the spot, then the column files off from the
    // leading flank. The trail is seeded along the middle rank, so each man already stands on it with his rank as a lane;
    // the head starts at the leading flank and the men follow it as a snake. No hold beyond the turn, no path of its own.
    const float SnakeLateral = OwnerCompany->FormationComponent->SoldierLateralSpacingCm;
    const float SnakeRank = OwnerCompany->FormationComponent->SoldierRankSpacingCm;
    const FVector SnakeFar = VisualPath.Center - SnakeFront * SnakeRank - SnakeAxis * (SnakeWidth * 0.5f);
    const FVector SnakeLead = SnakeFar + SnakeAxis * SnakeWidth;
    const FVector SnakeSide = FStrategyColumnDrill::Right(SnakeAxis);
    TArray<FSettle> SnakePlanned = SoldierSettle;
    TArray<int32> SnakeSlots;
    TSet<int32> SnakeUsed;
    FBox SnakeBounds(ForceInit);
    for (int32 SnakeIndex = 0; SnakeIndex < SoldierComponents.Num(); ++SnakeIndex)
    {
        if (!SoldierComponents[SnakeIndex] || !SoldierSettle[SnakeIndex].bPlaced)
        {
            UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill snake seed: man %d not placed"), SnakeIndex);
            return false;
        }
        const FVector SnakeWorld = VisualPath.PreviousUnit.TransformPosition(SoldierComponents[SnakeIndex]->GetRelativeLocation());
        const float SnakeAlong = FVector::DotProduct(SnakeWorld - SnakeFar, SnakeAxis);
        const float SnakeAcross = FVector::DotProduct(SnakeWorld - SnakeFar, SnakeSide);
        const int32 SnakeFileFromFar = FMath::RoundToInt(SnakeAlong / SnakeLateral);
        int32 SnakeLane = FMath::RoundToInt(SnakeAcross / SnakeRank) + 1;
        // A man off the grid (the odd man of the remainder, or one displaced by the colour bearer leaving) takes the nearest
        // free place instead of cancelling the drill.
        int32 SnakeRow = SnakeRows - 1 - FMath::Clamp(SnakeFileFromFar, 0, SnakeRows - 1);   // row 0 is the leading flank's file, the head
        SnakeLane = FMath::Clamp(SnakeLane, 0, 2);
        int32 SnakeId = SnakeRow * 3 + SnakeLane;
        if (SnakeUsed.Contains(SnakeId))
        {
            int32 SnakeBest = INDEX_NONE;
            float SnakeBestCost = TNumericLimits<float>::Max();
            for (int32 SnakeTry = 0; SnakeTry < SnakeRows * 3; ++SnakeTry)
            {
                if (SnakeUsed.Contains(SnakeTry)) continue;
                const float SnakeCost = FMath::Abs(SnakeTry / 3 - SnakeRow) * SnakeLateral + FMath::Abs(SnakeTry % 3 - SnakeLane) * SnakeRank;
                if (SnakeCost < SnakeBestCost) { SnakeBestCost = SnakeCost; SnakeBest = SnakeTry; }
            }
            if (SnakeBest == INDEX_NONE)
            {
                UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill snake seed: no free place for man %d"), SnakeIndex);
                return false;
            }
            SnakeId = SnakeBest;
            SnakeRow = SnakeId / 3;
            SnakeLane = SnakeId % 3;
        }
        SnakeUsed.Add(SnakeId);
        FSettle& SnakeMan = SnakePlanned[SnakeIndex];
        SnakeMan.SnakeRow = SnakeRow;
        SnakeMan.SnakeLane = SnakeLane;
        // Starting slot = where he stands (file spacing along the trail, rank spacing across); UpdateSnakeRamp opens it
        // to the column's row/lane spacing as the head marches off.
        SnakeMan.Slot = FVector(-SnakeRow * SnakeLateral, (SnakeLane - 1) * SnakeRank, 0.f);
        SnakeMan.SlotYaw = 0.f;
        SnakeMan.ReformDeadline = -1.f;
        SnakeMan.bActive = true;
        SnakeMan.WorldVelocity = FVector::ZeroVector;
        SnakeBounds += SnakeMan.Slot;
        SnakeSlots.Add(SnakeId);
    }
    SoldierSettle = MoveTemp(SnakePlanned);
    SoldierSlots = MoveTemp(SnakeSlots);
    VisualPath.SlotBounds = SnakeBounds;
    const float SnakeYaw = SnakeAxis.Rotation().Yaw;
    VisualPath.Trail.Reset();
    VisualPath.Trail.Add({SnakeFar - SnakeAxis * 1000.f, SnakeYaw, -1000.f});
    VisualPath.Trail.Add({SnakeFar, SnakeYaw, 0.f});
    VisualPath.Trail.Add({SnakeLead, SnakeYaw, SnakeWidth});
    VisualPath.TrailDistance = SnakeWidth;
    VisualPath.HeadPos = SnakeLead;
    VisualPath.HeadYaw = SnakeYaw;
    VisualPath.bHeadLocked = false;   // the head curves off the flank first, then locks on the actor's track
    VisualPath.Facing = SnakeYaw;
    // Right face and shoulder arms on the spot (ShoulderUp: from the line's ready stance to the shoulder), then march off.
    const float SnakeShoulderSeconds = ShoulderUpAsset.Get() ? ShoulderUpAsset.Get()->GetPlayLength() : 0.f;
    const float SnakeHoldSeconds = FMath::Max(0.9f, SnakeShoulderSeconds);
    SnakeHoldStart = VisualAnimationTime;
    SnakeHoldUntil = VisualAnimationTime + SnakeHoldSeconds;
    // The shoulder transition is done standing in the hold; the march starts with the rifle already on the shoulder.
    ShoulderTransitionEnd = GetWorld()->GetTimeSeconds() + SnakeHoldSeconds;
    SnakeRowSpacing = SnakeLateral;
    bSnakeRamp = true;
    SnakeRampFrom = SnakeWidth;
    bArrivalReform = false;
    bFormationPlanPending = false;
    // In a column the actor is the head (generated column slots are head-anchored): it moves to the leading flank's file
    // now, and the march is re-routed from there after the turn on the spot.
    UStrategyMovementExecutorComponent* SnakeMove = OwnerCompany->MovementExecutor;
    SnakeMove->RelocateAfterFormationDrill(FTransform(FRotator(0.f, SnakeYaw, 0.f), SnakeLead));
    if (SnakeMove->HasMovementGoal()) SnakeMove->RestartCurrentOrderExecution();
    SnakeMove->FinishFormationDrillHold(SnakeHoldSeconds);
    SnakeMove->PauseMovementForSeconds(SnakeHoldSeconds);
    if (OwnerCompany->FormationTransition) OwnerCompany->FormationTransition->ReformRemainingSeconds =
        FMath::Min(OwnerCompany->FormationTransition->ReformRemainingSeconds, 1.f);
    UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s start kind=to-column-snake flank=%s rows=%d head=%s yaw=%.1f goal=%s"),
        *OwnerCompany->StableUnitId.ToString(), SnakeFlank > 0.f ? TEXT("right") : TEXT("left"), SnakeRows,
        *SnakeLead.ToString(), SnakeYaw, *SnakeMove->GetMovementGoal().ToString());
    OwnerCompany->AddUnitLog(TEXT("Formation"),
        FString::Printf(TEXT("eksercits linje -> kolonne, %s fløj fører"), SnakeFlank > 0.f ? TEXT("højre") : TEXT("venstre")),
        TEXT("vender på stedet og går i fil efter teten"), 0.0f);
    return true;
}

void UStrategyInfantryVisualComponent::UpdateSnakeRamp()
{
    if (!bSnakeRamp || !OwnerCompany || !OwnerCompany->FormationComponent) return;
    // Over the first four metres the head walks, the lanes close from rank to file spacing. The rows keep the line's file
    // spacing along the trail: opening them while marching off would pull the tail backwards (review 2026-10-18: a global
    // 75 -> 90 cm ramp moved row 63 of 190 men 9.5 m back during 4 m of head travel).
    const float SnakeLateral = OwnerCompany->FormationComponent->SoldierLateralSpacingCm;
    const float SnakeRank = OwnerCompany->FormationComponent->SoldierRankSpacingCm;
    const float SnakeAlpha = FMath::Clamp((VisualPath.TrailDistance - SnakeRampFrom) / 400.f, 0.f, 1.f);
    for (FSettle& SnakeMan : SoldierSettle)
        SnakeMan.Slot = FVector(-SnakeMan.SnakeRow * SnakeLateral,
            (SnakeMan.SnakeLane - 1) * FMath::Lerp(SnakeRank, SnakeLateral, SnakeAlpha), 0.f);
    if (SnakeAlpha >= 1.f) bSnakeRamp = false;
}

bool UStrategyInfantryVisualComponent::CloseRanksTowardColours()
{
    // "Slut op mod fanen" (user's recording 2026-10-18: the whole line used to turn sideways and walk in at once). After the
    // rear men have stepped up, the line is still as wide as before but no longer 3 deep. It now closes to the centre: the
    // line becomes ceil(men / 3) files wide, every rank keeps its men in their left-right order, the front ranks' surplus
    // flank men fall back one rank, and everyone side-steps inwards with the front kept, slowly.
    if (!OwnerCompany || !OwnerCompany->FormationComponent || !bRigidCompany || !SideStepAsset.Get() ||
        OwnerCompany->FormationComponent->CurrentFormation != EStrategyFormationType::Line ||
        (OwnerCompany->StanceComponent && OwnerCompany->StanceComponent->Stance == EStrategyStance::Prone) ||
        SoldierSettle.Num() != SoldierComponents.Num() || SoldierSettle.IsEmpty()) return false;
    const float CloseLateral = FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierLateralSpacingCm);
    const float CloseRankSpacing = FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierRankSpacingCm);
    const int32 CloseRankCount = FMath::Max(1, OwnerCompany->FormationComponent->RankCount);
    TArray<TArray<int32>> CloseRanks;
    CloseRanks.SetNum(CloseRankCount);
    // Where every man is going (a step-up still under way counts at its end), not where he passes.
    const auto ClosePlace = [this](int32 ClosePlaceMan) { const FSettle& ClosePlaceSettle = SoldierSettle[ClosePlaceMan]; return ClosePlaceSettle.CloseStart >= 0.f ? ClosePlaceSettle.CloseTo : ClosePlaceSettle.Slot; };
    float CloseFrontX = -TNumericLimits<float>::Max();
    for (int32 CloseFrontIndex = 0; CloseFrontIndex < SoldierSettle.Num(); ++CloseFrontIndex) CloseFrontX = FMath::Max(CloseFrontX, float(ClosePlace(CloseFrontIndex).X));
    for (int32 CloseIndex = 0; CloseIndex < SoldierSettle.Num(); ++CloseIndex)
    {
        const FSettle& CloseMan = SoldierSettle[CloseIndex];
        if (!CloseMan.bPlaced) return false;
        const int32 CloseRank = FMath::Clamp(FMath::RoundToInt((CloseFrontX - ClosePlace(CloseIndex).X) / CloseRankSpacing), 0, CloseRankCount - 1);
        CloseRanks[CloseRank].Add(CloseIndex);
    }
    const int32 CloseFiles = FMath::DivideAndRoundUp(SoldierSettle.Num(), CloseRankCount);
    auto CloseByY = [&ClosePlace](int32 CloseA, int32 CloseB) { return ClosePlace(CloseA).Y < ClosePlace(CloseB).Y; };
    // Front to rear: a rank with more men than the new width sends its outermost men (alternately right and left) back.
    for (int32 CloseRank = 0; CloseRank < CloseRankCount; ++CloseRank)
    {
        CloseRanks[CloseRank].Sort(CloseByY);
        const int32 CloseWant = CloseRank < CloseRankCount - 1 ? CloseFiles :
            SoldierSettle.Num() - CloseFiles * (CloseRankCount - 1);
        bool bCloseRightEnd = true;
        while (CloseRank < CloseRankCount - 1 && CloseRanks[CloseRank].Num() > CloseWant)
        {
            const int32 CloseSurplus = bCloseRightEnd ? CloseRanks[CloseRank].Pop() : CloseRanks[CloseRank][0];
            if (!bCloseRightEnd) CloseRanks[CloseRank].RemoveAt(0);
            CloseRanks[CloseRank + 1].Add(CloseSurplus);
            bCloseRightEnd = !bCloseRightEnd;
        }
    }
    const float CloseSpeed = 35.f;   // cm/s: a deliberate side-step, about 1.35x the clip's own pace
    const float CloseNow = VisualAnimationTime;
    int32 CloseMovers = 0;
    for (int32 CloseRank = 0; CloseRank < CloseRankCount; ++CloseRank)
    {
        TArray<int32>& CloseRow = CloseRanks[CloseRank];
        CloseRow.Sort(CloseByY);
        for (int32 CloseOrder = 0; CloseOrder < CloseRow.Num(); ++CloseOrder)
        {
            FSettle& CloseMan = SoldierSettle[CloseRow[CloseOrder]];
            // Centred on the colours; a short rear rank stands in the middle, on the front rank's file grid (centred on its own count
            // it stood half a file off when the counts differed in odd/even, and the next step-up read two files as one and sent the
            // fourth to sixth man of each behind the third rank: the 'deep block' of the user's video at 3:00, 2026-10-22).
            const int32 CloseGridStart = (CloseFiles - CloseRow.Num()) / 2;
            const FVector CloseTo(CloseFrontX - CloseRank * CloseRankSpacing, (CloseGridStart + CloseOrder - (CloseFiles - 1) * 0.5f) * CloseLateral, 0.f);
            const float CloseDistance = FVector::Dist2D(CloseMan.Slot, CloseTo);
            if (CloseDistance < 5.f) { CloseMan.CloseStart = -1.f; CloseMan.Slot = CloseTo; continue; }
            CloseMan.CloseFrom = CloseMan.Slot;
            CloseMan.CloseTo = CloseTo;
            // The flanks have furthest to go and start at once; men nearer the centre follow a moment later (a ripple inwards).
            CloseMan.CloseStart = CloseNow + FMath::Clamp(0.5f - CloseDistance / 400.f, 0.f, 0.5f) + FMath::FRandRange(0.f, 0.25f);
            CloseMan.CloseDuration = FMath::Max(0.6f, CloseDistance / CloseSpeed);
            CloseMan.bActive = true;
            ++CloseMovers;
        }
    }
    bClosingRanksStep = CloseMovers > 0;
    UE_LOG(LogTemp, Display, TEXT("Strategy1864Close %s close ranks towards the colours: men=%d files=%d movers=%d"),
        *OwnerCompany->StableUnitId.ToString(), SoldierSettle.Num(), CloseFiles, CloseMovers);
    if (CloseMovers > 0) OwnerCompany->AddUnitLog(TEXT("Formation"), TEXT("slutter op mod fanen"),
        FString::Printf(TEXT("%d mand rykker ind sidelæns; linjen er nu %d filer"), CloseMovers, CloseFiles), 0.0f);
    return true;
}

void UStrategyInfantryVisualComponent::UpdateCloseRanks()
{
    bool bCloseStill = false;
    // A march column keeps its front where it was while men step up into a hole in the front row: the snake's head is set from the
    // front, and a front that slid back folded the trail and stacked men of different rows on one spot (2026-10-22 test).
    const bool bCloseColumn = OwnerCompany && OwnerCompany->FormationComponent &&
        OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn && VisualPath.SlotBounds.IsValid;
    const float CloseColumnFront = bCloseColumn ? VisualPath.SlotBounds.Max.X : 0.f;
    VisualPath.SlotBounds = FBox(ForceInit);
    for (FSettle& CloseMan : SoldierSettle)
    {
        if (CloseMan.CloseStart >= 0.f)
        {
            const float CloseAlpha = FMath::Clamp((VisualAnimationTime - CloseMan.CloseStart) / FMath::Max(0.01f, CloseMan.CloseDuration), 0.f, 1.f);
            CloseMan.Slot = FMath::Lerp(CloseMan.CloseFrom, CloseMan.CloseTo, CloseAlpha);
            CloseMan.bActive = true;
            if (CloseAlpha >= 1.f) CloseMan.CloseStart = -1.f;
            else bCloseStill = true;
        }
        if (CloseMan.bPlaced) VisualPath.SlotBounds += CloseMan.Slot;
    }
    if (bCloseColumn && VisualPath.SlotBounds.IsValid && VisualPath.SlotBounds.Max.X < CloseColumnFront) VisualPath.SlotBounds.Max.X = CloseColumnFront;
    bClosingRanksStep = bCloseStill;
}

FVector UStrategyInfantryVisualComponent::ColourBearerTarget(float& OutBearerYaw) const
{
    // In front: one rank ahead of the front rank in line, one row ahead of the head in column (he leads the snake).
    const float BearerRank = OwnerCompany && OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierRankSpacingCm : 90.f;
    const FTransform BearerUnit = OwnerCompany->GetActorTransform();
    OutBearerYaw = BearerUnit.Rotator().Yaw;
    // Line (user 2026-10-20): beside the right flank, level with the front rank, one file out (the officer will stand one further out).
    const float BearerLateral = OwnerCompany && OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierLateralSpacingCm : 60.f;
    if (DrillActive && !DrillToColumn)
    {
        // Column -> line: he keeps one row ahead of the head man (the right-flank man of the front rank in the line to be) until that
        // man is nearly in his place, then takes his own place beside him (2026-10-21 recording: he walked straight to the end
        // spot, 20-30 m ahead of the curving column).
        const float BearerFrontX = VisualPath.SlotBounds.IsValid ? VisualPath.SlotBounds.Max.X : 0.f;
        int32 BearerHeadMan = INDEX_NONE;
        float BearerHeadY = -TNumericLimits<float>::Max();
        for (int32 BearerSlotIndex = 0; BearerSlotIndex < SoldierSettle.Num(); ++BearerSlotIndex)
        {
            const FSettle& BearerSlotMan = SoldierSettle[BearerSlotIndex];
            if (!BearerSlotMan.bPlaced || BearerSlotMan.Slot.X < BearerFrontX - BearerRank * 0.5f) continue;
            if (BearerSlotMan.Slot.Y > BearerHeadY) { BearerHeadY = BearerSlotMan.Slot.Y; BearerHeadMan = BearerSlotIndex; }
        }
        if (SoldierComponents.IsValidIndex(BearerHeadMan) && SoldierComponents[BearerHeadMan])
        {
            const FVector BearerHeadAt = SoldierComponents[BearerHeadMan]->GetComponentLocation();
            const FVector BearerHeadPlace = DrillFinalFrame.TransformPosition(SoldierSettle[BearerHeadMan].Slot);
            if (FVector::Dist2D(BearerHeadAt, BearerHeadPlace) > 3.f * BearerRank)
            {
                const float BearerHeadYaw = SoldierComponents[BearerHeadMan]->GetComponentRotation().Yaw - SoldierMeshYawOffset;
                OutBearerYaw = BearerHeadYaw;
                const float BearerLead = ColourColumnLeadCm;
                return BearerHeadAt + FRotator(0.f, BearerHeadYaw, 0.f).Vector() * BearerLead;
            }
        }
        OutBearerYaw = DrillFinalFrame.Rotator().Yaw;
        const float BearerFinalFront = VisualPath.SlotBounds.IsValid ? VisualPath.SlotBounds.Max.X : 0.f;
        const float BearerFinalRight = VisualPath.SlotBounds.IsValid ? VisualPath.SlotBounds.Max.Y : 0.f;
        return DrillFinalFrame.TransformPosition(FVector(BearerFinalFront, BearerFinalRight + BearerLateral, 0.f));
    }
    // Line -> column (the snake seeded, every man with his row): one row ahead of the front row where its men really walk, also
    // before the column has any trail (2026-10-22 test: on KOLONNE while standing he went 20 m away from the nearest man).
    if (OwnerCompany->FormationComponent && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn)
    {
        bool bBearerSeeded = false;
        for (const FSettle& BearerSeedMan : SoldierSettle) bBearerSeeded |= BearerSeedMan.SnakeRow > 0;
        FVector BearerFrontSum = FVector::ZeroVector;
        int32 BearerFrontMen = 0;
        float BearerFrontYaw = 0.f;
        for (int32 BearerFrontIndex = 0; bBearerSeeded && BearerFrontIndex < SoldierSettle.Num() && BearerFrontIndex < SoldierComponents.Num(); ++BearerFrontIndex)
        {
            if (SoldierSettle[BearerFrontIndex].SnakeRow != 0 || !SoldierComponents[BearerFrontIndex]) continue;
            BearerFrontSum += SoldierComponents[BearerFrontIndex]->GetComponentLocation();
            BearerFrontYaw = SoldierComponents[BearerFrontIndex]->GetComponentRotation().Yaw - SoldierMeshYawOffset;
            ++BearerFrontMen;
        }
        const float BearerFrontAhead = ColourColumnLeadCm;
        if (BearerFrontMen > 0 && (VisualPath.Trail.Num() < 2 || FVector::Dist2D(BearerFrontSum / BearerFrontMen, VisualPath.HeadPos) > 2.f * BearerFrontAhead))
        {
            OutBearerYaw = BearerFrontYaw;
            return BearerFrontSum / BearerFrontMen + FRotator(0.f, BearerFrontYaw, 0.f).Vector() * BearerFrontAhead;
        }
    }
    // In column he leads the visible snake (its head), not the unit's logical position, which marches on ahead of the drawn column
    // (2026-10-21 recording: the bearer walked 30-40 m ahead of the men). The snake is active while HeadSpeedCap > 0.
    const bool BearerColumn = VisualPath.Trail.Num() >= 2 && (VisualPath.HeadSpeedCap > 0.f || (DrillActive && DrillToColumn) ||
        (OwnerCompany->FormationComponent && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn));
    if (BearerColumn)
    {
        OutBearerYaw = VisualPath.HeadYaw;
        const float BearerAhead = ColourColumnLeadCm;
        return VisualPath.HeadPos + FRotator(0.f, VisualPath.HeadYaw, 0.f).Vector() * BearerAhead;
    }
    const float BearerFront = VisualPath.SlotBounds.IsValid ? VisualPath.SlotBounds.Max.X : 0.f;
    const float BearerRight = VisualPath.SlotBounds.IsValid ? VisualPath.SlotBounds.Max.Y : 0.f;
    // Beside the man who actually stands on the right of the front rank (where the men are drawn, also just after a re-form when
    // the unit frame and the drawn line can differ for a moment).
    int32 BearerFlankMan = INDEX_NONE;
    float BearerFlankY = -TNumericLimits<float>::Max();
    for (int32 BearerLineIndex = 0; BearerLineIndex < SoldierSettle.Num(); ++BearerLineIndex)
    {
        const FSettle& BearerLineMan = SoldierSettle[BearerLineIndex];
        if (!BearerLineMan.bPlaced || BearerLineMan.Slot.X < BearerFront - BearerRank * 0.5f) continue;
        if (BearerLineMan.Slot.Y > BearerFlankY) { BearerFlankY = BearerLineMan.Slot.Y; BearerFlankMan = BearerLineIndex; }
    }
    if (SoldierComponents.IsValidIndex(BearerFlankMan) && SoldierComponents[BearerFlankMan])
        return SoldierComponents[BearerFlankMan]->GetComponentLocation() + FRotator(0.f, OutBearerYaw, 0.f).RotateVector(FVector(0.f, BearerLateral, 0.f));
    return BearerUnit.TransformPosition(FVector(BearerFront, BearerRight + BearerLateral, 0.f));
}

void UStrategyInfantryVisualComponent::PromoteColourBearer(bool bBearerInitial)
{
    // The nearest front-rank man (to the fallen colours; at the start, to the centre of the front) leaves his file.
    if (!OwnerCompany || SoldierComponents.Num() < 4) return;
    const FTransform BearerUnit = OwnerCompany->GetActorTransform();
    const FVector BearerNear = bBearerInitial
        ? BearerUnit.TransformPosition(FVector(0.f, VisualPath.SlotBounds.IsValid ? VisualPath.SlotBounds.Max.Y : 0.f, 0.f)) : ColourDownLocation;
    float BearerFrontX = -TNumericLimits<float>::Max();
    for (const USkeletalMeshComponent* BearerMan : SoldierComponents)
        if (BearerMan) BearerFrontX = FMath::Max(BearerFrontX, float(BearerMan->GetRelativeLocation().X));
    const float BearerBand = OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierRankSpacingCm * 0.6f : 60.f;
    int32 BearerPick = INDEX_NONE;
    double BearerBest = TNumericLimits<double>::Max();
    for (int32 BearerIndex = 0; BearerIndex < SoldierComponents.Num(); ++BearerIndex)
    {
        const USkeletalMeshComponent* BearerMan = SoldierComponents[BearerIndex];
        if (!BearerMan || BearerMan->GetRelativeLocation().X < BearerFrontX - BearerBand) continue;
        const double BearerDistance = FVector::DistSquared2D(BearerMan->GetComponentLocation(), BearerNear);
        if (BearerDistance < BearerBest) { BearerBest = BearerDistance; BearerPick = BearerIndex; }
    }
    if (BearerPick == INDEX_NONE) return;
    USkeletalMeshComponent* BearerMan = SoldierComponents[BearerPick];
    // He drops his rifle (it stays on the ground at his feet) and is taken out of the grid; his file fills from behind.
    if (WeaponComponents.IsValidIndex(BearerPick) && WeaponComponents[BearerPick])
        WeaponComponents[BearerPick]->DestroyComponent();
    if (!bBearerInitial)
    {
        if (UStaticMesh* BearerRifleMesh = CurrentRifleMesh())
        {
            UStaticMeshComponent* BearerRifle = NewObject<UStaticMeshComponent>(GetOwner());
            BearerRifle->SetStaticMesh(BearerRifleMesh);
            BearerRifle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            BearerRifle->SetUsingAbsoluteLocation(true);
            BearerRifle->SetUsingAbsoluteRotation(true);
            BearerRifle->RegisterComponent();
            const FVector BearerFeet = UStrategyTerrainQueryLibrary::ProjectPointToTerrain(this, BearerMan->GetComponentLocation());
            // Lay it flat on the ground: if the mesh is modelled standing (long axis Z) tip it over; its thin side down.
            const FVector BearerRifleSize = BearerRifleMesh->GetBoundingBox().GetSize();
            const bool bBearerRifleUpright = BearerRifleSize.Z > BearerRifleSize.X && BearerRifleSize.Z > BearerRifleSize.Y;
            const FRotator BearerRifleRotation(bBearerRifleUpright ? 90.f : 0.f, BearerUnit.Rotator().Yaw + 90.f + FMath::FRandRange(-40.f, 40.f), 0.f);
            const float BearerRifleThick = bBearerRifleUpright ? FMath::Min(BearerRifleSize.X, BearerRifleSize.Y) : BearerRifleSize.Z;
            BearerRifle->SetWorldLocationAndRotation(BearerFeet + FVector(0.f, 0.f, 0.5f * BearerRifleThick + 1.f), BearerRifleRotation);
            DroppedRifles.Add(BearerRifle);
        }
    }
    if (CrowdNearFigures.Remove(BearerMan))
        if (UStrategyCrowdSubsystem* BearerCrowd = GetWorld()->GetSubsystem<UStrategyCrowdSubsystem>())
            BearerCrowd->NearGeometryCount = FMath::Max(0, BearerCrowd->NearGeometryCount - 1);
    SoldierComponents.RemoveAt(BearerPick);
    if (SoldierSettle.IsValidIndex(BearerPick)) SoldierSettle.RemoveAt(BearerPick);
    if (SoldierClips.IsValidIndex(BearerPick)) SoldierClips.RemoveAt(BearerPick);
    if (SoldierSlots.IsValidIndex(BearerPick)) SoldierSlots.RemoveAt(BearerPick);
    if (WeaponComponents.IsValidIndex(BearerPick)) WeaponComponents.RemoveAt(BearerPick);
    if (SoldierFireAt.IsValidIndex(BearerPick)) SoldierFireAt.RemoveAt(BearerPick);
    if (SoldierBusyUntil.IsValidIndex(BearerPick)) SoldierBusyUntil.RemoveAt(BearerPick);
    if (SoldierFirePhase.IsValidIndex(BearerPick)) SoldierFirePhase.RemoveAt(BearerPick);
    // The bearer is always drawn as a full skeletal figure (no rifle), whatever the crowd mode. A crowd figure has no mesh of its
    // own (the baked crowd draws it), so he gets the soldier mesh back (2026-10-21: the colours floated with nobody under them).
    if (!BearerMan->GetSkeletalMeshAsset() && LoadedSoldierMesh) BearerMan->SetSkeletalMeshAsset(LoadedSoldierMesh);
    BearerMan->SetVisibility(true, true);
    BearerMan->SetComponentTickEnabled(true);
    BearerMan->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    BearerMan->bPauseAnims = false;
    if (UAnimSequence* BearerIdle = ColourIdleAsset.Get() ? ColourIdleAsset.Get() : IdleStandingAsset.Get()) BearerMan->PlayAnimation(BearerIdle, true);
    ColourBearer = BearerMan;
    bColourBearerWorldValid = false;
    bColourBearerAlive = true;
    ColourPromoteAt = -1.f;
    bCrowdDirty = true;
    if (!bBearerInitial)
    {
        // Only the man behind steps up into his file; the line does not close up for a colour bearer (2026-10-21: the whole line
        // side-stepped when one man took the colours).
        ScheduleStepUp();
        OwnerCompany->AddUnitLog(TEXT("Fane"), TEXT("ny fanebærer"), TEXT("en mand fra forreste geled smider geværet og tager fanen"), 0.0f);
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Colours %s new bearer"), *OwnerCompany->StableUnitId.ToString());
    }
}

void UStrategyInfantryVisualComponent::KillColourBearer()
{
    if (!ColourBearer) return;
    USkeletalMeshComponent* BearerFallen = ColourBearer;
    BearerFallen->bPauseAnims = false;
    UAnimSequence* BearerDeath = DeathAsset.Get() ? DeathAsset.Get() : IdleProneAsset.Get();
    if (BearerDeath) { BearerFallen->PlayAnimation(BearerDeath, false); BearerFallen->SetPlayRate(FMath::FRandRange(0.9f, 1.1f)); }
    FallenColourBearers.Add(BearerFallen);
    ColourBearer = nullptr;
    bColourBearerWorldValid = false;
    bColourBearerAlive = false;
    bColoursDown = true;
    ColourDownLocation = BearerFallen->GetComponentLocation();
    ColourDownYaw = BearerFallen->GetComponentRotation().Yaw - SoldierMeshYawOffset;
    ColourPromoteAt = GetWorld()->GetTimeSeconds() + FMath::FRandRange(2.f, 4.f);
    // The colours down: a small blow to the company (an estimate, not a sourced figure); most of it returns when they are raised.
    OwnerCompany->Morale = FMath::Max(0.f, OwnerCompany->Morale - 5.f);
    OwnerCompany->AddUnitLog(TEXT("Fane"), TEXT("fanebæreren er faldet"), TEXT("fanen ligger nede"), 0.0f);
    UE_LOG(LogTemp, Display, TEXT("Strategy1864Colours %s bearer fell"), *OwnerCompany->StableUnitId.ToString());
}

void UStrategyInfantryVisualComponent::UpdateColourBearer(float BearerDt)
{
    if (!bHasColours || !OwnerCompany || !GetWorld() || BearerDt <= 0.f) return;
    if (!ColourBearer && !bColoursDown && VisualPath.bInitialized && SoldierComponents.Num() >= 4) PromoteColourBearer(true);
    if (!bColourBearerAlive && ColourPromoteAt >= 0.f && GetWorld()->GetTimeSeconds() >= ColourPromoteAt) PromoteColourBearer(false);
    if (!ColourBearer || !bColourBearerAlive) return;
    if (ColourPickupPhase > 0)
    {
        // He kneels by the colours, takes them up and rises with them (stand-to-kneel, then kneel-to-stand).
        const float PickupNow = GetWorld()->GetTimeSeconds();
        if (PickupNow < ColourPickupEnd) return;
        UAnimSequence* PickupRise = CrowdStanceClips.IsValidIndex(1) ? CrowdStanceClips[1].Get() : nullptr;
        if (ColourPickupPhase == 1)
        {
            bColoursDown = false;
            OwnerCompany->Morale = FMath::Min(100.f, OwnerCompany->Morale + 3.f);
            OwnerCompany->AddUnitLog(TEXT("Fane"), TEXT("fanen er rejst igen"), TEXT("ny fanebærer"), 0.0f);
            UE_LOG(LogTemp, Display, TEXT("Strategy1864Colours %s colours raised"), *OwnerCompany->StableUnitId.ToString());
            ColourPickupPhase = 2;
            ColourPickupEnd = PickupNow + (PickupRise ? PickupRise->GetPlayLength() : 0.5f);
            if (PickupRise) { ColourBearer->PlayAnimation(PickupRise, false); ColourBearer->SetPlayRate(1.f); }
            return;
        }
        ColourPickupPhase = 0;
    }
    float BearerYaw = 0.f;
    FVector BearerGoal = bColoursDown ? ColourDownLocation : ColourBearerTarget(BearerYaw);
    if (bColoursDown)
    {
        // He kneels beside the fallen pole, about 70 cm up it from the foot, on the company's side, facing it, so the hand that
        // takes it is at the pole (2026-10-22: he knelt at the foot, facing the front, a metre or more from where it lay). The
        // pole lies along the colour actor's Z axis, which StrategyColourFlag turns as (0, yaw + 70, 84).
        const FVector BearerPoleDir = FRotator(0.f, ColourDownYaw + 70.f, 84.f).RotateVector(FVector::UpVector).GetSafeNormal2D();
        const FVector BearerGrip = ColourDownLocation + BearerPoleDir * 70.f;
        FVector BearerSide(-BearerPoleDir.Y, BearerPoleDir.X, 0.f);
        if (FVector::DotProduct(BearerSide, OwnerCompany->GetActorLocation() - BearerGrip) < 0.f) BearerSide = -BearerSide;
        BearerGoal = BearerGrip + BearerSide * 40.f;
        BearerYaw = (-BearerSide).Rotation().Yaw;
    }
    if (!bColoursDown)
    {
        // He keeps with the men: never more than two ranks from the nearest of them, whatever the formation's target says
        // (2026-10-22 tests: 20-26 m away when a column formed standing and right after a drill into line).
        const float BearerKeepCm = FMath::Max(ColourColumnLeadCm + 30.f, 2.f * (OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierRankSpacingCm : 90.f));
        FVector BearerNearest = BearerGoal;
        float BearerNearestGap = TNumericLimits<float>::Max();
        for (const USkeletalMeshComponent* BearerKeepMan : SoldierComponents)
        {
            if (!BearerKeepMan) continue;
            const float BearerKeepGap = FVector::Dist2D(BearerKeepMan->GetComponentLocation(), BearerGoal);
            if (BearerKeepGap < BearerNearestGap) { BearerNearestGap = BearerKeepGap; BearerNearest = BearerKeepMan->GetComponentLocation(); }
        }
        if (BearerNearestGap < TNumericLimits<float>::Max() && BearerNearestGap > BearerKeepCm)
            BearerGoal = BearerNearest + (BearerGoal - BearerNearest).GetSafeNormal2D() * BearerKeepCm;
    }
    BearerGoal = UStrategyTerrainQueryLibrary::ProjectPointToTerrain(this, BearerGoal);
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugFormation")) && !ColourLastGoal.IsZero() && FVector::Dist2D(ColourLastGoal, BearerGoal) > 500.f)
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Colours %s target jumped %.0f cm: drill=%d to-column=%d formation=%d trail=%d plan=%d arrival=%d goal=%s"),
            *OwnerCompany->StableUnitId.ToString(), FVector::Dist2D(ColourLastGoal, BearerGoal), DrillActive ? 1 : 0, DrillToColumn ? 1 : 0,
            OwnerCompany->FormationComponent ? int32(OwnerCompany->FormationComponent->CurrentFormation) : -1, VisualPath.Trail.Num(),
            bFormationPlanPending ? 1 : 0, bArrivalReform ? 1 : 0, *BearerGoal.ToString());
    ColourLastGoal = BearerGoal;
    // He walks on from where he stood last frame in the world: he hangs on the unit's root, so a jump of the unit's actor (it moves
    // to the column's head when a column forms, and to the new line when a drill ends) would otherwise carry him 20 m and more
    // in one frame (2026-10-22 test).
    FVector BearerHere = bColourBearerWorldValid ? ColourBearerWorld : ColourBearer->GetComponentLocation();
    FRotator BearerRotation = bColourBearerWorldValid ? ColourBearerWorldRotation : ColourBearer->GetComponentRotation();
    const FVector BearerTo = BearerGoal - BearerHere;
    const float BearerDistance = BearerTo.Size2D();
    const float BearerMarch = OwnerCompany->MovementExecutor ? OwnerCompany->MovementExecutor->MoveSpeedCmPerSecond : 150.f;
    // Left behind (more than two ranks from the nearest man) he catches up at a run; as fast as the flank itself when the rigid
    // line swings round its centre (its ends move at up to 14 m/s: 2026-10-22 test, he fell 9 m behind).
    float BearerOwnGap = TNumericLimits<float>::Max();
    for (const USkeletalMeshComponent* BearerGapMan : SoldierComponents)
        if (BearerGapMan) BearerOwnGap = FMath::Min(BearerOwnGap, float(FVector::Dist2D(BearerGapMan->GetComponentLocation(), BearerHere)));
    const float BearerCatchUp = BearerOwnGap > 2.f * (OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierRankSpacingCm : 90.f) ? FMath::Max(450.f, BearerOwnGap * 2.f) : 0.f;
    const float BearerSpeed = FMath::Min(BearerDistance * 3.f, FMath::Max3(150.f, BearerMarch * 1.3f, BearerCatchUp));
    const float BearerStep = FMath::Min(BearerDistance, BearerSpeed * BearerDt);
    if (BearerDistance > 1.f) BearerHere += BearerTo.GetSafeNormal2D() * BearerStep;
    BearerHere.Z = BearerGoal.Z;
    const bool BearerWalking = BearerStep / BearerDt > 25.f;
    const float BearerFacing = BearerWalking ? BearerTo.Rotation().Yaw : BearerYaw;
    BearerRotation.Pitch = 0.f; BearerRotation.Roll = 0.f;
    BearerRotation.Yaw = FStrategyVisualFormationPath::SmoothFacing(BearerRotation.Yaw, ColourBearerYawVelocity, BearerFacing + SoldierMeshYawOffset, BearerDt, 160.f);
    ColourBearer->SetWorldLocationAndRotation(BearerHere, BearerRotation);
    ColourBearerWorld = BearerHere;
    ColourBearerWorldRotation = BearerRotation;
    bColourBearerWorldValid = true;
    // Walk (rifle-less arms of the shoulder walk) at his own pace, stand otherwise.
    UAnimSequence* BearerWalkClip = ColourWalkAsset.Get() ? ColourWalkAsset.Get() : (WalkShoulderAsset.Get() ? WalkShoulderAsset.Get() : WalkStandingAsset.Get());
    UAnimSequence* BearerClip = BearerWalking ? BearerWalkClip : (ColourIdleAsset.Get() ? ColourIdleAsset.Get() : IdleStandingAsset.Get());
    if (BearerClip && ColourBearer->GetSingleNodeInstance() && ColourBearer->GetSingleNodeInstance()->GetAnimationAsset() != BearerClip)
        ColourBearer->PlayAnimation(BearerClip, true);
    ColourBearer->SetPlayRate(BearerWalking ? FMath::Clamp((BearerStep / BearerDt) / (BearerClip == WalkStandingAsset.Get() ? 76.99816f : 137.70097f), 0.3f, 2.5f) : 1.f);
    // He reaches the fallen colours: he raises them.
    // He reaches his place beside the pole and has turned to it: he kneels (at 60 cm and still facing the way he walked he knelt
    // beside it, not at it).
    if (bColoursDown && BearerDistance < 15.f && ColourPickupPhase == 0 &&
        FMath::Abs(FMath::FindDeltaAngleDegrees(BearerRotation.Yaw, BearerYaw + SoldierMeshYawOffset)) < 15.f)
    {
        UAnimSequence* PickupKneel = CrowdStanceClips.IsValidIndex(0) ? CrowdStanceClips[0].Get() : nullptr;
        ColourPickupPhase = 1;
        ColourPickupEnd = GetWorld()->GetTimeSeconds() + (PickupKneel ? PickupKneel->GetPlayLength() : 0.5f) + 0.4f;
        if (PickupKneel) { ColourBearer->PlayAnimation(PickupKneel, false); ColourBearer->SetPlayRate(1.f); }
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Colours %s new bearer kneels at the colours: at %s yaw %.0f, colours foot %s"), *OwnerCompany->StableUnitId.ToString(),
            *ColourBearer->GetComponentLocation().ToString(), ColourBearer->GetComponentRotation().Yaw - SoldierMeshYawOffset, *ColourDownLocation.ToString());
    }
}

FVector UStrategyInfantryVisualComponent::PalmLocation(const USkeletalMeshComponent* PalmMesh, FName PalmHandBone)
{
    const FVector PalmWrist = PalmMesh->GetSocketLocation(PalmHandBone);
    const int32 PalmIndex = PalmMesh->GetBoneIndex(PalmHandBone);
    const USkinnedAsset* PalmAsset = PalmMesh->GetSkinnedAsset();
    if (PalmIndex == INDEX_NONE || !PalmAsset) return PalmWrist;
    // The middle finger's root (the first child is often the thumb, which sits almost on the wrist).
    const FReferenceSkeleton& PalmSkeleton = PalmAsset->GetRefSkeleton();
    int32 PalmFinger = INDEX_NONE;
    for (int32 PalmChild = PalmIndex + 1; PalmChild < PalmSkeleton.GetNum(); ++PalmChild)
    {
        if (PalmSkeleton.GetParentIndex(PalmChild) != PalmIndex) continue;
        if (PalmFinger == INDEX_NONE || PalmSkeleton.GetBoneName(PalmChild).ToString().Contains(TEXT("Middle"))) PalmFinger = PalmChild;
    }
    return PalmFinger == INDEX_NONE ? PalmWrist : (PalmWrist + PalmMesh->GetBoneTransform(PalmFinger).GetLocation()) * 0.5f;
}

bool UStrategyInfantryVisualComponent::GetColourTransform(FVector& OutColourLocation, float& OutColourYaw, bool& bOutColoursDown) const
{
    if (!bHasColours) return false;
    bOutColoursDown = bColoursDown;
    if (bColoursDown)
    {
        OutColourLocation = ColourDownLocation;
        OutColourYaw = ColourDownYaw;
        return true;
    }
    if (!ColourBearer) return false;
    // As in the user's reference (Flag/Fane_March_*.png): the pole in his right hand, the hand at chest height, the pole leaning a
    // little back and its foot at about knee height, the cloth streaming behind him.
    const FRotator BearerFacing(0.f, ColourBearer->GetComponentRotation().Yaw - SoldierMeshYawOffset, 0.f);
    // The pole runs through his hand, its foot 65 cm below the hand along the pole (so it follows his stride), leaning back
    // ColourCarryLeanDeg: the foot sits about 14 cm in front of the hand (measured on the bearer clips, 2026-10-20).
    const FVector BearerHand = PalmLocation(ColourBearer, RightHandBoneName);
    const float BearerLean = FMath::DegreesToRadians(ColourCarryLeanDeg);
    const FVector BearerPole = BearerFacing.RotateVector(FVector(-FMath::Sin(BearerLean), 0.f, FMath::Cos(BearerLean)));
    OutColourLocation = BearerHand - BearerPole * 65.f;
    OutColourYaw = BearerFacing.Yaw;
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864ColourDiag")))
    {
        static double ColourDiagNext = 0.0;
        if (FPlatformTime::Seconds() >= ColourDiagNext)
        {
            ColourDiagNext = FPlatformTime::Seconds() + 2.0;
            const FTransform DiagFrame(BearerFacing, ColourBearer->GetComponentLocation());
            const int32 DiagHandIndex = ColourBearer->GetBoneIndex(RightHandBoneName);
            const FVector DiagHandBone = DiagHandIndex != INDEX_NONE ? ColourBearer->GetBoneTransform(DiagHandIndex).GetLocation() : FVector::ZeroVector;
            UE_LOG(LogTemp, Display, TEXT("Strategy1864ColourDiag hand(local)=%s bone(local)=%s left(local)=%s scale=%s anim=%s"),
                *DiagFrame.InverseTransformPosition(BearerHand).ToString(), *DiagFrame.InverseTransformPosition(DiagHandBone).ToString(),
                *DiagFrame.InverseTransformPosition(ColourBearer->GetSocketLocation(LeftHandBoneName)).ToString(),
                *ColourBearer->GetComponentScale().ToString(),
                ColourBearer->GetSingleNodeInstance() && ColourBearer->GetSingleNodeInstance()->GetAnimationAsset() ? *ColourBearer->GetSingleNodeInstance()->GetAnimationAsset()->GetName() : TEXT("-"));
        }
    }
    return true;
}

FString UStrategyInfantryVisualComponent::DebugCrowdStat() const
{
    FVector StatMean = FVector::ZeroVector;
    int32 StatMen = 0, StatVisible = 0;
    for (const USkeletalMeshComponent* StatMan : SoldierComponents)
        if (StatMan) { StatMean += StatMan->GetComponentLocation(); ++StatMen; StatVisible += StatMan->IsVisible() ? 1 : 0; }
    if (StatMen) StatMean /= StatMen;
    auto StatIsm = [](const UInstancedStaticMeshComponent* StatInstances)
    {
        if (!StatInstances) return FString(TEXT("none"));
        int32 StatShown = 0;
        for (int32 StatIndex = 0; StatIndex < StatInstances->GetInstanceCount(); ++StatIndex)
        {
            FTransform StatPose;
            if (StatInstances->GetInstanceTransform(StatIndex, StatPose, true) && !StatPose.GetScale3D().IsNearlyZero()) ++StatShown;
        }
        return FString::Printf(TEXT("%d/%d vis=%d bounds=%s r=%.0f"), StatShown, StatInstances->GetInstanceCount(), StatInstances->IsVisible() ? 1 : 0,
            *StatInstances->Bounds.Origin.ToString(), StatInstances->Bounds.SphereRadius);
    };
    FString StatOut = FString::Printf(TEXT("crowdMode=%d men=%d skeletalVisible=%d meanMen=%s unit=%s yaw=%.0f living[%s]"), bCrowdMode ? 1 : 0, StatMen, StatVisible,
        *StatMean.ToString(), OwnerCompany ? *OwnerCompany->GetActorLocation().ToString() : TEXT("-"), OwnerCompany ? OwnerCompany->GetActorRotation().Yaw : 0.f,
        *StatIsm(CrowdLiving.Get()));
    for (int32 StatBand = 0; StatBand < CrowdLODBands.Num(); StatBand += 2)
        StatOut += FString::Printf(TEXT(" band%d[%s]"), 1 + StatBand / 2, *StatIsm(CrowdLODBands[StatBand].Instances.Get()));
    return StatOut;
}

FVector UStrategyInfantryVisualComponent::FormationPlace(int32 PlaceIndex, bool bPlaceColumn) const
{
    const FSettle& PlaceMan = SoldierSettle[PlaceIndex];
    // Arriving with a new front: his place in the new line. A drill into line: his place in the line it forms.
    if (bArrivalReform) return PlaceMan.ReformTarget;
    if (DrillActive && !DrillToColumn) return DrillFinalFrame.TransformPosition(PlaceMan.Slot);
    // In column: his spot on the column as it walks (the snake's own goal for him this tick).
    if (bPlaceColumn && OwnerCompany) return OwnerCompany->GetActorTransform().TransformPosition(PlaceMan.Goal);
    float PlaceYaw = 0.f;
    return VisualPath.Goal(PlaceMan.Slot, false, PlaceYaw);
}

UStrategyInfantryVisualComponent::FFormationFit UStrategyInfantryVisualComponent::MeasureFormationFit(bool bNeighbours) const
{
    FFormationFit Fit;
    if (!OwnerCompany || !OwnerCompany->FormationComponent || !VisualPath.bInitialized) return Fit;
    const bool bFitColumn = (OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
        OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn) && VisualPath.Trail.Num() >= 2;
    TArray<float> FitGaps;
    TArray<FVector> FitMen;
    TArray<int32> FitIndices;
    for (int32 FitIndex = 0; FitIndex < SoldierSettle.Num() && FitIndex < SoldierComponents.Num(); ++FitIndex)
    {
        const USkeletalMeshComponent* FitMan = SoldierComponents[FitIndex];
        if (!FitMan) continue;
        // The formation's own place for him, not the passing goal of a drill or a morph.
        const FVector FitPlace = FormationPlace(FitIndex, bFitColumn);
        const FVector FitAt = FitMan->GetComponentLocation();
        FitGaps.Add(FVector::Dist2D(FitAt, FitPlace));
        FitMen.Add(FitAt);
        FitIndices.Add(FitIndex);
    }
    Fit.Men = FitGaps.Num();
    if (Fit.Men == 0) return Fit;
    for (const float FitGap : FitGaps) Fit.Placed += FitGap <= FormationReadyGapCm ? 1 : 0;
    Fit.Ready = float(Fit.Placed) / float(Fit.Men);
    FitGaps.Sort();
    Fit.MaxGapCm = FitGaps.Last();
    Fit.P95GapCm = FitGaps[FMath::Clamp(FMath::CeilToInt32(0.95f * Fit.Men) - 1, 0, Fit.Men - 1)];
    if (bNeighbours && Fit.Men > 1)
    {
        Fit.MinNeighbourCm = TNumericLimits<float>::Max();
        for (int32 FitA = 0; FitA < FitMen.Num(); ++FitA)
            for (int32 FitB = FitA + 1; FitB < FitMen.Num(); ++FitB)
            {
                const float FitPair = float(FVector::Dist2D(FitMen[FitA], FitMen[FitB]));
                if (FitPair < Fit.MinNeighbourCm) { Fit.MinNeighbourCm = FitPair; Fit.ClosestA = FitIndices[FitA]; Fit.ClosestB = FitIndices[FitB]; }
            }
    }
    if (ColourBearer && bColourBearerAlive)
    {
        Fit.BearerGapCm = TNumericLimits<float>::Max();
        for (const FVector& FitAt : FitMen) Fit.BearerGapCm = FMath::Min(Fit.BearerGapCm, float(FVector::Dist2D(FitAt, ColourBearer->GetComponentLocation())));
    }
    return Fit;
}

void UStrategyInfantryVisualComponent::LogFormationMetric(const FString& ShownStatus) const
{
    if (!OwnerCompany || !GetWorld()) return;
    const FFormationFit MetricFit = MeasureFormationFit(true);
    const TCHAR* MetricFormation = !OwnerCompany->FormationComponent ? TEXT("-") :
        OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Line ? TEXT("Line") :
        OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Square ? TEXT("Square") : TEXT("Column");
    FVector MetricMean = FVector::ZeroVector;
    int32 MetricMeanMen = 0;
    for (const USkeletalMeshComponent* MetricMan : SoldierComponents) if (MetricMan) { MetricMean += MetricMan->GetComponentLocation(); ++MetricMeanMen; }
    if (MetricMeanMen) MetricMean /= MetricMeanMen;
    UE_LOG(LogTemp, Display, TEXT("Strategy1864FormationWhere %s t=%.1f men-mean=(%.0f,%.0f) actor=(%.0f,%.0f) bearer=(%.0f,%.0f) bearer-attached=%s"),
        *OwnerCompany->StableUnitId.ToString(), GetWorld()->GetTimeSeconds(), MetricMean.X, MetricMean.Y, OwnerCompany->GetActorLocation().X, OwnerCompany->GetActorLocation().Y,
        ColourBearer ? ColourBearer->GetComponentLocation().X : 0.f, ColourBearer ? ColourBearer->GetComponentLocation().Y : 0.f,
        ColourBearer && ColourBearer->GetAttachParent() ? *ColourBearer->GetAttachParent()->GetName() : TEXT("-"));
    UE_LOG(LogTemp, Display, TEXT("Strategy1864FormationMetric %s t=%.1f status=%s formation=%s ready=%d/%d max-gap-cm=%.0f p95-gap-cm=%.0f min-neighbour-cm=%.0f bearer-gap-cm=%.0f drill=%d plan=%d arrival=%d speed=%.0f"),
        *OwnerCompany->StableUnitId.ToString(), GetWorld()->GetTimeSeconds(), *ShownStatus, MetricFormation, MetricFit.Placed, MetricFit.Men,
        MetricFit.MaxGapCm, MetricFit.P95GapCm, MetricFit.MinNeighbourCm, MetricFit.BearerGapCm, DrillActive ? 1 : 0, bFormationPlanPending ? 1 : 0,
        bArrivalReform ? 1 : 0, OwnerCompany->MovementExecutor ? OwnerCompany->MovementExecutor->GetExecutedVelocity().Size2D() : 0.f);
    if (VisualPath.Trail.Num() >= 2)
    {
        float MetricMinSlotX = 0.f;
        for (const FSettle& MetricSlotMan : SoldierSettle) MetricMinSlotX = FMath::Min(MetricMinSlotX, float(MetricSlotMan.Slot.X));
        UE_LOG(LogTemp, Display, TEXT("Strategy1864FormationTrail %s t=%.1f trail=%d first=%.0f now=%.0f bounds-max-x=%.0f min-slot-x=%.0f wanted-tail=%.0f ramp=%d closing=%d"),
            *OwnerCompany->StableUnitId.ToString(), GetWorld()->GetTimeSeconds(), VisualPath.Trail.Num(), VisualPath.Trail[0].Distance, VisualPath.TrailDistance,
            VisualPath.SlotBounds.IsValid ? VisualPath.SlotBounds.Max.X : -1.f, MetricMinSlotX,
            VisualPath.TrailDistance - ((VisualPath.SlotBounds.IsValid ? VisualPath.SlotBounds.Max.X : 0.f) - MetricMinSlotX), bSnakeRamp ? 1 : 0, bClosingRanksStep ? 1 : 0);
    }
    if (SoldierSettle.IsValidIndex(MetricFit.ClosestA) && SoldierSettle.IsValidIndex(MetricFit.ClosestB) && MetricFit.MinNeighbourCm < 35.f)
        UE_LOG(LogTemp, Display, TEXT("Strategy1864FormationClosest %s t=%.1f %.0f cm: man %d row %d lane %d slot %s / man %d row %d lane %d slot %s at %s"),
            *OwnerCompany->StableUnitId.ToString(), GetWorld()->GetTimeSeconds(), MetricFit.MinNeighbourCm,
            MetricFit.ClosestA, SoldierSettle[MetricFit.ClosestA].SnakeRow, SoldierSettle[MetricFit.ClosestA].SnakeLane, *SoldierSettle[MetricFit.ClosestA].Slot.ToString(),
            MetricFit.ClosestB, SoldierSettle[MetricFit.ClosestB].SnakeRow, SoldierSettle[MetricFit.ClosestB].SnakeLane, *SoldierSettle[MetricFit.ClosestB].Slot.ToString(),
            SoldierComponents[MetricFit.ClosestA] ? *SoldierComponents[MetricFit.ClosestA]->GetComponentLocation().ToString() : TEXT("-"));
    if (SoldierSettle.IsValidIndex(MetricFit.ClosestA) && SoldierSettle.IsValidIndex(MetricFit.ClosestB) && MetricFit.MinNeighbourCm < 35.f)
        for (const int32 MetricPairMan : {MetricFit.ClosestA, MetricFit.ClosestB})
            if (SoldierComponents[MetricPairMan])
                UE_LOG(LogTemp, Display, TEXT("Strategy1864FormationClosestMan %d goal-gap=%.0f active=%d close-start-in=%.1f placed=%d visible=%d clip=%s"),
                    MetricPairMan, FVector::Dist2D(SoldierComponents[MetricPairMan]->GetRelativeLocation(), SoldierSettle[MetricPairMan].Goal),
                    SoldierSettle[MetricPairMan].bActive ? 1 : 0, SoldierSettle[MetricPairMan].CloseStart >= 0.f ? SoldierSettle[MetricPairMan].CloseStart - VisualAnimationTime : -1.f,
                    SoldierSettle[MetricPairMan].bPlaced ? 1 : 0, SoldierComponents[MetricPairMan]->IsVisible() ? 1 : 0,
                    SoldierClips.IsValidIndex(MetricPairMan) && SoldierClips[MetricPairMan].Clip.IsValid() ? *SoldierClips[MetricPairMan].Clip->GetName() : TEXT("-"));
    if (!FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugFormation")) || !VisualPath.bInitialized || !OwnerCompany->FormationComponent) return;
    const bool bDrawColumn = (OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
        OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn) && VisualPath.Trail.Num() >= 2;
    for (int32 DrawIndex = 0; DrawIndex < SoldierSettle.Num() && DrawIndex < SoldierComponents.Num(); ++DrawIndex)
    {
        const USkeletalMeshComponent* DrawMan = SoldierComponents[DrawIndex];
        if (!DrawMan) continue;
        const FVector DrawPlace = FormationPlace(DrawIndex, bDrawColumn) + FVector(0.f, 0.f, 20.f);
        const FVector DrawAt = DrawMan->GetComponentLocation() + FVector(0.f, 0.f, 20.f);
        DrawDebugPoint(GetWorld(), DrawPlace, 7.f, FColor::Blue, false, 1.05f);
        DrawDebugPoint(GetWorld(), DrawAt, 5.f, FColor::Yellow, false, 1.05f);
        if (FVector::Dist2D(DrawPlace, DrawAt) > 200.f) DrawDebugLine(GetWorld(), DrawAt, DrawPlace, FColor::Red, false, 1.05f, 0, 3.f);
    }
}

bool UStrategyInfantryVisualComponent::CanUseColumnDrill() const
{
    int32 DrillRigidSetting = 1;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Rigid="), DrillRigidSetting);
    return bEnabled && bRigidCompany && DrillRigidSetting != 0 && FStrategyColumnDrill::Enabled() &&
        OwnerCompany && OwnerCompany->FormationComponent &&
        OwnerCompany->FormationComponent->RankCount == 3 && OwnerCompany->FormationComponent->ColumnWidth == 3 &&
        !SoldierComponents.IsEmpty() &&
        !(OwnerCompany->StanceComponent && OwnerCompany->StanceComponent->Stance == EStrategyStance::Prone) &&
        !(OwnerCompany->SkirmisherComponent && (OwnerCompany->SkirmisherComponent->IsDeployed() ||
            OwnerCompany->SkirmisherComponent->State == EStrategySkirmisherState::Deploying));
}
