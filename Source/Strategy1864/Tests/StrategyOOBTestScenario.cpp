#include "StrategyOOBTestScenario.h"

#include "../Command/StrategyCommandComponent.h"
#include "../Units/StrategyCompanyUnit.h"
#include "../Units/StrategyHQUnit.h"
#include "../Units/StrategyUnit.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyDragoonComponent.h"
#include "../Navigation/StrategyRiverBarrier.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Navigation/StrategyNavigationObstacle.h"
#include "../AI/StrategyOfficerProfileComponent.h"
#include "../AI/StrategyCommandDelayComponent.h"
#include "../Combat/StrategyConditionComponent.h"
#include "../Combat/StrategyContactComponent.h"
#include "../AI/StrategyReconComponent.h"
#include "../AI/StrategyAutonomousBattleAIComponent.h"
#include "../AI/StrategyRoutRecoveryComponent.h"
#include "../AI/StrategyCavalryScreenAIComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
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
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryDeploymentComponent.h"
#include "../Artillery/StrategyArtilleryAmmunitionComponent.h"
#include "../Artillery/StrategyArtilleryFireMissionComponent.h"
#include "../Artillery/StrategyArtilleryDamageComponent.h"
#include "../Artillery/StrategyArtilleryCaptureComponent.h"
#include "../Artillery/StrategyArtilleryTraverseComponent.h"
#include "../Artillery/StrategyArtilleryRepairComponent.h"
#include "../Logistics/StrategySupplyWagonUnit.h"
#include "../Logistics/StrategySupplyCargoComponent.h"
#include "../Logistics/StrategySupplyCaptureComponent.h"
#include "../Terrain/StrategyTerrainFeature.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "../Terrain/StrategyTerrainAwarenessComponent.h"
#include "../Artillery/StrategyArtilleryPositioningComponent.h"
#include "../Artillery/StrategyArtilleryProjectilePresentationComponent.h"
#include "../Artillery/StrategyArtilleryTrajectoryLibrary.h"
#include "../Artillery/StrategyArtilleryProjectileTypes.h"
#include "../Artillery/StrategyArtilleryCrewAnimationComponent.h"
#include "../Artillery/StrategyMortarBatteryUnit.h"
#include "../Artillery/StrategyMortarDeploymentComponent.h"
#include "../Artillery/StrategyMortarFireComponent.h"
#include "../Engineering/StrategyDefensivePosition.h"
#include "../Combat/StrategyDetachmentComponent.h"
#include "../AI/StrategyNCOComponent.h"
#include "../Combat/StrategyFireDrillComponent.h"
#include "../Engineering/StrategyWorkingPartyComponent.h"
#include "StrategySpecialistStateComponent.h"
#include "../Visual/StrategyUniformAppearanceComponent.h"
#include "../Visual/StrategyUniformPresetLibrary.h"
#include "../Visual/StrategyHumanAnimationStateComponent.h"
#include "../Visual/StrategyEquipmentVisualComponent.h"
#include "../Visual/StrategyVisualCompatibilityComponent.h"
#include "../Visual/StrategyAnimationManifestLibrary.h"
#include "../Visual/StrategyHorseAnimationStateComponent.h"
#include "../Visual/StrategyMountedAnimationSyncComponent.h"
#include "../Visual/StrategyInfantryVisualComponent.h"
#include "Engine/World.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

AStrategyOOBTestScenario::AStrategyOOBTestScenario()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    QAMapRoot =
        CreateDefaultSubobject<USceneComponent>(TEXT("QAMapRoot"));
    SetRootComponent(QAMapRoot);

    QAFlatGround =
        CreateDefaultSubobject<UStaticMeshComponent>(TEXT("QAFlatGround"));
    QAFlatGround->SetupAttachment(QAMapRoot);
    QAFlatGround->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    QAFlatGround->SetCollisionObjectType(ECC_WorldStatic);
    QAFlatGround->SetCollisionResponseToAllChannels(ECR_Block);
    QAFlatGround->SetGenerateOverlapEvents(false);
    QAFlatGround->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (CubeMesh.Succeeded())
    {
        QAFlatGround->SetStaticMesh(CubeMesh.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GridMaterial(
        TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));

    if (GridMaterial.Succeeded())
    {
        QAFlatGround->SetMaterial(0, GridMaterial.Object);
    }
}

void AStrategyOOBTestScenario::BeginPlay()
{
    Super::BeginPlay();

    if (QAFlatGround)
    {
        const float XYScale =
            FMath::Max(10000.0f, FlatMapSizeCm) / 100.0f;

        QAFlatGround->SetRelativeScale3D(
            FVector(XYScale, XYScale, 0.10f));

        // Engine BasicShapes Cube is 100 cm tall. With Z scale 0.10,
        // placing the center at -5 cm makes the upper surface exactly Z=0.
        QAFlatGround->SetRelativeLocation(
            FVector(0.0f, 0.0f, -5.0f));

        QAFlatGround->SetVisibility(bUseFlatQAMap, true);
        QAFlatGround->SetCollisionEnabled(
            bUseFlatQAMap
            ? ECollisionEnabled::QueryAndPhysics
            : ECollisionEnabled::NoCollision);
    }

    if (bBuildOnBeginPlay)
    {
        BuildTestOOB();
    }
}

void AStrategyOOBTestScenario::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bDrawRuntimeQAVisuals)
    {
        DrawRuntimeQAVisuals();
    }
}

void AStrategyOOBTestScenario::BuildTestOOB()
{
    ClearSpawnedUnits();

    if (bSpawnTerrainQA)
    {
        SpawnTerrainFeature(
            EStrategyTerrainFeatureType::Hill,
            Origin + FVector(1800.0f, 7200.0f, 0.0f),
            FVector2D(5200.0f, 4300.0f),
            750.0f,
            0.0f);

        SpawnTerrainFeature(
            EStrategyTerrainFeatureType::Ridge,
            Origin + FVector(13000.0f, 1500.0f, 0.0f),
            FVector2D(2200.0f, 3200.0f),
            1800.0f,
            0.0f);

        SpawnTerrainFeature(
            EStrategyTerrainFeatureType::Depression,
            Origin + FVector(17500.0f, -9000.0f, 0.0f),
            FVector2D(3500.0f, 2600.0f),
            550.0f,
            15.0f);
    }

    AStrategyHQUnit* Division = SpawnHQ(
        TEXT("DK-DIV-1"),
        TEXT("1. Division"),
        static_cast<uint8>(EStrategyHQLevel::Division),
        Origin + FVector(0.0f, 0.0f, 0.0f),
        nullptr);

    if (Division && Division->SupplyComponent)
    {
        Division->SupplyComponent->bActsAsSupplySource =
            !bSpawnSupplyWagonQA;

        Division->SupplyComponent->StoredAmmunitionRounds =
            bSpawnSupplyWagonQA ? 0 : 50000;

        Division->SupplyComponent->MaxStoredAmmunitionRounds =
            bSpawnSupplyWagonQA ? 0 : 50000;

        Division->SupplyComponent->ResupplyRadiusCm = 30000.0f;
    }

    AStrategyHQUnit* Brigade = SpawnHQ(
        TEXT("DK-BDE-1"),
        TEXT("1. Brigade"),
        static_cast<uint8>(EStrategyHQLevel::Brigade),
        Origin + FVector(1800.0f, 0.0f, 0.0f),
        Division);

    AStrategyHQUnit* Regiment = SpawnHQ(
        TEXT("DK-REG-1"),
        TEXT("1. Regiment"),
        static_cast<uint8>(EStrategyHQLevel::Regiment),
        Origin + FVector(3600.0f, 0.0f, 0.0f),
        Brigade);

    AStrategyHQUnit* MajorA = SpawnHQ(
        TEXT("DK-REG-1-MAJ-A"),
        TEXT("Major A"),
        static_cast<uint8>(EStrategyHQLevel::Battalion),
        Origin + FVector(5400.0f, -2200.0f, 0.0f),
        Regiment);

    AStrategyHQUnit* MajorB = SpawnHQ(
        TEXT("DK-REG-1-MAJ-B"),
        TEXT("Major B"),
        static_cast<uint8>(EStrategyHQLevel::Battalion),
        Origin + FVector(5400.0f, 2200.0f, 0.0f),
        Regiment);

    if (bSpawnCavalryQA)
    {
        SpawnCavalry(
            TEXT("DK-CAV-1"),
            TEXT("Gardehusar QA"),
            Origin + FVector(2500.0f, -5000.0f, 0.0f),
            Division);

        if (ACavalryUnit* Dragoon = SpawnCavalry(
            TEXT("DK-CAV-2"),
            TEXT("Dragon QA"),
            Origin + FVector(2500.0f, 5000.0f, 0.0f),
            Division))
        {
            if (Dragoon->DragoonComponent)
            {
                Dragoon->DragoonComponent->Role =
                    EStrategyCavalryRole::Dragoon;
            }
        }
    }

    if (bSpawnArtilleryQA)
    {
        SpawnArtilleryBattery(
            TEXT("DK-ART-BAT-1"),
            TEXT("Artilleribatteri QA"),
            Origin + FVector(1500.0f, 7800.0f, 0.0f),
            Division);
    }

    if (bSpawnSupplyWagonQA)
    {
        SpawnSupplyWagon(
            TEXT("DK-SUP-WAGON-1"),
            TEXT("Ammunitionsvogn QA"),
            Origin + FVector(1500.0f, 5600.0f, 0.0f),
            Division);
    }

    if (bSpawnMortarQA)
    {
        SpawnMortarBattery(
            TEXT("DK-MORTAR-1"),
            TEXT("Tungt morterbatteri QA"),
            Origin + FVector(1800.0f, 10000.0f, 0.0f),
            Division);
    }

    if (bSpawnFortificationQA && GetWorld())
    {
        AStrategyDefensivePosition* Position =
            GetWorld()->SpawnActor<AStrategyDefensivePosition>(
                AStrategyDefensivePosition::StaticClass(),
                Origin + FVector(10800.0f, -7200.0f, 0.0f),
                FRotator::ZeroRotator);

        if (Position)
        {
            Position->PositionType =
                EStrategyDefensivePositionType::GunEmplacement;
            Position->OwningSide = EStrategySide::Denmark;
            Position->LengthCm = 6200.0f;
            Position->DepthCm = 900.0f;
            Position->CapacityMen = 240;
            Position->Condition = 100.0f;
            Position->RefreshNavigationObstacle();
            SpawnedDefensivePositions.Add(Position);
        }
    }

    for (int32 Index = 0; Index < 4; ++Index)
    {
        const int32 CompanyNumber = Index + 1;
        AStrategyCompanyUnit* Company = SpawnCompany(
            FName(*FString::Printf(TEXT("DK-REG-1-A-C%d"), CompanyNumber)),
            FString::Printf(TEXT("%d. Kompagni"), CompanyNumber),
            CompanyNumber,
            Origin + FVector(7600.0f, -4300.0f + Index * CompanySpacing, 0.0f),
            MajorA,
            static_cast<uint8>(EStrategySide::Denmark));

        if (bSpawnRealInfantryVisualQA &&
            Company &&
            CompanyNumber == 1 &&
            Company->InfantryVisualComponent)
        {
            Company->InfantryVisualComponent->SetEnabled(true);
        }

        if (bSpawnSpecialistQA && Company && CompanyNumber == 1)
        {
            if (Company->DetachmentComponent)
            {
                Company->DetachmentComponent->CreateDetachment(
                    EStrategyDetachmentType::Marksman,
                    12,
                    72,
                    Company->GetActorLocation() + FVector(1800.0f, -400.0f, 0.0f));
            }

            if (Company->NCOComponent)
            {
                Company->NCOComponent->NCOStrength = 10;
                Company->NCOComponent->NCOQuality = 65.0f;
            }

            if (Company->FireDrillComponent)
            {
                Company->FireDrillComponent->SetLoadingMethod(
                    EStrategyLoadingMethod::MuzzleLoader);
                Company->FireDrillComponent->SetResearchLevel(
                    EStrategyFireDrillResearchLevel::FireByRank);
                Company->FireDrillComponent->SetDrillMode(
                    EStrategyFireDrillMode::FireByRank);
                Company->FireDrillComponent->DrillTraining = 62.0f;
                Company->FireDrillComponent->FireDiscipline = 60.0f;
            }

            if (Company->WorkingPartyComponent)
            {
                Company->WorkingPartyComponent->AvailableWorkers = 18;
            }

            if (Company->SpecialistStateComponent)
            {
                Company->SpecialistStateComponent->CaptureSnapshot();
            }
        }
    }

    for (int32 Index = 0; Index < 4; ++Index)
    {
        const int32 CompanyNumber = Index + 5;
        SpawnCompany(
            FName(*FString::Printf(TEXT("DK-REG-1-B-C%d"), CompanyNumber)),
            FString::Printf(TEXT("%d. Kompagni"), CompanyNumber),
            CompanyNumber,
            Origin + FVector(9800.0f, -4300.0f + Index * CompanySpacing, 0.0f),
            MajorB,
            static_cast<uint8>(EStrategySide::Denmark));
    }

    if (bSpawnRiverQA && GetWorld())
    {
        SpawnedRiverBarrier = GetWorld()->SpawnActor<AStrategyRiverBarrier>(
            AStrategyRiverBarrier::StaticClass(),
            Origin + FVector(15000.0f, 0.0f, 0.0f),
            FRotator::ZeroRotator);

        if (SpawnedRiverBarrier)
        {
            SpawnedRiverBarrier->RiverAxisDirection = FVector(0.0f, 1.0f, 0.0f);
            SpawnedRiverBarrier->RiverHalfWidthCm = 1200.0f;
            SpawnedRiverBarrier->BankAApproachOffset = FVector(-1800.0f, 0.0f, 0.0f);
            SpawnedRiverBarrier->BankBApproachOffset = FVector(1800.0f, 0.0f, 0.0f);
            SpawnedRiverBarrier->ExitClearanceCm = 3600.0f;
        }
    }

    if (bSpawnObstacleQA && GetWorld())
    {
        SpawnedNavigationObstacle =
            GetWorld()->SpawnActor<AStrategyNavigationObstacle>(
                AStrategyNavigationObstacle::StaticClass(),
                Origin + FVector(9000.0f, -12000.0f, 0.0f),
                FRotator::ZeroRotator);

        if (SpawnedNavigationObstacle)
        {
            SpawnedNavigationObstacle->ObstacleType =
                EStrategyObstacleType::Fence;
            SpawnedNavigationObstacle->HalfExtentCm =
                FVector(1800.0f, 250.0f, 150.0f);
            SpawnedNavigationObstacle->ClearanceCm = 700.0f;
        }
    }

    if (bSpawnEnemyQAUnits)
    {
        SpawnCompany(
            TEXT("PR-QA-C1"),
            TEXT("PR. 1. KOMPAGNI"),
            1,
            Origin + FVector(22000.0f, -3500.0f, 0.0f),
            nullptr,
            static_cast<uint8>(EStrategySide::Prussia));

        SpawnCompany(
            TEXT("PR-QA-C2"),
            TEXT("PR. 2. KOMPAGNI"),
            2,
            Origin + FVector(22000.0f, 3500.0f, 0.0f),
            nullptr,
            static_cast<uint8>(EStrategySide::Prussia));
    }

    for (AStrategyUnit* Unit : SpawnedUnitObjects)
    {
        if (!IsValid(Unit))
        {
            continue;
        }

        if (Unit->UniformAppearanceComponent)
        {
            FStrategyUniformPreset Preset =
                UStrategyUniformPresetLibrary::MakeNeutralQAPreset();

            if (Unit->Echelon == EStrategyEchelon::Artillery)
            {
                Preset =
                    UStrategyUniformPresetLibrary::MakeArtilleryQAPreset();
            }
            else if (Unit->Side == EStrategySide::Denmark)
            {
                Preset =
                    UStrategyUniformPresetLibrary::MakeDanishQAPreset();
            }
            else if (Unit->Side == EStrategySide::Prussia)
            {
                Preset =
                    UStrategyUniformPresetLibrary::MakePrussianQAPreset();
            }

            Unit->UniformAppearanceComponent->SetPreset(
                Preset,
                true);
        }

        if (Unit->EquipmentVisualComponent)
        {
            if (Unit->Echelon == EStrategyEchelon::Company)
            {
                Unit->EquipmentVisualComponent->PrimaryWeaponId =
                    TEXT("RIFLE_1864");
            }
            else if (Unit->Echelon == EStrategyEchelon::Cavalry)
            {
                Unit->EquipmentVisualComponent->PrimaryWeaponId =
                    TEXT("SABRE_1864");
            }
            else if (Unit->Echelon == EStrategyEchelon::Artillery)
            {
                Unit->EquipmentVisualComponent->PrimaryWeaponId =
                    TEXT("ARTILLERY_TOOL");
            }
        }

        if (Unit->StableUnitId == FName(TEXT("PR-QA-C2")) &&
            Unit->UniformAppearanceComponent)
        {
            FStrategyUniformOverrides Overrides;
            Overrides.bOverrideAccent = true;
            Overrides.Accent =
                FLinearColor(0.15f, 0.65f, 0.85f, 1.0f);

            Unit->UniformAppearanceComponent->SetOverrides(
                Overrides,
                true);
        }
    }

    for (AStrategyUnit* Unit : SpawnedUnitObjects)
    {
        ConfigureRuntimeQALabel(Unit);
    }

    for (AStrategyUnit* Unit : SpawnedUnitObjects)
    {
        if (IsValid(Unit) && Unit->CombatComponent)
        {
            const int32 UnitSeed =
                QARandomSeed ^
                static_cast<int32>(GetTypeHash(Unit->StableUnitId));

            Unit->CombatComponent->SetDeterministicRandomSeed(UnitSeed);
        }

        if (AStrategyArtilleryBatteryUnit* Battery =
            Cast<AStrategyArtilleryBatteryUnit>(Unit))
        {
            const int32 ArtillerySeed =
                QARandomSeed ^
                static_cast<int32>(GetTypeHash(Battery->StableUnitId)) ^
                0x7719;

            if (Battery->ArtilleryDamageComponent)
            {
                Battery->ArtilleryDamageComponent
                    ->SetDeterministicRandomSeed(ArtillerySeed);
            }
        }
    }

    TArray<FString> ValidationErrors;
    const bool bHierarchyValid = ValidateStableIdsAndHierarchy(ValidationErrors);

    if (bHierarchyValid)
    {
        UE_LOG(
            LogTemp,
            Display,
            TEXT("PROJECT1864-QA: StableId/Hierarchy validation PASS (%d errors)"),
            ValidationErrors.Num());
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("PROJECT1864-QA: StableId/Hierarchy validation FAIL (%d errors)"),
            ValidationErrors.Num());
    }

    for (const FString& Error : ValidationErrors)
    {
        UE_LOG(LogTemp, Error, TEXT("PROJECT1864-QA: %s"), *Error);
    }

    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-OOB: spawned %d units"), SpawnedUnitObjects.Num());

    for (AStrategyUnit* Unit : SpawnedUnitObjects)
    {
        if (!IsValid(Unit))
        {
            continue;
        }

        const FString ParentName =
            Unit->CommandComponent && Unit->CommandComponent->CurrentCommandParent
            ? Unit->CommandComponent->CurrentCommandParent->DisplayName.ToString()
            : TEXT("<ROOT>");

        UE_LOG(
            LogTemp,
            Display,
            TEXT("PROJECT1864-OOB: %s [%s] CurrentParent=%s OrganicSubordinates=%d CurrentSubordinates=%d"),
            *Unit->DisplayName.ToString(),
            *Unit->StableUnitId.ToString(),
            *ParentName,
            Unit->CommandComponent ? Unit->CommandComponent->OrganicSubordinates.Num() : 0,
            Unit->CommandComponent ? Unit->CommandComponent->CurrentSubordinates.Num() : 0);
    }

    TArray<FString> RegressionFailures;
    const bool bRegressionPass = RunRegressionChecklist(RegressionFailures);

    if (bRegressionPass)
    {
        UE_LOG(
            LogTemp,
            Display,
            TEXT("PROJECT1864-QA: regression checklist PASS (%d failures)"),
            RegressionFailures.Num());
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("PROJECT1864-QA: regression checklist FAIL (%d failures)"),
            RegressionFailures.Num());
    }

    for (const FString& Failure : RegressionFailures)
    {
        UE_LOG(LogTemp, Error, TEXT("PROJECT1864-QA: %s"), *Failure);
    }
}

void AStrategyOOBTestScenario::ClearSpawnedUnits()
{
    for (AStrategyUnit* Unit : SpawnedUnitObjects)
    {
        if (IsValid(Unit))
        {
            Unit->Destroy();
        }
    }

    SpawnedUnitObjects.Reset();

    if (IsValid(SpawnedRiverBarrier))
    {
        SpawnedRiverBarrier->Destroy();
    }

    SpawnedRiverBarrier = nullptr;

    if (IsValid(SpawnedNavigationObstacle))
    {
        SpawnedNavigationObstacle->Destroy();
    }

    SpawnedNavigationObstacle = nullptr;

    for (AStrategyTerrainFeature* Feature : SpawnedTerrainFeatures)
    {
        if (IsValid(Feature))
        {
            Feature->Destroy();
        }
    }

    SpawnedTerrainFeatures.Reset();

    for (AStrategyDefensivePosition* Position : SpawnedDefensivePositions)
    {
        if (IsValid(Position))
        {
            Position->Destroy();
        }
    }

    SpawnedDefensivePositions.Reset();
}

AStrategyHQUnit* AStrategyOOBTestScenario::SpawnHQ(
    const FName StableId,
    const FString& Name,
    uint8 HQLevelValue,
    const FVector& Location,
    AStrategyUnit* OrganicParent)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const FVector SpawnLocation =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            this,
            Location);

    AStrategyHQUnit* HQ = World->SpawnActor<AStrategyHQUnit>(
        AStrategyHQUnit::StaticClass(),
        SpawnLocation,
        FRotator::ZeroRotator);

    if (!HQ)
    {
        return nullptr;
    }

    HQ->StableUnitId = StableId;
    HQ->DisplayName = FText::FromString(Name);
    HQ->HQLevel = static_cast<EStrategyHQLevel>(HQLevelValue);
    HQ->ApplyHQLevelDefaults();
    HQ->InitialStrength = 1;
    HQ->CurrentStrength = 1;
    HQ->Side = EStrategySide::Denmark;
    HQ->RefreshDebugLabel();

    if (HQ->CommandComponent)
    {
        HQ->CommandComponent->SetOrganicParent(OrganicParent);
    }

    SpawnedUnitObjects.Add(HQ);
    return HQ;
}

AStrategyCompanyUnit* AStrategyOOBTestScenario::SpawnCompany(
    const FName StableId,
    const FString& Name,
    int32 CompanyNumber,
    const FVector& Location,
    AStrategyUnit* OrganicParent,
    uint8 SideValue)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const FVector SpawnLocation =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            this,
            Location);

    AStrategyCompanyUnit* Company = World->SpawnActor<AStrategyCompanyUnit>(
        AStrategyCompanyUnit::StaticClass(),
        SpawnLocation,
        FRotator::ZeroRotator);

    if (!Company)
    {
        return nullptr;
    }

    Company->StableUnitId = StableId;
    Company->DisplayName = FText::FromString(Name);
    Company->CompanyNumber = CompanyNumber;
    Company->InitialStrength = 190;
    Company->CurrentStrength = 190;
    Company->Side = static_cast<EStrategySide>(SideValue);
    Company->bPlayerControllable = Company->Side == EStrategySide::Denmark;
    Company->RefreshDebugLabel();

    if (Company->CommandComponent)
    {
        Company->CommandComponent->SetOrganicParent(OrganicParent);
    }

    const bool bPrussian =
        Company->Side == EStrategySide::Prussia;

    if (Company->FireControlComponent)
    {
        Company->FireControlComponent->SetFirePolicy(
            EStrategyFirePolicy::Medium);
    }

    if (Company->FireDisciplineComponent)
    {
        Company->FireDisciplineComponent->Discipline =
            EStrategyFireDiscipline::Volley;
        Company->FireDisciplineComponent->bConserveAmmunition = false;
    }

    if (Company->DoctrineComponent)
    {
        Company->DoctrineComponent->Doctrine =
            bPrussian
            ? EStrategyDoctrine::Offensive
            : EStrategyDoctrine::Defensive;

        Company->DoctrineComponent->CommanderOrderAggression =
            bPrussian ? 65.0f : 35.0f;
    }

    if (Company->AutonomyComponent)
    {
        Company->AutonomyComponent->Autonomy =
            bPrussian
            ? EStrategyAutonomyLevel::Independent
            : EStrategyAutonomyLevel::Normal;
    }

    if (Company->MissionConstraintsComponent)
    {
        Company->MissionConstraintsComponent->bDoNotPursue = true;
        Company->MissionConstraintsComponent->bConserveAmmunition = false;
    }

    if (Company->OfficerProfileComponent)
    {
        if (bPrussian && CompanyNumber == 1)
        {
            Company->OfficerProfileComponent->Leadership = 62.0f;
            Company->OfficerProfileComponent->Inspiration = 55.0f;
            Company->OfficerProfileComponent->TacticalSkill = 68.0f;
            Company->OfficerProfileComponent->Initiative = 65.0f;
            Company->OfficerProfileComponent->StaffQuality = 60.0f;
            Company->OfficerProfileComponent->Aggression = 72.0f;
            Company->OfficerProfileComponent->Caution = 28.0f;
            Company->OfficerProfileComponent->Discipline = 65.0f;
            Company->OfficerProfileComponent->Composure = 60.0f;
            Company->OfficerProfileComponent->Experience = 55.0f;
        }
        else if (bPrussian)
        {
            Company->OfficerProfileComponent->Leadership = 48.0f;
            Company->OfficerProfileComponent->Inspiration = 44.0f;
            Company->OfficerProfileComponent->TacticalSkill = 52.0f;
            Company->OfficerProfileComponent->Initiative = 45.0f;
            Company->OfficerProfileComponent->StaffQuality = 50.0f;
            Company->OfficerProfileComponent->Aggression = 58.0f;
            Company->OfficerProfileComponent->Caution = 42.0f;
            Company->OfficerProfileComponent->Discipline = 54.0f;
            Company->OfficerProfileComponent->Composure = 46.0f;
            Company->OfficerProfileComponent->Experience = 45.0f;
        }
        else
        {
            const float Variant =
                static_cast<float>(CompanyNumber % 4) * 3.0f;

            Company->OfficerProfileComponent->Leadership = 55.0f + Variant;
            Company->OfficerProfileComponent->Inspiration = 52.0f + Variant;
            Company->OfficerProfileComponent->TacticalSkill = 50.0f + Variant;
            Company->OfficerProfileComponent->Initiative = 48.0f + Variant;
            Company->OfficerProfileComponent->StaffQuality = 54.0f + Variant;
            Company->OfficerProfileComponent->Aggression = 42.0f + Variant;
            Company->OfficerProfileComponent->Caution = 58.0f - Variant;
            Company->OfficerProfileComponent->Discipline = 60.0f + Variant;
            Company->OfficerProfileComponent->Composure = 56.0f + Variant;
            Company->OfficerProfileComponent->Experience = 50.0f + Variant;
        }
    }

    SpawnedUnitObjects.Add(Company);
    return Company;
}

ACavalryUnit* AStrategyOOBTestScenario::SpawnCavalry(
    const FName StableId,
    const FString& Name,
    const FVector& Location,
    AStrategyUnit* OrganicParent)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const FVector SpawnLocation =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            this,
            Location);

    ACavalryUnit* Cavalry = World->SpawnActor<ACavalryUnit>(
        ACavalryUnit::StaticClass(),
        SpawnLocation,
        FRotator::ZeroRotator);

    if (!Cavalry)
    {
        return nullptr;
    }

    Cavalry->StableUnitId = StableId;
    Cavalry->DisplayName = FText::FromString(Name);
    Cavalry->InitialStrength = 80;
    Cavalry->CurrentStrength = 80;
    Cavalry->Side = EStrategySide::Denmark;
    Cavalry->bPlayerControllable = true;
    Cavalry->RefreshDebugLabel();

    if (Cavalry->CommandComponent)
    {
        Cavalry->CommandComponent->SetOrganicParent(OrganicParent);
    }

    SpawnedUnitObjects.Add(Cavalry);
    return Cavalry;
}

AStrategyArtilleryBatteryUnit* AStrategyOOBTestScenario::SpawnArtilleryBattery(
    const FName StableId,
    const FString& Name,
    const FVector& Location,
    AStrategyUnit* OrganicParent)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const FVector SpawnLocation =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            this,
            Location);

    AStrategyArtilleryBatteryUnit* Battery =
        World->SpawnActor<AStrategyArtilleryBatteryUnit>(
            AStrategyArtilleryBatteryUnit::StaticClass(),
            SpawnLocation,
            FRotator(0.0f, 0.0f, 0.0f));

    if (!Battery)
    {
        return nullptr;
    }

    Battery->StableUnitId = StableId;
    Battery->DisplayName = FText::FromString(Name);
    Battery->Side = EStrategySide::Denmark;
    Battery->bPlayerControllable = true;

    Battery->GunCount = 6;
    Battery->CrewStrength = 72;
    Battery->DriverStrength = 18;
    Battery->HorseStrength = 48;
    Battery->HorsesRequiredForFullMobility = 36;
    Battery->DriversRequiredForFullMobility = 12;
    Battery->InitialStrength = 90;
    Battery->CurrentStrength = 90;
    Battery->Experience = 45.0f;

    if (Battery->DeploymentComponent)
    {
        Battery->DeploymentComponent->MobilityState =
            EStrategyArtilleryMobilityState::Deployed;
    }

    if (Battery->ArtilleryFireMissionComponent)
    {
        Battery->ArtilleryFireMissionComponent->SetHoldFire(false);
        Battery->ArtilleryFireMissionComponent->SetAutoTargetEnabled(false);
        Battery->ArtilleryFireMissionComponent->SetMissionLimits(4, 90.0f);
        Battery->ArtilleryFireMissionComponent->SetConserveAmmunition(
            true,
            0.20f);
    }

    if (Battery->ArtilleryAmmunitionComponent)
    {
        Battery->ArtilleryAmmunitionComponent->RoundShotRounds = 15;
        Battery->ArtilleryAmmunitionComponent->ShellRounds = 15;
        Battery->ArtilleryAmmunitionComponent->ShrapnelRounds = 10;
        Battery->ArtilleryAmmunitionComponent->CanisterRounds = 10;
    }

    if (Battery->SupplyComponent)
    {
        Battery->SupplyComponent->RequestResupply();
    }

    if (Battery->CommandComponent)
    {
        Battery->CommandComponent->SetOrganicParent(OrganicParent);
    }

    Battery->RefreshDebugLabel();
    SpawnedUnitObjects.Add(Battery);
    return Battery;
}

AStrategyMortarBatteryUnit* AStrategyOOBTestScenario::SpawnMortarBattery(
    const FName StableId,
    const FString& Name,
    const FVector& Location,
    AStrategyUnit* OrganicParent)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const FVector SpawnLocation =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(this, Location);

    AStrategyMortarBatteryUnit* Mortar =
        World->SpawnActor<AStrategyMortarBatteryUnit>(
            AStrategyMortarBatteryUnit::StaticClass(),
            SpawnLocation,
            FRotator::ZeroRotator);

    if (!Mortar)
    {
        return nullptr;
    }

    Mortar->StableUnitId = StableId;
    Mortar->DisplayName = FText::FromString(Name);
    Mortar->Side = EStrategySide::Denmark;
    Mortar->bPlayerControllable = true;
    Mortar->MortarPieceCount = 4;
    Mortar->MortarCrewStrength = 32;
    Mortar->InitialStrength = 40;
    Mortar->CurrentStrength = 40;

    if (Mortar->MortarDeploymentComponent)
    {
        Mortar->MortarDeploymentComponent->MortarClass =
            EStrategyMortarClass::HeavySiege;
        Mortar->MortarDeploymentComponent->MobilityState =
            EStrategyMortarMobilityState::Deployed;
        Mortar->MortarDeploymentComponent->WorkingPartyStrength = 24;
    }

    if (Mortar->MortarFireComponent)
    {
        Mortar->MortarFireComponent->AmmunitionBombs = 36;
        Mortar->MortarFireComponent->MaxAmmunitionBombs = 36;
        Mortar->MortarFireComponent->HoldFire();
    }

    if (Mortar->CommandComponent)
    {
        Mortar->CommandComponent->SetOrganicParent(OrganicParent);
    }

    Mortar->RefreshDebugLabel();
    SpawnedUnitObjects.Add(Mortar);
    return Mortar;
}

AStrategySupplyWagonUnit* AStrategyOOBTestScenario::SpawnSupplyWagon(
    const FName StableId,
    const FString& Name,
    const FVector& Location,
    AStrategyUnit* OrganicParent)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const FVector SpawnLocation =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            this,
            Location);

    AStrategySupplyWagonUnit* Wagon =
        World->SpawnActor<AStrategySupplyWagonUnit>(
            AStrategySupplyWagonUnit::StaticClass(),
            SpawnLocation,
            FRotator::ZeroRotator);

    if (!Wagon)
    {
        return nullptr;
    }

    Wagon->StableUnitId = StableId;
    Wagon->DisplayName = FText::FromString(Name);
    Wagon->Side = EStrategySide::Denmark;
    Wagon->bPlayerControllable = true;
    Wagon->DriverStrength = 4;
    Wagon->HorseStrength = 12;
    Wagon->DriversRequiredForFullMobility = 2;
    Wagon->HorsesRequiredForFullMobility = 8;
    Wagon->WagonCondition = 100.0f;
    Wagon->InitialStrength = 4;
    Wagon->CurrentStrength = 4;

    if (Wagon->CargoComponent)
    {
        Wagon->CargoComponent->SmallArmsRounds = 8000;
        Wagon->CargoComponent->ArtilleryRounds = 420;
        Wagon->CargoComponent->ArtilleryAmmunitionFamilyTag =
            TEXT("FIELD_ARTILLERY_GENERIC");
    }

    if (Wagon->SupplyComponent)
    {
        Wagon->SupplyComponent->bActsAsSupplySource = true;
        Wagon->SupplyComponent->ResupplyRadiusCm = 3000.0f;
        Wagon->SupplyComponent->TransferRoundsPerSecond = 90.0f;
    }

    if (Wagon->CommandComponent)
    {
        Wagon->CommandComponent->SetOrganicParent(OrganicParent);
    }

    Wagon->RefreshDebugLabel();
    SpawnedUnitObjects.Add(Wagon);
    return Wagon;
}

AStrategyTerrainFeature* AStrategyOOBTestScenario::SpawnTerrainFeature(
    EStrategyTerrainFeatureType FeatureType,
    const FVector& Location,
    const FVector2D& RadiusCm,
    float PeakHeightCm,
    float YawDegrees)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    AStrategyTerrainFeature* Feature =
        World->SpawnActor<AStrategyTerrainFeature>(
            AStrategyTerrainFeature::StaticClass(),
            Location,
            FRotator(0.0f, YawDegrees, 0.0f));

    if (!Feature)
    {
        return nullptr;
    }

    Feature->FeatureType = FeatureType;
    Feature->RadiusXcm = FMath::Max(100.0f, RadiusCm.X);
    Feature->RadiusYcm = FMath::Max(100.0f, RadiusCm.Y);
    Feature->PeakHeightCm = PeakHeightCm;
    Feature->bAffectsGameplay = true;

    SpawnedTerrainFeatures.Add(Feature);
    return Feature;
}

TArray<AStrategyUnit*> AStrategyOOBTestScenario::GetSpawnedUnits() const
{
    TArray<AStrategyUnit*> Result;
    Result.Reserve(SpawnedUnitObjects.Num());

    for (AStrategyUnit* Unit : SpawnedUnitObjects)
    {
        if (IsValid(Unit))
        {
            Result.Add(Unit);
        }
    }

    return Result;
}



void AStrategyOOBTestScenario::ConfigureRuntimeQALabel(
    AStrategyUnit* Unit) const
{
    if (!IsValid(Unit) || !Unit->DebugLabel)
    {
        return;
    }

    // Runtime QA uses DrawDebugString below. Hide the legacy TextRender label
    // to prevent duplicate world-space text at operational zoom.
    Unit->DebugLabel->SetVisibility(false);
    Unit->RefreshQAPlaceholderVisual();
}

void AStrategyOOBTestScenario::DrawRuntimeQAVisuals() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (const AStrategyUnit* Unit : SpawnedUnitObjects)
    {
        if (!IsValid(Unit))
        {
            continue;
        }

        FColor Color(190, 190, 190);

        switch (Unit->Side)
        {
            case EStrategySide::Denmark:
                Color = FColor(35, 115, 255);
                break;

            case EStrategySide::Prussia:
                Color = FColor(205, 45, 40);
                break;

            case EStrategySide::Austria:
                Color = FColor(230, 210, 150);
                break;

            case EStrategySide::Enemy:
                Color = FColor(255, 55, 55);
                break;

            default:
                break;
        }

        if (Unit->bSelected)
        {
            Color = FColor(255, 225, 45);
        }

        FVector Extent(300.0f, 130.0f, 70.0f);

        switch (Unit->Echelon)
        {
            case EStrategyEchelon::Battalion:
                Extent = FVector(260.0f, 240.0f, 95.0f);
                break;

            case EStrategyEchelon::Regiment:
                Extent = FVector(300.0f, 280.0f, 110.0f);
                break;

            case EStrategyEchelon::Brigade:
                Extent = FVector(340.0f, 320.0f, 125.0f);
                break;

            case EStrategyEchelon::Division:
                Extent = FVector(390.0f, 360.0f, 145.0f);
                break;

            case EStrategyEchelon::Cavalry:
                Extent = FVector(360.0f, 145.0f, 85.0f);
                break;

            case EStrategyEchelon::Artillery:
                Extent = FVector(360.0f, 190.0f, 75.0f);
                break;

            case EStrategyEchelon::Supply:
                Extent = FVector(320.0f, 175.0f, 95.0f);
                break;

            case EStrategyEchelon::Headquarters:
                Extent = FVector(250.0f, 250.0f, 100.0f);
                break;

            case EStrategyEchelon::Company:
            default:
                break;
        }

        const FVector Center =
            Unit->GetActorLocation() +
            FVector(0.0f, 0.0f, Extent.Z + 18.0f);

        DrawDebugBox(
            World,
            Center,
            Extent,
            Unit->GetActorQuat(),
            Color,
            false,
            0.0f,
            0,
            QAVisualThickness);

        const FVector Forward =
            Unit->GetActorForwardVector().GetSafeNormal2D();

        DrawDebugDirectionalArrow(
            World,
            Center,
            Center + Forward * (Extent.X + 260.0f),
            90.0f,
            Color,
            false,
            0.0f,
            0,
            QAVisualThickness);

        FString UnitLabel;

        if (Unit->Echelon == EStrategyEchelon::Company)
        {
            FString Prefix;

            if (Unit->Side == EStrategySide::Prussia)
            {
                Prefix = TEXT("PR ");
            }
            else if (Unit->Side == EStrategySide::Austria)
            {
                Prefix = TEXT("AT ");
            }

            const AStrategyCompanyUnit* Company =
                Cast<AStrategyCompanyUnit>(Unit);

            const int32 CompanyNumber =
                Company ? Company->CompanyNumber : 0;

            UnitLabel =
                FString::Printf(
                    TEXT("%s%d.K"),
                    *Prefix,
                    CompanyNumber);
        }
        else
        {
            UnitLabel =
                FString::Printf(
                    TEXT("%s  [%s]  %d/%d"),
                    *Unit->DisplayName.ToString(),
                    *Unit->GetNATOEchelonSymbol(),
                    Unit->CurrentStrength,
                    Unit->InitialStrength);
        }

        const float LabelScale =
            Unit->Echelon == EStrategyEchelon::Company
            ? 0.50f
            : 0.72f;

        const float LabelHeight =
            Unit->Echelon == EStrategyEchelon::Company
            ? 70.0f
            : 140.0f;

        DrawDebugString(
            World,
            Center + FVector(0.0f, 0.0f, Extent.Z + LabelHeight),
            UnitLabel,
            nullptr,
            Color,
            0.0f,
            true,
            LabelScale);
    }

    if (IsValid(SpawnedRiverBarrier))
    {
        FVector Axis =
            SpawnedRiverBarrier->RiverAxisDirection.GetSafeNormal2D();

        if (Axis.IsNearlyZero())
        {
            Axis = FVector::RightVector;
        }

        const FVector Normal(-Axis.Y, Axis.X, 0.0f);
        const FVector Center =
            SpawnedRiverBarrier->GetActorLocation() +
            FVector(0.0f, 0.0f, 25.0f);

        const FVector A0 =
            Center - Axis * QARiverVisualHalfLengthCm +
            Normal * SpawnedRiverBarrier->RiverHalfWidthCm;
        const FVector A1 =
            Center + Axis * QARiverVisualHalfLengthCm +
            Normal * SpawnedRiverBarrier->RiverHalfWidthCm;
        const FVector B0 =
            Center - Axis * QARiverVisualHalfLengthCm -
            Normal * SpawnedRiverBarrier->RiverHalfWidthCm;
        const FVector B1 =
            Center + Axis * QARiverVisualHalfLengthCm -
            Normal * SpawnedRiverBarrier->RiverHalfWidthCm;

        DrawDebugLine(World, A0, A1, FColor(40, 155, 255), false, 0.0f, 0, 8.0f);
        DrawDebugLine(World, B0, B1, FColor(40, 155, 255), false, 0.0f, 0, 8.0f);

        const FVector BridgeA =
            SpawnedRiverBarrier->GetBridgeApproachForSide(-1) +
            FVector(0.0f, 0.0f, 40.0f);
        const FVector BridgeB =
            SpawnedRiverBarrier->GetBridgeApproachForSide(1) +
            FVector(0.0f, 0.0f, 40.0f);

        DrawDebugLine(
            World,
            BridgeA,
            BridgeB,
            FColor(255, 220, 70),
            false,
            0.0f,
            0,
            12.0f);
    }

    if (IsValid(SpawnedNavigationObstacle))
    {
        DrawDebugBox(
            World,
            SpawnedNavigationObstacle->GetActorLocation() +
                FVector(0.0f, 0.0f, SpawnedNavigationObstacle->HalfExtentCm.Z),
            SpawnedNavigationObstacle->HalfExtentCm,
            SpawnedNavigationObstacle->GetActorQuat(),
            FColor(255, 145, 40),
            false,
            0.0f,
            0,
            6.0f);
    }

    for (const AStrategyTerrainFeature* Feature : SpawnedTerrainFeatures)
    {
        if (!IsValid(Feature))
        {
            continue;
        }

        FColor Color(80, 220, 100);

        if (Feature->FeatureType == EStrategyTerrainFeatureType::Ridge)
        {
            Color = FColor(255, 175, 60);
        }
        else if (Feature->FeatureType == EStrategyTerrainFeatureType::Depression)
        {
            Color = FColor(175, 90, 255);
        }

        const int32 Segments = 48;
        FVector Previous = FVector::ZeroVector;

        for (int32 Index = 0; Index <= Segments; ++Index)
        {
            const float Angle =
                2.0f * PI *
                static_cast<float>(Index) /
                static_cast<float>(Segments);

            FVector LocalPoint(
                FMath::Cos(Angle) * Feature->RadiusXcm,
                FMath::Sin(Angle) * Feature->RadiusYcm,
                30.0f);

            const FVector Point =
                Feature->GetActorTransform().TransformPosition(LocalPoint);

            if (Index > 0)
            {
                DrawDebugLine(
                    World,
                    Previous,
                    Point,
                    Color,
                    false,
                    0.0f,
                    0,
                    4.0f);
            }

            Previous = Point;
        }

        const FVector Center =
            Feature->GetActorLocation() +
            FVector(0.0f, 0.0f, 40.0f);

        const float MarkerHeight =
            FMath::Clamp(
                FMath::Abs(Feature->PeakHeightCm),
                250.0f,
                1800.0f);

        DrawDebugLine(
            World,
            Center,
            Center + FVector(0.0f, 0.0f, MarkerHeight),
            Color,
            false,
            0.0f,
            0,
            5.0f);
    }
}

void AStrategyOOBTestScenario::ResetScenario()
{
    BuildTestOOB();
}

bool AStrategyOOBTestScenario::ValidateStableIdsAndHierarchy(
    TArray<FString>& OutErrors) const
{
    OutErrors.Reset();

    TSet<FName> SeenIds;

    for (AStrategyUnit* Unit : SpawnedUnitObjects)
    {
        if (!IsValid(Unit))
        {
            OutErrors.Add(TEXT("Spawned unit reference is invalid."));
            continue;
        }

        if (Unit->StableUnitId.IsNone())
        {
            OutErrors.Add(
                FString::Printf(
                    TEXT("Unit '%s' has no StableUnitId."),
                    *Unit->DisplayName.ToString()));
        }
        else if (SeenIds.Contains(Unit->StableUnitId))
        {
            OutErrors.Add(
                FString::Printf(
                    TEXT("Duplicate StableUnitId: %s"),
                    *Unit->StableUnitId.ToString()));
        }
        else
        {
            SeenIds.Add(Unit->StableUnitId);
        }

        if (!Unit->CommandComponent)
        {
            continue;
        }

        AStrategyUnit* CurrentParent =
            Unit->CommandComponent->CurrentCommandParent;

        if (IsValid(CurrentParent) &&
            (!CurrentParent->CommandComponent ||
             !CurrentParent->CommandComponent->CurrentSubordinates.Contains(Unit)))
        {
            OutErrors.Add(
                FString::Printf(
                    TEXT("%s current parent does not contain child backlink."),
                    *Unit->StableUnitId.ToString()));
        }

        AStrategyUnit* OrganicParent =
            Unit->CommandComponent->OrganicParent;

        if (IsValid(OrganicParent) &&
            (!OrganicParent->CommandComponent ||
             !OrganicParent->CommandComponent->OrganicSubordinates.Contains(Unit)))
        {
            OutErrors.Add(
                FString::Printf(
                    TEXT("%s organic parent does not contain child backlink."),
                    *Unit->StableUnitId.ToString()));
        }

        TSet<const AStrategyUnit*> ParentChain;
        const AStrategyUnit* Cursor = Unit;

        while (IsValid(Cursor) && Cursor->CommandComponent)
        {
            Cursor = Cursor->CommandComponent->CurrentCommandParent;
            if (!IsValid(Cursor))
            {
                break;
            }

            if (ParentChain.Contains(Cursor))
            {
                OutErrors.Add(
                    FString::Printf(
                        TEXT("Command cycle detected from %s."),
                        *Unit->StableUnitId.ToString()));
                break;
            }

            ParentChain.Add(Cursor);
        }
    }

    return OutErrors.Num() == 0;
}


bool AStrategyOOBTestScenario::RunRegressionChecklist(
    TArray<FString>& OutFailures) const
{
    OutFailures.Reset();

    TArray<FString> HierarchyErrors;
    if (!ValidateStableIdsAndHierarchy(HierarchyErrors))
    {
        OutFailures.Append(HierarchyErrors);
    }

    const int32 ExpectedCount =
        13 +
        (bSpawnCavalryQA ? 2 : 0) +
        (bSpawnEnemyQAUnits ? 2 : 0) +
        (bSpawnArtilleryQA ? 1 : 0) +
        (bSpawnSupplyWagonQA ? 1 : 0) +
        (bSpawnMortarQA ? 1 : 0);

    int32 ValidCount = 0;
    int32 EnemySelectableCount = 0;
    int32 DanishCavalryCount = 0;
    int32 DanishArtilleryCount = 0;
    int32 DanishSupplyWagonCount = 0;
    bool bFoundDragoon = false;

    const AStrategyArtilleryBatteryUnit* QABattery = nullptr;
    const AStrategyUnit* QADeadGroundEnemy = nullptr;
    const AStrategyUnit* QAClearEnemy = nullptr;

    for (AStrategyUnit* Unit : SpawnedUnitObjects)
    {
        if (!IsValid(Unit))
        {
            continue;
        }

        ++ValidCount;

        if (Unit->StableUnitId == FName(TEXT("DK-ART-BAT-1")))
        {
            QABattery = Cast<AStrategyArtilleryBatteryUnit>(Unit);
        }
        else if (Unit->StableUnitId == FName(TEXT("PR-QA-C1")))
        {
            QADeadGroundEnemy = Unit;
        }
        else if (Unit->StableUnitId == FName(TEXT("PR-QA-C2")))
        {
            QAClearEnemy = Unit;
        }

        if (Unit->Side == EStrategySide::Prussia &&
            Unit->bPlayerControllable)
        {
            ++EnemySelectableCount;
        }

        if (Unit->Side == EStrategySide::Denmark &&
            Unit->Echelon == EStrategyEchelon::Company &&
            Unit->FormationComponent &&
            !Unit->FormationComponent->IsFullCompanyFrontageWithinBaseline(190))
        {
            OutFailures.Add(
                FString::Printf(
                    TEXT("%s full-company frontage is outside the 48m QA baseline."),
                    *Unit->StableUnitId.ToString()));
        }

        if (!Unit->MissionAnchorComponent ||
            !Unit->OfficerProfileComponent ||
            !Unit->CommandDelayComponent ||
            !Unit->ConditionComponent ||
            !Unit->ContactComponent ||
            !Unit->ReconComponent ||
            !Unit->AutonomousBattleAIComponent ||
            !Unit->RoutRecoveryComponent ||
            !Unit->FireDisciplineComponent ||
            !Unit->StanceComponent ||
            !Unit->DirectionalCoverComponent ||
            !Unit->FieldworksComponent ||
            !Unit->SkirmisherComponent ||
            !Unit->SupplyComponent ||
            !Unit->DoctrineComponent ||
            !Unit->AutonomyComponent ||
            !Unit->AIDifficultyComponent ||
            !Unit->AITelemetryComponent ||
            !Unit->MissionConstraintsComponent ||
            !Unit->TerrainAwarenessComponent ||
            !Unit->UniformAppearanceComponent ||
            !Unit->HumanAnimationStateComponent ||
            !Unit->EquipmentVisualComponent ||
            !Unit->VisualCompatibilityComponent ||
            !Unit->DetachmentComponent ||
            !Unit->NCOComponent ||
            !Unit->FireDrillComponent ||
            !Unit->PositionOccupancyComponent ||
            !Unit->FortificationAssaultComponent ||
            !Unit->WorkingPartyComponent ||
            !Unit->SpecialistStateComponent)
        {
            OutFailures.Add(
                FString::Printf(
                    TEXT("%s is missing one or more gameplay-core components."),
                    *Unit->StableUnitId.ToString()));
        }

        if (Unit->SpecialistStateComponent)
        {
            FString SpecialistFailure;
            if (!Unit->SpecialistStateComponent->ValidateCurrentState(
                    SpecialistFailure))
            {
                OutFailures.Add(
                    FString::Printf(
                        TEXT("%s specialist state invalid: %s"),
                        *Unit->StableUnitId.ToString(),
                        *SpecialistFailure));
            }
        }

        if (Unit->UniformAppearanceComponent &&
            Unit->VisualCompatibilityComponent)
        {
            FString VisualFailure;

            if (!Unit->VisualCompatibilityComponent->ValidateProfile(
                    Unit->UniformAppearanceComponent->VisualProfile,
                    VisualFailure))
            {
                OutFailures.Add(
                    FString::Printf(
                        TEXT("%s visual compatibility failed: %s"),
                        *Unit->StableUnitId.ToString(),
                        *VisualFailure));
            }

            if (Unit->UniformAppearanceComponent
                    ->BasePreset.PresetId.IsNone())
            {
                OutFailures.Add(
                    FString::Printf(
                        TEXT("%s has no uniform preset id."),
                        *Unit->StableUnitId.ToString()));
            }
        }

        const float ExpectedTerrainZ =
            UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(
                this,
                Unit->GetActorLocation());

        if (FMath::Abs(
                Unit->GetActorLocation().Z -
                ExpectedTerrainZ) > 5.0f)
        {
            OutFailures.Add(
                FString::Printf(
                    TEXT("%s is not projected onto tactical terrain (actorZ=%.1f terrainZ=%.1f)."),
                    *Unit->StableUnitId.ToString(),
                    Unit->GetActorLocation().Z,
                    ExpectedTerrainZ));
        }

        if (Unit->CommandDelayComponent &&
            Unit->CommandDelayComponent->CalculateDelayFromCurrentParent() < 0.0f)
        {
            OutFailures.Add(
                FString::Printf(
                    TEXT("%s produced a negative command delay."),
                    *Unit->StableUnitId.ToString()));
        }

        if (Unit->CombatComponent &&
            Unit->CombatComponent->AmmunitionRounds < 0)
        {
            OutFailures.Add(
                FString::Printf(
                    TEXT("%s has invalid negative ammunition."),
                    *Unit->StableUnitId.ToString()));
        }

        if (Unit->Side == EStrategySide::Denmark &&
            Unit->Echelon == EStrategyEchelon::Artillery)
        {
            ++DanishArtilleryCount;

            const AStrategyArtilleryBatteryUnit* Battery =
                Cast<AStrategyArtilleryBatteryUnit>(Unit);

            if (!Battery ||
                !Battery->DeploymentComponent ||
                !Battery->ArtilleryAmmunitionComponent ||
                !Battery->ArtilleryFireMissionComponent ||
                !Battery->ArtilleryDamageComponent ||
                !Battery->ArtilleryCaptureComponent ||
                !Battery->ArtilleryTraverseComponent ||
                !Battery->ArtilleryRepairComponent ||
                !Battery->ArtilleryPositioningComponent ||
                !Battery->ProjectilePresentationComponent ||
                !Battery->CrewAnimationComponent)
            {
                OutFailures.Add(
                    TEXT("Artillery QA battery is missing one or more artillery-core components."));
            }
            else
            {
                if (Battery->GunCount <= 0 ||
                    Battery->CrewStrength <= 0 ||
                    Battery->HorseStrength <= 0 ||
                    Battery->GetOperationalGunCount() <= 0)
                {
                    OutFailures.Add(
                        TEXT("Artillery QA battery has invalid gun/crew/horse operational state."));
                }

                if (Battery->ArtilleryAmmunitionComponent->GetTotalRounds() <= 0)
                {
                    OutFailures.Add(TEXT("Artillery QA battery has no ammunition."));
                }

                if (!Battery->DeploymentComponent->IsDeployed())
                {
                    OutFailures.Add(TEXT("Artillery QA battery did not start deployed."));
                }

                if (Battery->CrewAnimationComponent &&
                    Battery->CrewAnimationComponent
                        ->GetActiveCrewStationCount() <= 0)
                {
                    OutFailures.Add(
                        TEXT("Artillery QA battery has no visual crew stations."));
                }
            }
        }

        if (Unit->Side == EStrategySide::Denmark &&
            Unit->Echelon == EStrategyEchelon::Supply)
        {
            ++DanishSupplyWagonCount;

            const AStrategySupplyWagonUnit* Wagon =
                Cast<AStrategySupplyWagonUnit>(Unit);

            if (!Wagon ||
                !Wagon->CargoComponent ||
                !Wagon->SupplyCaptureComponent ||
                !Wagon->SupplyComponent)
            {
                OutFailures.Add(
                    TEXT("Supply wagon QA entity is missing logistics-core components."));
            }
            else
            {
                if (!Wagon->SupplyComponent->bActsAsSupplySource ||
                    Wagon->CargoComponent->SmallArmsRounds <= 0 ||
                    Wagon->CargoComponent->ArtilleryRounds <= 0)
                {
                    OutFailures.Add(
                        TEXT("Supply wagon QA cargo/source state is invalid."));
                }

                if (!Wagon->CanMoveSupplyWagon())
                {
                    OutFailures.Add(
                        TEXT("Supply wagon QA mobility state is invalid."));
                }

                if (Wagon->CargoComponent->ArtilleryAmmunitionFamilyTag !=
                    FName(TEXT("FIELD_ARTILLERY_GENERIC")))
                {
                    OutFailures.Add(
                        TEXT("Supply wagon QA artillery compatibility tag is invalid."));
                }
            }
        }

        if (Unit->Side == EStrategySide::Denmark &&
            Unit->Echelon == EStrategyEchelon::Cavalry)
        {
            ++DanishCavalryCount;

            const ACavalryUnit* Cavalry = Cast<ACavalryUnit>(Unit);

            if (Cavalry &&
                (!Cavalry->HorseAnimationStateComponent ||
                 !Cavalry->MountedAnimationSyncComponent))
            {
                OutFailures.Add(
                    FString::Printf(
                        TEXT("%s cavalry visual horse/rider sync component is missing."),
                        *Unit->StableUnitId.ToString()));
            }

            if (Cavalry && !Cavalry->ScreenAIComponent)
            {
                OutFailures.Add(
                    FString::Printf(
                        TEXT("%s cavalry screen AI component is missing."),
                        *Unit->StableUnitId.ToString()));
            }

            if (Cavalry &&
                Cavalry->DragoonComponent &&
                Cavalry->DragoonComponent->Role == EStrategyCavalryRole::Dragoon)
            {
                bFoundDragoon = true;
            }
        }
    }

    const TArray<FName> CoreAnimations =
        UStrategyAnimationManifestLibrary::GetCoreHumanAnimationNames();

    const TArray<FName> ArtilleryAnimations =
        UStrategyAnimationManifestLibrary::GetArtilleryCrewAnimationNames();

    if (CoreAnimations.Num() < 30)
    {
        OutFailures.Add(
            TEXT("Core human animation manifest is unexpectedly incomplete."));
    }

    if (ArtilleryAnimations.Num() < 30)
    {
        OutFailures.Add(
            TEXT("Artillery crew animation manifest is unexpectedly incomplete."));
    }

    if (IsValid(QAClearEnemy) &&
        QAClearEnemy->UniformAppearanceComponent)
    {
        const FStrategyUniformColors Colors =
            QAClearEnemy->UniformAppearanceComponent
                ->GetResolvedColors();

        const FLinearColor ExpectedAccent(
            0.15f,
            0.65f,
            0.85f,
            1.0f);

        if (!Colors.Accent.Equals(ExpectedAccent, 0.001f))
        {
            OutFailures.Add(
                TEXT("Runtime uniform accent override did not resolve correctly."));
        }
    }

    if (ValidCount != ExpectedCount)
    {
        OutFailures.Add(
            FString::Printf(
                TEXT("Expected %d strategy entities, found %d."),
                ExpectedCount,
                ValidCount));
    }

    if (EnemySelectableCount > 0)
    {
        OutFailures.Add(
            FString::Printf(
                TEXT("%d Prussian QA units are incorrectly player-controllable."),
                EnemySelectableCount));
    }

    if (bSpawnCavalryQA && DanishCavalryCount != 2)
    {
        OutFailures.Add(
            FString::Printf(
                TEXT("Expected 2 Danish cavalry units, found %d."),
                DanishCavalryCount));
    }

    if (bSpawnCavalryQA && !bFoundDragoon)
    {
        OutFailures.Add(TEXT("Dragoon QA unit was not configured."));
    }

    if (bSpawnArtilleryQA && DanishArtilleryCount != 1)
    {
        OutFailures.Add(
            FString::Printf(
                TEXT("Expected 1 Danish artillery battery, found %d."),
                DanishArtilleryCount));
    }

    if (bSpawnSupplyWagonQA && DanishSupplyWagonCount != 1)
    {
        OutFailures.Add(
            FString::Printf(
                TEXT("Expected 1 Danish supply wagon, found %d."),
                DanishSupplyWagonCount));
    }

    if (bSpawnTerrainQA)
    {
        if (SpawnedTerrainFeatures.Num() != 3)
        {
            OutFailures.Add(
                FString::Printf(
                    TEXT("Expected 3 tactical terrain QA features, found %d."),
                    SpawnedTerrainFeatures.Num()));
        }

        const float BatteryHillOffset =
            UStrategyTerrainQueryLibrary::GetFeatureElevationOffset(
                this,
                Origin + FVector(1800.0f, 7200.0f, 0.0f));

        if (BatteryHillOffset < 500.0f)
        {
            OutFailures.Add(
                TEXT("Battery hill QA feature does not provide expected elevation."));
        }

        if (bSpawnArtilleryQA &&
            bSpawnEnemyQAUnits &&
            IsValid(QABattery) &&
            IsValid(QADeadGroundEnemy) &&
            IsValid(QAClearEnemy))
        {
            const bool bDeadGround =
                UStrategyTerrainQueryLibrary::IsPointInDeadGroundFrom(
                    this,
                    QABattery->GetActorLocation(),
                    QADeadGroundEnemy->GetActorLocation(),
                    160.0f,
                    120.0f);

            if (!bDeadGround)
            {
                OutFailures.Add(
                    TEXT("Central ridge did not create expected artillery dead ground for PR-QA-C1."));
            }

            const bool bClearLaneDeadGround =
                UStrategyTerrainQueryLibrary::IsPointInDeadGroundFrom(
                    this,
                    QABattery->GetActorLocation(),
                    QAClearEnemy->GetActorLocation(),
                    160.0f,
                    120.0f);

            if (bClearLaneDeadGround)
            {
                OutFailures.Add(
                    TEXT("PR-QA-C2 should provide the clear artillery terrain lane but is classified dead ground."));
            }

            FVector Start =
                UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
                    this,
                    QABattery->GetActorLocation());

            FVector End =
                UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
                    this,
                    QADeadGroundEnemy->GetActorLocation());

            Start.Z += 160.0f;
            End.Z += 120.0f;

            FVector CrestPoint;
            float CrestExcessCm = 0.0f;

            if (!UStrategyTerrainQueryLibrary::FindCrestPoint(
                    this,
                    Start,
                    End,
                    CrestPoint,
                    CrestExcessCm,
                    40))
            {
                OutFailures.Add(
                    TEXT("Central ridge failed explicit crest detection."));
            }

            if (QABattery->TerrainAwarenessComponent &&
                QABattery->TerrainAwarenessComponent
                    ->GetObservationRangeMultiplierTo(QAClearEnemy) <= 1.0f)
            {
                OutFailures.Add(
                    TEXT("Battery high-ground QA position did not produce an observation-range advantage."));
            }

            if (QABattery->ArtilleryPositioningComponent)
            {
                FVector BestPosition;
                FStrategyTerrainPositionAssessment Assessment;

                if (!QABattery->ArtilleryPositioningComponent
                        ->FindBestDirectFirePosition(
                            QAClearEnemy->GetActorLocation(),
                            5000.0f,
                            18,
                            BestPosition,
                            Assessment))
                {
                    OutFailures.Add(
                        TEXT("Artillery positioning QA found no valid direct-fire candidate."));
                }
                else if (!Assessment.bHasDirectLOS ||
                         Assessment.bInDeadGroundFromThreat ||
                         Assessment.LocalSlopeDegrees >
                            QABattery->ArtilleryPositioningComponent
                                ->MaximumDirectFireSlopeDegrees)
                {
                    OutFailures.Add(
                        TEXT("Artillery positioning QA returned an invalid best position."));
                }
            }

            if (QABattery->ProjectilePresentationComponent)
            {
                FStrategyArtilleryProjectileSpec RoundSpec;
                RoundSpec.AmmoType =
                    EStrategyArtilleryAmmoType::RoundShot;
                RoundSpec.Style =
                    EStrategyProjectilePresentationStyle::RoundShot;
                RoundSpec.LaunchLocation =
                    QABattery->GetActorLocation() +
                    FVector(0.0f, 0.0f, 145.0f);
                RoundSpec.AimLocation =
                    QAClearEnemy->GetActorLocation();
                RoundSpec.PrimaryImpactLocation =
                    QAClearEnemy->GetActorLocation();
                RoundSpec.FlightSeconds =
                    UStrategyArtilleryTrajectoryLibrary::EstimateFlightSeconds(
                        RoundSpec.AmmoType,
                        FVector::Dist2D(
                            RoundSpec.LaunchLocation,
                            RoundSpec.PrimaryImpactLocation));

                FVector RoundFinal;
                const TArray<FVector> RoundPath =
                    UStrategyArtilleryTrajectoryLibrary::BuildTrajectory(
                        this,
                        RoundSpec,
                        32,
                        true,
                        RoundFinal);

                FStrategyArtilleryProjectileSpec ShellSpec = RoundSpec;
                ShellSpec.AmmoType = EStrategyArtilleryAmmoType::Shell;
                ShellSpec.Style =
                    EStrategyProjectilePresentationStyle::Shell;

                FVector ShellFinal;
                const TArray<FVector> ShellPath =
                    UStrategyArtilleryTrajectoryLibrary::BuildTrajectory(
                        this,
                        ShellSpec,
                        32,
                        false,
                        ShellFinal);

                if (RoundPath.Num() < 8 ||
                    ShellPath.Num() < 8 ||
                    RoundSpec.FlightSeconds <= 0.0f)
                {
                    OutFailures.Add(
                        TEXT("Projectile trajectory QA produced an invalid path or flight time."));
                }
                else
                {
                    float RoundMaxZ = -TNumericLimits<float>::Max();
                    float ShellMaxZ = -TNumericLimits<float>::Max();

                    for (const FVector& Point : RoundPath)
                    {
                        RoundMaxZ = FMath::Max(RoundMaxZ, Point.Z);
                    }

                    for (const FVector& Point : ShellPath)
                    {
                        ShellMaxZ = FMath::Max(ShellMaxZ, Point.Z);
                    }

                    if (ShellMaxZ <= RoundMaxZ)
                    {
                        OutFailures.Add(
                            TEXT("Shell projectile QA arc is not higher than Round Shot arc."));
                    }

                    if (FVector::Dist2D(
                            RoundFinal,
                            RoundSpec.PrimaryImpactLocation) <= 100.0f)
                    {
                        OutFailures.Add(
                            TEXT("Round Shot projectile QA did not produce expected post-impact ricochet travel."));
                    }
                }

                if (UStrategyArtilleryTrajectoryLibrary::GetPresentationStyle(
                        EStrategyArtilleryAmmoType::Canister) !=
                    EStrategyProjectilePresentationStyle::Canister)
                {
                    OutFailures.Add(
                        TEXT("Canister projectile presentation style mapping is invalid."));
                }
            }
        }
    }

    if (bSpawnRiverQA && !IsValid(SpawnedRiverBarrier))
    {
        OutFailures.Add(TEXT("River QA barrier was not spawned."));
    }

    if (bSpawnObstacleQA && !IsValid(SpawnedNavigationObstacle))
    {
        OutFailures.Add(TEXT("Navigation obstacle QA actor was not spawned."));
    }

    bool bFoundConfiguredSupplySource = false;

    for (const AStrategyUnit* Unit : SpawnedUnitObjects)
    {
        if (!IsValid(Unit) ||
            !Unit->SupplyComponent ||
            !Unit->SupplyComponent->bActsAsSupplySource)
        {
            continue;
        }

        if (const AStrategySupplyWagonUnit* Wagon =
            Cast<AStrategySupplyWagonUnit>(Unit))
        {
            bFoundConfiguredSupplySource =
                Wagon->CargoComponent &&
                Wagon->CargoComponent->GetTotalAmmunitionRounds() > 0;
        }
        else
        {
            bFoundConfiguredSupplySource =
                Unit->SupplyComponent->StoredAmmunitionRounds > 0;
        }

        if (bFoundConfiguredSupplySource)
        {
            break;
        }
    }

    if (!bFoundConfiguredSupplySource)
    {
        OutFailures.Add(TEXT("No configured tactical ammunition supply source was found."));
    }

    return OutFailures.Num() == 0;
}