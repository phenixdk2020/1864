#include "StrategyFormationTransitionComponent.h"

#include "StrategyFormationComponent.h"
#include "../Visual/StrategyEquipmentVisualComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Units/CavalryUnit.h"
#include "../AI/StrategyNCOComponent.h"

UStrategyFormationTransitionComponent::UStrategyFormationTransitionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyFormationTransitionComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    if (!OwnerUnit || !OwnerUnit->FormationComponent)
    {
        SetComponentTickEnabled(false);
        return;
    }

    OwnerUnit->FormationComponent->OnFormationChanged.AddDynamic(
        this,
        &UStrategyFormationTransitionComponent::HandleFormationChanged);
}

void UStrategyFormationTransitionComponent::HandleFormationChanged(
    EStrategyFormationType OldFormation,
    EStrategyFormationType NewFormation)
{
    if (!OwnerUnit || OldFormation == NewFormation)
    {
        return;
    }

    {
        // The unit log: which formation to which, and why (whoever asked set FormationReason).
        auto FormationText = [](EStrategyFormationType Formation) -> const TCHAR*
        {
            switch (Formation)
            {
                case EStrategyFormationType::Line: return TEXT("linje");
                case EStrategyFormationType::MarchColumn: return TEXT("kolonne");
                case EStrategyFormationType::Square: return TEXT("karré");
                case EStrategyFormationType::CavalryLine: return TEXT("rytterlinje");
                case EStrategyFormationType::CavalryColumn: return TEXT("rytterkolonne");
                case EStrategyFormationType::DefileColumn: return TEXT("defilé-kolonne");
            }
            return TEXT("?");
        };
        OwnerUnit->AddUnitLog(TEXT("Formation"), FString::Printf(TEXT("%s -> %s"), FormationText(OldFormation), FormationText(NewFormation)),
            OwnerUnit->FormationReason.IsEmpty() ? FString(TEXT("årsag ikke angivet (spillerens valg eller en intern regel)")) : OwnerUnit->FormationReason, 0.0f);
        OwnerUnit->FormationReason.Reset();
    }

    const float StrengthFactor =
        FMath::Max(0.0f, static_cast<float>(OwnerUnit->CurrentStrength) / 100.0f);

    const float NCOFormationFactor =
        OwnerUnit->NCOComponent
        ? OwnerUnit->NCOComponent->GetFormationSpeedMultiplier()
        : 1.0f;

    ReformRemainingSeconds =
        FMath::Max(
            0.1f,
            (BaseReformSeconds + StrengthFactor * SecondsPer100Men) /
            FMath::Max(0.20f, NCOFormationFactor));

    if (NewFormation == EStrategyFormationType::Square)
    {
        ReformRemainingSeconds = SquareReformSeconds * SquareFormTimeFactor;
    }
    if (OwnerUnit->EquipmentVisualComponent)
    {
        OwnerUnit->EquipmentVisualComponent->SetBayonetFixed(NewFormation == EStrategyFormationType::Square);
    }

    const bool bColumnToLine =
        (OldFormation == EStrategyFormationType::MarchColumn || OldFormation == EStrategyFormationType::CavalryColumn) &&
        (NewFormation == EStrategyFormationType::Line || NewFormation == EStrategyFormationType::CavalryLine);
    const bool bDeployWhileWalking = bColumnToLine && OwnerUnit->CombatComponent &&
        !OwnerUnit->CombatComponent->FindBestTarget(false);
    // The men gather in column first, then the march begins (the visual morph into column takes up to six seconds).
    // Cavalry does not wait: its column forms on the move.
    const bool bEnterMarchWhileWalking = NewFormation == EStrategyFormationType::CavalryColumn || OwnerUnit->IsA<ACavalryUnit>();
    if (NewFormation == EStrategyFormationType::MarchColumn && !OwnerUnit->IsA<ACavalryUnit>())
        ReformRemainingSeconds = FMath::Max(ReformRemainingSeconds, 6.0f);
    if (!bEnterMarchWhileWalking && !bDeployWhileWalking && OwnerUnit->MovementExecutor &&
        OwnerUnit->MovementExecutor->HasMovementGoal())
    {
        OwnerUnit->MovementExecutor->PauseMovementForSeconds(
            ReformRemainingSeconds);
    }

    // Formation transition owns the visible state even though the same
    // movement-pause mechanism is reused under the hood.
    OwnerUnit->SetUnitState(EStrategyUnitState::Reforming);

    SetComponentTickEnabled(true);
}

void UStrategyFormationTransitionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit || ReformRemainingSeconds <= 0.0f)
    {
        SetComponentTickEnabled(false);
        return;
    }

    ReformRemainingSeconds =
        FMath::Max(0.0f, ReformRemainingSeconds - DeltaTime);

    if (ReformRemainingSeconds <= 0.0f)
    {
        CompleteReform();
    }
}

void UStrategyFormationTransitionComponent::CompleteReform()
{
    if (!OwnerUnit || !OwnerUnit->IsCombatEffective())
    {
        SetComponentTickEnabled(false);
        return;
    }

    const bool bMissionStillMoving =
        OwnerUnit->MovementExecutor &&
        OwnerUnit->MovementExecutor->HasMovementGoal() &&
        OwnerUnit->OrderComponent &&
        OwnerUnit->OrderComponent->IsPhysicallyExecuting();

    OwnerUnit->SetUnitState(
        bMissionStillMoving
        && !OwnerUnit->MovementExecutor->IsHoldingForFire()
        ? EStrategyUnitState::Moving
        : EStrategyUnitState::Ready);

    SetComponentTickEnabled(false);
}
