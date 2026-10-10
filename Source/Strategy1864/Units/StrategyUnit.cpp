#include "StrategyUnit.h"
#include "Engine/World.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../AI/StrategyOfficerProfileComponent.h"
#include "../Artillery/StrategyArtilleryAmmunitionComponent.h"
#include "../Artillery/StrategyMortarBatteryUnit.h"
#include "../Artillery/StrategyMortarFireComponent.h"
#include "../Logistics/StrategySupplyWagonUnit.h"
#include "../Visual/StrategyCavalryVisualComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Formations/StrategyFormationPolicyComponent.h"
#include "../AI/StrategyFieldOfficerComponent.h"
#include "../Formations/StrategyFormationTransitionComponent.h"
#include "../Orders/StrategyParentExecutionComponent.h"
#include "../Navigation/StrategyRoutePlannerComponent.h"
#include "../Combat/StrategyVisibilityComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../AI/StrategyOfficerAIComponent.h"
#include "../Combat/StrategyThreatReactionComponent.h"
#include "../Command/StrategyOOBStatusComponent.h"
#include "../UI/StrategySemanticZoomComponent.h"
#include "../UI/StrategyWorldDebugComponent.h"
#include "../UI/StrategyPresentationSnapshotComponent.h"
#include "../Movement/StrategyLocalDeconflictionComponent.h"
#include "../Orders/StrategyMissionAnchorComponent.h"
#include "../AI/StrategyCommandDelayComponent.h"
#include "../Combat/StrategyConditionComponent.h"
#include "../Combat/StrategyContactComponent.h"
#include "../AI/StrategyReconComponent.h"
#include "../AI/StrategyAutonomousBattleAIComponent.h"
#include "../AI/StrategyRoutRecoveryComponent.h"
#include "../Combat/StrategyFireDisciplineComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../Combat/StrategyDirectionalCoverComponent.h"
#include "../Combat/StrategyFieldworksComponent.h"
#include "../Combat/StrategySkirmisherComponent.h"
#include "../Logistics/StrategySupplyComponent.h"
#include "../AI/StrategyDoctrineComponent.h"
#include "../AI/StrategyAutonomyComponent.h"
#include "../AI/StrategyAIDifficultyComponent.h"
#include "../AI/StrategyAITelemetryComponent.h"
#include "../AI/StrategyMissionConstraintsComponent.h"
#include "../Terrain/StrategyTerrainAwarenessComponent.h"
#include "../Visual/StrategyUniformAppearanceComponent.h"
#include "../Visual/StrategyHumanAnimationStateComponent.h"
#include "../Visual/StrategyEquipmentVisualComponent.h"
#include "../Visual/StrategyVisualCompatibilityComponent.h"
#include "../Combat/StrategyDetachmentComponent.h"
#include "../AI/StrategyNCOComponent.h"
#include "../Combat/StrategyFireDrillComponent.h"
#include "../Engineering/StrategyPositionOccupancyComponent.h"
#include "../Engineering/StrategyFortificationAssaultComponent.h"
#include "../Engineering/StrategyWorkingPartyComponent.h"
#include "../Tests/StrategySpecialistStateComponent.h"
#include "Components/MeshComponent.h"
#include "../Visual/StrategyInfantryVisualComponent.h"

AStrategyUnit::AStrategyUnit()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SelectionCollider = CreateDefaultSubobject<USphereComponent>(TEXT("SelectionCollider"));
    SelectionCollider->SetupAttachment(SceneRoot);
    SelectionCollider->InitSphereRadius(180.0f);
    SelectionCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SelectionCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
    SelectionCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    DebugLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DebugLabel"));
    DebugLabel->SetupAttachment(SceneRoot);
    DebugLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
    DebugLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    DebugLabel->SetWorldSize(80.0f);
    DebugLabel->SetText(FText::FromString(TEXT("StrategyUnit")));

    QAPlaceholderMesh =
        CreateDefaultSubobject<UStaticMeshComponent>(TEXT("QAPlaceholderMesh"));
    QAPlaceholderMesh->SetupAttachment(SceneRoot);
    QAPlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    QAPlaceholderMesh->SetGenerateOverlapEvents(false);
    QAPlaceholderMesh->SetCastShadow(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> QACubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (QACubeMesh.Succeeded())
    {
        QAPlaceholderMesh->SetStaticMesh(QACubeMesh.Object);
    }

    QAPlaceholderMesh->SetRelativeScale3D(FVector(4.0f, 2.0f, 0.5f));
    QAPlaceholderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 25.0f));

    OrderComponent = CreateDefaultSubobject<UStrategyOrderComponent>(TEXT("OrderComponent"));
    CommandComponent = CreateDefaultSubobject<UStrategyCommandComponent>(TEXT("CommandComponent"));
    MovementExecutor = CreateDefaultSubobject<UStrategyMovementExecutorComponent>(TEXT("MovementExecutor"));
    FormationComponent = CreateDefaultSubobject<UStrategyFormationComponent>(TEXT("FormationComponent"));
    FormationPolicy = CreateDefaultSubobject<UStrategyFormationPolicyComponent>(TEXT("FormationPolicy"));
    FormationTransition =
        CreateDefaultSubobject<UStrategyFormationTransitionComponent>(TEXT("FormationTransition"));
    ParentExecution = CreateDefaultSubobject<UStrategyParentExecutionComponent>(TEXT("ParentExecution"));
    RoutePlanner = CreateDefaultSubobject<UStrategyRoutePlannerComponent>(TEXT("RoutePlanner"));
    VisibilityComponent = CreateDefaultSubobject<UStrategyVisibilityComponent>(TEXT("VisibilityComponent"));
    FireControlComponent = CreateDefaultSubobject<UStrategyFireControlComponent>(TEXT("FireControlComponent"));
    CombatComponent = CreateDefaultSubobject<UStrategyCombatComponent>(TEXT("CombatComponent"));
    OfficerAIComponent = CreateDefaultSubobject<UStrategyOfficerAIComponent>(TEXT("OfficerAIComponent"));
    FieldOfficerComponent = CreateDefaultSubobject<UStrategyFieldOfficerComponent>(TEXT("FieldOfficerComponent"));
    ThreatReactionComponent = CreateDefaultSubobject<UStrategyThreatReactionComponent>(TEXT("ThreatReactionComponent"));
    OOBStatusComponent = CreateDefaultSubobject<UStrategyOOBStatusComponent>(TEXT("OOBStatusComponent"));
    SemanticZoomComponent = CreateDefaultSubobject<UStrategySemanticZoomComponent>(TEXT("SemanticZoomComponent"));
    WorldDebugComponent = CreateDefaultSubobject<UStrategyWorldDebugComponent>(TEXT("WorldDebugComponent"));
    PresentationSnapshotComponent =
        CreateDefaultSubobject<UStrategyPresentationSnapshotComponent>(TEXT("PresentationSnapshotComponent"));
    LocalDeconflictionComponent =
        CreateDefaultSubobject<UStrategyLocalDeconflictionComponent>(TEXT("LocalDeconflictionComponent"));
    MissionAnchorComponent =
        CreateDefaultSubobject<UStrategyMissionAnchorComponent>(TEXT("MissionAnchorComponent"));
    OfficerProfileComponent =
        CreateDefaultSubobject<UStrategyOfficerProfileComponent>(TEXT("OfficerProfileComponent"));
    CommandDelayComponent =
        CreateDefaultSubobject<UStrategyCommandDelayComponent>(TEXT("CommandDelayComponent"));
    ConditionComponent =
        CreateDefaultSubobject<UStrategyConditionComponent>(TEXT("ConditionComponent"));
    ContactComponent =
        CreateDefaultSubobject<UStrategyContactComponent>(TEXT("ContactComponent"));
    ReconComponent =
        CreateDefaultSubobject<UStrategyReconComponent>(TEXT("ReconComponent"));
    AutonomousBattleAIComponent =
        CreateDefaultSubobject<UStrategyAutonomousBattleAIComponent>(TEXT("AutonomousBattleAIComponent"));
    RoutRecoveryComponent =
        CreateDefaultSubobject<UStrategyRoutRecoveryComponent>(TEXT("RoutRecoveryComponent"));
    FireDisciplineComponent =
        CreateDefaultSubobject<UStrategyFireDisciplineComponent>(TEXT("FireDisciplineComponent"));
    StanceComponent =
        CreateDefaultSubobject<UStrategyStanceComponent>(TEXT("StanceComponent"));
    DirectionalCoverComponent =
        CreateDefaultSubobject<UStrategyDirectionalCoverComponent>(TEXT("DirectionalCoverComponent"));
    FieldworksComponent =
        CreateDefaultSubobject<UStrategyFieldworksComponent>(TEXT("FieldworksComponent"));
    SkirmisherComponent =
        CreateDefaultSubobject<UStrategySkirmisherComponent>(TEXT("SkirmisherComponent"));
    SupplyComponent =
        CreateDefaultSubobject<UStrategySupplyComponent>(TEXT("SupplyComponent"));
    DoctrineComponent =
        CreateDefaultSubobject<UStrategyDoctrineComponent>(TEXT("DoctrineComponent"));
    AutonomyComponent =
        CreateDefaultSubobject<UStrategyAutonomyComponent>(TEXT("AutonomyComponent"));
    AIDifficultyComponent =
        CreateDefaultSubobject<UStrategyAIDifficultyComponent>(TEXT("AIDifficultyComponent"));
    AITelemetryComponent =
        CreateDefaultSubobject<UStrategyAITelemetryComponent>(TEXT("AITelemetryComponent"));
    MissionConstraintsComponent =
        CreateDefaultSubobject<UStrategyMissionConstraintsComponent>(TEXT("MissionConstraintsComponent"));
    TerrainAwarenessComponent =
        CreateDefaultSubobject<UStrategyTerrainAwarenessComponent>(TEXT("TerrainAwarenessComponent"));
    UniformAppearanceComponent =
        CreateDefaultSubobject<UStrategyUniformAppearanceComponent>(TEXT("UniformAppearanceComponent"));
    HumanAnimationStateComponent =
        CreateDefaultSubobject<UStrategyHumanAnimationStateComponent>(TEXT("HumanAnimationStateComponent"));
    EquipmentVisualComponent =
        CreateDefaultSubobject<UStrategyEquipmentVisualComponent>(TEXT("EquipmentVisualComponent"));
    VisualCompatibilityComponent =
        CreateDefaultSubobject<UStrategyVisualCompatibilityComponent>(TEXT("VisualCompatibilityComponent"));
    DetachmentComponent =
        CreateDefaultSubobject<UStrategyDetachmentComponent>(TEXT("DetachmentComponent"));
    NCOComponent =
        CreateDefaultSubobject<UStrategyNCOComponent>(TEXT("NCOComponent"));
    FireDrillComponent =
        CreateDefaultSubobject<UStrategyFireDrillComponent>(TEXT("FireDrillComponent"));
    PositionOccupancyComponent =
        CreateDefaultSubobject<UStrategyPositionOccupancyComponent>(TEXT("PositionOccupancyComponent"));
    FortificationAssaultComponent =
        CreateDefaultSubobject<UStrategyFortificationAssaultComponent>(TEXT("FortificationAssaultComponent"));
    WorkingPartyComponent =
        CreateDefaultSubobject<UStrategyWorkingPartyComponent>(TEXT("WorkingPartyComponent"));
    SpecialistStateComponent =
        CreateDefaultSubobject<UStrategySpecialistStateComponent>(TEXT("SpecialistStateComponent"));
}

void AStrategyUnit::SetSelected(bool bNewSelected)
{
    if (bSelected == bNewSelected)
    {
        return;
    }

    bSelected = bNewSelected;
    OnSelectionChanged(bSelected);
}

FVector AStrategyUnit::GetVisualCentroid() const
{
    FVector LocalCentroid;
    if (const UStrategyInfantryVisualComponent* Visual = FindComponentByClass<UStrategyInfantryVisualComponent>())
    {
        if (Visual->GetFigureLocalCentroid(LocalCentroid)) { return GetActorTransform().TransformPosition(LocalCentroid); }
    }
    if (const UStrategyCavalryVisualComponent* Visual = FindComponentByClass<UStrategyCavalryVisualComponent>())
    {
        if (Visual->GetFigureLocalCentroid(LocalCentroid)) { return GetActorTransform().TransformPosition(LocalCentroid); }
    }
    return GetActorLocation();
}

void AStrategyUnit::RefreshDebugLabel()
{
    if (!DebugLabel)
    {
        return;
    }

    const FString NameText =
        DisplayName.IsEmpty()
        ? StableUnitId.ToString()
        : DisplayName.ToString();

    const FString StrengthText =
        FString::Printf(TEXT("%d/%d"), CurrentStrength, InitialStrength);

    DebugLabel->SetText(
        FText::FromString(
            FString::Printf(
                TEXT("[%s] %s\n%s"),
                *GetNATOEchelonSymbol(),
                *NameText,
                *StrengthText)));
}

void AStrategyUnit::RefreshQAPlaceholderVisual()
{
    if (!QAPlaceholderMesh)
    {
        return;
    }

    FVector Scale(4.8f, 2.0f, 0.55f);

    switch (Echelon)
    {
        case EStrategyEchelon::Battalion:
            Scale = FVector(4.5f, 4.5f, 0.85f);
            break;

        case EStrategyEchelon::Regiment:
            Scale = FVector(5.2f, 5.2f, 1.0f);
            break;

        case EStrategyEchelon::Brigade:
            Scale = FVector(6.0f, 6.0f, 1.15f);
            break;

        case EStrategyEchelon::Division:
            Scale = FVector(6.8f, 6.8f, 1.30f);
            break;

        case EStrategyEchelon::Cavalry:
            Scale = FVector(6.5f, 2.2f, 0.75f);
            break;

        case EStrategyEchelon::Artillery:
            Scale = FVector(6.0f, 3.2f, 0.60f);
            break;

        case EStrategyEchelon::Supply:
            Scale = FVector(5.2f, 2.8f, 0.90f);
            break;

        case EStrategyEchelon::Headquarters:
            Scale = FVector(4.2f, 4.2f, 0.90f);
            break;

        case EStrategyEchelon::Company:
        default:
            break;
    }

    QAPlaceholderMesh->SetRelativeScale3D(Scale);
    QAPlaceholderMesh->SetRelativeLocation(
        FVector(0.0f, 0.0f, Scale.Z * 50.0f));
    QAPlaceholderMesh->SetVisibility(true, true);

    if (!QAPlaceholderMaterial &&
        QAPlaceholderMesh->GetMaterial(0))
    {
        QAPlaceholderMaterial =
            UMaterialInstanceDynamic::Create(
                QAPlaceholderMesh->GetMaterial(0),
                this);

        if (QAPlaceholderMaterial)
        {
            QAPlaceholderMesh->SetMaterial(
                0,
                QAPlaceholderMaterial);
        }
    }

    if (QAPlaceholderMaterial)
    {
        FLinearColor SideColor(0.55f, 0.55f, 0.55f, 1.0f);

        switch (Side)
        {
            case EStrategySide::Denmark:
                SideColor = FLinearColor(0.08f, 0.28f, 0.90f, 1.0f);
                break;

            case EStrategySide::Prussia:
                SideColor = FLinearColor(0.75f, 0.05f, 0.04f, 1.0f);
                break;

            case EStrategySide::Austria:
                SideColor = FLinearColor(0.85f, 0.72f, 0.38f, 1.0f);
                break;

            case EStrategySide::Allied:
                SideColor = FLinearColor(0.15f, 0.70f, 0.35f, 1.0f);
                break;

            case EStrategySide::Enemy:
                SideColor = FLinearColor(0.90f, 0.08f, 0.08f, 1.0f);
                break;

            default:
                break;
        }

        // BasicShapeMaterial variants differ between engine versions.
        // Setting both common parameter names is harmless when one is absent.
        QAPlaceholderMaterial->SetVectorParameterValue(
            TEXT("Color"),
            SideColor);
        QAPlaceholderMaterial->SetVectorParameterValue(
            TEXT("BaseColor"),
            SideColor);
    }
}

int32 AStrategyUnit::ApplyStrengthLoss(int32 RequestedLoss)
{
    return ApplyStrengthLossWithCause(RequestedLoss, TEXT("Unknown"));
}

int32 AStrategyUnit::ApplyStrengthLossWithCause(int32 RequestedLoss, FName Cause, AStrategyUnit* Inflictor)
{
    if (BattleLedger.bFrozen || RequestedLoss <= 0 || CurrentStrength <= 0)
    {
        return 0;
    }

    const TWeakObjectPtr<AStrategyUnit> ReportPreviousInflictor = BattleInflictor;
    BattleInflictor = Inflictor;
    const int32 AppliedLoss = FMath::Min(RequestedLoss, CurrentStrength);
    if (AStrategyArtilleryBatteryUnit* ReportBattery = Cast<AStrategyArtilleryBatteryUnit>(this))
    {
        const FName ReportPreviousCause = BattleCasualtyCause;
        BattleCasualtyCause = Cause;
        ReportBattery->ApplyBatteryDamage(AppliedLoss, 0, 0, 0);
        BattleCasualtyCause = ReportPreviousCause;
    }
    else if (AStrategySupplyWagonUnit* ReportWagon = Cast<AStrategySupplyWagonUnit>(this))
    {
        const FName ReportPreviousCause = BattleCasualtyCause;
        BattleCasualtyCause = Cause;
        ReportWagon->ApplySupplyDamage(AppliedLoss, 0, 0.f, 0.f);
        BattleCasualtyCause = ReportPreviousCause;
    }
    else
    {
        RecordBattleLoss(AppliedLoss, Cause);
        CurrentStrength -= AppliedLoss;
    }
    BattleInflictor = ReportPreviousInflictor;
    if (CombatComponent) CombatComponent->CancelMarchUnderFire();

    if (CurrentStrength <= 0)
    {
        CurrentStrength = 0;
        SetUnitState(EStrategyUnitState::Destroyed);
    }

    if (CombatComponent) { CombatComponent->EvaluateRoutState(); }
    RefreshDebugLabel();

    OnCasualtyVisualEvent.Broadcast(
        AppliedLoss,
        GetActorLocation());
    CasualtySourceLocation = FVector::ZeroVector;

    return AppliedLoss;
}

void AStrategyUnit::SetUnitState(EStrategyUnitState NewState)
{
    if (UnitState == NewState)
    {
        return;
    }

    if (NewState == EStrategyUnitState::Destroyed || NewState == EStrategyUnitState::Abandoned)
    {
        RecordEquipmentAbandonment();
    }
    UnitState = NewState;
    OnUnitStateChanged(UnitState);
}

bool AStrategyUnit::IsCombatEffective() const
{
    return !bOutOfPlay && CurrentStrength > 0 &&
        UnitState != EStrategyUnitState::Routed &&
        UnitState != EStrategyUnitState::Disabled &&
        UnitState != EStrategyUnitState::Abandoned &&
        UnitState != EStrategyUnitState::Destroyed;
}


void AStrategyUnit::SetSemanticZoomState(EStrategySemanticZoomState NewState)
{
    const bool bChanged = SemanticZoomState != NewState;
    SemanticZoomState = NewState;

    const bool bStrategic =
        SemanticZoomState == EStrategySemanticZoomState::Strategic ||
        SemanticZoomState == EStrategySemanticZoomState::VeryFar;

    TArray<UMeshComponent*> MeshComponents;
    GetComponents<UMeshComponent>(MeshComponents);

    for (UMeshComponent* Mesh : MeshComponents)
    {
        if (IsValid(Mesh))
        {
            Mesh->SetVisibility(!bStrategic, true);
        }
    }

    const UStrategyInfantryVisualComponent* InfantryVisual =
        FindComponentByClass<UStrategyInfantryVisualComponent>();
    if (QAPlaceholderMesh && InfantryVisual && InfantryVisual->GetRenderedSoldierCount() > 0)
    {
        QAPlaceholderMesh->SetVisibility(false, true);
    }

    if (DebugLabel)
    {
        const bool bHQ =
            Echelon == EStrategyEchelon::Battalion ||
            Echelon == EStrategyEchelon::Regiment ||
            Echelon == EStrategyEchelon::Brigade ||
            Echelon == EStrategyEchelon::Division;

        const bool bShowLabel =
            bSelected ||
            (SemanticZoomState == EStrategySemanticZoomState::Medium && bHQ) ||
            SemanticZoomState == EStrategySemanticZoomState::Operational ||
            bStrategic;

        const bool bHasSoldiers = InfantryVisual && InfantryVisual->GetRenderedSoldierCount() > 0;
        DebugLabel->SetWorldLocation(GetVisualCentroid() + FVector(0.0f, 0.0f, 220.0f));
        DebugLabel->SetVisibility(bShowLabel && !bHasSoldiers);
        // Always readable: the text turns to face the camera (it was fixed to the unit's own heading, mirrored from behind).
        if (bShowLabel && !bHasSoldiers)
        {
            if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
            {
                if (PC->PlayerCameraManager)
                {
                    const FVector ToCamera = PC->PlayerCameraManager->GetCameraLocation() - DebugLabel->GetComponentLocation();
                    DebugLabel->SetWorldRotation(FRotator(0.0f, ToCamera.Rotation().Yaw, 0.0f));
                }
            }
        }
    }

    if (bChanged)
    {
        OnSemanticZoomChanged(SemanticZoomState);
    }
}

FString AStrategyUnit::GetNATOEchelonSymbol() const
{
    switch (Echelon)
    {
        case EStrategyEchelon::Company:
            return TEXT("I");

        case EStrategyEchelon::Battalion:
            return TEXT("II");

        case EStrategyEchelon::Regiment:
            return TEXT("III");

        case EStrategyEchelon::Brigade:
            return TEXT("X");

        case EStrategyEchelon::Division:
            return TEXT("XX");

        case EStrategyEchelon::Cavalry:
            return TEXT("CAV");

        case EStrategyEchelon::Artillery:
            return TEXT("I ART");

        case EStrategyEchelon::Supply:
            return TEXT("LOG");

        case EStrategyEchelon::Headquarters:
        default:
            return TEXT("HQ");
    }
}

// After-action ledger: initialized after scenario setup, before the first casualty or shot.

void AStrategyUnit::EnsureBattleLedger()
{
    if (BattleLedger.bInitialized) { return; }
    BattleLedger.bInitialized = true;
    BattleLedger.OriginalSide = static_cast<uint8>(Side);
    BattleLedger.Name = DisplayName.ToString();
    BattleLedger.StartMen = FMath::Max(0, CurrentStrength);
    BattleLedger.StartMorale = Morale;
    BattleLedger.StartAmmo = CombatComponent ? CombatComponent->AmmunitionRounds : 0;
    if (const AStrategyArtilleryBatteryUnit* ReportBattery = Cast<AStrategyArtilleryBatteryUnit>(this))
    {
        BattleLedger.StartAmmo = ReportBattery->ArtilleryAmmunitionComponent ? ReportBattery->ArtilleryAmmunitionComponent->GetTotalRounds() : 0;
    }
    if (const AStrategyMortarBatteryUnit* ReportMortar = Cast<AStrategyMortarBatteryUnit>(this))
    {
        BattleLedger.StartAmmo = ReportMortar->MortarFireComponent ? ReportMortar->MortarFireComponent->AmmunitionBombs : 0;
    }
}

void AStrategyUnit::RecordBattleLoss(int32 AppliedLoss, FName Cause)
{
    EnsureBattleLedger();
    if (BattleLedger.bFrozen || AppliedLoss <= 0) { return; }
    AppliedLoss = FMath::Min(AppliedLoss, FMath::Max(0, BattleLedger.StartMen - BattleLedger.Killed - BattleLedger.Wounded - BattleLedger.Prisoners));
    if (AppliedLoss <= 0) { return; }
    // Game estimate: campaign SplitLosses has 50% wounded and 25% prisoners; among hit men, two thirds survive.
    // Accumulated rounding prevents small volleys from making every casualty fatal.
    const int32 ReportTotal = BattleLedger.Killed + BattleLedger.Wounded + AppliedLoss;
    const int32 ReportWoundedBefore = BattleLedger.Wounded;
    BattleLedger.Wounded = (ReportTotal * 2) / 3;
    if (WorkingPartyComponent) { WorkingPartyComponent->AddWoundedForCollection(BattleLedger.Wounded - ReportWoundedBefore); }
    if (GetWorld()) { BattleLedger.CombatUntil = GetWorld()->GetTimeSeconds() + 10.f; }
    const int32 ReportKilledBefore = BattleLedger.Killed;
    BattleLedger.Killed = ReportTotal - BattleLedger.Wounded;
    if (AStrategyUnit* ReportAttacker = BattleInflictor.Get())
    {
        ReportAttacker->EnsureBattleLedger();
        if (!ReportAttacker->BattleLedger.bFrozen && ReportAttacker->BattleLedger.OriginalSide != BattleLedger.OriginalSide)
        {
            ReportAttacker->BattleLedger.EnemyKilled += BattleLedger.Killed - ReportKilledBefore;
        }
    }
    BattleLedger.LossByCause.FindOrAdd(Cause) += AppliedLoss;
    if (Echelon == EStrategyEchelon::Company || Echelon == EStrategyEchelon::Cavalry)
    {
        // Casualty weapons remain on the field; only recovered enemy weapons become booty.
        BattleLedger.Lost.SmallArms += AppliedLoss;
        BattleLedger.Abandoned.SmallArms += AppliedLoss;
        if (Echelon == EStrategyEchelon::Cavalry) { BattleLedger.Lost.Horses += AppliedLoss; } // One mount per lost rider: game estimate.
    }
}

void AStrategyUnit::RecordBattleVolley(int32 Rounds)
{
    EnsureBattleLedger();
    if (BattleLedger.bFrozen || Rounds <= 0) { return; }
    BattleLedger.AmmoFired += Rounds;
    if (GetWorld()) { BattleLedger.CombatUntil = GetWorld()->GetTimeSeconds() + 10.f; }
    BattleLedger.LastVolleyRounds = Rounds;
    ++BattleLedger.Volleys;
    BattleLedger.VolleyRounds.Add(Rounds);
}

FStrategyReportEquipment AStrategyUnit::RemainingReportEquipment() const
{
    FStrategyReportEquipment ReportKit;
    if (const AStrategyArtilleryBatteryUnit* ReportBattery = Cast<AStrategyArtilleryBatteryUnit>(this))
    {
        ReportKit.Guns = FMath::Max(0, ReportBattery->GunCount - ReportBattery->DestroyedGunCount); // Disabled guns can be repaired after capture.
        ReportKit.Horses = ReportBattery->HorseStrength;
        if (const AStrategyMortarBatteryUnit* ReportMortar = Cast<AStrategyMortarBatteryUnit>(this))
        {
            ReportKit.Guns = 0;
            ReportKit.Mortars = FMath::Max(0, ReportMortar->MortarPieceCount - ReportMortar->DestroyedGunCount);
        }
    }
    else if (const AStrategySupplyWagonUnit* ReportWagon = Cast<AStrategySupplyWagonUnit>(this))
    {
        ReportKit.Wagons = ReportWagon->WagonCondition > 0.f ? 1 : 0;
        ReportKit.Horses = ReportWagon->HorseStrength;
    }
    else if (Echelon == EStrategyEchelon::Company || Echelon == EStrategyEchelon::Cavalry)
    {
        ReportKit.SmallArms = FMath::Max(0, CurrentStrength);
        ReportKit.Horses = Echelon == EStrategyEchelon::Cavalry ? FMath::Max(0, CurrentStrength) : 0;
        ReportKit.Colours = 1; // Abstract company/squadron colour; game estimate.
    }
    return ReportKit;
}

void AStrategyUnit::RecordEquipmentAbandonment()
{
    EnsureBattleLedger();
    if (BattleLedger.bFrozen || BattleLedger.bEquipmentAbandoned) { return; }
    BattleLedger.bEquipmentAbandoned = true;
    const FStrategyReportEquipment ReportRemaining = RemainingReportEquipment();
    BattleLedger.Abandoned.Add(ReportRemaining);
    BattleLedger.Lost.Add(ReportRemaining);
}

void AStrategyUnit::RecordEquipmentCapture(AStrategyUnit* Captor)
{
    if (!Captor || Captor->Side == Side) { return; }
    RecordEquipmentAbandonment();
    Captor->EnsureBattleLedger();
    if (BattleLedger.bFrozen || Captor->BattleLedger.bFrozen) { return; }
    Captor->BattleLedger.Captured.Add(BattleLedger.Abandoned);
    BattleLedger.Abandoned = FStrategyReportEquipment(); // Each abandoned item can only be awarded once.
}

void AStrategyUnit::CaptureBattlePrisoners(AStrategyUnit* Captor)
{
    EnsureBattleLedger();
    if (!Captor || Captor->Side == Side || !Captor->IsCombatEffective() || BattleLedger.bFrozen || CurrentStrength <= 0 || UnitState != EStrategyUnitState::Routed) { return; }
    const int32 ReportCapturedMen = FMath::Min(CurrentStrength, FMath::Max(0, BattleLedger.StartMen - BattleLedger.Killed - BattleLedger.Wounded - BattleLedger.Prisoners));
    BattleLedger.Prisoners += ReportCapturedMen;
    if (OfficerProfileComponent && OfficerProfileComponent->Fate < 2)
    {
        OfficerProfileComponent->Fate = 2;
        OfficerProfileComponent->Impairment = .4f;
    }
    BattleLedger.LossByCause.FindOrAdd(TEXT("Overrun")) += ReportCapturedMen;
    RecordEquipmentCapture(Captor);
    CurrentStrength = 0;
    if (AStrategyArtilleryBatteryUnit* ReportBattery = Cast<AStrategyArtilleryBatteryUnit>(this))
    {
        ReportBattery->CrewStrength = 0;
        ReportBattery->DriverStrength = 0;
    }
    if (MovementExecutor) { MovementExecutor->StopMovement(); }
    if (OrderComponent) { OrderComponent->ClearOrder(); }
    SetUnitState(EStrategyUnitState::Destroyed);
    RefreshDebugLabel();
}
