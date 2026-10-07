#include "StrategyOOBTestScenario.h"

#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "../Campaign/StrategyCampaignBattlefield.h"
#include "../Visual/StrategyColourFlag.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Camera/CameraActor.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Player/StrategyCameraPawn.h"
#include "../Visual/StrategyBattleAtmosphere.h"
#include "EngineUtils.h"
#include "../Visual/StrategyInfantryVisualComponent.h"

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
#include "Engine/SkeletalMesh.h"
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

    // The battle's light and air (a low warm sun, haze, grading) on every battle map.
    if (AStrategyBattleAtmosphere* Atmosphere = GetWorld()->SpawnActor<AStrategyBattleAtmosphere>(AStrategyBattleAtmosphere::StaticClass(), Origin, FRotator::ZeroRotator))
    {
        Atmosphere->Apply();
    }

    // The test fields get a meadow instead of the grey box (-Strategy1864FlatQA keeps the box).
    const FString MeadowMap = FPackageName::GetShortName(GetWorld()->GetOutermost()->GetName());
    FString FieldArgument;
    if (bUseFlatQAMap && !MeadowMap.Contains(TEXT("Field")) && !FParse::Param(FCommandLine::Get(), TEXT("Strategy1864FlatQA")) &&
        !FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Field="), FieldArgument) &&
        GetWorld()->URL.GetOption(TEXT("Battle="), nullptr) == nullptr)
    {
        FActorSpawnParameters MeadowParams;
        MeadowParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (AStrategyCampaignBattlefield* Meadow = GetWorld()->SpawnActor<AStrategyCampaignBattlefield>(AStrategyCampaignBattlefield::StaticClass(),
            Origin + FVector(15000.0f, -7000.0f, 0.0f), FRotator::ZeroRotator, MeadowParams))
        {
            Meadow->BuildMeadow(FMath::Max(1200.0f, FlatMapSizeCm / 100.0f * 2.0f), QARandomSeed);
            if (QAFlatGround)
            {
                QAFlatGround->SetVisibility(false, true);
                QAFlatGround->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
        }
    }

    // The infantry duel (Livgarden against Swedish infantry, both in 3D): on a map named *Duel* or with
    // -Strategy1864Duel on the command line, whatever the map's saved setting.
    const FString MapName = FPackageName::GetShortName(GetWorld()->GetOutermost()->GetName());
    if (MapName.Contains(TEXT("Duel")) || FParse::Param(FCommandLine::Get(), TEXT("Strategy1864Duel")))
    {
        bLivgardenVsSwedishTest = true;
    }
    // The full order of battle (division, brigade, regiments, cavalry, artillery): a map named *OOB* or -Strategy1864FullOOB.
    if (MapName.Contains(TEXT("OOB")) || FParse::Param(FCommandLine::Get(), TEXT("Strategy1864FullOOB")))
    {
        bLivgardenVsSwedishTest = false;
    }
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-QA: map %s, test %s"), *MapName, bLivgardenVsSwedishTest ? TEXT("Livgarden vs Swedish duel") : TEXT("full OOB"));

    // The battle from the campaign: ?Battle=N (from the campaign map), -Strategy1864Battle=N, -Strategy1864Field=<file>,
    // or a map named *Field* (the newest battlefield file).
    {
        int32 BattleId = 0;
        FString FieldFile;
        const FString Option = GetWorld()->URL.GetOption(TEXT("Battle="), TEXT(""));
        if (!Option.IsEmpty())
        {
            BattleId = FCString::Atoi(*Option);
            bReturnToCampaign = true;
        }
        FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Battle="), BattleId);
        FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Field="), FieldFile);
        // A battlefield given in the map's address (a test battle from the campaign's SLAGMARK window): back to the campaign after.
        const FString FieldOption = GetWorld()->URL.GetOption(TEXT("Field="), TEXT(""));
        if (!FieldOption.IsEmpty())
        {
            FieldFile = FieldOption;
            bReturnToCampaign = true;
        }
        if (BattleId > 0 && FieldFile.IsEmpty())
        {
            FieldFile = FString::Printf(TEXT("Battlefield_Battle_%d.json"), BattleId);
        }
        if (FieldFile.IsEmpty() && MapName.Contains(TEXT("Field")))
        {
            TArray<FString> Files;
            const FString Dir = FPaths::ProjectSavedDir() / TEXT("Battle");
            IFileManager::Get().FindFiles(Files, *(Dir / TEXT("Battlefield_*.json")), true, false);
            FDateTime Newest = FDateTime::MinValue();
            for (const FString& F : Files)
            {
                const FDateTime Stamp = IFileManager::Get().GetTimeStamp(*(Dir / F));
                if (Stamp > Newest) { Newest = Stamp; FieldFile = F; }
            }
        }
        if (!FieldFile.IsEmpty())
        {
            bLivgardenVsSwedishTest = false;
            if (BuildCampaignBattle(FieldFile, BattleId))
            {
                return;
            }
        }
    }

    // The small test battle: map *Skirmish* or -Strategy1864Skirmish=<enemy companies>.
    {
        int32 SkirmishEnemies = MapName.Contains(TEXT("Skirmish")) ? 2 : 0;
        FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Skirmish="), SkirmishEnemies);
        if (SkirmishEnemies > 0)
        {
            BuildSkirmish(FMath::Clamp(SkirmishEnemies, 1, 4));
            return;
        }
    }

    if (bBuildOnBeginPlay)
    {
        BuildTestOOB();
    }
}

void AStrategyOOBTestScenario::BuildSkirmish(int32 EnemyCompanies)
{
    ClearSpawnedUnits();
    bLivgardenVsSwedishTest = false;
    bSkirmish = true;
    BattleOutcome.Reset();
    // On the meadow (its middle): the Danes in the south facing north, the enemy 400 m north.
    const FVector Middle = Origin + FVector(15000.0f, -7000.0f, 0.0f);
    const FVector DanishLine = Middle - FVector(20000.0f, 0.0f, 0.0f), EnemyLine = Middle + FVector(20000.0f, 0.0f, 0.0f);
    int32 Lod = 1;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864FieldLOD="), Lod);

    AStrategyHQUnit* Major = SpawnHQ(TEXT("DK-SKIRMISH-HQ"), TEXT("1. Bataillon"), static_cast<uint8>(EStrategyHQLevel::Battalion), DanishLine - FVector(9000.0f, 0.0f, 0.0f), nullptr);
    AStrategyHQUnit* EnemyMajor = SpawnHQ(TEXT("EN-SKIRMISH-HQ"), TEXT("Pr. Bataillon"), static_cast<uint8>(EStrategyHQLevel::Battalion), EnemyLine + FVector(9000.0f, 0.0f, 0.0f), nullptr);
    if (EnemyMajor)
    {
        EnemyMajor->Side = EStrategySide::Prussia;
        EnemyMajor->bPlayerControllable = false;
        EnemyMajor->bOfficerAIEnabled = true;
        EnemyMajor->SetActorRotation(FRotator(0.0f, 180.0f, 0.0f));
        EnemyMajor->RefreshDebugLabel();
    }
    TArray<AStrategyCompanyUnit*> All;
    for (int32 c = 0; c < 2; ++c)
    {
        if (AStrategyCompanyUnit* Company = SpawnCompany(FName(*FString::Printf(TEXT("DK-SKIRMISH-C%d"), c + 1)), FString::Printf(TEXT("%d. Kompagni"), c + 1), c + 1,
            DanishLine + FVector(0.0f, (c - 0.5f) * 7200.0f, 0.0f), Major, static_cast<uint8>(EStrategySide::Denmark)))
        {
            Company->bPlayerControllable = true;
            if (Company->InfantryVisualComponent)
            {
                Company->InfantryVisualComponent->SoldierMeshAsset = TSoftObjectPtr<USkeletalMesh>(
                    FSoftObjectPath(TEXT("/Game/Units/Danish/Infantry1864/Mesh/SK_DK_Infantry_1864.SK_DK_Infantry_1864")));
            }
            All.Add(Company);
        }
    }
    for (int32 e = 0; e < EnemyCompanies; ++e)
    {
        if (AStrategyCompanyUnit* Company = SpawnCompany(FName(*FString::Printf(TEXT("EN-SKIRMISH-C%d"), e + 1)), FString::Printf(TEXT("Pr. %d. Kp."), e + 1), e + 1,
            EnemyLine + FVector(0.0f, (e - (EnemyCompanies - 1) * 0.5f) * 7200.0f, 0.0f), EnemyMajor, static_cast<uint8>(EStrategySide::Prussia)))
        {
            Company->SetActorRotation(FRotator(0.0f, 180.0f, 0.0f));
            Company->bPlayerControllable = false;
            if (Company->AutonomousBattleAIComponent)
            {
                Company->AutonomousBattleAIComponent->bEnableForNonPlayerSides = true;
            }
            if (Company->InfantryVisualComponent)
            {
                // No Prussian model yet: the Swedish stands in.
                Company->InfantryVisualComponent->SoldierMeshAsset = TSoftObjectPtr<USkeletalMesh>(
                    FSoftObjectPath(TEXT("/Game/Units/Swedish/Infantry1864/Mesh/SK_SE_Infantry_1864.SK_SE_Infantry_1864")));
            }
            All.Add(Company);
        }
    }
    for (AStrategyCompanyUnit* Company : All)
    {
        Company->bOfficerAIEnabled = true;
        if (Company->InfantryVisualComponent)
        {
            Company->InfantryVisualComponent->SetVisualScaleDivisor(Lod);
            Company->InfantryVisualComponent->SetEnabled(true);
        }
        // The meadow has no contact simulation around it: sight is enough to engage (as the duel).
        if (Company->FireControlComponent)
        {
            Company->FireControlComponent->bRequireCurrentContact = false;
        }
        if (Company->CombatComponent)
        {
            Company->CombatComponent->SetDeterministicRandomSeed(QARandomSeed ^ static_cast<int32>(GetTypeHash(Company->StableUnitId)));
        }
        ConfigureRuntimeQALabel(Company);
        if (AStrategyColourFlag* Flag = GetWorld()->SpawnActor<AStrategyColourFlag>(AStrategyColourFlag::StaticClass(), Company->GetActorLocation(), FRotator::ZeroRotator))
        {
            Flag->Setup(Company, Company->Side == EStrategySide::Denmark ? TEXT("DK") : TEXT("PR"), FVector(-250.0f, 60.0f, 0.0f));
        }
    }
    // -Strategy1864SkirmishArms: a battery, a mortar and a squadron on each side (to see the guns and the horse).
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864SkirmishArms")) && All.Num() >= 2)
    {
        AStrategyCompanyUnit* DanishTarget = All[1];
        AStrategyCompanyUnit* EnemyTarget = All.Last();
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const bool bDane = Side == 0;
            const FVector Line = bDane ? DanishLine : EnemyLine;
            const float Back = bDane ? -1.0f : 1.0f;
            AStrategyHQUnit* Hq = bDane ? Major : EnemyMajor;
            const FString Tag = bDane ? TEXT("DK") : TEXT("EN");
            auto Own = [&](AStrategyUnit* U)
            {
                U->Side = bDane ? EStrategySide::Denmark : EStrategySide::Prussia;
                U->bPlayerControllable = bDane;
                U->bOfficerAIEnabled = true;
                U->SetActorRotation(FRotator(0.0f, bDane ? 0.0f : 180.0f, 0.0f));
                U->RefreshDebugLabel();
            };
            if (AStrategyArtilleryBatteryUnit* Battery = SpawnArtilleryBattery(FName(*(Tag + TEXT("-SKIRMISH-BTY"))), bDane ? TEXT("2. Batteri") : TEXT("Pr. 4-pdr Batterie"),
                Line + FVector(Back * 4000.0f, 0.0f, 0.0f), Hq))
            {
                Own(Battery);
                if (Battery->ArtilleryFireMissionComponent)
                {
                    Battery->ArtilleryFireMissionComponent->SetMissionLimits(1000, 100000.0f);
                    Battery->ArtilleryFireMissionComponent->SetConserveAmmunition(false, 0.0f);
                    Battery->ArtilleryFireMissionComponent->SetAutoTargetEnabled(true);
                }
                if (Battery->ArtilleryAmmunitionComponent)
                {
                    Battery->ArtilleryAmmunitionComponent->RoundShotRounds = 120;
                    Battery->ArtilleryAmmunitionComponent->ShellRounds = 80;
                    Battery->ArtilleryAmmunitionComponent->ShrapnelRounds = 60;
                    Battery->ArtilleryAmmunitionComponent->CanisterRounds = 40;
                }
            }
            if (AStrategyMortarBatteryUnit* Mortar = SpawnMortarBattery(FName(*(Tag + TEXT("-SKIRMISH-MRT"))), bDane ? TEXT("Morterbatteri") : TEXT("Pr. Mörser"),
                Line + FVector(Back * 9000.0f, 9000.0f, 0.0f), Hq))
            {
                Own(Mortar);
                if (Mortar->MortarFireComponent)
                {
                    Mortar->MortarFireComponent->SetUnitTarget(bDane ? EnemyTarget : DanishTarget);
                }
            }
            if (ACavalryUnit* Squadron = SpawnCavalry(FName(*(Tag + TEXT("-SKIRMISH-SQN"))), bDane ? TEXT("1. Eskadron") : TEXT("Pr. Husaren"),
                Line + FVector(Back * 2000.0f, -16000.0f, 0.0f), Hq))
            {
                Own(Squadron);
                if (!bDane && Squadron->OrderComponent)
                {
                    // The enemy's hussars ride at the Danish flank company.
                    FStrategyOrder Order;
                    Order.Type = EStrategyOrderType::AttackHere;
                    Order.TargetLocation = DanishTarget->GetActorLocation();
                    Order.Authority = EStrategyOrderAuthority::OfficerAI;
                    Squadron->OrderComponent->SetOrder(Order);
                }
            }
        }
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864EnemyDefends")))
    {
        SetEnemyAttacking(false);
    }
    // -Strategy1864SkirmishAttack: the major orders his battalion to attack (his captains lead it from there).
    if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864SkirmishAttack")) && Major && Major->OrderComponent)
    {
        FStrategyOrder Attack;
        Attack.Type = EStrategyOrderType::AttackHere;
        Attack.TargetLocation = Middle;
        Attack.FacingYaw = 0.0f;
        Attack.bHasFacing = true;
        Attack.Authority = EStrategyOrderAuthority::DirectPlayer;
        Major->OrderComponent->SetOrder(Attack);
    }
    // The camera behind the Danish line (as the campaign's battles).
    FieldCameraTarget = DanishLine - FVector(6000.0f, 0.0f, 0.0f);
    FieldCameraYaw = 0.0f;
    bFieldCameraPlaced = false;
    SetupObjectives(DanishLine, EnemyLine);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-SKIRMISH: 2 Danish companies against %d enemy companies, 400 m apart, figures 1:%d"), EnemyCompanies, Lod);
}

void AStrategyOOBTestScenario::GetBattleScore(int32& OutDanesStart, int32& OutDanesNow, int32& OutEnemyStart, int32& OutEnemyNow, int32& OutDanesBroken, int32& OutEnemyBroken) const
{
    OutDanesStart = OutDanesNow = OutEnemyStart = OutEnemyNow = OutDanesBroken = OutEnemyBroken = 0;
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        const AStrategyUnit* Unit = *It;
        if (!IsValid(Unit) || Unit->Side == EStrategySide::Neutral || Unit->Echelon == EStrategyEchelon::Headquarters ||
            Unit->Echelon == EStrategyEchelon::Supply || Unit->Echelon == EStrategyEchelon::Battalion || Unit->Echelon == EStrategyEchelon::Regiment ||
            Unit->Echelon == EStrategyEchelon::Brigade || Unit->Echelon == EStrategyEchelon::Division)
        {
            continue;
        }
        const bool bDane = Unit->Side == EStrategySide::Denmark;
        const bool bBroken = !Unit->IsCombatEffective() || Unit->UnitState == EStrategyUnitState::Routed || Unit->UnitState == EStrategyUnitState::Destroyed;
        (bDane ? OutDanesStart : OutEnemyStart) += FMath::Max(0, Unit->InitialStrength);
        (bDane ? OutDanesNow : OutEnemyNow) += bBroken ? 0 : FMath::Clamp(Unit->CurrentStrength, 0, FMath::Max(0, Unit->InitialStrength));
        (bDane ? OutDanesBroken : OutEnemyBroken) += bBroken ? 1 : 0;
    }
}

void AStrategyOOBTestScenario::GetObjectivePoints(int32& OutDanes, int32& OutEnemy) const
{
    OutDanes = OutEnemy = 0;
    for (const FBattleObjective& O : Objectives)
    {
        (O.Owner == 1 ? OutDanes : O.Owner == 2 ? OutEnemy : OutDanes) += O.Owner == 0 ? 0 : O.Points;
    }
}

void AStrategyOOBTestScenario::SetupObjectives(const FVector& DanishLine, const FVector& EnemyLine)
{
    Objectives.Reset();
    BattleClock = 0.0f;
    AllHeldFor = 0.0f;
    AllHeldBy = 0;
    FVector Axis = EnemyLine - DanishLine;
    Axis.Z = 0.0f;
    const float Distance = Axis.Size();
    if (Distance < 1000.0f)
    {
        return;
    }
    Axis /= Distance;
    const FVector Left(-Axis.Y, Axis.X, 0.0f);
    auto Add = [&](const FVector& At, const TCHAR* Name, int32 Points, int32 FirstOwner)
    {
        FBattleObjective O;
        O.Location = At;
        O.Location.Z = CampaignField ? CampaignField->GroundZ(At) : At.Z;
        O.Name = Name;
        O.Points = Points;
        O.Owner = FirstOwner;
        O.Progress = FirstOwner == 1 ? -1.0f : FirstOwner == 2 ? 1.0f : 0.0f;
        O.Radius = FMath::Clamp(Distance * 0.12f, 6000.0f, 12000.0f);
        Objectives.Add(O);
    };
    // The two lines are each side's own (held from the start); the middle and the wings are to be taken.
    const FVector Mid = DanishLine + Axis * Distance * 0.5f;
    Add(DanishLine, TEXT("Vor stilling"), 100, 1);
    Add(Mid, TEXT("Midten"), 150, 0);
    Add(Mid + Left * Distance * 0.35f, TEXT("Venstre fløj"), 100, 0);
    Add(Mid - Left * Distance * 0.35f, TEXT("Højre fløj"), 100, 0);
    Add(EnemyLine, TEXT("Fjendens stilling"), 100, 2);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-OBJECTIVES: %d objectives over %.0f m, radius %.0f m, time limit %.0f min"), Objectives.Num(), Distance / 100.0f, Objectives[0].Radius / 100.0f, ObjectiveTimeLimit / 60.0f);
}

void AStrategyOOBTestScenario::TickObjectives(float DeltaSeconds)
{
    if (Objectives.Num() == 0 || !GetWorld())
    {
        return;
    }
    BattleClock += DeltaSeconds;
    for (FBattleObjective& O : Objectives)
    {
        O.DanesIn = O.EnemyIn = 0;
    }
    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        const AStrategyUnit* Unit = *It;
        if (!IsValid(Unit) || Unit->Side == EStrategySide::Neutral || Unit->Echelon == EStrategyEchelon::Headquarters || Unit->Echelon == EStrategyEchelon::Supply ||
            Unit->Echelon == EStrategyEchelon::Battalion || Unit->Echelon == EStrategyEchelon::Regiment || Unit->Echelon == EStrategyEchelon::Brigade ||
            Unit->Echelon == EStrategyEchelon::Division || !Unit->IsCombatEffective() || Unit->UnitState == EStrategyUnitState::Routed || Unit->UnitState == EStrategyUnitState::Destroyed)
        {
            continue;
        }
        const bool bDane = Unit->Side == EStrategySide::Denmark;
        for (FBattleObjective& O : Objectives)
        {
            if (FVector::DistSquared2D(Unit->GetActorLocation(), O.Location) <= O.Radius * O.Radius)
            {
                (bDane ? O.DanesIn : O.EnemyIn) += FMath::Max(0, Unit->CurrentStrength);
            }
        }
    }
    int32 Danish = 0, Enemy = 0;
    for (FBattleObjective& O : Objectives)
    {
        // A side takes a place by being there with the men and the other not (a third of them at most); it takes 90 s of holding it.
        const bool bDanes = O.DanesIn >= 30 && O.EnemyIn * 3 < O.DanesIn;
        const bool bEnemy = O.EnemyIn >= 30 && O.DanesIn * 3 < O.EnemyIn;
        if (bDanes)
        {
            O.Progress = FMath::Max(-1.0f, O.Progress - DeltaSeconds / 90.0f);
        }
        else if (bEnemy)
        {
            O.Progress = FMath::Min(1.0f, O.Progress + DeltaSeconds / 90.0f);
        }
        const int32 Before = O.Owner;
        O.Owner = O.Progress < -0.5f ? 1 : O.Progress > 0.5f ? 2 : 0;
        if (O.Owner != Before && O.Owner != 0)
        {
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-OBJECTIVES: %s taken by %s"), *O.Name, O.Owner == 1 ? TEXT("the Danes") : TEXT("the enemy"));
        }
        Danish += O.Owner == 1 ? 1 : 0;
        Enemy += O.Owner == 2 ? 1 : 0;
    }
    const int32 Holder = Danish == Objectives.Num() ? 1 : Enemy == Objectives.Num() ? 2 : 0;
    if (Holder != 0 && Holder == AllHeldBy)
    {
        AllHeldFor += DeltaSeconds;
    }
    else
    {
        AllHeldBy = Holder;
        AllHeldFor = 0.0f;
    }
}

void AStrategyOOBTestScenario::DrawObjectives() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    for (const FBattleObjective& O : Objectives)
    {
        const FColor Colour = O.Owner == 1 ? FColor(70, 120, 235) : O.Owner == 2 ? FColor(220, 70, 60) : FColor(210, 200, 150);
        DrawDebugCircle(World, O.Location + FVector(0.0f, 0.0f, 60.0f), O.Radius, 56, Colour, false, 0.0f, SDPG_World, 40.0f, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);
        DrawDebugLine(World, O.Location, O.Location + FVector(0.0f, 0.0f, 1800.0f), Colour, false, 0.0f, SDPG_World, 18.0f);
    }
}

void AStrategyOOBTestScenario::UpdateBattleOutcome()
{
    if (!BattleOutcome.IsEmpty() || (!bSkirmish && !bCampaignBattle))
    {
        return;
    }
    int32 DS, DN, ES, EN, DB, EB;
    GetBattleScore(DS, DN, ES, EN, DB, EB);
    if (DS <= 0 || ES <= 0)
    {
        return;
    }
    const bool bDanesBeaten = DN < DS * 0.35f;
    const bool bEnemyBeaten = EN < ES * 0.35f;
    if (!bDanesBeaten && !bEnemyBeaten)
    {
        // Not decided by the men: by the places. All of them held for a minute, or the points when the time is up.
        int32 DanishPoints = 0, EnemyPoints = 0;
        GetObjectivePoints(DanishPoints, EnemyPoints);
        if (Objectives.Num() > 0 && AllHeldBy != 0 && AllHeldFor >= 60.0f)
        {
            bDanishVictory = AllHeldBy == 1;
            BattleOutcome = bDanishVictory ? FString(TEXT("SEJR — alle mål er taget og holdt"))
                : FString(TEXT("NEDERLAG — fjenden har taget alle mål"));
        }
        else if (Objectives.Num() > 0 && BattleClock >= ObjectiveTimeLimit)
        {
            if (DanishPoints == EnemyPoints)
            {
                BattleOutcome = FString(TEXT("UAFGJORT — tredje dag er omme, målene er delt"));
            }
            else
            {
                bDanishVictory = DanishPoints > EnemyPoints;
                BattleOutcome = bDanishVictory ? FString::Printf(TEXT("SEJR PÅ POINT — tredje dag er omme, vi holder målene (%d mod %d)"), DanishPoints, EnemyPoints)
                    : FString::Printf(TEXT("NEDERLAG PÅ POINT — tredje dag er omme, fjenden holder målene (%d mod %d)"), EnemyPoints, DanishPoints);
            }
        }
        if (!BattleOutcome.IsEmpty())
        {
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-OUTCOME: %s"), *BattleOutcome);
        }
        return;
    }
    bDanishVictory = bEnemyBeaten && !bDanesBeaten;
    BattleOutcome = bDanishVictory
        ? FString::Printf(TEXT("SEJR — fjenden er slået (%d af %d mand står endnu, %d enheder brudt)"), EN, ES, EB)
        : bEnemyBeaten ? FString(TEXT("UAFGJORT — begge sider er udmattede"))
        : FString::Printf(TEXT("NEDERLAG — vore tropper er slået (%d af %d mand står endnu, %d enheder brudt)"), DN, DS, DB);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-OUTCOME: %s"), *BattleOutcome);
}

bool AStrategyOOBTestScenario::BuildCampaignBattle(const FString& BattlefieldFile, int32 BattleId)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    ClearSpawnedUnits();
    // The flat QA ground away; the campaign's field in its place.
    if (QAFlatGround)
    {
        QAFlatGround->SetVisibility(false, true);
        QAFlatGround->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    CampaignField = World->SpawnActor<AStrategyCampaignBattlefield>(AStrategyCampaignBattlefield::StaticClass(), Origin, FRotator::ZeroRotator, Params);
    if (!CampaignField || !CampaignField->BuildFromFile(BattlefieldFile))
    {
        if (QAFlatGround)
        {
            QAFlatGround->SetVisibility(true, true);
            QAFlatGround->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }
        return false;
    }
    bCampaignBattle = true;
    CampaignBattleId = BattleId;
    CampaignUnitOf.Reset();
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Battle");

    // The request: which Danish units fight, the enemy, the rules (research, doctrine, AI defaults).
    TArray<FString> DanishIds;
    double EnemyMen = 1520.0;
    FString EnemyNation = TEXT("PR");
    FString EnemyRifle;
    double EnemyQuality = 1.0;
    double BearingDeg = -90.0;   // the enemy from the south when the request does not say
    TSharedPtr<FJsonObject> Request;
    {
        FString Text;
        if (BattleId > 0 && FFileHelper::LoadFileToString(Text, *(Dir / FString::Printf(TEXT("BattleRequest_%d.json"), BattleId))) &&
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Request) && Request.IsValid())
        {
            for (const TSharedPtr<FJsonValue>& V : Request->GetArrayField(TEXT("danishUnitIds"))) { DanishIds.Add(V->AsString()); }
            const TSharedPtr<FJsonObject> Enemy = Request->GetObjectField(TEXT("enemy"));
            EnemyMen = Enemy->GetNumberField(TEXT("men"));
            EnemyNation = Enemy->GetStringField(TEXT("nation"));
            Enemy->TryGetStringField(TEXT("rifle"), EnemyRifle);
            Enemy->TryGetNumberField(TEXT("quality"), EnemyQuality);
            Enemy->TryGetNumberField(TEXT("bearingDeg"), BearingDeg);
        }
        else
        {
            Request.Reset();
        }
    }
    // A number from the request (Group.Key, or Key at the top), with a default.
    auto RuleNumber = [&](const TCHAR* Group, const TCHAR* Sub, const TCHAR* Key, double Default)
    {
        double Value = Default;
        const TSharedPtr<FJsonObject>* G = nullptr;
        if (Request && Request->TryGetObjectField(Group, G))
        {
            const TSharedPtr<FJsonObject>* S = nullptr;
            if (Sub && (*G)->TryGetObjectField(Sub, S))
            {
                (*S)->TryGetNumberField(Key, Value);
            }
            else if (!Sub)
            {
                (*G)->TryGetNumberField(Key, Value);
            }
        }
        return Value;
    };
    const double ReloadRule = RuleNumber(TEXT("battleRules"), TEXT("infantry"), TEXT("reloadFactor"), 1.0);
    const double LossRule = RuleNumber(TEXT("battleRules"), nullptr, TEXT("lossFactor"), 1.0) * RuleNumber(TEXT("doctrine"), nullptr, TEXT("lossFactor"), 1.0);
    const double InfantryRule = RuleNumber(TEXT("doctrine"), nullptr, TEXT("infantryFactor"), 1.0);
    bPioneerBridges = false;
    {
        const TSharedPtr<FJsonObject>* Rules = nullptr;
        const TSharedPtr<FJsonObject>* Engineering = nullptr;
        if (Request && Request->TryGetObjectField(TEXT("battleRules"), Rules) && (*Rules)->TryGetObjectField(TEXT("engineering"), Engineering))
        {
            (*Engineering)->TryGetBoolField(TEXT("pioneerBridge"), bPioneerBridges);
        }
        // -Strategy1864Pontoon: the pioneers whatever the research (tests).
        bPioneerBridges |= FParse::Param(FCommandLine::Get(), TEXT("Strategy1864Pontoon"));
    }
    EStrategyFirePolicy DanishPolicy = EStrategyFirePolicy::Medium;
    {
        FString Policy;
        const TSharedPtr<FJsonObject>* Ai = nullptr;
        if (Request && Request->TryGetObjectField(TEXT("aiDefaults"), Ai) && (*Ai)->TryGetStringField(TEXT("firePolicy"), Policy))
        {
            DanishPolicy = Policy == TEXT("HOLD") ? EStrategyFirePolicy::Hold : Policy == TEXT("CLOSE") ? EStrategyFirePolicy::Close
                : Policy == TEXT("LONG") ? EStrategyFirePolicy::Long : EStrategyFirePolicy::Medium;
        }
    }
    // The units as the campaign has them.
    TArray<TSharedPtr<FJsonObject>> Units;
    {
        FString Text;
        TSharedPtr<FJsonObject> Json;
        if (FFileHelper::LoadFileToString(Text, *(Dir / TEXT("Units.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json.IsValid())
        {
            int32 Infantry = 0, Cavalry = 0, Batteries = 0;
            for (const TSharedPtr<FJsonValue>& V : Json->GetArrayField(TEXT("units")))
            {
                const TSharedPtr<FJsonObject> U = V->AsObject();
                const TSharedPtr<FJsonObject>* Battle = nullptr;
                if (!U->TryGetObjectField(TEXT("battle"), Battle))
                {
                    continue;
                }
                const FString Type = (*Battle)->GetStringField(TEXT("type"));
                if (DanishIds.Num() > 0)
                {
                    if (DanishIds.Contains(U->GetStringField(TEXT("id")))) { Units.Add(U); }
                    continue;
                }
                // A test without a request: two battalions, a cavalry regiment and a battery.
                const bool bInf = Type.Contains(TEXT("battalion")), bCav = Type.Contains(TEXT("regiment")), bArt = Type.Contains(TEXT("battery"));
                if ((bInf && Infantry < 2) || (bCav && Cavalry < 1) || (bArt && Batteries < 1))
                {
                    Units.Add(U);
                    Infantry += bInf ? 1 : 0;
                    Cavalry += bCav ? 1 : 0;
                    Batteries += bArt ? 1 : 0;
                }
            }
        }
    }

    // -Strategy1864FieldLOD=<1|2|5|10>: how many men a figure stands for (the simulation keeps every man).
    int32 Lod = 5;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864FieldLOD="), Lod);

    TArray<TSharedPtr<FJsonObject>> Battalions, Horse, Guns;
    for (const TSharedPtr<FJsonObject>& U : Units)
    {
        const FString Type = U->GetObjectField(TEXT("battle"))->GetStringField(TEXT("type"));
        (Type.Contains(TEXT("battalion")) ? Battalions : Type.Contains(TEXT("battery")) ? Guns : Horse).Add(U);
    }
    const float FieldSpacing = 7200.0f;   // 72 m centre to centre (design 8)
    const float BattalionWidth = 4.0f * FieldSpacing + 3000.0f;
    const int32 EnemyCompanies = FMath::Clamp(FMath::RoundToInt(EnemyMen / 190.0), 1, 16);
    const int32 PerLine = FMath::Min(EnemyCompanies, 8);

    // The two lines on open ground along the enemy's approach: the Danes in front of the place they hold (between
    // it and the enemy), at the first distance from the middle where the whole front is on open ground (no town,
    // wood, water, ways or knicks), at least 350 m out; the enemy 700 m beyond them or more, on open ground too.
    const double Bearing = FMath::DegreesToRadians(BearingDeg);
    const FVector ToEnemy(FMath::Sin(Bearing), FMath::Cos(Bearing), 0.0);   // world +X north, +Y east
    const FVector Lateral(-ToEnemy.Y, ToEnemy.X, 0.0);                      // the Danes' right
    const float DanishYaw = ToEnemy.Rotation().Yaw;
    auto OpenShare = [&](const FVector& Centre, const FVector& Forward, float HalfWidthCm)
    {
        int32 Open = 0, All = 0;
        const FVector Side(-Forward.Y, Forward.X, 0.0);
        for (float Depth = -6000.0f; Depth <= 6000.0f; Depth += 3000.0f)
        {
            for (float S = -HalfWidthCm; S <= HalfWidthCm; S += 3000.0f)
            {
                ++All;
                Open += CampaignField->IsOpenGround(Centre - Forward * Depth + Side * S) ? 1 : 0;
            }
        }
        return All > 0 ? float(Open) / float(All) : 0.0f;
    };
    auto FindLine = [&](const FVector& Outward, float MinCm, float HalfWidthCm)
    {
        float Best = MinCm, BestShare = -1.0f;
        for (float D = MinCm; D <= 280000.0f; D += 5000.0f)
        {
            const float Share = OpenShare(Origin + Outward * D, -Outward, HalfWidthCm);
            if (Share >= 0.8f)
            {
                return D;
            }
            if (Share > BestShare + 0.02f)
            {
                BestShare = Share;
                Best = D;
            }
        }
        return Best;
    };
    const float DanishHalf = FMath::Max(1, Battalions.Num()) * BattalionWidth * 0.5f + (Horse.Num() > 0 ? 15000.0f : 3000.0f);
    const float EnemyHalf = PerLine * FieldSpacing * 0.5f;
    const float DanishOut = FindLine(ToEnemy, 35000.0f, DanishHalf);
    const float EnemyOut = FindLine(ToEnemy, DanishOut + 70000.0f, EnemyHalf);
    const FVector DanishLine = Origin + ToEnemy * DanishOut;
    const FVector EnemyLine = Origin + ToEnemy * EnemyOut;
    auto Danish = [&](float Forward, float Right) { return DanishLine + ToEnemy * Forward + Lateral * Right; };
    auto Hostile = [&](float Forward, float Right) { return EnemyLine - ToEnemy * Forward - Lateral * Right; };
    auto Face = [&](AStrategyUnit* Unit, bool bEnemy)
    {
        if (Unit)
        {
            Unit->SetActorRotation(FRotator(0.0f, bEnemy ? DanishYaw + 180.0f : DanishYaw, 0.0f));
        }
    };

    // The rules on a Danish company: the regiment's own training (reload, accuracy, morale, cohesion), the
    // research (reload factor, the fire methods it has drilled to 60), the doctrine and the AI defaults.
    auto ApplyDanish = [&](AStrategyCompanyUnit* Company, const TSharedPtr<FJsonObject>& U)
    {
        double Reload = 1.0, Accuracy = 1.0, Morale = -1.0, Cohesion = -1.0;
        const TSharedPtr<FJsonObject>* Factors = nullptr;
        if (U->TryGetObjectField(TEXT("battleFactors"), Factors))
        {
            (*Factors)->TryGetNumberField(TEXT("reloadTime"), Reload);
            (*Factors)->TryGetNumberField(TEXT("accuracy"), Accuracy);
            (*Factors)->TryGetNumberField(TEXT("morale"), Morale);
            (*Factors)->TryGetNumberField(TEXT("cohesion"), Cohesion);
        }
        if (Company->CombatComponent)
        {
            Company->CombatComponent->ReloadSeconds *= float(Reload * ReloadRule);
            Company->CombatComponent->BaseHitChance *= float(Accuracy * InfantryRule);
        }
        if (Morale >= 0.0) { Company->Morale = float(FMath::Clamp(Morale * 100.0, 10.0, 100.0)); }
        if (Cohesion >= 0.0) { Company->Cohesion = float(FMath::Clamp(Cohesion * 100.0, 10.0, 100.0)); }
        if (Company->FireControlComponent)
        {
            Company->FireControlComponent->SetFirePolicy(DanishPolicy);
        }
        if (UStrategyFireDrillComponent* Drill = Company->FireDrillComponent)
        {
            // The highest fire method the regiment has drilled to 60 (front rank fire is always there).
            double Two = 0.0, ByRank = 0.0, Volley = 0.0, Free = 0.0;
            const TSharedPtr<FJsonObject>* Drills = nullptr;
            if (U->TryGetObjectField(TEXT("fireDrills"), Drills))
            {
                (*Drills)->TryGetNumberField(TEXT("twoRank"), Two);
                (*Drills)->TryGetNumberField(TEXT("byRank"), ByRank);
                (*Drills)->TryGetNumberField(TEXT("volley"), Volley);
                (*Drills)->TryGetNumberField(TEXT("independent"), Free);
            }
            const EStrategyFireDrillResearchLevel Level = Free >= 60.0 ? EStrategyFireDrillResearchLevel::IndependentFire
                : Volley >= 60.0 ? EStrategyFireDrillResearchLevel::ControlledVolley : ByRank >= 60.0 ? EStrategyFireDrillResearchLevel::FireByRank
                : Two >= 60.0 ? EStrategyFireDrillResearchLevel::TwoRankFire : EStrategyFireDrillResearchLevel::FrontRankFire;
            Drill->SetResearchLevel(Level);
            Drill->DrillTraining = float(FMath::Clamp(FMath::Max(FMath::Max(Two, ByRank), FMath::Max(Volley, Free)), 40.0, 100.0));
            // Fire by rank when drilled (the steady fire of the line), else the best there is.
            Drill->SetDrillMode(Drill->IsDrillModeUnlocked(EStrategyFireDrillMode::FireByRank) ? EStrategyFireDrillMode::FireByRank : Drill->GetHighestUnlockedDrillMode());
        }
    };

    AStrategyHQUnit* Army = SpawnHQ(TEXT("DK-FIELD-HQ"), TEXT("Felthæren"), static_cast<uint8>(EStrategyHQLevel::Division), Danish(-30000.0f, 0.0f), nullptr);
    Face(Army, false);
    float Y = -(Battalions.Num() - 1) * BattalionWidth * 0.5f;
    for (const TSharedPtr<FJsonObject>& U : Battalions)
    {
        const FString Id = U->GetStringField(TEXT("id"));
        const TSharedPtr<FJsonObject> Battle = U->GetObjectField(TEXT("battle"));
        AStrategyHQUnit* Major = SpawnHQ(FName(*FString::Printf(TEXT("DK-%s-HQ"), *Id)), U->GetStringField(TEXT("name")), static_cast<uint8>(EStrategyHQLevel::Battalion), Danish(-9000.0f, Y), Army);
        Face(Major, false);
        CampaignUnitOf.Add(Major, Id);
        const TArray<TSharedPtr<FJsonValue>>& Subs = Battle->GetArrayField(TEXT("subunits"));
        int32 Number = 0;
        for (int32 c = 0; c < Subs.Num(); ++c)
        {
            const TSharedPtr<FJsonObject> Sub = Subs[c]->AsObject();
            const int32 Men = int32(Sub->GetNumberField(TEXT("men")));
            if (Men <= 0)
            {
                continue;   // in a fort, or no men with the colours
            }
            ++Number;
            AStrategyCompanyUnit* Company = SpawnCompany(FName(*FString::Printf(TEXT("DK-%s-C%d"), *Id, c + 1)),
                FString::Printf(TEXT("%s %s"), *Id, *Sub->GetStringField(TEXT("name")).Left(3)), c + 1,
                Danish(0.0f, Y + (c - 1.5f) * FieldSpacing), Major, static_cast<uint8>(EStrategySide::Denmark));
            if (Company)
            {
                Face(Company, false);
                Company->InitialStrength = Company->CurrentStrength = Men;
                ApplyDanish(Company, U);
                if (Company->InfantryVisualComponent)
                {
                    // The model by arm: the jægerkorps in the jæger uniform, the guard as Livgarden, the line in the standard.
                    const FString Kind = Battle->GetStringField(TEXT("type"));
                    const TCHAR* Mesh = Kind.Contains(TEXT("jager")) ? TEXT("/Game/Units/Danish/Jager1864/Mesh/SK_DK_Jager_1864.SK_DK_Jager_1864")
                        : Kind.Contains(TEXT("guard")) ? TEXT("/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864.SK_DK_Livgarden_1864")
                        : TEXT("/Game/Units/Danish/Infantry1864/Mesh/SK_DK_Infantry_1864.SK_DK_Infantry_1864");
                    Company->InfantryVisualComponent->SoldierMeshAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Mesh));
                    Company->InfantryVisualComponent->SetVisualScaleDivisor(Lod);
                    Company->InfantryVisualComponent->SetEnabled(true);
                }
                ConfigureRuntimeQALabel(Company);
                CampaignUnitOf.Add(Company, Id);
            }
        }
        Y += BattalionWidth;
    }
    // Cavalry on the flanks, the batteries behind the middle.
    for (int32 h = 0; h < Horse.Num(); ++h)
    {
        const TSharedPtr<FJsonObject>& U = Horse[h];
        const FString Id = U->GetStringField(TEXT("id"));
        const float Side = (h % 2 == 0 ? -1.0f : 1.0f) * (Battalions.Num() * BattalionWidth * 0.5f + 12000.0f + (h / 2) * 15000.0f);
        if (ACavalryUnit* Cav = SpawnCavalry(FName(*FString::Printf(TEXT("DK-%s"), *Id)), U->GetStringField(TEXT("name")), Danish(-6000.0f, Side), Army))
        {
            Face(Cav, false);
            const int32 Men = FMath::Max(1, int32(U->GetNumberField(TEXT("presentMen"))));
            Cav->InitialStrength = Cav->CurrentStrength = Men;
            ConfigureRuntimeQALabel(Cav);
            CampaignUnitOf.Add(Cav, Id);
        }
    }
    for (int32 g = 0; g < Guns.Num(); ++g)
    {
        const TSharedPtr<FJsonObject>& U = Guns[g];
        const FString Id = U->GetStringField(TEXT("id"));
        if (AStrategyArtilleryBatteryUnit* Battery = SpawnArtilleryBattery(FName(*FString::Printf(TEXT("DK-%s"), *Id)), U->GetStringField(TEXT("name")),
            Danish(-14000.0f, (g - (Guns.Num() - 1) * 0.5f) * 14000.0f), Army))
        {
            Face(Battery, false);
            ConfigureRuntimeQALabel(Battery);
            CampaignUnitOf.Add(Battery, Id);
        }
    }

    // The enemy: a brigade staff, a battalion staff for every four companies, companies of 190 (at most sixteen)
    // in two lines, the AI on. His rifle and quality from the request (the Prussian needle gun loads lying and
    // three times as fast; the Danish loss factors make his fire count for more or less).
    const EStrategySide EnemySide = EnemyNation == TEXT("AT") ? EStrategySide::Austria : EStrategySide::Prussia;
    const bool bNeedleGun = EnemyRifle.Contains(TEXT("Dreyse")) || EnemyRifle.Contains(TEXT("ndnål")) || (EnemyRifle.IsEmpty() && EnemySide == EStrategySide::Prussia);
    const FString EnemyLabel = EnemySide == EStrategySide::Austria ? TEXT("Østr.") : TEXT("Pr.");
    AStrategyHQUnit* EnemyBrigade = SpawnHQ(FName(*FString::Printf(TEXT("EN-%s-BDE"), *EnemyNation)), FString::Printf(TEXT("%s brigade"), *EnemyLabel),
        static_cast<uint8>(EStrategyHQLevel::Brigade), Hostile(-30000.0f, 0.0f), nullptr);
    if (EnemyBrigade)
    {
        EnemyBrigade->Side = EnemySide;
        EnemyBrigade->bPlayerControllable = false;
        EnemyBrigade->bOfficerAIEnabled = true;
        EnemyBrigade->RefreshDebugLabel();
        Face(EnemyBrigade, true);
    }
    TArray<AStrategyHQUnit*> EnemyBattalions;
    for (int32 b = 0; b < (EnemyCompanies + 3) / 4; ++b)
    {
        const int32 First = b * 4, Line = First / PerLine;
        const float Right = ((First % PerLine) + 1.5f - (PerLine - 1) * 0.5f) * FieldSpacing;
        AStrategyHQUnit* HQ = SpawnHQ(FName(*FString::Printf(TEXT("EN-%s-BN%d"), *EnemyNation, b + 1)), FString::Printf(TEXT("%s %d. bataillon"), *EnemyLabel, b + 1),
            static_cast<uint8>(EStrategyHQLevel::Battalion), Hostile(-9000.0f - Line * 15000.0f, Right), EnemyBrigade);
        if (HQ)
        {
            HQ->Side = EnemySide;
            HQ->bPlayerControllable = false;
            HQ->bOfficerAIEnabled = true;
            HQ->RefreshDebugLabel();
            Face(HQ, true);
        }
        EnemyBattalions.Add(HQ);
    }
    for (int32 e = 0; e < EnemyCompanies; ++e)
    {
        const int32 Line = e / PerLine, InLine = e % PerLine;
        AStrategyCompanyUnit* Company = SpawnCompany(FName(*FString::Printf(TEXT("EN-%s-C%d"), *EnemyNation, e + 1)),
            FString::Printf(TEXT("%s %d. Kp."), *EnemyLabel, e + 1), e + 1,
            Hostile(-Line * 15000.0f, (InLine - (PerLine - 1) * 0.5f) * FieldSpacing), EnemyBattalions.IsValidIndex(e / 4) ? EnemyBattalions[e / 4] : nullptr,
            static_cast<uint8>(EnemySide));
        if (Company)
        {
            Face(Company, true);
            Company->bOfficerAIEnabled = true;
            if (Company->AutonomousBattleAIComponent)
            {
                Company->AutonomousBattleAIComponent->bEnableForNonPlayerSides = true;
            }
            if (Company->CombatComponent)
            {
                Company->CombatComponent->BaseHitChance *= float(EnemyQuality * LossRule);
                if (bNeedleGun)
                {
                    Company->CombatComponent->ReloadSeconds *= 0.35f;
                }
            }
            if (UStrategyFireDrillComponent* Drill = Company->FireDrillComponent)
            {
                // The Prussians fought in open order with the needle gun (independent fire); the Austrians by volley.
                Drill->SetLoadingMethod(bNeedleGun ? EStrategyLoadingMethod::BreechLoader : EStrategyLoadingMethod::MuzzleLoader);
                Drill->SetResearchLevel(EStrategyFireDrillResearchLevel::AdvancedFireDrill);
                Drill->DrillTraining = 70.0f;
                Drill->SetDrillMode(bNeedleGun ? EStrategyFireDrillMode::Independent : EStrategyFireDrillMode::Volley);
            }
            if (Company->InfantryVisualComponent)
            {
                // No Prussian model yet: the Swedish stands in.
                Company->InfantryVisualComponent->SoldierMeshAsset = TSoftObjectPtr<USkeletalMesh>(
                    FSoftObjectPath(TEXT("/Game/Units/Swedish/Infantry1864/Mesh/SK_SE_Infantry_1864.SK_SE_Infantry_1864")));
                Company->InfantryVisualComponent->SetVisualScaleDivisor(Lod);
                Company->InfantryVisualComponent->SetEnabled(true);
            }
            ConfigureRuntimeQALabel(Company);
            CampaignUnitOf.Add(Company, FString());
        }
    }
    // A check of the crossings: from the Danish line 2.5 km away from the enemy (over the place they hold).
    {
        TArray<FVector> Via;
        const bool bWay = CampaignField->RouteAcrossRivers(DanishLine, DanishLine - ToEnemy * 250000.0f, Via);
        UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: %d bridges; the way 2.5 km back %s (%d bridge points)"), CampaignField->GetBridgeCount(),
            bWay ? TEXT("is open") : TEXT("is blocked by a broad river"), Via.Num());
    }
    // The camera behind the Danish line, looking towards the enemy (placed on the first tick, after the QA
    // bootstrap has placed its own).
    FieldCameraTarget = Danish(-6000.0f, 0.0f);
    FieldCameraTarget.Z = CampaignField->GroundZ(FieldCameraTarget);
    FieldCameraYaw = DanishYaw;
    bFieldCameraPlaced = false;
    SetupObjectives(DanishLine, EnemyLine);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: battle %d at %s: %d Danish units (%d battalions, %d cavalry, %d batteries), %d enemy companies (%s, %.0f men%s), figures 1:%d"),
        BattleId, *CampaignField->Place, Units.Num(), Battalions.Num(), Horse.Num(), Guns.Num(), EnemyCompanies, *EnemyNation, EnemyMen, bNeedleGun ? TEXT(", needle gun") : TEXT(""), Lod);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: the enemy from %.0f deg; the Danish line %.0f m out, the enemy %.0f m out (open %.0f%% / %.0f%%); rules reload x%.2f, losses x%.2f, policy %d"),
        BearingDeg, DanishOut / 100.0f, EnemyOut / 100.0f, OpenShare(DanishLine, ToEnemy, DanishHalf) * 100.0f, OpenShare(EnemyLine, -ToEnemy, EnemyHalf) * 100.0f,
        ReloadRule, LossRule, int32(DanishPolicy));
    return true;
}

void AStrategyOOBTestScenario::FinishCampaignBattle()
{
    if (!bCampaignBattle)
    {
        return;
    }
    // Losses per campaign unit, the enemy's, and the outcome by the share each side has left.
    TMap<FString, int32> Losses, Kills;
    int32 DanesStart = 0, DanesNow = 0, EnemyStart = 0, EnemyNow = 0;
    for (const TPair<TWeakObjectPtr<AStrategyUnit>, FString>& It : CampaignUnitOf)
    {
        const AStrategyUnit* Unit = It.Key.Get();
        if (!Unit || Unit->Echelon == EStrategyEchelon::Battalion || Unit->Echelon == EStrategyEchelon::Division)
        {
            continue;
        }
        const int32 Lost = FMath::Max(0, Unit->InitialStrength - FMath::Max(0, Unit->CurrentStrength));
        if (It.Value.IsEmpty())
        {
            EnemyStart += Unit->InitialStrength;
            EnemyNow += FMath::Max(0, Unit->CurrentStrength);
        }
        else
        {
            Losses.FindOrAdd(It.Value) += Lost;
            Kills.FindOrAdd(It.Value) += Unit->CombatComponent ? Unit->CombatComponent->TotalHitsInflicted : 0;
            DanesStart += Unit->InitialStrength;
            DanesNow += FMath::Max(0, Unit->CurrentStrength);
        }
    }
    const float DanesLeft = DanesStart > 0 ? float(DanesNow) / DanesStart : 0.0f;
    const float EnemyLeft = EnemyStart > 0 ? float(EnemyNow) / EnemyStart : 0.0f;
    // The battle's own decision when it came (a side broken), else by the share each side has left.
    int32 DanishPoints = 0, EnemyPoints = 0;
    GetObjectivePoints(DanishPoints, EnemyPoints);
    const FString Outcome = !BattleOutcome.IsEmpty() ? (bDanishVictory ? TEXT("danish_victory") : BattleOutcome.StartsWith(TEXT("UAFGJORT")) ? TEXT("draw") : TEXT("enemy_victory"))
        : DanishPoints >= EnemyPoints + 100 ? TEXT("danish_victory") : EnemyPoints >= DanishPoints + 100 ? TEXT("enemy_victory")
        : DanesLeft > EnemyLeft + 0.05f ? TEXT("danish_victory") : EnemyLeft > DanesLeft + 0.05f ? TEXT("enemy_victory") : TEXT("draw");
    TSharedRef<FJsonObject> Doc = MakeShared<FJsonObject>();
    Doc->SetStringField(TEXT("format"), TEXT("PROJECT1864-BattleResult-1"));
    Doc->SetNumberField(TEXT("battleId"), CampaignBattleId);
    Doc->SetStringField(TEXT("outcome"), Outcome);
    Doc->SetNumberField(TEXT("enemyLosses"), EnemyStart - EnemyNow);
    Doc->SetNumberField(TEXT("danishPoints"), DanishPoints);
    Doc->SetNumberField(TEXT("enemyPoints"), EnemyPoints);
    TArray<TSharedPtr<FJsonValue>> UnitList;
    for (const TPair<FString, int32>& L : Losses)
    {
        TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
        O->SetStringField(TEXT("id"), L.Key);
        O->SetNumberField(TEXT("losses"), L.Value);
        O->SetNumberField(TEXT("kills"), Kills.FindRef(L.Key));
        UnitList.Add(MakeShared<FJsonValueObject>(O));
    }
    Doc->SetArrayField(TEXT("units"), UnitList);
    FString Text;
    FJsonSerializer::Serialize(Doc, TJsonWriterFactory<>::Create(&Text));
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Battle");
    const int32 Id = FMath::Max(1, CampaignBattleId);
    FFileHelper::SaveStringToFile(Text, *(Dir / FString::Printf(TEXT("BattleResult_%d.json"), Id)), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: result %s (Danes %d of %d left, enemy %d of %d) -> BattleResult_%d.json"), *Outcome, DanesNow, DanesStart, EnemyNow, EnemyStart, Id);
    if (bReturnToCampaign)
    {
        // The campaign loads its autosave when it finds this (whatever its command line says).
        FFileHelper::SaveStringToFile(FString::FromInt(Id), *(Dir / TEXT("ReturnToCampaign.flag")));
        UGameplayStatics::OpenLevel(this, FName(TEXT("Campaign1851")));
    }
}

bool AStrategyOOBTestScenario::OrderPontoonBridge(const AStrategyUnit* By)
{
    if (!CanLayPontoonBridges() || !By || !GetWorld() || PendingPontoons.Num() > 0)
    {
        return false;
    }
    FPendingPontoon Pending;
    Pending.Where = By->GetActorLocation();
    Pending.ReadyAt = GetWorld()->GetTimeSeconds() + 300.0f;
    PendingPontoons.Add(Pending);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: %s orders a pontoon bridge (five minutes)"), *By->DisplayName.ToString());
    return true;
}

float AStrategyOOBTestScenario::GetPontoonSecondsLeft() const
{
    return PendingPontoons.Num() > 0 && GetWorld() ? FMath::Max(0.0f, PendingPontoons[0].ReadyAt - GetWorld()->GetTimeSeconds()) : 0.0f;
}

void AStrategyOOBTestScenario::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TickShots();
    // The enemy holds fire (a test): kept so, also for units the AI or a new order would set to fire again.
    EnemyFireTimer -= DeltaSeconds;
    if (!bEnemyFiring && EnemyFireTimer <= 0.0f)
    {
        EnemyFireTimer = 0.5f;
        EnforceEnemyHoldFire();
    }

    if (bSkirmish || bCampaignBattle)
    {
        if (BattleOutcome.IsEmpty())
        {
            TickObjectives(DeltaSeconds);
        }
        DrawObjectives();
    }

    BattleScoreTimer -= DeltaSeconds;
    if (BattleScoreTimer <= 0.0f)
    {
        BattleScoreTimer = 0.5f;
        UpdateBattleOutcome();
    }

    // The pioneers' bridges: laid when their time is up (over the broad river nearest the ordering staff).
    for (int32 p = PendingPontoons.Num() - 1; p >= 0; --p)
    {
        if (GetWorld() && GetWorld()->GetTimeSeconds() >= PendingPontoons[p].ReadyAt)
        {
            if (!CampaignField || !CampaignField->LayPontoonBridge(PendingPontoons[p].Where, 60000.0f))
            {
                UE_LOG(LogTemp, Display, TEXT("PROJECT1864-FIELD: no broad river within 600 m of the pioneers"));
            }
            PendingPontoons.RemoveAt(p);
        }
    }

    if (bDrawRuntimeQAVisuals)
    {
        DrawRuntimeQAVisuals();
    }

    // -Strategy1864AutoFinish=<seconds>: the battle ends by itself (test of the way back).
    if (bCampaignBattle)
    {
        float AutoFinish = 0.0f;
        if (FParse::Value(FCommandLine::Get(), TEXT("Strategy1864AutoFinish="), AutoFinish) && AutoFinish > 0.0f &&
            GetWorld()->GetTimeSeconds() > AutoFinish && !bCampaignFinished)
        {
            bCampaignFinished = true;
            FinishCampaignBattle();
            return;
        }
    }

    if ((bCampaignBattle || bSkirmish) && !bFieldCameraPlaced)
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            if (AStrategyCameraPawn* Camera = Cast<AStrategyCameraPawn>(PC->GetPawn()))
            {
                Camera->SetActorRotation(FRotator(0.0f, FieldCameraYaw, 0.0f));
                Camera->FocusOnWorldLocation(FieldCameraTarget);
                if (Camera->SpringArm)
                {
                    Camera->SpringArm->TargetArmLength = 22000.0f;
                    Camera->SpringArm->SetRelativeRotation(FRotator(-40.0f, 0.0f, 0.0f));
                }
                bFieldCameraPlaced = true;
            }
        }
    }

    if (bLivgardenVsSwedishTest && DuelCompanies.Num() == 2)
    {
        TickDuel(DeltaSeconds);
        // The cones are the HUD's (AStrategyHUD::DrawFireCone).
    }
}

void AStrategyOOBTestScenario::SetEnemyFiring(bool bFire)
{
    bEnemyFiring = bFire;
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AStrategyUnit> It(World); It; ++It)
        {
            AStrategyUnit* U = *It;
            if (!IsValid(U) || U->Side == EStrategySide::Denmark || U->Side == EStrategySide::Neutral)
            {
                continue;
            }
            if (bFire)
            {
                if (U->FireControlComponent && U->FireControlComponent->FirePolicy == EStrategyFirePolicy::Hold)
                {
                    U->FireControlComponent->SetFirePolicy(EStrategyFirePolicy::Long);
                }
                if (AStrategyArtilleryBatteryUnit* Battery = Cast<AStrategyArtilleryBatteryUnit>(U))
                {
                    if (Battery->ArtilleryFireMissionComponent) { Battery->ArtilleryFireMissionComponent->SetHoldFire(false); }
                }
            }
        }
    }
    if (!bFire)
    {
        EnforceEnemyHoldFire();
    }
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ENEMY: fire %s"), bFire ? TEXT("on") : TEXT("off"));
}

void AStrategyOOBTestScenario::EnforceEnemyHoldFire()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    for (TActorIterator<AStrategyUnit> It(World); It; ++It)
    {
        AStrategyUnit* U = *It;
        if (!IsValid(U) || U->Side == EStrategySide::Denmark || U->Side == EStrategySide::Neutral)
        {
            continue;
        }
        if (U->FireControlComponent && U->FireControlComponent->FirePolicy != EStrategyFirePolicy::Hold)
        {
            U->FireControlComponent->SetFirePolicy(EStrategyFirePolicy::Hold);
        }
        if (AStrategyArtilleryBatteryUnit* Battery = Cast<AStrategyArtilleryBatteryUnit>(U))
        {
            if (Battery->ArtilleryFireMissionComponent) { Battery->ArtilleryFireMissionComponent->SetHoldFire(true); }
            if (AStrategyMortarBatteryUnit* Mortar = Cast<AStrategyMortarBatteryUnit>(Battery))
            {
                if (Mortar->MortarFireComponent) { Mortar->MortarFireComponent->HoldFire(); }
            }
        }
    }
}

void AStrategyOOBTestScenario::SetEnemyAttacking(bool bAttack)
{
    bEnemyAttacking = bAttack;
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    // Where the Danes stand (the middle of their fighting units).
    FVector Danes = FVector::ZeroVector;
    int32 Count = 0;
    for (TActorIterator<AStrategyUnit> It(World); It; ++It)
    {
        if (It->Side == EStrategySide::Denmark && It->IsCombatEffective() && It->Echelon != EStrategyEchelon::Headquarters)
        {
            Danes += It->GetActorLocation();
            ++Count;
        }
    }
    if (Count == 0)
    {
        return;
    }
    Danes /= float(Count);
    for (TActorIterator<AStrategyUnit> It(World); It; ++It)
    {
        AStrategyUnit* U = *It;
        if (!IsValid(U) || U->Side == EStrategySide::Denmark || U->Side == EStrategySide::Neutral || !U->OrderComponent)
        {
            continue;
        }
        if (U->AutonomousBattleAIComponent)
        {
            U->AutonomousBattleAIComponent->bEnableForNonPlayerSides = bAttack;
        }
        if (U->DoctrineComponent)
        {
            U->DoctrineComponent->Doctrine = bAttack ? EStrategyDoctrine::Offensive : EStrategyDoctrine::Defensive;
        }
        const float Facing = (Danes - U->GetActorLocation()).Rotation().Yaw;
        FStrategyOrder Order;
        Order.FacingYaw = Facing;
        Order.bHasFacing = true;
        Order.Authority = EStrategyOrderAuthority::OfficerAI;
        if (bAttack && U->Echelon == EStrategyEchelon::Headquarters)
        {
            Order.Type = EStrategyOrderType::AttackHere;
            Order.TargetLocation = Danes;
        }
        else if (!bAttack && U->Echelon != EStrategyEchelon::Artillery)
        {
            Order.Type = EStrategyOrderType::DefendHere;
            Order.TargetLocation = U->GetActorLocation();
        }
        else
        {
            continue;
        }
        U->OrderComponent->SetOrder(Order);
    }
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ENEMY: %s"), bAttack ? TEXT("attacks") : TEXT("defends"));
}

void AStrategyOOBTestScenario::TickShots()
{
    if (!bShotsParsed)
    {
        bShotsParsed = true;
        FString Plan;
        if (FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Shots="), Plan, false))
        {
            Plan.ParseIntoArray(ShotPlan, TEXT(","));
        }
    }
    UWorld* World = GetWorld();
    if (!World || NextShot >= ShotPlan.Num())
    {
        return;
    }
    const float Now = World->GetTimeSeconds();
    if (ShotTakeAt > 0.0f)
    {
        if (Now >= ShotTakeAt)
        {
            const FString File = FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString::Printf(TEXT("battle_shot_%02d.png"), NextShot);
            FScreenshotRequest::RequestScreenshot(File, false, false);
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-SHOT: %s (%s)"), *File, *ShotPlan[NextShot]);
            ShotTakeAt = -1.0f;
            ++NextShot;
            if (NextShot >= ShotPlan.Num() && FParse::Param(FCommandLine::Get(), TEXT("Strategy1864ShotsQuit")))
            {
                FTimerHandle Quit;
                World->GetTimerManager().SetTimer(Quit, []() { FPlatformMisc::RequestExit(false); }, 2.0f, false);
            }
        }
        return;
    }
    TArray<FString> Parts;
    ShotPlan[NextShot].ParseIntoArray(Parts, TEXT(":"));
    if (Parts.Num() < 2 || Now < FCString::Atof(*Parts[0]))
    {
        return;
    }
    const float Distance = Parts.Num() > 2 ? FCString::Atof(*Parts[2]) : 4000.0f;
    const float Side = Parts.Num() > 3 ? FCString::Atof(*Parts[3]) : 0.5f;
    AStrategyUnit* Target = nullptr;
    for (TActorIterator<AStrategyUnit> It(World); It; ++It)
    {
        if (It->StableUnitId.ToString().Contains(Parts[1]))
        {
            Target = *It;
            break;
        }
    }
    if (!Target)
    {
        ++NextShot;
        return;
    }
    const FVector At = Target->GetActorLocation();
    const FVector Fwd = Target->GetActorForwardVector().GetSafeNormal2D();
    const FVector Right(-Fwd.Y, Fwd.X, 0.0f);
    const FVector Eye = At - Fwd * Distance * 0.8f + Right * Distance * Side + FVector(0.0f, 0.0f, Distance * 0.4f);
    if (!ShotCamera)
    {
        ShotCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Eye, FRotator::ZeroRotator);
    }
    if (ShotCamera)
    {
        ShotCamera->SetActorLocationAndRotation(Eye, (At + Fwd * Distance * 0.3f - Eye).Rotation());
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PC->SetViewTarget(ShotCamera);
        }
    }
    ShotTakeAt = Now + 0.8f;
}

void AStrategyOOBTestScenario::TickDuel(float DeltaSeconds)
{
    // Close in on the two companies once, so the soldiers can be seen (the QA bootstrap starts far out).
    if (!bDuelCameraPlaced)
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            if (AStrategyCameraPawn* Camera = Cast<AStrategyCameraPawn>(PC->GetPawn()))
            {
                // -Strategy1864DuelCamera=<cm>: how far out (and -Strategy1864DuelFocus=0/1: on one of the companies).
                float Arm = 9000.0f;
                int32 FocusOn = -1;
                FParse::Value(FCommandLine::Get(), TEXT("Strategy1864DuelCamera="), Arm);
                FParse::Value(FCommandLine::Get(), TEXT("Strategy1864DuelFocus="), FocusOn);
                const FVector Mid = DuelCompanies.IsValidIndex(FocusOn) ? DuelCompanies[FocusOn]->GetActorLocation()
                    : (DuelCompanies[0]->GetActorLocation() + DuelCompanies[1]->GetActorLocation()) * 0.5f;
                Camera->FocusOnWorldLocation(Mid);
                float Pitch = -55.0f;
                float CameraYaw = 0.0f;
                FParse::Value(FCommandLine::Get(), TEXT("Strategy1864DuelPitch="), Pitch);
                FParse::Value(FCommandLine::Get(), TEXT("Strategy1864DuelYaw="), CameraYaw);
                if (Camera->SpringArm)
                {
                    Camera->SpringArm->TargetArmLength = Arm;
                    Camera->SpringArm->SetRelativeRotation(FRotator(FMath::Clamp(Pitch, -85.0f, -5.0f), CameraYaw, 0.0f));
                }
                bDuelCameraPlaced = true;
            }
        }
    }

    // A company in focus: the camera follows it.
    int32 Follow = -1;
    if (FParse::Value(FCommandLine::Get(), TEXT("Strategy1864DuelFocus="), Follow) && DuelCompanies.IsValidIndex(Follow) && IsValid(DuelCompanies[Follow]))
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            if (AStrategyCameraPawn* Camera = Cast<AStrategyCameraPawn>(PC->GetPawn()))
            {
                Camera->FocusOnWorldLocation(DuelCompanies[Follow]->GetActorLocation());
            }
        }
    }

    DuelAccumulator += DeltaSeconds;
    if (DuelAccumulator < 0.5f)
    {
        return;
    }
    DuelAccumulator = 0.0f;

    float SideOffsetCm = 0.0f;
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864DuelOffset="), SideOffsetCm);
    for (int32 Me = 0; Me < 2; ++Me)
    {
        AStrategyCompanyUnit* Company = DuelCompanies[Me];
        AStrategyCompanyUnit* Enemy = DuelCompanies[1 - Me];
        if (!IsValid(Company) || !IsValid(Enemy) || !Company->IsCombatEffective() || !Enemy->IsCombatEffective() ||
            !Company->OrderComponent || !Company->FireControlComponent)
        {
            continue;
        }
        const FVector Here = Company->GetActorLocation();
        const FVector There = Enemy->GetActorLocation();
        const float Distance = FVector::Dist2D(Here, There);
        // The company's own agreed fire distance: the range of its active fire policy.
        const float Range = Company->FireControlComponent->GetActiveRangeCm();
        const FVector Away = (Here - There).GetSafeNormal2D();
        const float FacingYaw = FMath::IsNearlyZero(SideOffsetCm) ? (There - Here).Rotation().Yaw : Company->GetActorRotation().Yaw;
        const FStrategyOrder Current = Company->OrderComponent->GetCurrentOrder();
        // March in column until the enemy's longest range + 35 m (design 7), then form line.
        const float DeployCm = (Enemy->FireControlComponent ? Enemy->FireControlComponent->LongRangeCm : 10000.0f) + 3500.0f;
        if (Company->FormationComponent)
        {
            const EStrategyFormationType Want = Distance > DeployCm ? EStrategyFormationType::MarchColumn : EStrategyFormationType::Line;
            if (Company->FormationComponent->CurrentFormation != Want)
            {
                Company->FormationComponent->SetFormation(Want);
                UE_LOG(LogTemp, Display, TEXT("PROJECT1864-DUEL: %s forms %s (%.0f m from the enemy)"), *Company->DisplayName.ToString(),
                    Want == EStrategyFormationType::Line ? TEXT("line") : TEXT("column"), Distance / 100.0f);
            }
        }
        if (Distance > Range * 0.95f)
        {
            // Straight ahead to the range (the company does not wheel towards an enemy off to the side).
            const FVector Ahead = FVector(Company->GetActorForwardVector().X, Company->GetActorForwardVector().Y, 0.0f).GetSafeNormal();
            const float Along = FVector::DotProduct(There - Here, Ahead);
            const FVector Goal = FMath::IsNearlyZero(SideOffsetCm) ? There + Away * Range * 0.85f
                : Here + Ahead * FMath::Max(0.0f, Along - Range * 0.85f);
            const bool bAlreadyGoing = Current.Type == EStrategyOrderType::Advance && FVector::Dist2D(Current.TargetLocation, Goal) < 1500.0f;
            if (!bAlreadyGoing)
            {
                FStrategyOrder Order;
                Order.Type = EStrategyOrderType::Advance;
                Order.TargetLocation = Goal;
                Order.FacingYaw = FacingYaw;
                Order.bHasFacing = true;
                Order.Authority = EStrategyOrderAuthority::OfficerAI;
                Company->OrderComponent->SetOrder(Order);
                UE_LOG(LogTemp, Display, TEXT("PROJECT1864-DUEL: %s advances (%.0f m away, fires at %.0f m)"), *Company->DisplayName.ToString(), Distance / 100.0f, Range / 100.0f);
            }
        }
        else if (Current.Type != EStrategyOrderType::Hold)
        {
            FStrategyOrder Hold;
            Hold.Type = EStrategyOrderType::Hold;
            Hold.TargetLocation = Here;
            Hold.FacingYaw = FacingYaw;
            Hold.bHasFacing = true;
            Hold.Authority = EStrategyOrderAuthority::OfficerAI;
            Company->OrderComponent->SetOrder(Hold);
            int32 Bearing = 0, Total = 0;
            Company->FireControlComponent->GetBearingFraction(Enemy, &Bearing, &Total);
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-DUEL: %s halts and fires (%.0f m): %d of %d men can bear"), *Company->DisplayName.ToString(), Distance / 100.0f, Bearing, Total);
        }
    }
}

void AStrategyOOBTestScenario::DrawDuelCones() const
{
    // The fire cone from the two front corners of the formation: its sides go out at the half angle from
    // each corner, and the close, medium and long ranges follow the front (straight across the front,
    // rounded at the sides). The chosen range strong, the other two faint.
    for (const AStrategyCompanyUnit* Company : DuelCompanies)
    {
        if (!IsValid(Company) || !Company->FireControlComponent || !Company->IsCombatEffective())
        {
            continue;
        }
        const UStrategyFireControlComponent* Fire = Company->FireControlComponent;
        FVector Left, Right;
        Fire->GetFireFront(Left, Right, 0);
        const FVector Lateral = (Right - Left).GetSafeNormal2D();
        const FVector Forward(Lateral.Y, -Lateral.X, 0.0f);
        const FVector Lift(0.0f, 0.0f, 25.0f);
        Left += Lift;
        Right += Lift;
        const bool bDanish = Company->Side == EStrategySide::Denmark;
        const FColor Strong = bDanish ? FColor(40, 110, 255) : FColor(255, 60, 40);
        const FColor Faint = bDanish ? FColor(30, 55, 110) : FColor(110, 40, 30);
        const float Half = FMath::DegreesToRadians(Fire->FireConeHalfAngleDegrees);
        const float Active = Fire->GetActiveRangeCm();
        auto Dir = [&](float Angle) { return Forward * FMath::Cos(Angle) + Lateral * FMath::Sin(Angle); };   // Angle < 0: to the left
        for (const float Range : { Fire->CloseRangeCm, Fire->MediumRangeCm, Fire->LongRangeCm })
        {
            const bool bActive = FMath::IsNearlyEqual(Range, Active, 1.0f);
            const FColor C = bActive ? Strong : Faint;
            const float Thick = bActive ? 14.0f : 3.0f;
            TArray<FVector> Line;
            for (int32 s = 0; s <= 8; ++s) { Line.Add(Left + Dir(-Half + Half * s / 8.0f) * Range); }   // left corner: out to straight ahead
            for (int32 s = 0; s <= 8; ++s) { Line.Add(Right + Dir(Half * s / 8.0f) * Range); }          // right corner: straight ahead to out
            for (int32 i = 0; i + 1 < Line.Num(); ++i)
            {
                DrawDebugLine(GetWorld(), Line[i], Line[i + 1], C, false, -1.0f, 0, Thick);
            }
        }
        DrawDebugLine(GetWorld(), Left, Left + Dir(-Half) * Fire->LongRangeCm, Strong, false, -1.0f, 0, 4.0f);
        DrawDebugLine(GetWorld(), Right, Right + Dir(Half) * Fire->LongRangeCm, Strong, false, -1.0f, 0, 4.0f);
        DrawDebugLine(GetWorld(), Left, Right, Strong, false, -1.0f, 0, 4.0f);
    }
}

void AStrategyOOBTestScenario::BuildTestOOB()
{
    ClearSpawnedUnits();

    if (bLivgardenVsSwedishTest)
    {
        // 300 m apart: both have to march before they are in range.
        AStrategyCompanyUnit* Guard = SpawnCompany(
            TEXT("DK-LIVGARDEN-C1"), TEXT("Livgarden"), 1,
            Origin + FVector(1000.0f, -7000.0f, 0.0f), nullptr,
            static_cast<uint8>(EStrategySide::Denmark));
        // -Strategy1864DuelOffset=<cm>: the Swedes that far to the side (an oblique fight: fewer men can bear).
        float SideOffset = 0.0f;
        FParse::Value(FCommandLine::Get(), TEXT("Strategy1864DuelOffset="), SideOffset);
        AStrategyCompanyUnit* Swedish = SpawnCompany(
            TEXT("SE-INFANTRY-C1"), TEXT("Svensk infanteri"), 1,
            Origin + FVector(31000.0f, -7000.0f + SideOffset, 0.0f), nullptr,
            static_cast<uint8>(EStrategySide::Enemy));
        // -Strategy1864DuelDanish=Infantry|Jager: the Danes in the line infantry's or the jæger's uniform instead.
        FString DanishModel;
        if (Guard && Guard->InfantryVisualComponent && FParse::Value(FCommandLine::Get(), TEXT("Strategy1864DuelDanish="), DanishModel))
        {
            const bool bJager = DanishModel.Equals(TEXT("Jager"), ESearchCase::IgnoreCase);
            Guard->DisplayName = FText::FromString(bJager ? TEXT("Jægere") : TEXT("Linjeinfanteri"));
            Guard->InfantryVisualComponent->SoldierMeshAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(bJager
                ? TEXT("/Game/Units/Danish/Jager1864/Mesh/SK_DK_Jager_1864.SK_DK_Jager_1864")
                : TEXT("/Game/Units/Danish/Infantry1864/Mesh/SK_DK_Infantry_1864.SK_DK_Infantry_1864")));
        }
        DuelCompanies.Reset();
        bDuelCameraPlaced = false;
        if (Swedish)
        {
            Swedish->SetActorRotation(FRotator(0.0f, 180.0f, 0.0f));
            if (Swedish->InfantryVisualComponent)
            {
                Swedish->InfantryVisualComponent->SoldierMeshAsset = TSoftObjectPtr<USkeletalMesh>(
                    FSoftObjectPath(TEXT("/Game/Units/Swedish/Infantry1864/Mesh/SK_SE_Infantry_1864.SK_SE_Infantry_1864")));
            }
        }
        for (AStrategyCompanyUnit* Company : {Guard, Swedish})
        {
            if (!Company) continue;
            Company->bPlayerControllable = false;
            Company->bOfficerAIEnabled = true;
            // The duel drives them itself (TickDuel); the autonomous AI waits for a contact the duel lacks.
            if (Company->AutonomousBattleAIComponent)
            {
                Company->AutonomousBattleAIComponent->bEnableForNonPlayerSides = false;
            }
            DuelCompanies.Add(Company);
            if (Company->InfantryVisualComponent)
            {
                Company->InfantryVisualComponent->SetEnabled(true);
            }
            ConfigureRuntimeQALabel(Company);
            if (Company->CombatComponent)
            {
                Company->CombatComponent->SetDeterministicRandomSeed(
                    QARandomSeed ^ static_cast<int32>(GetTypeHash(Company->StableUnitId)));
            }
            // The focused duel has no full OOB/contact simulation around it;
            // direct line-of-sight is sufficient for the two test companies
            // to acquire one another and exchange fire.
            if (Company->FireControlComponent)
            {
                Company->FireControlComponent->bRequireCurrentContact = false;
            }
        }
        // The colours: Dannebrog and the Swedish flag a little behind the middle of each company.
        for (AStrategyCompanyUnit* Company : DuelCompanies)
        {
            if (AStrategyColourFlag* Flag = GetWorld()->SpawnActor<AStrategyColourFlag>(AStrategyColourFlag::StaticClass(), Company->GetActorLocation(), FRotator::ZeroRotator))
            {
                Flag->Setup(Company, Company->Side == EStrategySide::Denmark ? TEXT("DK") : TEXT("SE"), FVector(-250.0f, 60.0f, 0.0f));
            }
        }
        UE_LOG(LogTemp, Display, TEXT("PROJECT1864-DUEL: Livgarden and Swedish infantry, %d units; full OOB disabled"), SpawnedUnitObjects.Num());
        return;
    }

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

        FVector Center =
            Unit->GetActorLocation() +
            FVector(0.0f, 0.0f, Extent.Z + 18.0f);

        if (const UStrategyInfantryVisualComponent* InfantryVisual =
            Unit->FindComponentByClass<UStrategyInfantryVisualComponent>())
        {
            FBox FormationBounds;
            if (InfantryVisual->GetFormationLocalBounds(FormationBounds))
            {
                FormationBounds = FormationBounds.ExpandBy(15.0f);
                Extent = FormationBounds.GetExtent();
                Center = Unit->GetActorTransform().TransformPosition(FormationBounds.GetCenter());
            }
        }

        if (Unit->bSelected)
        {
            // Selection is a footprint on the ground, never a box around the soldiers.
            Center.Z = Unit->GetActorLocation().Z + 5.0f;
            const FVector Right = Unit->GetActorRightVector().GetSafeNormal2D();
            const FVector Forward = Unit->GetActorForwardVector().GetSafeNormal2D();
            const FVector Corners[] = {
                Center - Forward * Extent.X - Right * Extent.Y,
                Center + Forward * Extent.X - Right * Extent.Y,
                Center + Forward * Extent.X + Right * Extent.Y,
                Center - Forward * Extent.X + Right * Extent.Y
            };
            for (int32 Edge = 0; Edge < 4; ++Edge)
            {
                DrawDebugLine(World, Corners[Edge], Corners[(Edge + 1) % 4],
                    Color, false, 0.0f, 0, QAVisualThickness);
            }
        }

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
