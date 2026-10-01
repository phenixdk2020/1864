#include "CavalryUnit.h"
#include "Components/SkeletalMeshComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Combat/StrategyCavalryChargeComponent.h"
#include "StrategyDragoonComponent.h"
#include "../AI/StrategyCavalryScreenAIComponent.h"
#include "../Visual/StrategyHorseAnimationStateComponent.h"
#include "../Visual/StrategyMountedAnimationSyncComponent.h"
#include "../Visual/StrategyUniformAppearanceComponent.h"
#include "../Visual/StrategyHumanAnimationStateComponent.h"

ACavalryUnit::ACavalryUnit()
{
    Echelon = EStrategyEchelon::Cavalry;

    if (FormationComponent)
    {
        FormationComponent->RankCount = 4;
        FormationComponent->ColumnWidth = 4;
        FormationComponent->CurrentFormation = EStrategyFormationType::CavalryLine;
    }

    if (MovementExecutor)
    {
        MovementExecutor->MoveSpeedCmPerSecond = 900.0f;
    }

    ChargeComponent =
        CreateDefaultSubobject<UStrategyCavalryChargeComponent>(TEXT("ChargeComponent"));

    DragoonComponent =
        CreateDefaultSubobject<UStrategyDragoonComponent>(TEXT("DragoonComponent"));

    ScreenAIComponent =
        CreateDefaultSubobject<UStrategyCavalryScreenAIComponent>(TEXT("ScreenAIComponent"));

    HorseAnimationStateComponent =
        CreateDefaultSubobject<UStrategyHorseAnimationStateComponent>(TEXT("HorseAnimationStateComponent"));

    MountedAnimationSyncComponent =
        CreateDefaultSubobject<UStrategyMountedAnimationSyncComponent>(TEXT("MountedAnimationSyncComponent"));

    HorseMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HorseMesh"));
    HorseMesh->SetupAttachment(SceneRoot);

    RiderMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RiderMesh"));
    RiderMesh->SetupAttachment(HorseMesh, RiderSocketName);
}

void ACavalryUnit::BeginPlay()
{
    Super::BeginPlay();

    if (InitialStrength <= 0)
    {
        InitialStrength = 80;
    }

    if (CurrentStrength <= 0)
    {
        CurrentStrength = InitialStrength;
    }

    MaximumFireRangeCm = FMath::Min(MaximumFireRangeCm, 8000.0f);
    RefreshDebugLabel();

    if (HorseMesh && RiderMesh && HorseMesh->DoesSocketExist(RiderSocketName))
    {
        RiderMesh->AttachToComponent(
            HorseMesh,
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            RiderSocketName);
    }

    if (UniformAppearanceComponent)
    {
        UniformAppearanceComponent->TargetMeshComponentNames.Reset();
        UniformAppearanceComponent->TargetMeshComponentNames.Add(TEXT("RiderMesh"));
        UniformAppearanceComponent->ApplyAppearance();
    }

    if (HumanAnimationStateComponent)
    {
        HumanAnimationStateComponent->SetMounted(true);
    }
}


void ACavalryUnit::SetDefileMode(bool bEnable)
{
    if (!FormationComponent)
    {
        return;
    }

    if (bEnable)
    {
        if (FormationComponent->CurrentFormation != EStrategyFormationType::DefileColumn)
        {
            PreDefileFormation = FormationComponent->CurrentFormation;
            FormationComponent->SetFormation(EStrategyFormationType::DefileColumn);
        }
        return;
    }

    if (FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn)
    {
        FormationComponent->SetFormation(PreDefileFormation);
    }
}
