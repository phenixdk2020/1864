#include "StrategyQARuntimeSubsystem.h"

#include "StrategyOOBTestScenario.h"
#include "../Player/StrategyCameraPawn.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

bool UStrategyQARuntimeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    if (!World)
    {
        return false;
    }

    const bool bGameWorld =
        World->WorldType == EWorldType::PIE ||
        World->WorldType == EWorldType::Game ||
        World->WorldType == EWorldType::GamePreview;

    // Only on the battle's own maps (Strategy1864_*): the module is also built into the campaign
    // project (PROJECT 1864 Game1864), whose campaign map must not get a QA scenario and camera.
    const FString MapName = FPackageName::GetShortName(World->GetOutermost()->GetName());
    return bGameWorld && MapName.Contains(TEXT("Strategy1864"));
}

void UStrategyQARuntimeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    AStrategyOOBTestScenario* Scenario = FindOrSpawnScenario(InWorld);
    AStrategyCameraPawn* CameraPawn = EnsureStrategyCamera(InWorld);

    if (CameraPawn)
    {
        FocusCameraOnBattlefield(CameraPawn);
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("PROJECT1864-QA: runtime bootstrap scenario=%s camera=%s"),
        Scenario ? TEXT("OK") : TEXT("FAIL"),
        CameraPawn ? TEXT("OK") : TEXT("FAIL"));
}

AStrategyOOBTestScenario*
UStrategyQARuntimeSubsystem::FindOrSpawnScenario(UWorld& World)
{
    for (TActorIterator<AStrategyOOBTestScenario> It(&World); It; ++It)
    {
        if (IsValid(*It))
        {
            return *It;
        }
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    return World.SpawnActor<AStrategyOOBTestScenario>(
        AStrategyOOBTestScenario::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
}

AStrategyCameraPawn*
UStrategyQARuntimeSubsystem::EnsureStrategyCamera(UWorld& World)
{
    APlayerController* PC = World.GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("PROJECT1864-QA: no PlayerController available for QA camera bootstrap."));
        return nullptr;
    }

    if (AStrategyCameraPawn* ExistingCamera =
        Cast<AStrategyCameraPawn>(PC->GetPawn()))
    {
        return ExistingCamera;
    }

    APawn* PreviousPawn = PC->GetPawn();

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AStrategyCameraPawn* CameraPawn =
        World.SpawnActor<AStrategyCameraPawn>(
            AStrategyCameraPawn::StaticClass(),
            FVector(11000.0f, 0.0f, 1200.0f),
            FRotator::ZeroRotator,
            SpawnParameters);

    if (!CameraPawn)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("PROJECT1864-QA: failed to spawn StrategyCameraPawn."));
        return nullptr;
    }

    PC->Possess(CameraPawn);

    if (IsValid(PreviousPawn) &&
        PreviousPawn != CameraPawn)
    {
        PreviousPawn->Destroy();
    }

    return CameraPawn;
}

void UStrategyQARuntimeSubsystem::FocusCameraOnBattlefield(
    AStrategyCameraPawn* CameraPawn) const
{
    if (!IsValid(CameraPawn))
    {
        return;
    }

    CameraPawn->SetActorLocation(
        FVector(11000.0f, 0.0f, 1400.0f));

    CameraPawn->FocusOnWorldLocation(
        FVector(11000.0f, 0.0f, 0.0f));

    if (CameraPawn->SpringArm)
    {
        CameraPawn->SpringArm->TargetArmLength = 18000.0f;
    }
}
