#include "StrategyInfantryVisualComponent.h"
#include "Algo/Reverse.h"
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
#include "../AI/StrategyOfficerProfileComponent.h"
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
    OfficerWalkAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/Officer/A_Officer_Walk.A_Officer_Walk")));
    OfficerWalkSheathedAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/Officer/A_Officer_WalkSheathed.A_Officer_WalkSheathed")));
    OfficerSabreDrawAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/Officer/A_Officer_SabreDraw.A_Officer_SabreDraw")));
    OfficerSabreSheathAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/Officer/A_Officer_SabreSheath.A_Officer_SabreSheath")));
    OfficerIdleAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/Officer/A_Officer_Idle.A_Officer_Idle")));
    OfficerOrderAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/Officer/A_Officer_Order.A_Officer_Order")));
    OfficerDeathAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/Officer/A_Officer_Death.A_Officer_Death")));

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
    DeathMarchFrontAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_DeathMarchFront.A_Combat_DeathMarchFront")));
    DeathMarchBackAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_DeathMarchBack.A_Combat_DeathMarchBack")));
    DeathMarchCrumpleAsset = TSoftObjectPtr<UAnimSequence>(
        FSoftObjectPath(TEXT("/Game/Units/Human/CombatAnimations/A_Combat_DeathMarchCrumple.A_Combat_DeathMarchCrumple")));
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
    // The 1863 drill book: the line closes in towards the colours continually, also while it advances and fires; a column (and any
    // other formation) re-forms only when it halts.
    const bool bCloseWhileMoving = OwnerCompany->FormationComponent && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Line && bRigidCompany;
    if (bClosingRanksPending && PendingKillCount == 0 && !bFormationPlanPending && !bArrivalReform && !DrillActive &&
        (GetWorld()->GetTimeSeconds() - LastLossTimeSeconds > 3.0f * CloseRanksSlowdown || GetWorld()->GetTimeSeconds() - FirstPendingLossTimeSeconds > 8.0f * CloseRanksSlowdown) &&
        !(OwnerCompany->FormationTransition && OwnerCompany->FormationTransition->IsReforming()) && (bCloseWhileMoving || !VisualPath.bMovingVisuals))
    {
        bClosingRanksPending = false;
        // A square stands fast under losses: the men from behind have stepped up into the holes of the outer ranks (StepUpSquare),
        // and it is drawn in to a smaller square only when fewer than 45 % of its places are held (an estimate). It used to be
        // re-formed at every loss, everyone to a new place, the living over the fallen (QA 2026-11-05).
        const bool bSquareHolds = OwnerCompany->FormationComponent && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Square &&
            SquarePlaces.Num() > 0 && SoldierComponents.Num() >= FMath::CeilToInt(0.45f * SquarePlaces.Num());
        // In line: the files close in towards the colours by side-stepping; otherwise (column, square, spread) the old re-form.
        if (bSquareHolds) {}
        else if (!CloseRanksTowardColours())
        {
            bStepUpPending = false;   // the full re-form also fills the files
            RebuildFormation();
        }
    }
    UpdateSettling(DeltaTime);
    UpdateSlotMarkers();
    UpdateColourBearer(DeltaTime);
    UpdateOfficerFigure(DeltaTime);
    ApplyPendingStance(GetWorld()->GetTimeSeconds());
    MarchDust(GetWorld()->GetTimeSeconds());
    RefreshAnimation(false);
    UpdateStanceTransitions();
    UpdatePersonalActions();
    ApplySquareKneel();
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
    // A volley that took no man must not leave its picked victims for a later shell or an undirected loss.
    if (!bEnabled || !OwnerCompany || AppliedLoss <= 0) { AimedQueuedVictims.Reset(); return; }
    // Compare total represented strength, so fractional losses accumulate across the figure divisor.
    // Picked victims mean an aimed volley even from a shooter at the world origin: it is never queued as a shell's delayed kill.
    const bool bDirectedFire = !AimedQueuedVictims.IsEmpty() || !OwnerCompany->CasualtySourceLocation.IsNearlyZero();
    if (bDirectedFire) SourceLocation = OwnerCompany->CasualtySourceLocation;
    const int32 AimedFinalBearerLoss = !AimedQueuedVictims.IsEmpty() && bColourBearerAlive && ColourBearer &&
        OwnerCompany->CurrentStrength == 0 && AimedQueuedVictims.Contains(TWeakObjectPtr<USkeletalMeshComponent>(ColourBearer.Get())) ? 1 : 0;
    const int32 FiguresLost = FMath::Max(0, SoldierComponents.Num() - PendingKillCount - GetDesiredVisualCount()) + AimedFinalBearerLoss;
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
    // The 1863 drill book (user 2026-10-22): the man behind steps up at once; only the moment he needs to step forward, longer
    // when the company is shaken.
    if (!bStepUpPending) { StepUpDueSeconds = StepUpNow + 0.5f * StepUpSlowdown + (StepUpSlowdown - 1.f); }
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

void UStrategyInfantryVisualComponent::ApplySquareKneel()
{
    // In a square the front (outer) rank kneels and the ranks behind fire over it standing (the drill books; QA 2026-11-05: "de
    // yderste soldater kan knæle, mens de bageste står"). The company's clips are chosen for all; here the outer rank's standing
    // clip is swapped for its kneeling twin at the same point of the clip (the record of the chosen clip is left as it is).
    // Near men are skeletal figures (their clip is swapped); far men are drawn baked from the company's clip record, so they get the
    // twin through SquareKneelTwins (2026-11-07: the outer rank stood, all 64 of them drawn baked).
    if (!OwnerCompany || !OwnerCompany->FormationComponent || OwnerCompany->FormationComponent->CurrentFormation != EStrategyFormationType::Square ||
        SquarePlaces.IsEmpty() || SoldierSettle.Num() != SoldierComponents.Num())
    {
        if (!SquareKneelTwins.IsEmpty()) { SquareKneelTwins.Reset(); bCrowdDirty = true; }
        return;
    }
    if (SquareKneelTwins.Num() != SoldierComponents.Num()) SquareKneelTwins.SetNum(SoldierComponents.Num());
    const TPair<UAnimSequence*, UAnimSequence*> KneelTwins[] = {
        { IdleStandingAsset.Get(), IdleKneelingAsset.Get() }, { ReadyAsset.Get(), IdleKneelingAsset.Get() },
        { AimHoldAsset.Get(), AimKneelingAsset.Get() }, { FireStandingAsset.Get(), FireKneelingAsset.Get() },
        { ReloadStandingAsset.Get(), ReloadKneelingAsset.Get() }, { AimStandingAsset.Get(), AimKneelingAsset.Get() },
        { LoadAsset.Get(), ReloadKneelingAsset.Get() }, { BayonetChargeAsset.Get(), AimKneelingAsset.Get() } };
    static TSet<FName> KneelUnmatched;   // once per clip name: what the outer rank plays that has no kneeling twin
    FVector KneelMiddle = FVector::ZeroVector;
    for (const FVector& KneelPlace : SquarePlaces) KneelMiddle += KneelPlace;
    KneelMiddle /= float(SquarePlaces.Num());
    const auto KneelDepth = [&](const FVector& KneelAt) { const FVector KneelRel = KneelAt - KneelMiddle; return FMath::Max(FMath::Abs(float(KneelRel.X)), FMath::Abs(float(KneelRel.Y))); };
    float KneelOuter = 0.f;
    for (const FVector& KneelPlace : SquarePlaces) KneelOuter = FMath::Max(KneelOuter, KneelDepth(KneelPlace));
    const float KneelBand = 0.5f * FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierRankSpacingCm);
    for (int32 KneelMan = 0; KneelMan < SoldierComponents.Num(); ++KneelMan)
    {
        USkeletalMeshComponent* KneelSoldier = SoldierComponents[KneelMan];
        const bool bKneeler = KneelSoldier && SoldierSettle[KneelMan].ReformDeadline < 0.f && SoldierSettle[KneelMan].CloseStart < 0.f &&
            KneelOuter - KneelDepth(SoldierSettle[KneelMan].Slot) <= KneelBand;
        // The baked figure's twin, from the clip the company chose for him.
        UAnimSequence* KneelBakedTwin = nullptr;
        if (bKneeler && SoldierClips.IsValidIndex(KneelMan))
            for (const TPair<UAnimSequence*, UAnimSequence*>& KneelTwin : KneelTwins)
                if (KneelTwin.Key && KneelTwin.Value && SoldierClips[KneelMan].Clip.Get() == KneelTwin.Key) { KneelBakedTwin = KneelTwin.Value; break; }
        if (bKneeler && !KneelBakedTwin && SoldierClips.IsValidIndex(KneelMan) && SoldierClips[KneelMan].Clip.IsValid() &&
            !KneelUnmatched.Contains(SoldierClips[KneelMan].Clip->GetFName()))
        {
            KneelUnmatched.Add(SoldierClips[KneelMan].Clip->GetFName());
            UE_LOG(LogTemp, Display, TEXT("Strategy1864SquareKneel %s outer rank (baked) plays %s: no kneeling twin"), *OwnerCompany->StableUnitId.ToString(), *SoldierClips[KneelMan].Clip->GetName());
        }
        if (SquareKneelTwins[KneelMan] != KneelBakedTwin) { SquareKneelTwins[KneelMan] = KneelBakedTwin; bCrowdDirty = true; }
        if (!bKneeler || !KneelSoldier->IsVisible()) continue;
        UAnimSingleNodeInstance* KneelNode = KneelSoldier->GetSingleNodeInstance();
        const UAnimationAsset* KneelNow = KneelNode ? KneelNode->GetAnimationAsset() : nullptr;
        bool bKneelKnown = false;
        for (const TPair<UAnimSequence*, UAnimSequence*>& KneelTwin : KneelTwins) bKneelKnown |= KneelNow && (KneelNow == KneelTwin.Key || KneelNow == KneelTwin.Value);
        if (KneelNow && !bKneelKnown && !KneelUnmatched.Contains(KneelNow->GetFName()))
        {
            KneelUnmatched.Add(KneelNow->GetFName());
            UE_LOG(LogTemp, Display, TEXT("Strategy1864SquareKneel %s outer rank plays %s: no kneeling twin"), *OwnerCompany->StableUnitId.ToString(), *KneelNow->GetName());
        }
        for (const TPair<UAnimSequence*, UAnimSequence*>& KneelTwin : KneelTwins)
        {
            if (!KneelTwin.Key || !KneelTwin.Value || KneelNow != KneelTwin.Key) continue;
            const float KneelFraction = KneelTwin.Key->GetPlayLength() > 0.f ? KneelNode->GetCurrentTime() / KneelTwin.Key->GetPlayLength() : 0.f;
            const bool bKneelLoop = KneelNode->IsLooping();
            KneelSoldier->PlayAnimation(KneelTwin.Value, bKneelLoop);
            KneelSoldier->SetPosition(FMath::Clamp(KneelFraction, 0.f, 1.f) * KneelTwin.Value->GetPlayLength(), false);
            break;
        }
    }
}

void UStrategyInfantryVisualComponent::UpdateSlotMarkers()
{
    if (!OwnerCompany || !OwnerCompany->bPlayerControllable || !GetWorld()) return;
    if (!SlotMarkers)
    {
        if (bSlotMarkersTried) return;
        bSlotMarkersTried = true;
        UStaticMesh* MarkerMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        UMaterialInterface* MarkerBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        if (!MarkerMesh) return;
        SlotMarkers = NewObject<UInstancedStaticMeshComponent>(OwnerCompany, NAME_None, RF_Transient);
        SlotMarkers->SetStaticMesh(MarkerMesh);
        SlotMarkers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SlotMarkers->SetCastShadow(false);
        SlotMarkers->bVisibleInRayTracing = false;
        SlotMarkers->SetupAttachment(OwnerCompany->GetRootComponent());
        SlotMarkers->SetUsingAbsoluteLocation(true);
        SlotMarkers->SetUsingAbsoluteRotation(true);
        SlotMarkers->SetUsingAbsoluteScale(true);
        SlotMarkers->RegisterComponent();
        SlotMarkers->SetWorldTransform(FTransform::Identity);
        if (MarkerBase)
        {
            UMaterialInstanceDynamic* MarkerGold = UMaterialInstanceDynamic::Create(MarkerBase, SlotMarkers);
            MarkerGold->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.78f, 0.25f));
            SlotMarkers->SetMaterial(0, MarkerGold);
        }
    }
    // The places of the men still on their way (a fallen man is no longer in the list: his spot goes with him).
    TArray<FTransform> MarkerSpots;
    const FTransform MarkerUnit = OwnerCompany->GetActorTransform();
    for (int32 MarkerMan = 0; MarkerMan < SoldierSettle.Num() && MarkerMan < SoldierComponents.Num(); ++MarkerMan)
    {
        const FSettle& MarkerSettle = SoldierSettle[MarkerMan];
        if (!SoldierComponents[MarkerMan] || !MarkerSettle.bPlaced || MarkerSettle.ReformDeadline < 0.f) continue;
        FVector MarkerAt = bArrivalReform ? MarkerSettle.ReformTarget : MarkerUnit.TransformPosition(MarkerSettle.Goal);
        if (FVector::Dist2D(MarkerAt, SoldierComponents[MarkerMan]->GetComponentLocation()) < 40.f) continue;
        // A spot 32 cm across, lifted above the battlefield grass (it hid a flat one).
        MarkerAt.Z = UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, MarkerAt) + 18.f;
        MarkerSpots.Emplace(FRotator::ZeroRotator, MarkerAt, FVector(0.32f, 0.32f, 0.015f));
    }
    SlotMarkers->SetVisibility(!MarkerSpots.IsEmpty());
    if (SlotMarkers->GetInstanceCount() != MarkerSpots.Num())
    {
        SlotMarkers->ClearInstances();
        SlotMarkers->AddInstances(MarkerSpots, false, true);
    }
    else if (!MarkerSpots.IsEmpty())
    {
        SlotMarkers->BatchUpdateInstancesTransforms(0, MarkerSpots, true, true, false);
    }
}

void UStrategyInfantryVisualComponent::StepUpSquare()
{
    // Square (user 2026-11-05): a man of the outer rank falls, the nearest man of an inner rank on the same face steps out into his
    // place, and the hole he leaves is filled from further in; the square keeps its places, the holes gather in the inner rank.
    if (!OwnerCompany || !OwnerCompany->FormationComponent || SquarePlaces.Num() != SquarePlaceYaws.Num() || SquarePlaces.IsEmpty() ||
        SoldierSettle.Num() != SoldierComponents.Num()) return;
    const float SquareRankSpacing = FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierRankSpacingCm);
    const auto SquarePlaceOf = [this](int32 SquareMan) { const FSettle& SquareSettle = SoldierSettle[SquareMan]; return SquareSettle.CloseStart >= 0.f ? SquareSettle.CloseTo : SquareSettle.Slot; };
    // Depth is measured from the square's own middle (it stands behind the unit's origin: its front face is the line's front rank).
    FVector SquareMiddle = FVector::ZeroVector;
    for (const FVector& SquareMiddlePlace : SquarePlaces) SquareMiddle += SquareMiddlePlace;
    SquareMiddle /= float(SquarePlaces.Num());
    const auto SquareDepth = [&](const FVector& SquareAt, float SquareYaw) { return float(FVector::DotProduct(SquareAt - SquareMiddle, FRotator(0.f, SquareYaw, 0.f).Vector())); };
    // Each face's outermost depth: a place's rank is counted in from there.
    TMap<int32, float> SquareFaceOuter;
    for (int32 SquarePlace = 0; SquarePlace < SquarePlaces.Num(); ++SquarePlace)
    {
        const int32 SquareFace = FMath::RoundToInt(SquarePlaceYaws[SquarePlace] / 90.f);
        float& SquareOuter = SquareFaceOuter.FindOrAdd(SquareFace, -TNumericLimits<float>::Max());
        SquareOuter = FMath::Max(SquareOuter, SquareDepth(SquarePlaces[SquarePlace], SquarePlaceYaws[SquarePlace]));
    }
    const auto SquareRankOf = [&](const FVector& SquareAt, float SquareYaw)
    {
        const float* SquareOuter = SquareFaceOuter.Find(FMath::RoundToInt(SquareYaw / 90.f));
        return SquareOuter ? FMath::RoundToInt((*SquareOuter - SquareDepth(SquareAt, SquareYaw)) / SquareRankSpacing) : 0;
    };
    const int32 SquareRanks = FMath::Max(1, OwnerCompany->FormationComponent->RankCount);
    int32 SquareMovers = 0;
    for (int32 SquareFillRank = 0; SquareFillRank < SquareRanks - 1; ++SquareFillRank)
        for (int32 SquarePlace = 0; SquarePlace < SquarePlaces.Num(); ++SquarePlace)
        {
            const FVector SquareHole = SquarePlaces[SquarePlace];
            const float SquareYaw = SquarePlaceYaws[SquarePlace];
            if (SquareRankOf(SquareHole, SquareYaw) != SquareFillRank) continue;
            bool bSquareHeld = false;
            for (int32 SquareMan = 0; SquareMan < SoldierSettle.Num() && !bSquareHeld; ++SquareMan)
                bSquareHeld = SoldierSettle[SquareMan].bPlaced && FVector::Dist2D(SquarePlaceOf(SquareMan), SquareHole) < 25.f;
            if (bSquareHeld) continue;
            // The nearest man of a rank further in on the same face.
            int32 SquareFiller = INDEX_NONE;
            float SquareBest = 3.5f * SquareRankSpacing;
            for (int32 SquareMan = 0; SquareMan < SoldierSettle.Num(); ++SquareMan)
            {
                const FSettle& SquareSettle = SoldierSettle[SquareMan];
                if (!SquareSettle.bPlaced || FMath::Abs(FMath::FindDeltaAngleDegrees(SquareSettle.SlotYaw, SquareYaw)) > 10.f) continue;
                const FVector SquareFrom = SquarePlaceOf(SquareMan);
                if (SquareRankOf(SquareFrom, SquareYaw) <= SquareFillRank) continue;
                const float SquareGap = FVector::Dist2D(SquareFrom, SquareHole);
                if (SquareGap < SquareBest) { SquareBest = SquareGap; SquareFiller = SquareMan; }
            }
            if (SquareFiller == INDEX_NONE) continue;
            FSettle& SquareStepper = SoldierSettle[SquareFiller];
            SquareStepper.CloseFrom = SquareStepper.Slot;
            SquareStepper.CloseTo = FVector(SquareHole.X, SquareHole.Y, SquareStepper.Slot.Z);
            SquareStepper.CloseStart = VisualAnimationTime + 0.25f * SquareFillRank;
            SquareStepper.CloseDuration = FMath::Max(0.8f, FVector::Dist2D(SquareStepper.CloseTo, SquareStepper.CloseFrom) / 70.f);
            SquareStepper.Goal = SquareStepper.CloseTo;
            SquareStepper.bActive = true;
            bClosingRanksStep = true;
            ++SquareMovers;
        }
    if (SquareMovers > 0)
        UE_LOG(LogTemp, Display, TEXT("Strategy1864StepUp %s square: %d men step out into the holes of the outer ranks"), *OwnerCompany->StableUnitId.ToString(), SquareMovers);
}

void UStrategyInfantryVisualComponent::StepUpFiles()
{
    if (OwnerCompany && OwnerCompany->FormationComponent && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn)
    {
        StepUpColumn();
        return;
    }
    if (OwnerCompany && OwnerCompany->FormationComponent && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Square)
    {
        StepUpSquare();
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

void UStrategyInfantryVisualComponent::QueueAimedVictims(
    const TArray<TWeakObjectPtr<USkeletalMeshComponent>>& AimedVictims, bool bAimedBearer)
{
    AimedQueuedVictims = AimedVictims;
    if (bAimedBearer && bColourBearerAlive && ColourBearer &&
        !AimedQueuedVictims.Contains(TWeakObjectPtr<USkeletalMeshComponent>(ColourBearer.Get())))
        AimedQueuedVictims.Add(TWeakObjectPtr<USkeletalMeshComponent>(ColourBearer.Get()));
}

void UStrategyInfantryVisualComponent::GroundProbe(const TCHAR* ProbeWhere, const USceneComponent* ProbeSoldier)
{
    // Test (-Strategy1864GroundProbe): every 2 s a company logs man 0's feet against the computed and the visible ground, and
    // the lowest and highest man over his ground.
    static const bool bGroundProbe = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864GroundProbe"));
    if (!bGroundProbe || !ProbeSoldier || !OwnerCompany || !GetWorld()) return;
    const double ProbeNow = GetWorld()->GetTimeSeconds();
    if (ProbeNow - GroundProbeLast < 2.0) return;
    GroundProbeLast = ProbeNow;
    const FVector ProbeFeet = ProbeSoldier->GetComponentLocation();
    FHitResult ProbeHit;
    FCollisionQueryParams ProbeParams(SCENE_QUERY_STAT(Strategy1864GroundProbe), false, OwnerCompany);
    const bool bProbeHit = GetWorld()->LineTraceSingleByChannel(ProbeHit, ProbeFeet + FVector(0, 0, 5000), ProbeFeet - FVector(0, 0, 5000), ECC_Visibility, ProbeParams);
    float ProbeLow = 1.e9f, ProbeHigh = -1.e9f;
    int32 ProbeLowIndex = -1;
    for (int32 ProbeIndex = 0; ProbeIndex < SoldierComponents.Num(); ++ProbeIndex)
    {
        if (!SoldierComponents[ProbeIndex]) continue;
        const FVector ProbeAt = SoldierComponents[ProbeIndex]->GetComponentLocation();
        const float ProbeOff = float(ProbeAt.Z - UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, ProbeAt));
        if (ProbeOff < ProbeLow) { ProbeLow = ProbeOff; ProbeLowIndex = ProbeIndex; }
        ProbeHigh = FMath::Max(ProbeHigh, ProbeOff);
    }
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-GROUNDPROBE %s %s feet=%.0f computed=%.0f visible=%.0f (%s) actor=%.0f men=%d feet-over-ground min=%.0f (man %d) max=%.0f"), *OwnerCompany->StableUnitId.ToString(), ProbeWhere,
        ProbeFeet.Z, UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, ProbeFeet), bProbeHit ? ProbeHit.ImpactPoint.Z : -1.0,
        bProbeHit && ProbeHit.GetActor() ? *ProbeHit.GetActor()->GetName() : TEXT("-"), OwnerCompany->GetActorLocation().Z, SoldierComponents.Num(), ProbeLow, ProbeLowIndex, ProbeHigh);
}

FVector UStrategyInfantryVisualComponent::SoldierOnGround(int32 GroundIndex, const FVector& PlaneWorld, float HeightAboveGround, bool bGroundTarget)
{
    if (!OwnerCompany) return PlaneWorld;
    // Men fall and the list shrinks: then every man samples his ground afresh.
    // So does a unit lifted or lowered more than 2 m (set down on a newly built field, or a long climb): the samples were taken
    // on the old ground, and a man who has not moved sideways would keep them.
    const float GroundUnitZ = float(OwnerCompany->GetActorLocation().Z);
    if (SoldierGroundSamples.Num() != SoldierComponents.Num() || FMath::Abs(GroundUnitZ - SoldierGroundUnitZ) > 200.f)
    {
        SoldierGroundSamples.Init(FVector(0.f, 0.f, -1.e9f), SoldierComponents.Num());
        SoldierGroundTargetSamples.Init(FVector(0.f, 0.f, -1.e9f), SoldierComponents.Num());
        SoldierGroundUnitZ = GroundUnitZ;
    }
    // The target and the man's own place each keep their sample: one shared would be taken afresh twice a frame on the march.
    TArray<FVector>& GroundSamples = bGroundTarget ? SoldierGroundTargetSamples : SoldierGroundSamples;
    if (!GroundSamples.IsValidIndex(GroundIndex)) return PlaneWorld;
    FVector& GroundSample = GroundSamples[GroundIndex];
    if (GroundSample.Z < -1.e8f || FVector::DistSquared2D(GroundSample, PlaneWorld) > 30.f * 30.f)
    {
        GroundSample = FVector(PlaneWorld.X, PlaneWorld.Y, UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(this, PlaneWorld));
    }
    // His height above the ground is his slot's own (feet, kneeling), not the height his place has in the unit's frame: that
    // frame may lean with the slope (a man 20 m down the column would sink metres), and the origin may cut a crest between the
    // route's points. The men follow the ground, not the origin.
    return FVector(PlaneWorld.X, PlaneWorld.Y, GroundSample.Z + HeightAboveGround);
}

void UStrategyInfantryVisualComponent::KillSoldiers(int32 Count, const FVector* Near, const FVector* ConeOrigin)
{
    // Remove only the chosen figures; all surviving figure identities and slots remain intact.
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const bool bMoving = OwnerCompany && OwnerCompany->HumanAnimationStateComponent &&
        (OwnerCompany->HumanAnimationStateComponent->CurrentAction == EStrategyHumanAnimationAction::Walk ||
         OwnerCompany->HumanAnimationStateComponent->CurrentAction == EStrategyHumanAnimationAction::Run);
    int32 AimedQueueCursor = 0;
    for (int32 k = 0; k < Count && (SoldierComponents.Num() > 0 || AimedQueueCursor < AimedQueuedVictims.Num()); ++k)
    {
        int32 AimedChosenIndex = INDEX_NONE;
        bool AimedChosenBearer = false;
        while (AimedQueueCursor < AimedQueuedVictims.Num())
        {
            USkeletalMeshComponent* AimedQueuedMan = AimedQueuedVictims[AimedQueueCursor++].Get();
            if (!IsValid(AimedQueuedMan)) continue;
            if (bColourBearerAlive && AimedQueuedMan == ColourBearer.Get()) { AimedChosenBearer = true; break; }
            AimedChosenIndex = SoldierComponents.IndexOfByKey(AimedQueuedMan);
            if (AimedChosenIndex != INDEX_NONE) break;
        }
        if (AimedChosenBearer) { KillColourBearer(); continue; }
        if (SoldierComponents.IsEmpty()) break;
        // The colour bearer is one man of the company: he falls as often as any other (front-rank fire hits him more).
        if (AimedChosenIndex == INDEX_NONE && bColourBearerAlive && ColourBearer && !(Near && ConeOrigin) && FMath::RandRange(0, SoldierComponents.Num()) == 0)
        {
            KillColourBearer();
            continue;
        }
        int32 i = AimedChosenIndex != INDEX_NONE ? AimedChosenIndex : FMath::RandRange(0, SoldierComponents.Num() - 1);
        if (AimedChosenIndex == INDEX_NONE && Near && ConeOrigin)
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
        if (AimedChosenIndex == INDEX_NONE && OwnerCompany && !OwnerCompany->CasualtySourceLocation.IsNearlyZero() && !(Near && ConeOrigin))
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
            // Marching with shouldered arms (a column): one of the three deaths that start from that very pose, so he does not jump out
            // of the march into a standing death (user 2026-10-22).
            const UAnimSequence* DeathFromClip = SoldierClips.IsValidIndex(i) ? SoldierClips[i].Clip.Get() : nullptr;
            const bool bDeathFromShoulder = DeathFromClip && (DeathFromClip == WalkShoulderAsset.Get() || DeathFromClip == ShoulderUpAsset.Get() ||
                DeathFromClip == ShoulderDownAsset.Get()) && DeathMarchFrontAsset.Get() && DeathMarchBackAsset.Get() && DeathMarchCrumpleAsset.Get();
            const TSoftObjectPtr<UAnimSequence>& Death = bDeathFromShoulder ?
                (Variant == 0 ? DeathMarchFrontAsset : Variant == 1 ? DeathMarchCrumpleAsset : DeathMarchBackAsset)
                : bMoving && !DeathWalkingAsset.IsNull() && FMath::FRand() < 0.5f ? DeathWalkingAsset
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
    AimedQueuedVictims.Reset();
    // A wiped-out company leaves no colour bearer standing alone, aimed fire or not.
    if (SoldierComponents.IsEmpty() && bColourBearerAlive && ColourBearer && OwnerCompany && OwnerCompany->CurrentStrength == 0)
        KillColourBearer();
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
    const TSoftObjectPtr<UAnimSequence>* BattleAnimationAssets[] = { &IdleStandingAsset, &WalkStandingAsset, &RunStandingAsset, &AimStandingAsset, &FireStandingAsset, &ReloadStandingAsset, &IdleKneelingAsset, &AimKneelingAsset, &FireKneelingAsset, &ReloadKneelingAsset, &IdleProneAsset, &CrawlProneAsset, &FireProneAsset, &ReloadProneAsset, &BayonetChargeAsset, &BayonetThrustAsset, &DeathAsset, &DeathAsset2, &DeathAsset3, &RaiseToAimAsset, &LoadAsset, &RiseFromLoadAsset, &ReadyAsset, &AimHoldAsset, &DeathWalkingAsset, &DeathMarchFrontAsset, &DeathMarchBackAsset, &DeathMarchCrumpleAsset, &WalkShoulderAsset, &ShoulderUpAsset, &ShoulderDownAsset, &SideStepAsset, &SideStepLeftAsset, &ColourWalkAsset, &ColourIdleAsset };
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
            const FVector InitialPlane = VisualPath.bInitialized ? VisualPath.Goal(Settle.Slot, false, InitialDrawnYaw)
                : OwnerCompany->GetActorTransform().TransformPosition(Settle.Slot);
            Soldier->SetRelativeLocation(OwnerCompany->GetActorTransform().InverseTransformPosition(SoldierOnGround(VisualIndex, InitialPlane, float(Settle.Slot.Z))));
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
    // A square's places are kept as formed, so its holes can be filled from within (StepUpSquare).
    if (!bScaleOnlyRebuild)
    {
        SquarePlaces.Reset();
        SquarePlaceYaws.Reset();
        if (OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Square)
            for (const FSettle& SquarePlaceMan : SoldierSettle) { SquarePlaces.Add(SquarePlaceMan.Slot); SquarePlaceYaws.Add(SquarePlaceMan.SlotYaw); }
    }
    // (A square forms orderly too: each man walks to his place. Left out, the men stood in the square in 0.2 s: QA 2026-11-05.)
    if (!bScaleOnlyRebuild && VisualPath.bInitialized &&
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
    SquareSwingRunning = false;
    bSquareWrapRunning = false;
    for (FSettle& SquareSwingClear : SoldierSettle) SquareSwingClear.SquareSwingKind = 0;
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
    // Into and out of a square the men keep their order (user 2026-11-05, QA: "de vælter rundt"; the two ends of the line crossed in
    // the middle and piled up): rank by rank, the line from left to right is laid round the square from the rear of its left face,
    // up that face, across the front and down the right face, the two ends closing the rear - the wings fold back. A square leaves
    // the same way into the line. Order kept, the paths do not cross.
    const bool bReformToSquare = OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Square;
    const bool bReformFromSquare = CachedFormationValue == uint8(EStrategyFormationType::Square) && !bReformToSquare;
    // Line <-> square: the wings swing (PlanSquareSwing). Where that cannot be done the ordered straight walk below stands in.
    float SquareSwingBudget = 0.f;
    const bool bSquareSwingPlannedLine = !bDestination && ReformMen.Num() == ReformTargets.Num() &&
        (bReformToSquare ? CachedFormationValue == uint8(EStrategyFormationType::Line) :
            bReformFromSquare && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Line) &&
        PlanSquareSwing(bReformToSquare, ReformFrame, ReformMen, ReformSlots, ReformStarts, ReformTargets, SquareSwingBudget);
    // March column <-> square: the column marches into the square and round it (PlanSquareWrap).
    bSquareWrapRunning = !bSquareSwingPlannedLine && !bDestination && ReformMen.Num() == ReformTargets.Num() &&
        (bReformToSquare ? CachedFormationValue == uint8(EStrategyFormationType::MarchColumn) :
            bReformFromSquare && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn) &&
        PlanSquareWrap(bReformToSquare, ReformFrame, ReformMen, ReformSlots, ReformStarts, ReformTargets, SquareSwingBudget);
    const bool bSquareSwingPlanned = bSquareSwingPlannedLine || bSquareWrapRunning;
    if (bSquareSwingPlanned) SquareSwingPlanFrame = ReformFrame;
    // Out of the square into column the column forms straight behind the unit, where the plan laid it: the old march's trail is
    // let go (2026-11-15: the column's places lay on the trail, a metre or two beside the plan's, and the men were drawn across to
    // them each by his own progress - neighbours came within 5 cm).
    if (bSquareWrapRunning && !bReformToSquare) { VisualPath.Trail.Reset(); VisualPath.TrailDistance = 0.f; }
    if (!bSquareSwingPlanned && !bDestination && (bReformToSquare || bReformFromSquare) && ReformMen.Num() == ReformTargets.Num())
    {
        // A place's rank and its order round the formation: in a square the rank counts in from the outer face and the order is
        // the angle about the centre (rear-left -180 .. front 0 .. rear-right +180); in a line the rank counts back from the front
        // and the order is left to right.
        const auto ReformSquareKey = [&](const TArray<FVector>& ReformPoints, bool bReformSquare, TArray<int32>& OutRanks, TArray<float>& OutOrder)
        {
            // A square is measured from its own middle (it stands behind the unit's origin).
            FVector ReformMiddle = FVector::ZeroVector;
            if (bReformSquare && ReformPoints.Num() > 0) { for (const FVector& ReformMid : ReformPoints) ReformMiddle += ReformMid; ReformMiddle /= float(ReformPoints.Num()); }
            const auto ReformDepthOf = [&](const FVector& ReformPoint)
            { const FVector ReformRel = ReformPoint - ReformMiddle; return bReformSquare ? FMath::Max(FMath::Abs(float(ReformRel.X)), FMath::Abs(float(ReformRel.Y))) : float(ReformPoint.X); };
            float ReformOuter = -TNumericLimits<float>::Max();
            for (const FVector& ReformPoint : ReformPoints) ReformOuter = FMath::Max(ReformOuter, ReformDepthOf(ReformPoint));
            OutRanks.Reset(); OutOrder.Reset();
            for (const FVector& ReformPoint : ReformPoints)
            {
                const FVector ReformRel = ReformPoint - ReformMiddle;
                OutRanks.Add(FMath::RoundToInt((ReformOuter - ReformDepthOf(ReformPoint)) / ReformRankSpacing));
                OutOrder.Add(bReformSquare ? FMath::RadiansToDegrees(FMath::Atan2(float(ReformRel.Y), float(ReformRel.X))) : float(ReformPoint.Y));
            }
        };
        TArray<FVector> ReformManPoints;
        for (int32 ReformMan : ReformMen) ReformManPoints.Add(ReformSourceFrame.InverseTransformPosition(ReformStarts[ReformMan]));
        TArray<int32> ReformManRanks, ReformSlotRanks;
        TArray<float> ReformManOrder, ReformSlotOrder;
        ReformSquareKey(ReformManPoints, bReformFromSquare, ReformManRanks, ReformManOrder);
        ReformSquareKey(ReformSlots, bReformToSquare, ReformSlotRanks, ReformSlotOrder);
        TArray<int32> ReformManOrderIndex, ReformSlotOrderIndex;
        for (int32 ReformK = 0; ReformK < ReformMen.Num(); ++ReformK) { ReformManOrderIndex.Add(ReformK); ReformSlotOrderIndex.Add(ReformK); }
        ReformManOrderIndex.Sort([&](int32 A, int32 B) { return ReformManRanks[A] != ReformManRanks[B] ? ReformManRanks[A] < ReformManRanks[B] : ReformManOrder[A] < ReformManOrder[B]; });
        ReformSlotOrderIndex.Sort([&](int32 A, int32 B) { return ReformSlotRanks[A] != ReformSlotRanks[B] ? ReformSlotRanks[A] < ReformSlotRanks[B] : ReformSlotOrder[A] < ReformSlotOrder[B]; });
        if (bReformFromSquare && bReformColumn)
        {
            // Square to column (QA 2026-11-12: "alle løber på tværs i en uformelig sværm", the outer rank went to the front third of
            // the column from all four faces): the men are taken round the square file by file (by angle, the outer man of a file
            // first), the column's places row by row along its length, so each face files into its own stretch of the column. The
            // column is walked from whichever end makes the shorter way.
            FVector ReformColumnMin(TNumericLimits<float>::Max()), ReformColumnMax(-TNumericLimits<float>::Max());
            for (const FVector& ReformColumnSlot : ReformSlots) { ReformColumnMin = ReformColumnMin.ComponentMin(ReformColumnSlot); ReformColumnMax = ReformColumnMax.ComponentMax(ReformColumnSlot); }
            const bool bReformColumnAlongX = (ReformColumnMax.X - ReformColumnMin.X) >= (ReformColumnMax.Y - ReformColumnMin.Y);
            const auto ReformColumnLength = [&](int32 ReformSlotIndex) { return bReformColumnAlongX ? float(ReformSlots[ReformSlotIndex].X) : float(ReformSlots[ReformSlotIndex].Y); };
            const auto ReformColumnAcross = [&](int32 ReformSlotIndex) { return bReformColumnAlongX ? float(ReformSlots[ReformSlotIndex].Y) : float(ReformSlots[ReformSlotIndex].X); };
            ReformSlotOrderIndex.Sort([&](int32 A, int32 B) { return !FMath::IsNearlyEqual(ReformColumnLength(A), ReformColumnLength(B), 10.f) ? ReformColumnLength(A) > ReformColumnLength(B) : ReformColumnAcross(A) < ReformColumnAcross(B); });
            // The ring round the square is opened at one of its four corners (never in the middle of a face, which would send the
            // face's two halves to the two ends of the column), the corner and the column's end chosen for the shortest ways.
            const int32 ReformColumnCount = FMath::Min(ReformManOrderIndex.Num(), ReformSlotOrderIndex.Num());
            double ReformBest = TNumericLimits<double>::Max();
            TArray<int32> ReformBestMen;
            bool bReformBestReversed = false;
            for (const float ReformCut : { -135.f, -45.f, 45.f, 135.f })
            {
                TArray<int32> ReformCutMen = ReformManOrderIndex;
                const auto ReformCutKey = [&](int32 ReformIndex) { return FMath::Fmod(ReformManOrder[ReformIndex] - ReformCut + 720.f, 360.f); };
                ReformCutMen.Sort([&](int32 A, int32 B) { return !FMath::IsNearlyEqual(ReformCutKey(A), ReformCutKey(B), 4.f) ? ReformCutKey(A) < ReformCutKey(B) : ReformManRanks[A] < ReformManRanks[B]; });
                for (const bool bReformReversed : { false, true })
                {
                    double ReformSum = 0.;
                    for (int32 ReformK = 0; ReformK < ReformColumnCount; ++ReformK)
                        ReformSum += FVector::Dist2D(ReformStarts[ReformMen[ReformCutMen[ReformK]]],
                            ReformEndpoints[ReformSlotOrderIndex[bReformReversed ? ReformColumnCount - 1 - ReformK : ReformK]]);
                    if (ReformSum < ReformBest) { ReformBest = ReformSum; ReformBestMen = ReformCutMen; bReformBestReversed = bReformReversed; }
                }
            }
            ReformManOrderIndex = ReformBestMen;
            if (bReformBestReversed) Algo::Reverse(ReformSlotOrderIndex);
        }
        const TArray<int32> ReformMenBefore = ReformMen;
        for (int32 ReformK = 0; ReformK < ReformMen.Num(); ++ReformK)
        {
            ReformMen[ReformK] = ReformMenBefore[ReformManOrderIndex[ReformK]];
            ReformTargets[ReformK] = ReformSlotOrderIndex[ReformK];
        }
    }
    // Uncross straight segments by exchanging endpoints. Every exchange strictly shortens total travel.
    // Collinear paths also exchange when the shorter pairing removes overtaking.
    FormationCrossingsAvoided = 0;
    bool bReformExchanged = !bSquareSwingPlanned;   // the swing's pairing is its own: an exchange would break the arm
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
    if (bSquareSwingPlanned) ReformBudget = SquareSwingBudget;
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
        if (bSquareSwingPlanned)
        {
            // One clock for the wings; the front only closes in.
            // (The men who only step to their place take the first swing's time: sooner, the corner man stood where the wing's
            // first man had not yet left.)
            // (Column <-> square: all on the one clock, as the plan was walked through. Out of the square each man with his own start
            // and his own half-second to get going was tried first: the men with a short way went at 5 m/s and ran into the others,
            // 5 cm measured where the plan had 41, 2026-11-15.)
            if (ReformSettle.SquareSwingKind == 0 && !bSquareWrapRunning)
            {
                ReformSettle.ReformDuration = FMath::Max(0.6f, ReformBudget * SquareSwingSplit);
                // Out of the square the rear opens first; the wings' first swing, and these men with it, comes after.
                if (!bReformToSquare) ReformSettle.ReformStartTime = VisualAnimationTime + FMath::Max(0.f, ReformBudget - ReformSettle.ReformDuration);
            }
            ReformSettle.ReformDeadline = ReformSettle.ReformStartTime + ReformSettle.ReformDuration;
        }
        else if (!bDestination && !bReformColumn)
        {
            // Each man his own walk (user 2026-11-05, QA: "det er ikke flydende" - all set off in one frame, the far men sprinted at
            // 7.5 m/s and all clicked into place together): a walking pace with half a second to get going and to stop, and the files
            // setting off one after the other (rank by rank a little later), so the company moves as men, not as one body.
            const float ReformManPath = float(FVector::Dist2D(ReformStart, ReformSettle.ReformTarget));
            const float ReformManPace = 220.f;
            // (Tried 2026-11-07: the longest walks first so all land within three seconds - Allan: "meget værre", 140 men landed at
            // once in a pile. Back to the file-by-file start, which he found much better.)
            const float ReformManDelay = FMath::Min(1.6f, 0.025f * float(ReformOrder % FMath::Max(1, ReformMen.Num() / FMath::Max(1, OwnerCompany->FormationComponent->RankCount))))
                + 0.15f * float(ReformOrder / FMath::Max(1, ReformMen.Num() / FMath::Max(1, OwnerCompany->FormationComponent->RankCount)));
            ReformSettle.ReformStartTime = VisualAnimationTime + ReformManDelay;
            ReformSettle.ReformDuration = FMath::Max(0.6f, ReformManPath / ReformManPace + 0.5f);
            ReformSettle.ReformDeadline = ReformSettle.ReformStartTime + ReformSettle.ReformDuration;
        }
        ReformSettle.WorldVelocity = FVector::ZeroVector;
        ReformSettle.bActive = true;
    }
}

namespace
{
    // A point a given way along a man's way round the square (up to five corners).
    FVector2D SquareWrapAlong(const FVector2D* SquareWrapPath, int32 SquareWrapPoints, float SquareWrapDistance)
    {
        for (int32 SquareWrapLeg = 0; SquareWrapLeg + 1 < SquareWrapPoints; ++SquareWrapLeg)
        {
            const float SquareWrapLegLength = float(FVector2D::Distance(SquareWrapPath[SquareWrapLeg], SquareWrapPath[SquareWrapLeg + 1]));
            if (SquareWrapDistance <= SquareWrapLegLength || SquareWrapLeg + 2 == SquareWrapPoints)
                return FMath::Lerp(SquareWrapPath[SquareWrapLeg], SquareWrapPath[SquareWrapLeg + 1],
                    SquareWrapLegLength > 0.f ? FMath::Clamp(SquareWrapDistance / SquareWrapLegLength, 0.f, 1.f) : 0.f);
            SquareWrapDistance -= SquareWrapLegLength;
        }
        return SquareWrapPoints > 0 ? SquareWrapPath[SquareWrapPoints - 1] : FVector2D::ZeroVector;
    }
}

FVector UStrategyInfantryVisualComponent::SquareWrapPoint(const FSettle& SquareWrapSettle, float SquareWrapTravel) const
{
    const FVector2D SquareWrapAt = SquareWrapAlong(SquareWrapSettle.SquareWrapPath, SquareWrapSettle.SquareWrapPoints, FMath::Clamp(SquareWrapTravel, 0.f, SquareWrapSettle.SquareWrapDelta));
    return FVector(SquareWrapAt.X, SquareWrapAt.Y, SquareWrapSettle.Slot.Z);
}

bool UStrategyInfantryVisualComponent::PlanSquareWrap(bool bSquareWrapInto, const FTransform& SquareWrapFrame, const TArray<int32>& SquareWrapMen,
    const TArray<FVector>& SquareWrapSlots, const TArray<FVector>& SquareWrapStarts, TArray<int32>& SquareWrapTargets, float& OutSquareWrapSeconds)
{
    // March column <-> square (Allan 2026-11-15: "Ja, videre med kolonne"). The column marches on into the square it is to form:
    // the men of the two outer files turn out to their own side at the front and go round that side, the middle file's men turn
    // right and left by turns along the front rank; each goes to the far end of his ring first, so nobody overtakes. The last rows
    // close the two places the column itself came through: the middle of the front, then the middle of the rear. Out of the square
    // the same film runs back. The ways are laid here and then walked through in thought: if two men come nearer than 30 cm
    // anywhere on the way the plan is dropped and the straight walk is used as before.
    const TCHAR* SquareWrapKindText = bSquareWrapInto ? TEXT("column-to-square") : TEXT("square-to-column");
    const auto SquareWrapReject = [&](const TCHAR* SquareWrapWhy)
    {
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s start kind=%s men=%d accepted=0 reason=%s"),
            *OwnerCompany->StableUnitId.ToString(), SquareWrapKindText, SquareWrapMen.Num(), SquareWrapWhy);
        return false;
    };
    const int32 SquareWrapCount = SquareWrapMen.Num();
    if (SquareWrapCount < 24 || SquareWrapCount != SquareWrapSlots.Num() || SquareWrapCount != SquareWrapTargets.Num()) return SquareWrapReject(TEXT("too-few-or-uneven"));
    if (OwnerCompany->StanceComponent && OwnerCompany->StanceComponent->Stance == EStrategyStance::Prone) return SquareWrapReject(TEXT("prone"));
    if (OwnerCompany->SkirmisherComponent && (OwnerCompany->SkirmisherComponent->State == EStrategySkirmisherState::Deploying || OwnerCompany->SkirmisherComponent->IsDeployed()))
        return SquareWrapReject(TEXT("screen-out"));
    const int32 SquareWrapRanks = FMath::Max(1, OwnerCompany->FormationComponent->RankCount);
    if (OwnerCompany->FormationComponent->ColumnWidth != 3) return SquareWrapReject(TEXT("column-not-three-wide"));
    const float SquareWrapRankSpacing = FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierRankSpacingCm);
    const float SquareWrapFileSpacing = FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierLateralSpacingCm);
    TArray<FVector> SquareWrapStandPoints;
    for (int32 SquareWrapMan : SquareWrapMen) SquareWrapStandPoints.Add(SquareWrapFrame.InverseTransformPosition(SquareWrapStarts[SquareWrapMan]));
    const TArray<FVector>& SquareWrapColumnPoints = bSquareWrapInto ? SquareWrapStandPoints : SquareWrapSlots;
    const TArray<FVector>& SquareWrapRingPoints = bSquareWrapInto ? SquareWrapSlots : SquareWrapStandPoints;
    float SquareWrapFront = -TNumericLimits<float>::Max(), SquareWrapRear = TNumericLimits<float>::Max(), SquareWrapWide = 0.f;
    for (const FVector& SquareWrapRingPoint : SquareWrapRingPoints)
    {
        SquareWrapFront = FMath::Max(SquareWrapFront, float(SquareWrapRingPoint.X));
        SquareWrapRear = FMath::Min(SquareWrapRear, float(SquareWrapRingPoint.X));
        SquareWrapWide = FMath::Max(SquareWrapWide, FMath::Abs(float(SquareWrapRingPoint.Y)));
    }
    const float SquareWrapLength = SquareWrapFront - SquareWrapRear;
    if (SquareWrapWide < 2.f * SquareWrapRankSpacing || FMath::Abs(SquareWrapLength - 2.f * SquareWrapWide) > SquareWrapRankSpacing) return SquareWrapReject(TEXT("not-a-square"));
    // The places of the square: ring, side of the column's axis, and how far along the ring from the middle of the front.
    struct FSquareWrapPlace { int32 Index = INDEX_NONE; int32 Ring = 0; int32 Side = 1; float Along = 0.f; FVector2D At = FVector2D::ZeroVector; bool bRear = false; bool bFront = false; bool bUsed = false; };
    struct FSquareWrapWalker { int32 Index = INDEX_NONE; FVector2D At = FVector2D::ZeroVector; int32 Row = 0; int32 Lane = 0; bool bDone = false; };
    struct FSquareWrapWay { int32 Walker = INDEX_NONE; int32 Place = INDEX_NONE; FVector2D Path[5]; int32 Points = 0; float Delta = 0.f; };
    const float SquareWrapTolerance = 0.3f * SquareWrapRankSpacing;
    TArray<FSquareWrapPlace> SquareWrapPlaces;
    for (int32 SquareWrapRingIndex = 0; SquareWrapRingIndex < SquareWrapRingPoints.Num(); ++SquareWrapRingIndex)
    {
        const FVector& SquareWrapRingPoint = SquareWrapRingPoints[SquareWrapRingIndex];
        const float SquareWrapFromFront = SquareWrapFront - float(SquareWrapRingPoint.X), SquareWrapFromRear = float(SquareWrapRingPoint.X) - SquareWrapRear;
        const float SquareWrapFromLeft = float(SquareWrapRingPoint.Y) + SquareWrapWide, SquareWrapFromRight = SquareWrapWide - float(SquareWrapRingPoint.Y);
        FSquareWrapPlace SquareWrapPlace;
        SquareWrapPlace.Index = SquareWrapRingIndex;
        SquareWrapPlace.Ring = FMath::RoundToInt32(FMath::Min(FMath::Min(SquareWrapFromFront, SquareWrapFromRear), FMath::Min(SquareWrapFromLeft, SquareWrapFromRight)) / SquareWrapRankSpacing);
        if (SquareWrapPlace.Ring < 0 || SquareWrapPlace.Ring >= SquareWrapRanks) return SquareWrapReject(TEXT("ring-out-of-range"));
        const float SquareWrapRingDepth = SquareWrapPlace.Ring * SquareWrapRankSpacing;
        const float SquareWrapRingHalf = SquareWrapWide - SquareWrapRingDepth;
        SquareWrapPlace.Side = SquareWrapRingPoint.Y >= 0. ? 1 : -1;
        SquareWrapPlace.At = FVector2D(SquareWrapRingPoint.X, SquareWrapRingPoint.Y);
        const bool bSquareWrapOnFront = FMath::Abs(SquareWrapFromFront - SquareWrapRingDepth) <= SquareWrapTolerance;
        const bool bSquareWrapOnRear = FMath::Abs(SquareWrapFromRear - SquareWrapRingDepth) <= SquareWrapTolerance;
        const bool bSquareWrapOnSide = FMath::Abs((SquareWrapPlace.Side > 0 ? SquareWrapFromRight : SquareWrapFromLeft) - SquareWrapRingDepth) <= SquareWrapTolerance;
        if (bSquareWrapOnFront) SquareWrapPlace.Along = FMath::Abs(float(SquareWrapRingPoint.Y));
        else if (bSquareWrapOnSide) SquareWrapPlace.Along = SquareWrapRingHalf + (SquareWrapFromFront - SquareWrapRingDepth);
        else if (bSquareWrapOnRear) SquareWrapPlace.Along = SquareWrapRingHalf + (SquareWrapLength - 2.f * SquareWrapRingDepth) + (SquareWrapRingHalf - FMath::Abs(float(SquareWrapRingPoint.Y)));
        else return SquareWrapReject(TEXT("square-not-formed"));
        SquareWrapPlace.bFront = bSquareWrapOnFront;
        SquareWrapPlace.bRear = bSquareWrapOnRear && !bSquareWrapOnSide && !bSquareWrapOnFront;
        SquareWrapPlaces.Add(SquareWrapPlace);
    }
    // The column: three files along the unit's own axis, row behind row.
    float SquareWrapHead = -TNumericLimits<float>::Max();
    for (const FVector& SquareWrapColumnPoint : SquareWrapColumnPoints) SquareWrapHead = FMath::Max(SquareWrapHead, float(SquareWrapColumnPoint.X));
    if (SquareWrapHead > SquareWrapFront + SquareWrapRankSpacing) return SquareWrapReject(TEXT("column-ahead-of-square"));
    // The column as it stands: on the march its files are a rank's spacing apart and its rows a file's (the line turned to its
    // flank), formed up at the halt the other way round. Both are read off the men themselves.
    float SquareWrapLaneSpacing = 0.f;
    for (const FVector& SquareWrapColumnPoint : SquareWrapColumnPoints) SquareWrapLaneSpacing = FMath::Max(SquareWrapLaneSpacing, FMath::Abs(float(SquareWrapColumnPoint.Y)));
    if (SquareWrapLaneSpacing < 0.5f * SquareWrapFileSpacing || SquareWrapLaneSpacing > 1.6f * SquareWrapRankSpacing) return SquareWrapReject(TEXT("column-not-straight"));
    TArray<FSquareWrapWalker> SquareWrapWalkers;
    TArray<float> SquareWrapRowLevels;
    for (int32 SquareWrapColumnIndex = 0; SquareWrapColumnIndex < SquareWrapColumnPoints.Num(); ++SquareWrapColumnIndex)
    {
        const FVector& SquareWrapColumnPoint = SquareWrapColumnPoints[SquareWrapColumnIndex];
        FSquareWrapWalker SquareWrapWalker;
        SquareWrapWalker.Index = SquareWrapColumnIndex;
        SquareWrapWalker.At = FVector2D(SquareWrapColumnPoint.X, SquareWrapColumnPoint.Y);
        SquareWrapWalker.Lane = FMath::RoundToInt32(float(SquareWrapColumnPoint.Y) / SquareWrapLaneSpacing);
        if (FMath::Abs(SquareWrapWalker.Lane) > 1 || FMath::Abs(float(SquareWrapColumnPoint.Y) - SquareWrapWalker.Lane * SquareWrapLaneSpacing) > 0.25f * SquareWrapLaneSpacing)
            return SquareWrapReject(TEXT("column-not-straight"));
        bool bSquareWrapKnownRow = false;
        for (const float SquareWrapRowLevel : SquareWrapRowLevels) bSquareWrapKnownRow |= FMath::Abs(SquareWrapRowLevel - float(SquareWrapColumnPoint.X)) < 20.f;
        if (!bSquareWrapKnownRow) SquareWrapRowLevels.Add(float(SquareWrapColumnPoint.X));
        SquareWrapWalkers.Add(SquareWrapWalker);
    }
    SquareWrapRowLevels.Sort([](float A, float B) { return A > B; });
    for (FSquareWrapWalker& SquareWrapWalker : SquareWrapWalkers)
        for (int32 SquareWrapRowIndex = 0; SquareWrapRowIndex < SquareWrapRowLevels.Num(); ++SquareWrapRowIndex)
            if (FMath::Abs(SquareWrapRowLevels[SquareWrapRowIndex] - float(SquareWrapWalker.At.X)) < 20.f) { SquareWrapWalker.Row = SquareWrapRowIndex; break; }
    const TArray<FSquareWrapPlace> SquareWrapPlacesClean = SquareWrapPlaces;
    const TArray<FSquareWrapWalker> SquareWrapWalkersClean = SquareWrapWalkers;
    // Laid twice (the front closed by just the places in the column's way, or by whole rows) and the better one walked.
    TArray<FSquareWrapWay> SquareWrapBestWays;
    float SquareWrapBestNearest = -1.f, SquareWrapBestLongest = 0.f;
    int32 SquareWrapBestLeftOver = 0;
    const TCHAR* SquareWrapFailure = TEXT("no-plan");
    for (int32 SquareWrapAttempt = 0; SquareWrapAttempt < 2; ++SquareWrapAttempt)
    {
    const bool bSquareWrapWholeRows = SquareWrapAttempt == 1;
    SquareWrapPlaces = SquareWrapPlacesClean;
    SquareWrapWalkers = SquareWrapWalkersClean;
    bool bSquareWrapAttemptFailed = false;
    TArray<FSquareWrapWay> SquareWrapWays;
    // The places in the column's own way (within a man's width of one of its three files) are held back for the last rows: they
    // march straight up and step aside into them. Each time the rearmost men free take them; of these the last to come up takes
    // the place nearest his own file, those before him step further aside.
    const float SquareWrapInTheWay = SquareWrapLaneSpacing + 45.f;
    // Which places are held back: 1 the rear's middle (the gate), 2 the front's middle (closed last but for the gate).
    TArray<uint8> SquareWrapHeldFor;
    SquareWrapHeldFor.SetNumZeroed(SquareWrapPlaces.Num());
    int32 SquareWrapHeldCount = 0;
    for (int32 SquareWrapPlaceIndex = 0; SquareWrapPlaceIndex < SquareWrapPlaces.Num(); ++SquareWrapPlaceIndex)
    {
        const FSquareWrapPlace& SquareWrapPlace = SquareWrapPlaces[SquareWrapPlaceIndex];
        if (FMath::Abs(float(SquareWrapPlace.At.Y)) >= SquareWrapInTheWay) continue;
        SquareWrapHeldFor[SquareWrapPlaceIndex] = SquareWrapPlace.bRear ? 1 : SquareWrapPlace.bFront ? 2 : 0;
        SquareWrapHeldCount += SquareWrapHeldFor[SquareWrapPlaceIndex] != 0 ? 1 : 0;
    }
    if (bSquareWrapWholeRows)
    {
        // Whole rows close the front: the next front places from the middle outwards are held back too, until the rows come out even.
        for (int32 SquareWrapPad = (3 - SquareWrapHeldCount % 3) % 3; SquareWrapPad > 0; --SquareWrapPad)
        {
            int32 SquareWrapNext = INDEX_NONE;
            for (int32 SquareWrapPlaceIndex = 0; SquareWrapPlaceIndex < SquareWrapPlaces.Num(); ++SquareWrapPlaceIndex)
            {
                const FSquareWrapPlace& SquareWrapPlace = SquareWrapPlaces[SquareWrapPlaceIndex];
                if (!SquareWrapPlace.bFront || SquareWrapHeldFor[SquareWrapPlaceIndex] != 0) continue;
                if (SquareWrapNext == INDEX_NONE || FMath::Abs(SquareWrapPlace.At.Y) < FMath::Abs(SquareWrapPlaces[SquareWrapNext].At.Y) - 0.5 ||
                    (FMath::Abs(SquareWrapPlace.At.Y) < FMath::Abs(SquareWrapPlaces[SquareWrapNext].At.Y) + 0.5 && SquareWrapPlace.Ring > SquareWrapPlaces[SquareWrapNext].Ring))
                    SquareWrapNext = SquareWrapPlaceIndex;
            }
            if (SquareWrapNext == INDEX_NONE) break;
            SquareWrapHeldFor[SquareWrapNext] = 2;
        }
    }
    const auto SquareWrapReserve = [&](int32 SquareWrapRing, bool bSquareWrapRearFace, bool bSquareWrapMiddleFirst)
    {
        TArray<int32> SquareWrapHeld;
        for (int32 SquareWrapPlaceIndex = 0; SquareWrapPlaceIndex < SquareWrapPlaces.Num(); ++SquareWrapPlaceIndex)
        {
            const FSquareWrapPlace& SquareWrapPlace = SquareWrapPlaces[SquareWrapPlaceIndex];
            if (!SquareWrapPlace.bUsed && SquareWrapPlace.Ring == SquareWrapRing && SquareWrapHeldFor[SquareWrapPlaceIndex] == (bSquareWrapRearFace ? 1 : 2))
                SquareWrapHeld.Add(SquareWrapPlaceIndex);
        }
        TArray<int32> SquareWrapFree;
        for (int32 SquareWrapWalkerIndex = 0; SquareWrapWalkerIndex < SquareWrapWalkers.Num(); ++SquareWrapWalkerIndex)
            if (!SquareWrapWalkers[SquareWrapWalkerIndex].bDone) SquareWrapFree.Add(SquareWrapWalkerIndex);
        SquareWrapFree.Sort([&](int32 A, int32 B)
        {
            const FSquareWrapWalker& WalkerA = SquareWrapWalkers[A]; const FSquareWrapWalker& WalkerB = SquareWrapWalkers[B];
            if (WalkerA.Row != WalkerB.Row) return WalkerA.Row > WalkerB.Row;
            if (FMath::Abs(WalkerA.Lane) != FMath::Abs(WalkerB.Lane)) return bSquareWrapMiddleFirst ? FMath::Abs(WalkerA.Lane) < FMath::Abs(WalkerB.Lane) : FMath::Abs(WalkerA.Lane) > FMath::Abs(WalkerB.Lane);
            return WalkerA.Lane < WalkerB.Lane;
        });
        SquareWrapFree.SetNum(FMath::Min(SquareWrapFree.Num(), SquareWrapHeld.Num()));
        // The men and the places in order across; of two men of one file the one behind takes the place nearer the file (he comes
        // last and steps the least aside).
        SquareWrapFree.Sort([&](int32 A, int32 B)
        {
            const FSquareWrapWalker& WalkerA = SquareWrapWalkers[A]; const FSquareWrapWalker& WalkerB = SquareWrapWalkers[B];
            return WalkerA.Lane != WalkerB.Lane ? WalkerA.Lane < WalkerB.Lane : WalkerA.At.X < WalkerB.At.X;
        });
        SquareWrapHeld.Sort([&](int32 A, int32 B) { return SquareWrapPlaces[A].At.Y < SquareWrapPlaces[B].At.Y; });
        SquareWrapHeld.SetNum(FMath::Min(SquareWrapFree.Num(), SquareWrapHeld.Num()));
        for (bool bSquareWrapSwapped = true; bSquareWrapSwapped;)
        {
            bSquareWrapSwapped = false;
            for (int32 SquareWrapA = 0; SquareWrapA < SquareWrapHeld.Num(); ++SquareWrapA)
            for (int32 SquareWrapB = 0; SquareWrapB < SquareWrapHeld.Num(); ++SquareWrapB)
            {
                const FSquareWrapWalker& WalkerA = SquareWrapWalkers[SquareWrapFree[SquareWrapA]]; const FSquareWrapWalker& WalkerB = SquareWrapWalkers[SquareWrapFree[SquareWrapB]];
                if (SquareWrapA == SquareWrapB || WalkerA.Lane != WalkerB.Lane || WalkerA.At.X >= WalkerB.At.X) continue;
                if (FMath::Abs(SquareWrapPlaces[SquareWrapHeld[SquareWrapA]].At.Y - WalkerA.At.Y) > FMath::Abs(SquareWrapPlaces[SquareWrapHeld[SquareWrapB]].At.Y - WalkerB.At.Y) + 0.1)
                { SquareWrapHeld.Swap(SquareWrapA, SquareWrapB); bSquareWrapSwapped = true; }
            }
        }
        for (int32 SquareWrapK = 0; SquareWrapK < SquareWrapHeld.Num(); ++SquareWrapK)
        {
            const int32 SquareWrapWalkerIndex = SquareWrapFree[SquareWrapK];
            FSquareWrapWalker& SquareWrapWalker = SquareWrapWalkers[SquareWrapWalkerIndex];
            FSquareWrapPlace& SquareWrapPlace = SquareWrapPlaces[SquareWrapHeld[SquareWrapK]];
            SquareWrapWalker.bDone = true; SquareWrapPlace.bUsed = true;
            // Straight up his file, the last 60 cm turned towards the place.
            const float SquareWrapAhead = float(SquareWrapPlace.At.X - SquareWrapWalker.At.X);
            const float SquareWrapTurnIn = FMath::Min(60.f, FMath::Abs(SquareWrapAhead)) * (SquareWrapAhead >= 0.f ? 1.f : -1.f);
            FSquareWrapWay SquareWrapWay;
            SquareWrapWay.Walker = SquareWrapWalkerIndex; SquareWrapWay.Place = SquareWrapPlace.Index;
            SquareWrapWay.Path[0] = SquareWrapWalker.At;
            SquareWrapWay.Path[1] = FVector2D(SquareWrapPlace.At.X - SquareWrapTurnIn, SquareWrapWalker.At.Y);
            SquareWrapWay.Path[2] = SquareWrapPlace.At;
            SquareWrapWay.Points = 3;
            SquareWrapWays.Add(SquareWrapWay);
        }
    };
    for (int32 SquareWrapRing = 0; SquareWrapRing < SquareWrapRanks; ++SquareWrapRing) SquareWrapReserve(SquareWrapRing, true, false);
    for (int32 SquareWrapRing = SquareWrapRanks - 1; SquareWrapRing >= 0; --SquareWrapRing) SquareWrapReserve(SquareWrapRing, false, true);
    // The rest go round: each ring's places on each side, the furthest first.
    TMap<int32, TArray<int32>> SquareWrapOpen;   // key: side (0 left, 1 right) * 8 + ring
    for (int32 SquareWrapPlaceIndex = 0; SquareWrapPlaceIndex < SquareWrapPlaces.Num(); ++SquareWrapPlaceIndex)
        if (!SquareWrapPlaces[SquareWrapPlaceIndex].bUsed)
            SquareWrapOpen.FindOrAdd((SquareWrapPlaces[SquareWrapPlaceIndex].Side > 0 ? 8 : 0) + SquareWrapPlaces[SquareWrapPlaceIndex].Ring).Add(SquareWrapPlaceIndex);
    for (TPair<int32, TArray<int32>>& SquareWrapOpenList : SquareWrapOpen)
        SquareWrapOpenList.Value.Sort([&](int32 A, int32 B) { return SquareWrapPlaces[A].Along > SquareWrapPlaces[B].Along; });
    const auto SquareWrapTake = [&](int32 SquareWrapSide, int32 SquareWrapRing)
    {
        TArray<int32>* SquareWrapList = SquareWrapOpen.Find((SquareWrapSide > 0 ? 8 : 0) + SquareWrapRing);
        if (!SquareWrapList || SquareWrapList->IsEmpty()) return int32(INDEX_NONE);
        const int32 SquareWrapTaken = (*SquareWrapList)[0];
        SquareWrapList->RemoveAt(0);
        return SquareWrapTaken;
    };
    TArray<int32> SquareWrapOrder;
    for (int32 SquareWrapWalkerIndex = 0; SquareWrapWalkerIndex < SquareWrapWalkers.Num(); ++SquareWrapWalkerIndex)
        if (!SquareWrapWalkers[SquareWrapWalkerIndex].bDone) SquareWrapOrder.Add(SquareWrapWalkerIndex);
    // Row by row from the head; in a row the outer men before the middle man (on the front rank they are ahead of him).
    SquareWrapOrder.Sort([&](int32 A, int32 B)
    {
        const FSquareWrapWalker& WalkerA = SquareWrapWalkers[A]; const FSquareWrapWalker& WalkerB = SquareWrapWalkers[B];
        if (WalkerA.Row != WalkerB.Row) return WalkerA.Row < WalkerB.Row;
        if (FMath::Abs(WalkerA.Lane) != FMath::Abs(WalkerB.Lane)) return FMath::Abs(WalkerA.Lane) > FMath::Abs(WalkerB.Lane);
        return WalkerA.Lane < WalkerB.Lane;
    });
    TArray<int32> SquareWrapLeftOver;
    int32 SquareWrapMiddleTurn = 1, SquareWrapMiddleRow = MIN_int32, SquareWrapMiddleSide = 0, SquareWrapAlternate[2] = { 0, 0 };
    for (int32 SquareWrapWalkerIndex : SquareWrapOrder)
    {
        FSquareWrapWalker& SquareWrapWalker = SquareWrapWalkers[SquareWrapWalkerIndex];
        if (SquareWrapWalker.Row != SquareWrapMiddleRow)
        {
            // Which way this row's middle man turns: right and left by turns (set when the row has one going round).
            SquareWrapMiddleRow = SquareWrapWalker.Row;
            SquareWrapMiddleSide = 0;
            for (int32 SquareWrapOther : SquareWrapOrder)
                if (SquareWrapWalkers[SquareWrapOther].Row == SquareWrapWalker.Row && SquareWrapWalkers[SquareWrapOther].Lane == 0)
                { SquareWrapMiddleSide = SquareWrapMiddleTurn; SquareWrapMiddleTurn = -SquareWrapMiddleTurn; break; }
        }
        int32 SquareWrapGot = INDEX_NONE, SquareWrapSide = 0;
        if (SquareWrapWalker.Lane == 0)
        {
            // The middle file along the outer (front) rank.
            SquareWrapSide = SquareWrapMiddleSide;
            SquareWrapGot = SquareWrapTake(SquareWrapSide, 0);
        }
        else
        {
            SquareWrapSide = SquareWrapWalker.Lane;
            // The head row stands on the front rank already; the others turn out on the second and third rank by turns (never back to
            // a rank behind them), and onto the front rank only in a row whose middle man turns the same way after them.
            if (SquareWrapWalker.At.X > SquareWrapFront - 1.f) SquareWrapGot = SquareWrapTake(SquareWrapSide, 0);
            else
            {
                int32& SquareWrapTurn = SquareWrapAlternate[SquareWrapSide > 0 ? 1 : 0];
                bool bSquareWrapRankAhead = false;
                for (int32 SquareWrapTry = 0; SquareWrapTry < 2 && SquareWrapGot == INDEX_NONE; ++SquareWrapTry)
                {
                    const int32 SquareWrapRing = FMath::Min(SquareWrapRanks - 1, SquareWrapTry == 0 ? 1 + SquareWrapTurn % 2 : 2 - SquareWrapTurn % 2);
                    if (SquareWrapRing < 1 || SquareWrapFront - SquareWrapRing * SquareWrapRankSpacing < SquareWrapWalker.At.X - 1.f) continue;
                    bSquareWrapRankAhead = true;
                    SquareWrapGot = SquareWrapTake(SquareWrapSide, SquareWrapRing);
                }
                if (SquareWrapGot != INDEX_NONE) ++SquareWrapTurn;
                // Between two ranks with none of his own ahead of him: half a pace back to the rank just behind him.
                for (int32 SquareWrapRing = 1; !bSquareWrapRankAhead && SquareWrapGot == INDEX_NONE && SquareWrapRing < SquareWrapRanks; ++SquareWrapRing)
                {
                    const float SquareWrapBack = float(SquareWrapWalker.At.X) - (SquareWrapFront - SquareWrapRing * SquareWrapRankSpacing);
                    if (SquareWrapBack > 0.f && SquareWrapBack <= 0.5f * SquareWrapRankSpacing) SquareWrapGot = SquareWrapTake(SquareWrapSide, SquareWrapRing);
                }
                if (SquareWrapGot == INDEX_NONE && SquareWrapMiddleSide == SquareWrapSide) SquareWrapGot = SquareWrapTake(SquareWrapSide, 0);
            }
        }
        if (SquareWrapGot == INDEX_NONE) { SquareWrapLeftOver.Add(SquareWrapWalkerIndex); continue; }
        FSquareWrapPlace& SquareWrapPlace = SquareWrapPlaces[SquareWrapGot];
        const float SquareWrapRingDepth = SquareWrapPlace.Ring * SquareWrapRankSpacing;
        const float SquareWrapRingHalf = SquareWrapWide - SquareWrapRingDepth;
        FSquareWrapWay SquareWrapWay;
        SquareWrapWay.Walker = SquareWrapWalkerIndex; SquareWrapWay.Place = SquareWrapPlace.Index;
        SquareWrapWay.Path[0] = SquareWrapWalker.At;
        SquareWrapWay.Path[1] = FVector2D(SquareWrapFront - SquareWrapRingDepth, SquareWrapWalker.At.Y);
        SquareWrapWay.Path[2] = FVector2D(SquareWrapFront - SquareWrapRingDepth, SquareWrapSide * SquareWrapRingHalf);
        SquareWrapWay.Path[3] = FVector2D(SquareWrapRear + SquareWrapRingDepth, SquareWrapSide * SquareWrapRingHalf);
        SquareWrapWay.Path[4] = FVector2D(SquareWrapRear + SquareWrapRingDepth, 0.f);
        SquareWrapWay.Points = 5;
        SquareWrapWay.Delta = float(FMath::Abs(SquareWrapWay.Path[1].X - SquareWrapWalker.At.X)) + (SquareWrapPlace.Along - FMath::Abs(float(SquareWrapWalker.At.Y)));
        if (SquareWrapWay.Delta < 0.f) { bSquareWrapAttemptFailed = true; SquareWrapFailure = TEXT("place-behind-file"); break; }
        SquareWrapPlace.bUsed = true; SquareWrapWalker.bDone = true;
        SquareWrapWays.Add(SquareWrapWay);
    }
    // A man with no place on his ring takes the nearest place left: up his file, then across.
    for (int32 SquareWrapWalkerIndex : SquareWrapLeftOver)
    {
        FSquareWrapWalker& SquareWrapWalker = SquareWrapWalkers[SquareWrapWalkerIndex];
        int32 SquareWrapBest = INDEX_NONE;
        for (int32 SquareWrapPlaceIndex = 0; SquareWrapPlaceIndex < SquareWrapPlaces.Num(); ++SquareWrapPlaceIndex)
            if (!SquareWrapPlaces[SquareWrapPlaceIndex].bUsed && (SquareWrapBest == INDEX_NONE ||
                FVector2D::DistSquared(SquareWrapPlaces[SquareWrapPlaceIndex].At, SquareWrapWalker.At) < FVector2D::DistSquared(SquareWrapPlaces[SquareWrapBest].At, SquareWrapWalker.At)))
                SquareWrapBest = SquareWrapPlaceIndex;
        if (SquareWrapBest == INDEX_NONE) { bSquareWrapAttemptFailed = true; SquareWrapFailure = TEXT("places-missing"); break; }
        FSquareWrapPlace& SquareWrapPlace = SquareWrapPlaces[SquareWrapBest];
        SquareWrapPlace.bUsed = true; SquareWrapWalker.bDone = true;
        FSquareWrapWay SquareWrapWay;
        SquareWrapWay.Walker = SquareWrapWalkerIndex; SquareWrapWay.Place = SquareWrapPlace.Index;
        SquareWrapWay.Path[0] = SquareWrapWalker.At;
        SquareWrapWay.Path[1] = FVector2D(SquareWrapPlace.At.X, SquareWrapWalker.At.Y);
        SquareWrapWay.Path[2] = SquareWrapPlace.At;
        SquareWrapWay.Points = 3;
        SquareWrapWays.Add(SquareWrapWay);
    }
    if (bSquareWrapAttemptFailed || SquareWrapWays.Num() != SquareWrapCount) { if (!bSquareWrapAttemptFailed) SquareWrapFailure = TEXT("ways-missing"); continue; }
    float SquareWrapLongest = 0.f;
    for (FSquareWrapWay& SquareWrapWay : SquareWrapWays)
    {
        if (SquareWrapWay.Points == 3)
            SquareWrapWay.Delta = float(FVector2D::Distance(SquareWrapWay.Path[0], SquareWrapWay.Path[1]) + FVector2D::Distance(SquareWrapWay.Path[1], SquareWrapWay.Path[2]));
        SquareWrapLongest = FMath::Max(SquareWrapLongest, SquareWrapWay.Delta);
    }
    if (SquareWrapLongest <= 0.f) { SquareWrapFailure = TEXT("no-way"); continue; }
    // The walk through in thought: everybody at one pace, each halting at his place. The nearest two men come on the way.
    float SquareWrapNearest = TNumericLimits<float>::Max();
    {
        TArray<FVector2D> SquareWrapAtStep;
        SquareWrapAtStep.SetNum(SquareWrapWays.Num());
        const int32 SquareWrapSteps = FMath::Clamp(FMath::CeilToInt(SquareWrapLongest / 40.f), 20, 160);
        for (int32 SquareWrapStep = 0; SquareWrapStep <= SquareWrapSteps; ++SquareWrapStep)
        {
            const float SquareWrapTravel = SquareWrapLongest * float(SquareWrapStep) / float(SquareWrapSteps);
            for (int32 SquareWrapWayIndex = 0; SquareWrapWayIndex < SquareWrapWays.Num(); ++SquareWrapWayIndex)
                SquareWrapAtStep[SquareWrapWayIndex] = SquareWrapAlong(SquareWrapWays[SquareWrapWayIndex].Path, SquareWrapWays[SquareWrapWayIndex].Points,
                    FMath::Min(SquareWrapTravel, SquareWrapWays[SquareWrapWayIndex].Delta));
            for (int32 SquareWrapA = 0; SquareWrapA < SquareWrapAtStep.Num(); ++SquareWrapA)
            for (int32 SquareWrapB = SquareWrapA + 1; SquareWrapB < SquareWrapAtStep.Num(); ++SquareWrapB)
                SquareWrapNearest = FMath::Min(SquareWrapNearest, float(FVector2D::DistSquared(SquareWrapAtStep[SquareWrapA], SquareWrapAtStep[SquareWrapB])));
        }
        SquareWrapNearest = FMath::Sqrt(SquareWrapNearest);
    }
    if (SquareWrapNearest > SquareWrapBestNearest)
    {
        SquareWrapBestNearest = SquareWrapNearest;
        SquareWrapBestLongest = SquareWrapLongest;
        SquareWrapBestLeftOver = SquareWrapLeftOver.Num();
        SquareWrapBestWays = SquareWrapWays;
    }
    }
    // (22 cm for a moment on the way: tight, but the straight walk it stands in for brought men within 4-13 cm and crossed them.
    // Formed, the inner ring's men stand 52 cm apart.)
    if (SquareWrapBestNearest < 22.f)
    {
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s start kind=%s men=%d accepted=0 reason=%s nearest-cm=%.1f longest-cm=%.0f file-spacing-cm=%.0f rows=%d"),
            *OwnerCompany->StableUnitId.ToString(), SquareWrapKindText, SquareWrapCount, SquareWrapBestNearest < 0.f ? SquareWrapFailure : TEXT("too-close"),
            SquareWrapBestNearest, SquareWrapBestLongest, SquareWrapLaneSpacing, SquareWrapRowLevels.Num());
        return false;
    }
    const float SquareWrapLongest = SquareWrapBestLongest;
    const float SquareWrapNearest = SquareWrapBestNearest;
    const TArray<FSquareWrapWay>& SquareWrapWays = SquareWrapBestWays;
    SquareWrapWalkers = SquareWrapWalkersClean;
    // The clock. Into the square inside the time it takes to form, at a run if need be (never longer: it is open to cavalry until it
    // stands); out of it at a quick pace, the march held meanwhile as the straight walk held it.
    float SquareWrapSeconds = 0.f;
    if (bSquareWrapInto)
    {
        SquareWrapSeconds = SquareWrapLongest / 250.f;
        if (OwnerCompany->FormationTransition && OwnerCompany->FormationTransition->ReformRemainingSeconds > 0.f)
            SquareWrapSeconds = FMath::Min(SquareWrapSeconds, OwnerCompany->FormationTransition->ReformRemainingSeconds - 0.5f);
        if (SquareWrapSeconds < SquareWrapLongest / 420.f) return SquareWrapReject(TEXT("no-time"));
    }
    else
    {
        SquareWrapSeconds = FMath::Max(3.f, SquareWrapLongest / 330.f);
        if (OwnerCompany->FormationTransition) OwnerCompany->FormationTransition->ExtendReformSeconds(SquareWrapSeconds + 0.3f);
    }
    SquareSwingRunning = true;
    SquareSwingStarted = VisualAnimationTime;
    SquareWrapLongestCm = SquareWrapLongest;
    for (const FSquareWrapWay& SquareWrapWay : SquareWrapWays)
    {
        const int32 SquareWrapColumnIndex = SquareWrapWalkers[SquareWrapWay.Walker].Index;
        const int32 SquareWrapOrderIndex = bSquareWrapInto ? SquareWrapColumnIndex : SquareWrapWay.Place;
        SquareWrapTargets[SquareWrapOrderIndex] = bSquareWrapInto ? SquareWrapWay.Place : SquareWrapColumnIndex;
        FSettle& SquareWrapSettle = SoldierSettle[SquareWrapMen[SquareWrapOrderIndex]];
        SquareWrapSettle.SquareSwingKind = bSquareWrapInto ? 3 : 4;
        SquareWrapSettle.SquareWrapPoints = uint8(SquareWrapWay.Points);
        for (int32 SquareWrapCorner = 0; SquareWrapCorner < 5; ++SquareWrapCorner) SquareWrapSettle.SquareWrapPath[SquareWrapCorner] = SquareWrapWay.Path[FMath::Min(SquareWrapCorner, SquareWrapWay.Points - 1)];
        SquareWrapSettle.SquareWrapDelta = SquareWrapWay.Delta;
    }
    OutSquareWrapSeconds = SquareWrapSeconds;
    UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s start kind=%s men=%d accepted=1 longest-cm=%.0f seconds=%.2f nearest-cm=%.1f left-over=%d"),
        *OwnerCompany->StableUnitId.ToString(), SquareWrapKindText, SquareWrapCount, SquareWrapLongest, SquareWrapSeconds, SquareWrapNearest, SquareWrapBestLeftOver);
    return true;
}

FVector UStrategyInfantryVisualComponent::SquareSwingPoint(const FSettle& SquareSwingSettle, float SquareSwingProgress) const
{
    // Progress 0 is the line, 1 the square. The first swing (the whole wing about the front corner) takes SquareSwingSplit of the
    // clock, the second (the part beyond the rear corner, about that corner) the rest. A man's place along the arm and his depth
    // behind its outer line go from the line's to the square's during the first swing (the inner rings are shorter than the line),
    // so the second swing is rigid.
    const float SquareSwingFirst = SquareSwingSplit > 0.f ? FMath::Clamp(SquareSwingProgress / SquareSwingSplit, 0.f, 1.f) : 1.f;
    float SquareSwingSecond = SquareSwingSplit < 1.f ? FMath::Clamp((SquareSwingProgress - SquareSwingSplit) / (1.f - SquareSwingSplit), 0.f, 1.f) : 0.f;
    // The two rear halves a fifth of the second swing apart: swinging in together their tips met in the middle (worked out
    // 2026-11-15: 11 cm; apart 43 cm at the least).
    const float SquareSwingLead = 0.2f;
    SquareSwingSecond = SquareSwingSettle.SquareSwingSide == SquareSwingLeadSide ? FMath::Min(1.f, SquareSwingSecond / (1.f - SquareSwingLead)) :
        FMath::Max(0.f, (SquareSwingSecond - SquareSwingLead) / (1.f - SquareSwingLead));
    const float SquareSwingAlong = FMath::Lerp(SquareSwingSettle.SquareSwingLineAlong, SquareSwingSettle.SquareSwingPlaceAlong, SquareSwingFirst) +
        SquareSwingSettle.SquareSwingBulge * FMath::Sin(UE_PI * SquareSwingFirst);
    const float SquareSwingDepth = FMath::Lerp(SquareSwingSettle.SquareSwingLineDepth, SquareSwingSettle.SquareSwingPlaceDepth, SquareSwingFirst);
    const float SquareSwingAngle = SquareSwingFirst * UE_HALF_PI;
    const float SquareSwingFold = SquareSwingAngle + SquareSwingSecond * UE_HALF_PI;
    // The left wing (Y < 0), mirrored for the right. Along the arm from the front corner, and inwards from its outer line.
    const FVector2D SquareSwingCorner(SquareSwingFrontX, -SquareSwingHalf);
    const FVector2D SquareSwingArmDir(-FMath::Sin(SquareSwingAngle), -FMath::Cos(SquareSwingAngle));
    const FVector2D SquareSwingArmIn(-FMath::Cos(SquareSwingAngle), FMath::Sin(SquareSwingAngle));
    FVector2D SquareSwingAt;
    if (SquareSwingAlong <= SquareSwingArm)
    {
        SquareSwingAt = SquareSwingCorner + SquareSwingArmDir * SquareSwingAlong + SquareSwingArmIn * SquareSwingDepth;
    }
    else
    {
        const FVector2D SquareSwingRearCorner = SquareSwingCorner + SquareSwingArmDir * SquareSwingArm;
        const FVector2D SquareSwingRearDir(-FMath::Sin(SquareSwingFold), -FMath::Cos(SquareSwingFold));
        const FVector2D SquareSwingRearIn(-FMath::Cos(SquareSwingFold), FMath::Sin(SquareSwingFold));
        SquareSwingAt = SquareSwingRearCorner + SquareSwingRearDir * (SquareSwingAlong - SquareSwingArm) + SquareSwingRearIn * SquareSwingDepth;
    }
    return FVector(SquareSwingAt.X, SquareSwingAt.Y * (SquareSwingSettle.SquareSwingSide < 0.f ? 1.f : -1.f), SquareSwingSettle.Slot.Z);
}

bool UStrategyInfantryVisualComponent::PlanSquareSwing(bool bSquareSwingInto, const FTransform& SquareSwingFrame, const TArray<int32>& SquareSwingMen,
    const TArray<FVector>& SquareSwingSlots, const TArray<FVector>& SquareSwingStarts, TArray<int32>& SquareSwingTargets, float& OutSquareSwingSeconds)
{
    // Line <-> square (Allan 2026-11-14, after seeing both: "B: sving som en dør"). The middle of the line stands and is the square's
    // front; each wing swings back as one stiff arm about the front corner and becomes the side, and the part of it beyond the rear
    // corner swings in about that corner and closes half the rear. One clock for all, so the arm stays straight and nobody crosses;
    // the ranks keep their depth, the line's front rank is the square's outer ring. Out of the square the same, backwards.
    // The straight walk it replaces crossed (2026-11-13: 3 cm between men; the line is 46 m wide, the square 12 m). Everything is
    // worked out here first; a man is touched only when the whole plan holds, else the straight walk is used as before.
    const TCHAR* SquareSwingKindText = bSquareSwingInto ? TEXT("to-square-swing") : TEXT("from-square-swing");
    const auto SquareSwingReject = [&](const TCHAR* SquareSwingWhy)
    {
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s start kind=%s men=%d accepted=0 reason=%s"),
            *OwnerCompany->StableUnitId.ToString(), SquareSwingKindText, SquareSwingMen.Num(), SquareSwingWhy);
        return false;
    };
    const int32 SquareSwingCount = SquareSwingMen.Num();
    if (SquareSwingCount < 24 || SquareSwingCount != SquareSwingSlots.Num() || SquareSwingCount != SquareSwingTargets.Num()) return SquareSwingReject(TEXT("too-few-or-uneven"));
    if (OwnerCompany->StanceComponent && OwnerCompany->StanceComponent->Stance == EStrategyStance::Prone) return SquareSwingReject(TEXT("prone"));
    if (OwnerCompany->SkirmisherComponent && (OwnerCompany->SkirmisherComponent->State == EStrategySkirmisherState::Deploying || OwnerCompany->SkirmisherComponent->IsDeployed()))
        return SquareSwingReject(TEXT("screen-out"));
    const int32 SquareSwingRanks = FMath::Max(1, OwnerCompany->FormationComponent->RankCount);
    const float SquareSwingRankSpacing = FMath::Max(1.f, OwnerCompany->FormationComponent->SoldierRankSpacingCm);
    // Where the men stand and the places they go to, both in the unit's frame. One of the two sets is the line, the other the square.
    TArray<FVector> SquareSwingStandPoints;
    for (int32 SquareSwingMan : SquareSwingMen) SquareSwingStandPoints.Add(SquareSwingFrame.InverseTransformPosition(SquareSwingStarts[SquareSwingMan]));
    const TArray<FVector>& SquareSwingLinePoints = bSquareSwingInto ? SquareSwingStandPoints : SquareSwingSlots;
    const TArray<FVector>& SquareSwingRingPoints = bSquareSwingInto ? SquareSwingSlots : SquareSwingStandPoints;
    // The square's outline from its own points.
    float SquareSwingFront = -TNumericLimits<float>::Max(), SquareSwingRear = TNumericLimits<float>::Max(), SquareSwingWide = 0.f;
    for (const FVector& SquareSwingRingPoint : SquareSwingRingPoints)
    {
        SquareSwingFront = FMath::Max(SquareSwingFront, float(SquareSwingRingPoint.X));
        SquareSwingRear = FMath::Min(SquareSwingRear, float(SquareSwingRingPoint.X));
        SquareSwingWide = FMath::Max(SquareSwingWide, FMath::Abs(float(SquareSwingRingPoint.Y)));
    }
    const float SquareSwingLength = SquareSwingFront - SquareSwingRear;
    if (SquareSwingWide < 2.f * SquareSwingRankSpacing || FMath::Abs(SquareSwingLength - 2.f * SquareSwingWide) > SquareSwingRankSpacing) return SquareSwingReject(TEXT("not-a-square"));
    float SquareSwingLineFront = -TNumericLimits<float>::Max();
    for (const FVector& SquareSwingLinePoint : SquareSwingLinePoints) SquareSwingLineFront = FMath::Max(SquareSwingLineFront, float(SquareSwingLinePoint.X));
    if (FMath::Abs(SquareSwingLineFront - SquareSwingFront) > 0.5f * SquareSwingRankSpacing) return SquareSwingReject(TEXT("fronts-apart"));
    // A man or place on a wing's arm: how far along it from the front corner, how deep behind its outer line.
    struct FSquareSwingSpot { int32 Index = INDEX_NONE; float Along = 0.f; float Depth = 0.f; float Across = 0.f; };
    TArray<TArray<FSquareSwingSpot>> SquareSwingLineRanks, SquareSwingFrontPlaces, SquareSwingLeftPlaces, SquareSwingRightPlaces;
    SquareSwingLineRanks.SetNum(SquareSwingRanks); SquareSwingFrontPlaces.SetNum(SquareSwingRanks);
    SquareSwingLeftPlaces.SetNum(SquareSwingRanks); SquareSwingRightPlaces.SetNum(SquareSwingRanks);
    const float SquareSwingTolerance = 0.3f * SquareSwingRankSpacing;
    for (int32 SquareSwingLineIndex = 0; SquareSwingLineIndex < SquareSwingLinePoints.Num(); ++SquareSwingLineIndex)
    {
        const FVector& SquareSwingLinePoint = SquareSwingLinePoints[SquareSwingLineIndex];
        const float SquareSwingLineDepth = SquareSwingFront - float(SquareSwingLinePoint.X);
        const int32 SquareSwingLineRank = FMath::RoundToInt32(SquareSwingLineDepth / SquareSwingRankSpacing);
        if (SquareSwingLineRank < 0 || SquareSwingLineRank >= SquareSwingRanks || FMath::Abs(SquareSwingLineDepth - SquareSwingLineRank * SquareSwingRankSpacing) > SquareSwingTolerance)
            return SquareSwingReject(TEXT("line-not-formed"));
        FSquareSwingSpot SquareSwingLineSpot;
        SquareSwingLineSpot.Index = SquareSwingLineIndex; SquareSwingLineSpot.Depth = SquareSwingLineDepth; SquareSwingLineSpot.Across = float(SquareSwingLinePoint.Y);
        SquareSwingLineRanks[SquareSwingLineRank].Add(SquareSwingLineSpot);
    }
    for (int32 SquareSwingRingIndex = 0; SquareSwingRingIndex < SquareSwingRingPoints.Num(); ++SquareSwingRingIndex)
    {
        const FVector& SquareSwingRingPoint = SquareSwingRingPoints[SquareSwingRingIndex];
        const float SquareSwingFromFront = SquareSwingFront - float(SquareSwingRingPoint.X), SquareSwingFromRear = float(SquareSwingRingPoint.X) - SquareSwingRear;
        const float SquareSwingFromLeft = float(SquareSwingRingPoint.Y) + SquareSwingWide, SquareSwingFromRight = SquareSwingWide - float(SquareSwingRingPoint.Y);
        const int32 SquareSwingRing = FMath::RoundToInt32(FMath::Min(FMath::Min(SquareSwingFromFront, SquareSwingFromRear), FMath::Min(SquareSwingFromLeft, SquareSwingFromRight)) / SquareSwingRankSpacing);
        if (SquareSwingRing < 0 || SquareSwingRing >= SquareSwingRanks) return SquareSwingReject(TEXT("ring-out-of-range"));
        const float SquareSwingRingDepth = SquareSwingRing * SquareSwingRankSpacing;
        const bool bSquareSwingOnFront = FMath::Abs(SquareSwingFromFront - SquareSwingRingDepth) <= SquareSwingTolerance;
        const bool bSquareSwingOnRear = FMath::Abs(SquareSwingFromRear - SquareSwingRingDepth) <= SquareSwingTolerance;
        const bool bSquareSwingOnLeft = FMath::Abs(SquareSwingFromLeft - SquareSwingRingDepth) <= SquareSwingTolerance;
        const bool bSquareSwingOnRight = FMath::Abs(SquareSwingFromRight - SquareSwingRingDepth) <= SquareSwingTolerance;
        FSquareSwingSpot SquareSwingRingSpot;
        SquareSwingRingSpot.Index = SquareSwingRingIndex;
        SquareSwingRingSpot.Across = float(SquareSwingRingPoint.Y);
        if (bSquareSwingOnFront && !bSquareSwingOnRight)
        {
            // The front face with its left corner (the right corner is the right face's own, as the places are laid out).
            SquareSwingRingSpot.Depth = SquareSwingFromFront;
            SquareSwingFrontPlaces[SquareSwingRing].Add(SquareSwingRingSpot);
        }
        else if (bSquareSwingOnLeft || bSquareSwingOnRight)
        {
            SquareSwingRingSpot.Along = SquareSwingFromFront;
            SquareSwingRingSpot.Depth = bSquareSwingOnLeft ? SquareSwingFromLeft : SquareSwingFromRight;
            (bSquareSwingOnLeft ? SquareSwingLeftPlaces : SquareSwingRightPlaces)[SquareSwingRing].Add(SquareSwingRingSpot);
        }
        else if (bSquareSwingOnRear)
        {
            // The rear face, each half to the wing on its side: beyond the rear corner on that wing's arm.
            const bool bSquareSwingRearLeft = SquareSwingRingPoint.Y < 0.;
            SquareSwingRingSpot.Along = SquareSwingLength + (bSquareSwingRearLeft ? SquareSwingFromLeft : SquareSwingFromRight);
            SquareSwingRingSpot.Depth = SquareSwingFromRear;
            (bSquareSwingRearLeft ? SquareSwingLeftPlaces : SquareSwingRightPlaces)[SquareSwingRing].Add(SquareSwingRingSpot);
        }
        else return SquareSwingReject(TEXT("square-not-formed"));
    }
    // Rank by rank: the men nearest the corners go to the places nearest them on the same ring. The rings do not hold the same number
    // as the line's ranks (189 men: 64/64/61 against 63/63/63); the men over in one rank take the places left on a wing's other rings,
    // at the tip of the arm.
    // (A man who stands inside the corner - before it on the arm - only steps to his place: swung, he went out in front of the line.)
    struct FSquareSwingPair { int32 Line = INDEX_NONE; int32 Ring = INDEX_NONE; int32 Side = 0; float LineAlong = 0.f; float LineDepth = 0.f; float PlaceAlong = 0.f; float PlaceDepth = 0.f; };
    TArray<FSquareSwingPair> SquareSwingPairs;
    TArray<FSquareSwingSpot> SquareSwingSpareMen[2], SquareSwingSparePlaces[2];
    int32 SquareSwingBalance[2] = { 0, 0 };
    const auto SquareSwingAcrossLess = [](const FSquareSwingSpot& A, const FSquareSwingSpot& B) { return A.Across < B.Across; };
    const auto SquareSwingAlongLess = [](const FSquareSwingSpot& A, const FSquareSwingSpot& B) { return A.Along < B.Along; };
    for (int32 SquareSwingRank = 0; SquareSwingRank < SquareSwingRanks; ++SquareSwingRank)
    {
        TArray<FSquareSwingSpot>& SquareSwingRankMen = SquareSwingLineRanks[SquareSwingRank];
        SquareSwingRankMen.Sort(SquareSwingAcrossLess);
        SquareSwingFrontPlaces[SquareSwingRank].Sort(SquareSwingAcrossLess);
        SquareSwingLeftPlaces[SquareSwingRank].Sort(SquareSwingAlongLess);
        SquareSwingRightPlaces[SquareSwingRank].Sort(SquareSwingAlongLess);
        const int32 SquareSwingFrontCount = SquareSwingFrontPlaces[SquareSwingRank].Num();
        const int32 SquareSwingWingMen = SquareSwingRankMen.Num() - SquareSwingFrontCount;
        if (SquareSwingWingMen < 0) return SquareSwingReject(TEXT("rank-too-short"));
        int32 SquareSwingTake[2] = { SquareSwingLeftPlaces[SquareSwingRank].Num(), SquareSwingRightPlaces[SquareSwingRank].Num() };
        int32 SquareSwingOver = SquareSwingWingMen - SquareSwingTake[0] - SquareSwingTake[1];
        for (; SquareSwingOver > 0; --SquareSwingOver)
        {
            const int32 SquareSwingTo = SquareSwingBalance[0] <= SquareSwingBalance[1] ? 0 : 1;
            ++SquareSwingTake[SquareSwingTo]; ++SquareSwingBalance[SquareSwingTo];
        }
        for (; SquareSwingOver < 0; ++SquareSwingOver)
        {
            int32 SquareSwingFromSide = SquareSwingBalance[0] >= SquareSwingBalance[1] ? 0 : 1;
            if (SquareSwingTake[SquareSwingFromSide] <= 0) SquareSwingFromSide = 1 - SquareSwingFromSide;
            if (SquareSwingTake[SquareSwingFromSide] <= 0) return SquareSwingReject(TEXT("rank-too-short"));
            --SquareSwingTake[SquareSwingFromSide]; --SquareSwingBalance[SquareSwingFromSide];
        }
        // The front: the middle of the rank, left to right. They only close in a little.
        for (int32 SquareSwingK = 0; SquareSwingK < SquareSwingFrontCount; ++SquareSwingK)
        {
            FSquareSwingPair SquareSwingPair;
            SquareSwingPair.Line = SquareSwingRankMen[SquareSwingTake[0] + SquareSwingK].Index;
            SquareSwingPair.Ring = SquareSwingFrontPlaces[SquareSwingRank][SquareSwingK].Index;
            SquareSwingPairs.Add(SquareSwingPair);
        }
        for (int32 SquareSwingSideIndex = 0; SquareSwingSideIndex < 2; ++SquareSwingSideIndex)
        {
            const TArray<FSquareSwingSpot>& SquareSwingPlaces = SquareSwingSideIndex == 0 ? SquareSwingLeftPlaces[SquareSwingRank] : SquareSwingRightPlaces[SquareSwingRank];
            for (int32 SquareSwingK = 0; SquareSwingK < FMath::Max(SquareSwingTake[SquareSwingSideIndex], SquareSwingPlaces.Num()); ++SquareSwingK)
            {
                // The wing's men from the corner outwards: the left wing from its right end, the right wing from its left end.
                const int32 SquareSwingManAt = SquareSwingSideIndex == 0 ? SquareSwingTake[0] - 1 - SquareSwingK : SquareSwingRankMen.Num() - SquareSwingTake[1] + SquareSwingK;
                const bool bSquareSwingHasMan = SquareSwingK < SquareSwingTake[SquareSwingSideIndex];
                const bool bSquareSwingHasPlace = SquareSwingK < SquareSwingPlaces.Num();
                FSquareSwingSpot SquareSwingManSpot;
                if (bSquareSwingHasMan)
                {
                    SquareSwingManSpot = SquareSwingRankMen[SquareSwingManAt];
                    // (Not held at nil: an inner-rank man inside the corner starts a little before it on the arm, exactly where he stands.)
                    SquareSwingManSpot.Along = SquareSwingSideIndex == 0 ? -SquareSwingWide - SquareSwingManSpot.Across : SquareSwingManSpot.Across - SquareSwingWide;
                }
                if (bSquareSwingHasMan && bSquareSwingHasPlace)
                {
                    FSquareSwingPair SquareSwingPair;
                    SquareSwingPair.Line = SquareSwingManSpot.Index; SquareSwingPair.Ring = SquareSwingPlaces[SquareSwingK].Index;
                    SquareSwingPair.Side = SquareSwingManSpot.Along < 0.f ? 0 : (SquareSwingSideIndex == 0 ? -1 : 1);
                    SquareSwingPair.LineAlong = SquareSwingManSpot.Along; SquareSwingPair.LineDepth = SquareSwingManSpot.Depth;
                    SquareSwingPair.PlaceAlong = SquareSwingPlaces[SquareSwingK].Along; SquareSwingPair.PlaceDepth = SquareSwingPlaces[SquareSwingK].Depth;
                    SquareSwingPairs.Add(SquareSwingPair);
                }
                else if (bSquareSwingHasMan) SquareSwingSpareMen[SquareSwingSideIndex].Add(SquareSwingManSpot);
                else SquareSwingSparePlaces[SquareSwingSideIndex].Add(SquareSwingPlaces[SquareSwingK]);
            }
        }
    }
    for (int32 SquareSwingSideIndex = 0; SquareSwingSideIndex < 2; ++SquareSwingSideIndex)
    {
        if (SquareSwingSpareMen[SquareSwingSideIndex].Num() != SquareSwingSparePlaces[SquareSwingSideIndex].Num()) return SquareSwingReject(TEXT("wings-uneven"));
        SquareSwingSpareMen[SquareSwingSideIndex].Sort(SquareSwingAlongLess);
        SquareSwingSparePlaces[SquareSwingSideIndex].Sort(SquareSwingAlongLess);
        for (int32 SquareSwingK = 0; SquareSwingK < SquareSwingSpareMen[SquareSwingSideIndex].Num(); ++SquareSwingK)
        {
            FSquareSwingPair SquareSwingPair;
            SquareSwingPair.Line = SquareSwingSpareMen[SquareSwingSideIndex][SquareSwingK].Index; SquareSwingPair.Ring = SquareSwingSparePlaces[SquareSwingSideIndex][SquareSwingK].Index;
            SquareSwingPair.Side = SquareSwingSpareMen[SquareSwingSideIndex][SquareSwingK].Along < 0.f ? 0 : (SquareSwingSideIndex == 0 ? -1 : 1);
            SquareSwingPair.LineAlong = SquareSwingSpareMen[SquareSwingSideIndex][SquareSwingK].Along; SquareSwingPair.LineDepth = SquareSwingSpareMen[SquareSwingSideIndex][SquareSwingK].Depth;
            SquareSwingPair.PlaceAlong = SquareSwingSparePlaces[SquareSwingSideIndex][SquareSwingK].Along; SquareSwingPair.PlaceDepth = SquareSwingSparePlaces[SquareSwingSideIndex][SquareSwingK].Depth;
            SquareSwingPairs.Add(SquareSwingPair);
        }
    }
    if (SquareSwingPairs.Num() != SquareSwingCount) return SquareSwingReject(TEXT("pairs-missing"));
    // The clock: the tip of the arm walks the two arcs. Into the square inside the time the square takes to form (never longer:
    // it is open to cavalry until it stands); out of it at a brisk pace, the time held as the straight walk held it.
    float SquareSwingReach = 0.f, SquareSwingBeyond = 0.f, SquareSwingTip[2] = { 0.f, 0.f };
    for (const FSquareSwingPair& SquareSwingPair : SquareSwingPairs)
    {
        if (SquareSwingPair.Side == 0) continue;
        SquareSwingTip[SquareSwingPair.Side < 0 ? 0 : 1] = FMath::Max(SquareSwingTip[SquareSwingPair.Side < 0 ? 0 : 1], SquareSwingPair.PlaceAlong);
        SquareSwingReach = FMath::Max(SquareSwingReach, FMath::Max(SquareSwingPair.LineAlong, SquareSwingPair.PlaceAlong));
        SquareSwingBeyond = FMath::Max(SquareSwingBeyond, SquareSwingPair.PlaceAlong - SquareSwingLength);
    }
    SquareSwingBeyond = FMath::Max(0.f, SquareSwingBeyond);
    if (SquareSwingReach <= 0.f) return SquareSwingReject(TEXT("no-wings"));
    const float SquareSwingLongest = UE_HALF_PI * (SquareSwingReach + SquareSwingBeyond);
    float SquareSwingSeconds = 0.f;
    if (bSquareSwingInto)
    {
        SquareSwingSeconds = SquareSwingLongest / 160.f;
        if (OwnerCompany->FormationTransition && OwnerCompany->FormationTransition->ReformRemainingSeconds > 0.f)
            SquareSwingSeconds = FMath::Min(SquareSwingSeconds, OwnerCompany->FormationTransition->ReformRemainingSeconds - 0.5f);
        if (SquareSwingSeconds < SquareSwingLongest / 420.f) return SquareSwingReject(TEXT("no-time"));
    }
    else
    {
        // (300 cm/s was tried first: the rear halves opened in three seconds at a run and looked like a scramble, QA 2026-11-15.)
        SquareSwingSeconds = FMath::Max(3.f, SquareSwingLongest / 240.f);
        if (OwnerCompany->FormationTransition) OwnerCompany->FormationTransition->ExtendReformSeconds(SquareSwingSeconds + 0.3f);
    }
    // The plan holds: the places and the men's swings.
    SquareSwingFrontX = SquareSwingFront;
    SquareSwingHalf = SquareSwingWide;
    SquareSwingArm = SquareSwingLength;
    SquareSwingSplit = SquareSwingReach / (SquareSwingReach + SquareSwingBeyond);
    SquareSwingRunning = true;
    SquareSwingStarted = VisualAnimationTime;
    // The wing whose rear half is the shorter swings in first; the longer one closes the middle after it.
    SquareSwingLeadSide = SquareSwingTip[0] <= SquareSwingTip[1] ? -1.f : 1.f;
    for (const FSquareSwingPair& SquareSwingPair : SquareSwingPairs)
    {
        const int32 SquareSwingOrder = bSquareSwingInto ? SquareSwingPair.Line : SquareSwingPair.Ring;
        SquareSwingTargets[SquareSwingOrder] = bSquareSwingInto ? SquareSwingPair.Ring : SquareSwingPair.Line;
        FSettle& SquareSwingSettle = SoldierSettle[SquareSwingMen[SquareSwingOrder]];
        SquareSwingSettle.SquareSwingKind = SquareSwingPair.Side == 0 ? 0 : (bSquareSwingInto ? 1 : 2);
        SquareSwingSettle.SquareSwingSide = float(SquareSwingPair.Side);
        SquareSwingSettle.SquareSwingLineAlong = SquareSwingPair.LineAlong;
        SquareSwingSettle.SquareSwingLineDepth = SquareSwingPair.LineDepth;
        SquareSwingSettle.SquareSwingPlaceAlong = SquareSwingPair.PlaceAlong;
        SquareSwingSettle.SquareSwingPlaceDepth = SquareSwingPair.PlaceDepth;
        // A man who takes a place in another ring goes round the tip of the arm, not through the men of his own file.
        SquareSwingSettle.SquareSwingBulge = FMath::Abs(SquareSwingPair.PlaceDepth - SquareSwingPair.LineDepth) > 0.5f * SquareSwingRankSpacing ? 120.f : 0.f;
    }
    OutSquareSwingSeconds = SquareSwingSeconds;
    UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s start kind=%s men=%d accepted=1 longest-cm=%.0f seconds=%.2f arm-cm=%.0f beyond-cm=%.0f spare-left=%d spare-right=%d"),
        *OwnerCompany->StableUnitId.ToString(), SquareSwingKindText, SquareSwingCount, SquareSwingLongest, SquareSwingSeconds,
        SquareSwingReach, SquareSwingBeyond, SquareSwingSpareMen[0].Num(), SquareSwingSpareMen[1].Num());
    return true;
}

void UStrategyInfantryVisualComponent::KeepReformSpacing(float DeltaTime)
{
    // The square swing keeps its own order on one clock; a push aside would bend the arm.
    if (SquareSwingRunning) return;
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
                    if (bReformMoveA != bReformMoveB)
                    {
                        // Give way (Allan 2026-11-08 "ja, prøv den"): a man on his way past one who already stands slides aside,
                        // sideways to his own walk only (never back, so he does not walk on the spot), and fading out 2.0-1.2 m
                        // from his place so that he still lands in it. Into a square the last men otherwise brushed through the
                        // standing ranks at 3-14 cm.
                        const int32 ReformWalker = bReformMoveA ? ReformMan : ReformOther;
                        const int32 ReformStander = bReformMoveA ? ReformOther : ReformMan;
                        FVector ReformAside = FormationStepPositions[ReformWalker] - FormationStepPositions[ReformStander];
                        ReformAside.Z = 0.f;
                        FVector ReformHeading = SoldierSettle[ReformWalker].WorldVelocity;
                        ReformHeading.Z = 0.f;
                        if (ReformHeading.SizeSquared() > 1.f)
                        {
                            ReformHeading.Normalize();
                            ReformAside -= ReformHeading * FVector::DotProduct(ReformAside, ReformHeading);
                            if (ReformAside.SizeSquared() < 1.f) ReformAside = FVector(-ReformHeading.Y, ReformHeading.X, 0.f) * (ReformWalker < ReformStander ? 1.f : -1.f);
                        }
                        if (ReformAside.IsNearlyZero()) continue;
                        const float ReformToGoal = float(FVector::Dist2D(FormationStepPositions[ReformWalker], ReformUnit.TransformPosition(SoldierSettle[ReformWalker].Goal)));
                        const float ReformFade = FMath::Clamp((ReformToGoal - 120.f) / 80.f, 0.f, 1.f);
                        FormationStepPositions[ReformWalker] += ReformAside.GetSafeNormal() * (ReformClearance - ReformDistance) * 0.5f * ReformFade;
                        continue;
                    }
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
        // Out of the square into column the film runs back: he stands as he stood until the men behind him on his way have gone
        // (not frozen in a stride), about a metre before the clock reaches him.
        if (Settle.SquareSwingKind == 4 && Settle.ReformDeadline >= 0.f && Settle.ReformDuration > 0.f &&
            (1.f - FMath::Clamp((VisualAnimationTime - Settle.ReformStartTime) / Settle.ReformDuration, 0.f, 1.f)) * SquareWrapLongestCm > Settle.SquareWrapDelta + 100.f)
        {
            bAny = true;
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
        // Arrival is judged on the ground plane: his place is in the flat formation, his feet on the terrain, so the height between
        // them is never nil on a field that is not flat (QA 2026-11-07: the men stood mid-stride in the walk clip and the company
        // never counted as formed, "plan=1" for the rest of the battle).
        const float ArrivedDistance = Delta.Size2D();
        if (Settle.ReformDeadline >= 0.f && ArrivedDistance <= 0.5f)
            Settle.ReformDeadline = -1.f;
        if (ArrivedDistance <= 0.5f && !bVisualTravel)
        {
            Settle.ReformDeadline = -1.f;
            if (bArrivalReform && bArrivalHasLiveFront && Settle.Slot.X >= ArrivalLiveFrontX - 1.f)
                bArrivalFrontReached = true;
            Settle.WorldVelocity = FVector::ZeroVector;
            FRotator ArrivedRotation = Soldier->GetRelativeRotation();
            ArrivedRotation.Yaw = FStrategyVisualFormationPath::SmoothFacing(ArrivedRotation.Yaw, Settle.YawVelocity, Settle.Yaw, DeltaTime, 120.f);
            // Within a degree and a half he is turned: set exactly, so he stops (the spring crept towards 0.1 degree and never got
            // there - the men stood in the walk clip mid-stride and the company never counted as formed: QA 2026-11-07).
            if (FMath::Abs(FMath::FindDeltaAngleDegrees(ArrivedRotation.Yaw, Settle.Yaw)) <= 1.5f) { ArrivedRotation.Yaw = Settle.Yaw; Settle.YawVelocity = 0.f; }
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
        const FVector FigureTarget = SoldierOnGround(i, VisualUnit.TransformPosition(Settle.Goal), float(Settle.Slot.Z), true);
        // Integrate presentation velocity every frame, with braking near the slot.
        // Substeps affect no orders, simulation positions, collision or game clocks.
        float MorphAlpha = -1.f;
        if (!bSpreadCrawling && (bRigidCompany || Settle.SquareSwingKind != 0) && !bArrivalReform && Settle.ReformDeadline >= 0.f && Settle.ReformDuration > 0.f)
        {
            // v2: the change of formation is a straight glide from the old slot to the new one in the unit's own frame, so the
            // whole company morphs as one body while it marches (no men chasing a moving slot through world space).
            const float Alpha = FMath::Clamp((VisualAnimationTime - Settle.ReformStartTime) / Settle.ReformDuration, 0.f, 1.f);
            MorphAlpha = Alpha;
            // A walker's speed: half a second to get going, an even pace, half a second to stop (a trapezoid, not the S-curve that
            // made a long path a sprint in the middle).
            const float MorphT = Alpha * Settle.ReformDuration;
            const float MorphRamp = FMath::Min(0.5f, Settle.ReformDuration * 0.5f);
            const float MorphRate = 1.f / FMath::Max(0.01f, Settle.ReformDuration - MorphRamp);
            const float Eased = MorphT < MorphRamp ? 0.5f * MorphRate * MorphT * MorphT / MorphRamp
                : MorphT < Settle.ReformDuration - MorphRamp ? 0.5f * MorphRate * MorphRamp + MorphRate * (MorphT - MorphRamp)
                : 1.f - 0.5f * MorphRate * FMath::Square(Settle.ReformDuration - MorphT) / MorphRamp;
            // (Tried 2026-11-07: detours round the square's rear corners and behind the line, each rank its own lane - QA and Allan:
            // "mange glitches til og fra karré", sharp bends, lone runners and waiting groups. Removed; the straight walk stays.)
            // The square swing: his place on the swinging arm at this point of the clock (out of the square the clock runs back).
            // Column <-> square: his own way round the square; into it all at one pace, each halting at his place.
            const FVector Local = Settle.SquareSwingKind == 3 ? SquareWrapPoint(Settle, Eased * SquareWrapLongestCm) :
                Settle.SquareSwingKind == 4 ? SquareWrapPoint(Settle, (1.f - Eased) * SquareWrapLongestCm) :
                Settle.SquareSwingKind != 0 ? SquareSwingPoint(Settle, Settle.SquareSwingKind == 1 ? Eased : 1.f - Eased) :
                FMath::Lerp(Settle.ReformStartLocal, Settle.Goal, Eased);
            // The swing and the way round are drawn in the frame they were laid in (2026-11-15: drawn in the unit's own frame the
            // men on the move stood a metre or two off from those still waiting in the square, and walked into them - 4 cm).
            FVector MorphWorld = (Settle.SquareSwingKind != 0 ? SquareSwingPlanFrame : VisualUnit).TransformPosition(Local);
            FigureWorld = SoldierOnGround(i, MorphWorld, float(Settle.Slot.Z));
            Settle.WorldVelocity = DeltaTime > 0.f ? (FigureWorld - FigureBefore) / DeltaTime : FVector::ZeroVector;
            if (Alpha >= 1.f || (Settle.SquareSwingKind == 3 && Eased * SquareWrapLongestCm >= Settle.SquareWrapDelta)) { Settle.ReformDeadline = -1.f; Settle.SquareSwingKind = 0; }
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
        // The smoothing runs in three dimensions and lags behind the target on a slope (half a metre into a 13° rise): the man
        // himself is set on the ground where he is, not only his target.
        FigureWorld = SoldierOnGround(i, FigureWorld, float(Settle.Slot.Z));
        const FVector Step = FigureWorld - FigureBefore;
        Soldier->SetRelativeLocation(VisualUnit.InverseTransformPosition(FigureWorld));
        if (i == 0) GroundProbe(TEXT("walk"), Soldier);
        // He faces the way he runs while he is far from his place, and turns to the front as he comes in.
        const float RunYaw = VisualUnit.InverseTransformVectorNoScale(Settle.WorldVelocity).Rotation().Yaw + SoldierMeshYawOffset;
        const bool bFigureBackstep = OwnerCompany->OrderComponent &&
            OwnerCompany->OrderComponent->GetCurrentOrder().Type == EStrategyOrderType::Disengage;
        // Changing formation: the man walks facing the way he goes (never sideways or backwards), and turns to the front over the last
        // fifth of the way.
        // He faces the way he goes until the last metre and a half, then turns to his place's front (not at a fixed fraction of the
        // time, which turned the far men while they still ran: QA 2026-11-05).
        // (In the square swing every man of the arm faces the way the arm goes, the slow men by the corner too, until he is nearly
        // in his place: with the common rule the men by the corner faced their new front at once and the arm looked ragged, QA 2026-11-15.)
        const bool bSquareSwingStep = Settle.SquareSwingKind != 0 && !bArrivalReform && MorphAlpha > 0.f && MorphAlpha < 1.f &&
            Distance > 40.f && Settle.WorldVelocity.SizeSquared2D() > 4.f * 4.f;
        const bool bMorphWalking = bSquareSwingStep || ((bRigidCompany || Settle.SquareSwingKind != 0) && !bArrivalReform && !bFigureBackstep && MorphAlpha > 0.f && MorphAlpha < 1.f &&
            Distance > 150.f && Settle.WorldVelocity.SizeSquared2D() > 20.f * 20.f);
        const float Yaw = bMorphWalking ? RunYaw : bFigureBackstep || Step.Size() >= Distance - 0.1f || (bRigidCompany && !bArrivalReform) ? Settle.Yaw :
            (bVisualTravel || Distance > (bArrivalReform ? 0.1f : 30.f) ? RunYaw : Settle.Yaw);
        FRotator Rot = Soldier->GetRelativeRotation();
        const float FigureTurn = FMath::FindDeltaAngleDegrees(Rot.Yaw, Yaw);
        Rot.Roll = FMath::FInterpTo(Rot.Roll, FMath::Clamp(FigureTurn * 0.08f, -4.f, 4.f), DeltaTime, 8.f);
        Rot.Yaw = FStrategyVisualFormationPath::SmoothFacing(Rot.Yaw, Settle.YawVelocity, Yaw, DeltaTime, MorphAlpha >= 0.f ? 240.f : 120.f);
        Soldier->SetRelativeRotation(Rot);
        Soldier->bPauseAnims = false;
        UAnimSequence* TravelClip = (!bSlotReforming || bArrivalReform || bRigidCompany) && Walk ? Walk : Run;
        // Closing ranks: a side-step with the front kept, to the right or (mirrored clip) to the left.
        const bool bCloseStepping = Settle.CloseStart >= 0.f && VisualAnimationTime >= Settle.CloseStart;
        const bool bCloseSideways = FMath::Abs(Settle.CloseTo.Y - Settle.CloseFrom.Y) > FMath::Abs(Settle.CloseTo.X - Settle.CloseFrom.X);
        // Marching on while closing in: he walks on and edges across (no side-step clip on the move).
        if (bCloseStepping && bCloseSideways && SideStepAsset.Get() && ActualMarchSpeed < 20.f)
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
        if (SquareSwingRunning)
        {
            SquareSwingRunning = false;
            double SquareSwingNearest = TNumericLimits<double>::Max();
            for (int32 SquareSwingA = 0; SquareSwingA < SoldierComponents.Num(); ++SquareSwingA)
            for (int32 SquareSwingB = SquareSwingA + 1; SquareSwingB < SoldierComponents.Num(); ++SquareSwingB)
                if (SoldierComponents[SquareSwingA] && SoldierComponents[SquareSwingB])
                    SquareSwingNearest = FMath::Min(SquareSwingNearest, FVector::Dist2D(SoldierComponents[SquareSwingA]->GetComponentLocation(), SoldierComponents[SquareSwingB]->GetComponentLocation()));
            UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s complete kind=%s elapsed=%.3f min-2d-cm=%.3f"),
                *OwnerCompany->StableUnitId.ToString(), bSquareWrapRunning ? TEXT("square-wrap") : TEXT("square-swing"), VisualAnimationTime - SquareSwingStarted, SquareSwingNearest);
            bSquareWrapRunning = false;
        }
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
        &RaiseToAimAsset, &LoadAsset, &RiseFromLoadAsset, &ReadyAsset, &AimHoldAsset, &DeathWalkingAsset, &DeathMarchFrontAsset, &DeathMarchBackAsset, &DeathMarchCrumpleAsset, &SideStepAsset, &SideStepLeftAsset };
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
            // A square's kneeling outer rank (ApplySquareKneel): the baked figure kneels too.
            if (!bCrowdWorld && &CrowdFigures == &SoldierComponents && SquareKneelTwins.IsValidIndex(CrowdFigureIndex) && SquareKneelTwins[CrowdFigureIndex] &&
                CrowdClips.IsValidIndex(CrowdFigureIndex))
            {
                FPlayedClip CrowdKneel = CrowdClips[CrowdFigureIndex];
                CrowdKneel.Clip = SquareKneelTwins[CrowdFigureIndex].Get();
                DataFor(&CrowdKneel, Data);
            }
            else
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
    DrillSpareRow = INDEX_NONE;
    if (DrillEntering)
    {
        // The line as it stands: losses and the colour bearer (out of the grid) leave holes, so count its files from the
        // slots (symmetric about the centre), not from the head count.
        float DrillHalfSpan = 0.f;
        for (const FSettle& DrillSpanMan : SoldierSettle) DrillHalfSpan = FMath::Max(DrillHalfSpan, FMath::Abs(DrillSpanMan.Slot.Y));
        DrillRows = FMath::Max(DrillRows, FMath::RoundToInt(2.f * DrillHalfSpan / DrillFileSpacing) + 1);
    }
    else
    {
        // Column -> line is a quarter turn: each man keeps his row as his file. A column with losses is longer than its head count
        // (its rows keep their holes), so the line gets as many files as the column has rows; the clamp below otherwise piled the
        // rear rows onto one place and sent the extra men to a free place, at times the far end of the line, through the
        // company (user 2026-11-05: "en enkelt mand gik igennem hele formationen og forsinkede dem").
        for (const FSettle& DrillRowMan : SoldierSettle) DrillRows = FMath::Max(DrillRows, DrillRowMan.SnakeRow + 1);
        // One spare row behind the tail: two men read into the same place (Test 4 2026-11-05: man 188, row 62, in every company)
        // while the only free place was at the head, where the colour bearer left the front rank. The second man stays at the
        // tail in the spare row instead of walking 62 files up through the company (see the clash below).
        DrillSpareRow = DrillRows;
        ++DrillRows;
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
        // (The spare row is only for clashes: nobody is read into it.)
        const int32 DrillLastRow = DrillSpareRow >= 0 ? DrillSpareRow - 1 : DrillRows - 1;
        const int32 DrillRow = bDrillSnakeKnown ? FMath::Clamp(SoldierSettle[DrillIndex].SnakeRow, 0, DrillLastRow) :
            FMath::Clamp(FMath::RoundToInt((DrillHeadS - DrillS0) / DrillColumnRowSpacing), 0, DrillLastRow);
        const int32 DrillLane = bDrillSnakeKnown ? FMath::Clamp(SoldierSettle[DrillIndex].SnakeLane, 0, 2) :
            FMath::Clamp(FMath::RoundToInt(DrillOffset / DrillFileSpacing) + 1, 0, 2);
        const int32 DrillRank = DrillFlank > 0.f ? DrillLane : 2 - DrillLane;
        const int32 DrillFile = DrillFlank > 0.f ? DrillRows - 1 - DrillRow : DrillRow;
        int32 DrillSlotId = DrillFile * 3 + DrillRank;
        if (DrillUsed.Contains(DrillSlotId))
        {
            // A free place at most two files off; else the spare row behind the tail (a man never crosses the company to a hole
            // at the other end: the holes close up afterwards); only without either the nearest free place anywhere.
            const int32 DrillSpareFile = DrillSpareRow < 0 ? INDEX_NONE : (DrillFlank > 0.f ? DrillRows - 1 - DrillSpareRow : DrillSpareRow);
            int32 DrillFree = INDEX_NONE;
            int32 DrillFreeCost = TNumericLimits<int32>::Max();
            for (int32 DrillPass = 0; DrillPass < 3 && DrillFree == INDEX_NONE; ++DrillPass)
                for (int32 DrillTry = 0; DrillTry < DrillRows * 3; ++DrillTry)
                {
                    if (DrillUsed.Contains(DrillTry)) continue;
                    const int32 DrillTryFiles = FMath::Abs(DrillTry / 3 - DrillFile);
                    if ((DrillPass == 0 && DrillTryFiles > 2) || (DrillPass == 1 && DrillTry / 3 != DrillSpareFile)) continue;
                    const int32 DrillTryCost = DrillTryFiles * 3 + FMath::Abs(DrillTry % 3 - DrillRank);
                    if (DrillTryCost < DrillFreeCost) { DrillFreeCost = DrillTryCost; DrillFree = DrillTry; }
                }
            if (DrillFree == INDEX_NONE) return DrillReject(TEXT("slot assignment clash (no free place on the trail)"));
            UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s to-line clash: man %d row %d lane %d moved %d files to file %d rank %d"),
                *OwnerCompany->StableUnitId.ToString(), DrillIndex, DrillRow, DrillLane, FMath::Abs(DrillFree / 3 - DrillFile), DrillFree / 3, DrillFree % 3);
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
        // The drill path keeps the height the column's origin had where it passed (its trail), not today's: a man on it stands
        // on his own ground.
        const FVector DrillWorld = SoldierOnGround(DrillIndex, FStrategyColumnDrill::Sample(DrillPath, DrillS, DrillLateral, DrillYaw), 0.f);
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
        if (DrillIndex == 0) GroundProbe(TEXT("drill"), DrillSoldier);
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

FVector UStrategyInfantryVisualComponent::ColourBearerTarget(float& OutBearerYaw, bool* OutBearerLeads) const
{
    if (OutBearerLeads) *OutBearerLeads = false;
    // In a square the colours stand in its middle, where the men are drawn (user 2026-11-05: he went out into a corner, the
    // line's rule beside the right-flank man).
    if (OwnerCompany && OwnerCompany->FormationComponent && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Square &&
        !(DrillActive && !DrillToColumn))
    {
        OutBearerYaw = OwnerCompany->GetActorRotation().Yaw;
        FVector SquareMiddle = FVector::ZeroVector;
        int32 SquareMen = 0;
        for (const USkeletalMeshComponent* SquareMan : SoldierComponents)
            if (SquareMan) { SquareMiddle += SquareMan->GetComponentLocation(); ++SquareMen; }
        return SquareMen > 0 ? SquareMiddle / SquareMen : OwnerCompany->GetActorLocation();
    }
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
                if (OutBearerLeads) *OutBearerLeads = true;
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
            if (OutBearerLeads) *OutBearerLeads = true;
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
        if (OutBearerLeads) *OutBearerLeads = true;
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
    const bool bBearerInSquare = OwnerCompany->FormationComponent && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Square;
    if (!bColoursDown && !bBearerInSquare)   // (in a square the middle is far from every man)
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

bool UStrategyInfantryVisualComponent::SpawnOfficerFigure()
{
    // Which nation's coat: the unit's nation (DK for the Danish side, PR for any other when it has none); DK if that is not modelled.
    const FString OfficerId = OwnerCompany->StableUnitId.ToString();
    const FString OfficerNation = OwnerCompany->GetNationCodeOrDefault();
    USkeletalMesh* OfficerMesh = LoadObject<USkeletalMesh>(nullptr,
        *FString::Printf(TEXT("/Game/Units/Human/Officer/SK_Officer_1864_%s.SK_Officer_1864_%s"), *OfficerNation, *OfficerNation), nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!OfficerMesh)
        OfficerMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Units/Human/Officer/SK_Officer_1864_DK.SK_Officer_1864_DK"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!OfficerMesh)
    {
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Officer %s: no officer mesh imported (/Game/Units/Human/Officer)"), *OfficerId);
        return false;
    }
    OfficerFigure = NewObject<USkeletalMeshComponent>(OwnerCompany, NAME_None, RF_Transient);
    OfficerFigure->SetSkeletalMeshAsset(OfficerMesh);
    OfficerFigure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OfficerFigure->SetGenerateOverlapEvents(false);
    OfficerFigure->SetCastShadow(true);
    OfficerFigure->bVisibleInRayTracing = false;
    OfficerFigure->SetupAttachment(OwnerCompany->SceneRoot);
    OfficerFigure->RegisterComponent();
    OfficerFigure->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    OfficerFigure->AddTickPrerequisiteComponent(this);
    // Sabre in the right hand and scabbard at the left hip: the bone-relative transforms measured in Blender (Final_v4 notes.md;
    // the scale makes up for the rig's own scale).
    auto OfficerProp = [this](const TCHAR* OfficerPropPath, FName OfficerBone, const FTransform& OfficerPropTransform) -> UStaticMeshComponent*
    {
        UStaticMesh* OfficerPropMesh = LoadObject<UStaticMesh>(nullptr, OfficerPropPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
        if (!OfficerPropMesh) return nullptr;
        UStaticMeshComponent* OfficerPropComponent = NewObject<UStaticMeshComponent>(OwnerCompany, NAME_None, RF_Transient);
        OfficerPropComponent->SetStaticMesh(OfficerPropMesh);
        OfficerPropComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        OfficerPropComponent->SetGenerateOverlapEvents(false);
        OfficerPropComponent->SetCastShadow(false);
        OfficerPropComponent->bVisibleInRayTracing = false;
        OfficerPropComponent->RegisterComponent();
        OfficerPropComponent->AttachToComponent(OfficerFigure, FAttachmentTransformRules::SnapToTargetNotIncludingScale, OfficerBone);
        OfficerPropComponent->SetRelativeTransform(OfficerPropTransform);
        return OfficerPropComponent;
    };
    OfficerSaber = OfficerProp(TEXT("/Game/Units/Human/Officer/SM_Officer_Saber.SM_Officer_Saber"), RightHandBoneName,
        FTransform(FRotator(-38.611339f, -44.486661f, -148.198385f), FVector(8.859873f, -10.981565f, 4.255536f), FVector(1.106196f)));
    OfficerScabbard = OfficerProp(TEXT("/Game/Units/Human/Officer/SM_Officer_Scabbard.SM_Officer_Scabbard"), TEXT("Hips"),
        FTransform(FRotator(-20.730171f, 84.857239f, -90.000003f), FVector(33.243320f, -9.561045f, -19.432434f), FVector(1.106196f)));
    for (TSoftObjectPtr<UAnimSequence>* OfficerClipAsset : { &OfficerIdleAsset, &OfficerWalkAsset, &OfficerWalkSheathedAsset, &OfficerOrderAsset, &OfficerDeathAsset,
        &OfficerSabreDrawAsset, &OfficerSabreSheathAsset })
        if (UAnimSequence* OfficerLoadedClip = OfficerClipAsset->LoadSynchronous()) OfficerClipsHeld.AddUnique(OfficerLoadedClip);
    if (UAnimSequence* OfficerStartIdle = OfficerIdleAsset.Get()) OfficerFigure->PlayAnimation(OfficerStartIdle, true);
    UE_LOG(LogTemp, Display, TEXT("Strategy1864Officer %s: %s officer, sabre %d, scabbard %d, clips %d of 7"), *OfficerId, *OfficerMesh->GetName(),
        OfficerSaber ? 1 : 0, OfficerScabbard ? 1 : 0, OfficerClipsHeld.Num());
    return true;
}

void UStrategyInfantryVisualComponent::SetOfficerSaberDrawn(bool bOfficerDraw)
{
    if (!OfficerSaber || !OfficerFigure || bOfficerSaberDrawn == bOfficerDraw) return;
    bOfficerSaberDrawn = bOfficerDraw;
    // Sheathed it sits in the scabbard: the sabre and the scabbard share their origin and axis, so it takes the scabbard's transform
    // on the hips (Final_v5 notes.md, "Sabel i skede").
    OfficerSaber->AttachToComponent(OfficerFigure, FAttachmentTransformRules::SnapToTargetNotIncludingScale, bOfficerDraw ? RightHandBoneName : FName(TEXT("Hips")));
    OfficerSaber->SetRelativeTransform(bOfficerDraw
        ? FTransform(FRotator(-38.611339f, -44.486661f, -148.198385f), FVector(8.859873f, -10.981565f, 4.255536f), FVector(1.106196f))
        : FTransform(FRotator(-20.730171f, 84.857239f, -90.000003f), FVector(33.243320f, -9.561045f, -19.432434f), FVector(1.106196f)));
}

void UStrategyInfantryVisualComponent::UpdateOfficerFigure(float OfficerDt)
{
    if (!OwnerCompany || !GetWorld() || OfficerDt <= 0.f || bOfficerDown || !VisualPath.bInitialized || SoldierComponents.IsEmpty()) return;
    if (!OfficerFigure)
    {
        if (bOfficerTried) return;
        bOfficerTried = true;
        if (!SpawnOfficerFigure()) return;
    }
    // He falls when wounded or taken (an officer is never killed outright in the battle's ledger), or when his company is gone.
    const UStrategyOfficerProfileComponent* OfficerProfile = OwnerCompany->OfficerProfileComponent;
    if ((OfficerProfile && OfficerProfile->Fate != 0) || OwnerCompany->CurrentStrength <= 0)
    {
        bOfficerDown = true;
        if (UAnimSequence* OfficerFall = OfficerDeathAsset.Get() ? OfficerDeathAsset.Get() : DeathAsset.Get())
        { OfficerFigure->PlayAnimation(OfficerFall, false); OfficerFigure->SetPlayRate(1.f); }
        OwnerCompany->AddUnitLog(TEXT("Officer"), TEXT("kaptajnen er faldet"), OfficerProfile && OfficerProfile->Fate == 2 ? TEXT("taget til fange") : TEXT("såret"), 0.0f);
        UE_LOG(LogTemp, Display, TEXT("Strategy1864Officer %s officer fell (fate %d)"), *OwnerCompany->StableUnitId.ToString(), OfficerProfile ? OfficerProfile->Fate : -1);
        return;
    }
    // His place (user 2026-11-03, the drawn company; an estimate, not checked against the regulations): in line on the right of the
    // front rank, one file beyond the colours; on the march at the head, one lead ahead of the colours; in a square at its centre.
    const float OfficerLateral = OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierLateralSpacingCm : 60.f;
    float OfficerYaw = 0.f;
    bool bOfficerLeads = false;
    const FVector OfficerBase = ColourBearerTarget(OfficerYaw, &bOfficerLeads);
    FVector OfficerGoal;
    const bool bOfficerInSquare = OwnerCompany->FormationComponent && OwnerCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Square;
    if (bOfficerInSquare)
    {
        // The middle of the square (the colour bearer's target there), one file beside the colours.
        OfficerGoal = OfficerBase + FRotator(0.f, OfficerYaw, 0.f).RotateVector(FVector(0.f, OfficerLateral * 1.5f, 0.f));
    }
    else if (bOfficerLeads) OfficerGoal = OfficerBase + FRotator(0.f, OfficerYaw, 0.f).Vector() * ColourColumnLeadCm;
    else OfficerGoal = OfficerBase + FRotator(0.f, OfficerYaw, 0.f).RotateVector(FVector(0.f, OfficerLateral, 0.f));
    // Like the colour bearer he keeps with the men (never more than two ranks beyond the nearest) and walks on from his own place in
    // the world, so a jump of the unit's actor does not carry him.
    const float OfficerRank = OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->SoldierRankSpacingCm : 90.f;
    const float OfficerKeepCm = FMath::Max(2.f * ColourColumnLeadCm + 30.f, 3.f * OfficerRank);
    FVector OfficerNearest = OfficerGoal;
    float OfficerNearestGap = TNumericLimits<float>::Max();
    for (const USkeletalMeshComponent* OfficerKeepMan : SoldierComponents)
    {
        if (!OfficerKeepMan) continue;
        const float OfficerKeepGap = FVector::Dist2D(OfficerKeepMan->GetComponentLocation(), OfficerGoal);
        if (OfficerKeepGap < OfficerNearestGap) { OfficerNearestGap = OfficerKeepGap; OfficerNearest = OfficerKeepMan->GetComponentLocation(); }
    }
    if (!bOfficerInSquare && OfficerNearestGap < TNumericLimits<float>::Max() && OfficerNearestGap > OfficerKeepCm)
        OfficerGoal = OfficerNearest + (OfficerGoal - OfficerNearest).GetSafeNormal2D() * OfficerKeepCm;
    OfficerGoal = UStrategyTerrainQueryLibrary::ProjectPointToTerrain(this, OfficerGoal);
    FVector OfficerHere = bOfficerWorldValid ? OfficerWorld : OfficerGoal;
    FRotator OfficerRotation = bOfficerWorldValid ? OfficerWorldRotation : FRotator(0.f, OfficerYaw + SoldierMeshYawOffset, 0.f);
    const FVector OfficerTo = OfficerGoal - OfficerHere;
    const float OfficerDistance = OfficerTo.Size2D();
    const float OfficerMarch = OwnerCompany->MovementExecutor ? OwnerCompany->MovementExecutor->MoveSpeedCmPerSecond : 150.f;
    const float OfficerCatchUp = OfficerDistance > 2.f * OfficerRank ? FMath::Max(450.f, OfficerDistance * 2.f) : 0.f;
    const float OfficerSpeed = FMath::Min(OfficerDistance * 3.f, FMath::Max3(150.f, OfficerMarch * 1.3f, OfficerCatchUp));
    const float OfficerStep = FMath::Min(OfficerDistance, OfficerSpeed * OfficerDt);
    if (OfficerDistance > 1.f) OfficerHere += OfficerTo.GetSafeNormal2D() * OfficerStep;
    OfficerHere.Z = OfficerGoal.Z;
    const bool bOfficerWalking = OfficerStep / OfficerDt > 25.f;
    OfficerRotation.Pitch = 0.f; OfficerRotation.Roll = 0.f;
    OfficerRotation.Yaw = FStrategyVisualFormationPath::SmoothFacing(OfficerRotation.Yaw, OfficerYawVelocity,
        (bOfficerWalking ? OfficerTo.Rotation().Yaw : OfficerYaw) + SoldierMeshYawOffset, OfficerDt, 160.f);
    OfficerFigure->SetWorldLocationAndRotation(OfficerHere, OfficerRotation);
    OfficerWorld = OfficerHere;
    OfficerWorldRotation = OfficerRotation;
    bOfficerWorldValid = true;
    // In a march column the sabre is in its scabbard (user 2026-10-28, Backlog); formed up, in line or square or in a fight, it is
    // drawn: at the shoulder when he walks, forward when the company is engaged (giving the orders), at rest otherwise. Standing he
    // draws and sheathes it with his own clips (the sabre goes from scabbard to hand at the grip, Final_v5: draw 0.4 s, sheath 0.8 s);
    // walking, it changes place at once. (A line on the move counts as led too, so the formation decides, not bOfficerLeads.)
    const EStrategyFormationType OfficerFormation = OwnerCompany->FormationComponent ? OwnerCompany->FormationComponent->CurrentFormation : EStrategyFormationType::Line;
    const bool bOfficerWantSheathed = (OfficerFormation == EStrategyFormationType::MarchColumn || OfficerFormation == EStrategyFormationType::DefileColumn) &&
        OwnerCompany->UnitState != EStrategyUnitState::Engaged && OfficerWalkSheathedAsset.Get();
    const float OfficerSabreNow = GetWorld()->GetTimeSeconds();
    if (OfficerSabreAction == 0 && bOfficerWantSheathed == bOfficerSaberDrawn)
    {
        UAnimSequence* OfficerSabreClip = bOfficerWantSheathed ? OfficerSabreSheathAsset.Get() : OfficerSabreDrawAsset.Get();
        if (!bOfficerWalking && OfficerSabreClip)
        {
            OfficerSabreAction = bOfficerWantSheathed ? 2 : 1;
            OfficerSabreActionStart = OfficerSabreNow;
            OfficerFigure->PlayAnimation(OfficerSabreClip, false);
        }
        else
        {
            SetOfficerSaberDrawn(!bOfficerWantSheathed);
        }
    }
    UAnimSequence* OfficerClip = nullptr;
    if (OfficerSabreAction != 0)
    {
        UAnimSequence* OfficerActionClip = OfficerSabreAction == 1 ? OfficerSabreDrawAsset.Get() : OfficerSabreSheathAsset.Get();
        const float OfficerActionAge = OfficerSabreNow - OfficerSabreActionStart;
        SetOfficerSaberDrawn(OfficerSabreAction == 1 ? OfficerActionAge >= 0.4f : OfficerActionAge < 0.8f);
        if (!OfficerActionClip || OfficerActionAge >= OfficerActionClip->GetPlayLength()) OfficerSabreAction = 0;
        else OfficerClip = OfficerActionClip;
    }
    const bool bOfficerSheathed = !bOfficerSaberDrawn;
    if (!OfficerClip)
    {
        OfficerClip = bOfficerWalking ? (bOfficerSheathed ? OfficerWalkSheathedAsset.Get() : OfficerWalkAsset.Get())
            : (!bOfficerSheathed && OwnerCompany->UnitState == EStrategyUnitState::Engaged && OfficerOrderAsset.Get()) ? OfficerOrderAsset.Get() : OfficerIdleAsset.Get();
        if (OfficerClip && OfficerFigure->GetSingleNodeInstance() && OfficerFigure->GetSingleNodeInstance()->GetAnimationAsset() != OfficerClip)
            OfficerFigure->PlayAnimation(OfficerClip, true);
    }
    // -Strategy1864GroundProbe: what he plays and where the sabre's point is against his hand, every 2 s (2026-11-05: it looked down).
    static const bool bOfficerProbe = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864GroundProbe"));
    if (bOfficerProbe && OfficerSaber && GetWorld()->GetTimeSeconds() - OfficerProbeLast > 2.0 && OfficerFigure->GetSingleNodeInstance())
    {
        OfficerProbeLast = GetWorld()->GetTimeSeconds();
        const FBox OfficerSaberBox = OfficerSaber->Bounds.GetBox();
        const FVector OfficerHand = OfficerFigure->GetSocketLocation(RightHandBoneName);
        const UAnimationAsset* OfficerPlaying = OfficerFigure->GetSingleNodeInstance()->GetAnimationAsset();
        UE_LOG(LogTemp, Display, TEXT("PROJECT1864-OFFICERPROBE %s clip=%s walking=%d drawn=%d speed=%.0f over ground: hand %.0f sabre %.0f..%.0f feet L%.0f R%.0f toes L%.0f R%.0f"),
            *OwnerCompany->StableUnitId.ToString(), OfficerPlaying ? *OfficerPlaying->GetName() : TEXT("-"), bOfficerWalking ? 1 : 0, bOfficerSaberDrawn ? 1 : 0,
            OfficerStep / OfficerDt, OfficerHand.Z - OfficerGoal.Z, OfficerSaberBox.Min.Z - OfficerGoal.Z, OfficerSaberBox.Max.Z - OfficerGoal.Z,
            OfficerFigure->GetSocketLocation(TEXT("LeftFoot")).Z - OfficerGoal.Z, OfficerFigure->GetSocketLocation(TEXT("RightFoot")).Z - OfficerGoal.Z,
            OfficerFigure->GetSocketLocation(TEXT("LeftToeBase")).Z - OfficerGoal.Z, OfficerFigure->GetSocketLocation(TEXT("RightToeBase")).Z - OfficerGoal.Z);
    }
    // The walk is the soldiers' shoulder march under him (same root speed, 137.7 cm/s).
    OfficerFigure->SetPlayRate(bOfficerWalking && OfficerSabreAction == 0 ? FMath::Clamp((OfficerStep / OfficerDt) / 137.70097f, 0.3f, 2.5f) : 1.f);
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
