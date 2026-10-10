#include "StrategyGameMode.h"
#include "../Audio/StrategyBattleAudio.h"

#include "StrategyCameraPawn.h"
#include "StrategyHUD.h"
#include "StrategyBattleQuality.h"
#include "StrategyPlayerController.h"
#include "../Tests/StrategyOOBTestScenario.h"
#include "../Tests/StrategyScenarioStateComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

AStrategyGameMode::AStrategyGameMode()
{
    BattleAudio = CreateDefaultSubobject<UStrategyBattleAudio>(TEXT("BattleAudio"));
    DefaultPawnClass = AStrategyCameraPawn::StaticClass();
    PlayerControllerClass = AStrategyPlayerController::StaticClass();
    HUDClass = AStrategyHUD::StaticClass();
    OOBTestScenarioClass = AStrategyOOBTestScenario::StaticClass();
    ScenarioStateComponent =
        CreateDefaultSubobject<UStrategyScenarioStateComponent>(TEXT("ScenarioStateComponent"));
}

void AStrategyGameMode::BeginPlay()
{
    Super::BeginPlay();
    Strategy1864BattleQuality::ApplyPreset(GetWorld(), Strategy1864BattleQuality::GetPreset(), false);

    if (!bSpawnOOBTestScenario || !OOBTestScenarioClass || !GetWorld())
    {
        return;
    }

    for (TActorIterator<AStrategyOOBTestScenario> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It))
        {
            SpawnedQAScenario = *It;
            break;
        }
    }

    if (!IsValid(SpawnedQAScenario))
    {
        SpawnedQAScenario = GetWorld()->SpawnActor<AStrategyOOBTestScenario>(
            OOBTestScenarioClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator);
    }

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (AStrategyCameraPawn* StrategyCamera =
            Cast<AStrategyCameraPawn>(PC->GetPawn()))
        {
            StrategyCamera->SetActorLocation(
                FVector(11000.0f, 0.0f, 1200.0f));

            StrategyCamera->FocusOnWorldLocation(
                FVector(11000.0f, 0.0f, 0.0f));

            if (StrategyCamera->SpringArm)
            {
                StrategyCamera->SpringArm->TargetArmLength = 18000.0f;
            }
        }
    }
}


void AStrategyGameMode::ResetQAScenario()
{
    if (ScenarioStateComponent)
    {
        ScenarioStateComponent->ResetOutcome();
    }

    if (IsValid(SpawnedQAScenario))
    {
        SpawnedQAScenario->ResetScenario();
    }
}
