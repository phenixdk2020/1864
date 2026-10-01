#include "StrategyHQUnit.h"

#include "../Formations/StrategyParentFormationPlannerComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../AI/StrategyHQFollowComponent.h"
#include "../AI/StrategyCommandZoneComponent.h"
#include "../AI/StrategyCavalryTaskingComponent.h"

AStrategyHQUnit::AStrategyHQUnit()
{
    Echelon = EStrategyEchelon::Headquarters;
    FormationPlanner = CreateDefaultSubobject<UStrategyParentFormationPlannerComponent>(TEXT("FormationPlanner"));
    HQFollowComponent = CreateDefaultSubobject<UStrategyHQFollowComponent>(TEXT("HQFollowComponent"));
    CommandZoneComponent = CreateDefaultSubobject<UStrategyCommandZoneComponent>(TEXT("CommandZoneComponent"));
    CavalryTaskingComponent = CreateDefaultSubobject<UStrategyCavalryTaskingComponent>(TEXT("CavalryTaskingComponent"));
    ApplyHQLevelDefaults();
}

void AStrategyHQUnit::BeginPlay()
{
    Super::BeginPlay();

    if (OrderComponent)
    {
        OrderComponent->OnOrderChanged.AddDynamic(
            this,
            &AStrategyHQUnit::HandleHQOrderChanged);
    }
}

void AStrategyHQUnit::ApplyHQLevelDefaults()
{
    switch (HQLevel)
    {
        case EStrategyHQLevel::Battalion:
            Echelon = EStrategyEchelon::Battalion;
            CommandInnerRadius = 32000.0f;
            CommandOuterRadius = 45000.0f;
            break;

        case EStrategyHQLevel::Regiment:
            Echelon = EStrategyEchelon::Regiment;
            CommandInnerRadius = 80000.0f;
            CommandOuterRadius = 110000.0f;
            break;

        case EStrategyHQLevel::Brigade:
            Echelon = EStrategyEchelon::Brigade;
            CommandInnerRadius = 135000.0f;
            CommandOuterRadius = 185000.0f;
            break;

        case EStrategyHQLevel::Division:
            Echelon = EStrategyEchelon::Division;
            CommandInnerRadius = 210000.0f;
            CommandOuterRadius = 285000.0f;
            break;

        default:
            break;
    }
}

void AStrategyHQUnit::HandleHQOrderChanged(const FStrategyOrder& NewOrder)
{
    if (!FormationPlanner)
    {
        return;
    }

    const bool bFormationMission =
        NewOrder.Type == EStrategyOrderType::AttackHere ||
        NewOrder.Type == EStrategyOrderType::DefendHere ||
        NewOrder.Type == EStrategyOrderType::Advance ||
        NewOrder.Type == EStrategyOrderType::Withdraw ||
        NewOrder.Type == EStrategyOrderType::Assemble;

    if (!bFormationMission)
    {
        return;
    }

    const float FacingYaw =
        NewOrder.bHasFacing
        ? NewOrder.FacingYaw
        : GetActorRotation().Yaw;

    if (HQLevel == EStrategyHQLevel::Battalion)
    {
        FormationPlanner->IssueCompanySlotsForOrder(
            NewOrder.TargetLocation,
            FacingYaw,
            NewOrder.Type,
            NewOrder.Authority);
        return;
    }

    float ChildSpacingCm = FormationPlanner->RegimentChildSpacingCm;

    switch (HQLevel)
    {
        case EStrategyHQLevel::Regiment:
            ChildSpacingCm = FormationPlanner->RegimentChildSpacingCm;
            break;

        case EStrategyHQLevel::Brigade:
            ChildSpacingCm = FormationPlanner->BrigadeChildSpacingCm;
            break;

        case EStrategyHQLevel::Division:
            ChildSpacingCm = FormationPlanner->DivisionChildSpacingCm;
            break;

        default:
            break;
    }

    FormationPlanner->IssueDirectSubordinateSlots(
        NewOrder.TargetLocation,
        FacingYaw,
        NewOrder.Type,
        NewOrder.Authority,
        ChildSpacingCm);
}
