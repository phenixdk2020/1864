#include "StrategyUnit.h"
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
#include "../AI/StrategyOfficerProfileComponent.h"
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
    if (RequestedLoss <= 0 || CurrentStrength <= 0)
    {
        return 0;
    }

    const int32 AppliedLoss = FMath::Min(RequestedLoss, CurrentStrength);
    CurrentStrength -= AppliedLoss;

    if (CurrentStrength <= 0)
    {
        CurrentStrength = 0;
        SetUnitState(EStrategyUnitState::Destroyed);
    }

    RefreshDebugLabel();

    OnCasualtyVisualEvent.Broadcast(
        AppliedLoss,
        GetActorLocation());

    return AppliedLoss;
}

void AStrategyUnit::SetUnitState(EStrategyUnitState NewState)
{
    if (UnitState == NewState)
    {
        return;
    }

    UnitState = NewState;
    OnUnitStateChanged(UnitState);
}

bool AStrategyUnit::IsCombatEffective() const
{
    return CurrentStrength > 0 &&
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

        DebugLabel->SetVisibility(bShowLabel);
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
