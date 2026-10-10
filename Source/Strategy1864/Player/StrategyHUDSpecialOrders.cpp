#include "StrategyHUD.h"
#include "StrategyPlayerController.h"
#include "../Units/StrategyUnit.h"
#include "../AI/StrategyFieldOfficerComponent.h"
#include "../Combat/StrategyFieldworksComponent.h"
#include "../Combat/StrategySkirmisherComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AStrategyHUD::DrawSpecialOrders(AStrategyUnit* Unit, float X, float Y, float W)
{
    if (!Unit || Unit->Echelon != EStrategyEchelon::Company) return;
    const AStrategyPlayerController* SpecialPC = Cast<AStrategyPlayerController>(GetOwningPlayerController());
    if (!SpecialPC) return;
    bool SpecialAvailable[3] = {false, false, false};
    bool SpecialActive[3] = {false, false, false};
    for (const AStrategyUnit* SpecialUnit : SpecialPC->GetSelectedUnits())
    {
        if (!IsValid(SpecialUnit) || SpecialUnit->Echelon != EStrategyEchelon::Company || !SpecialUnit->IsCombatEffective()) continue;
        const bool SpecialStationary = !SpecialUnit->MovementExecutor || !SpecialUnit->MovementExecutor->HasMovementGoal();
        const bool SpecialSquare = SpecialUnit->FormationComponent && SpecialUnit->FormationComponent->CurrentFormation == EStrategyFormationType::Square;
        const bool SpecialCharge = SpecialUnit->FieldOfficerComponent && SpecialUnit->FieldOfficerComponent->IsCharging();
        SpecialAvailable[0] |= SpecialUnit->FieldworksComponent && SpecialStationary && !SpecialCharge;
        SpecialAvailable[1] |= SpecialUnit->SkirmisherComponent && !SpecialSquare && !SpecialCharge;
        SpecialAvailable[2] |= SpecialUnit->FieldOfficerComponent && !SpecialSquare && !SpecialCharge;
        SpecialActive[0] |= SpecialUnit->FieldworksComponent && SpecialUnit->FieldworksComponent->bBuilding;
        SpecialActive[1] |= SpecialUnit->SkirmisherComponent &&
            (SpecialUnit->SkirmisherComponent->State == EStrategySkirmisherState::Deployed ||
             SpecialUnit->SkirmisherComponent->State == EStrategySkirmisherState::Deploying);
        SpecialActive[2] |= SpecialUnit->FieldOfficerComponent && SpecialUnit->FieldOfficerComponent->IsTakingFireCover();
    }
    const EAction SpecialActions[] = {EAction::BuildFieldworks, EAction::SkirmishScreen, EAction::Spread};
    const TCHAR* SpecialLabels[] = {TEXT("BYG"), TEXT("SKYTTEKAEDE"), SpecialActive[2] ? TEXT("SAML") : TEXT("SPRED")};
    const float SpecialWidths[] = {W * 0.21f, W * 0.48f, W * 0.31f - 6.f};
    float SpecialX = X;
    for (int32 SpecialIndex = 0; SpecialIndex < 3; ++SpecialIndex)
    {
        DrawButton(SpecialX, Y, SpecialWidths[SpecialIndex], 17.f, SpecialLabels[SpecialIndex],
            SpecialAvailable[SpecialIndex] || SpecialActive[SpecialIndex] ? SpecialActions[SpecialIndex] : EAction::None,
            SpecialActive[SpecialIndex] ? 0 : 1, SpecialActive[SpecialIndex], Unit);
        SpecialX += SpecialWidths[SpecialIndex] + 3.f;
    }
}

void AStrategyHUD::HandleSpecialOrder(EAction Action, bool bEnable)
{
    AStrategyPlayerController* SpecialPC = Cast<AStrategyPlayerController>(GetOwningPlayerController());
    if (!SpecialPC) return;
    SpecialPC->CancelOrderPlacement();
    for (AStrategyUnit* SpecialUnit : SpecialPC->GetSelectedUnits())
    {
        if (!IsValid(SpecialUnit) || SpecialUnit->Echelon != EStrategyEchelon::Company || !SpecialUnit->IsCombatEffective()) continue;
        const FStrategyOrder SpecialPreviousRoute = SpecialPC->GetRequestedRoute(SpecialUnit);
        bool SpecialAccepted = false;
        switch (Action)
        {
            case EAction::BuildFieldworks:
                if (SpecialUnit->FieldworksComponent)
                {
                    if (bEnable) SpecialAccepted = SpecialUnit->FieldworksComponent->BeginHastyFieldworks();
                    else { SpecialUnit->FieldworksComponent->CancelFieldworks(); SpecialAccepted = true; }
                }
                break;
            case EAction::SkirmishScreen:
                if (SpecialUnit->SkirmisherComponent)
                    SpecialAccepted = bEnable ? SpecialUnit->SkirmisherComponent->DeploySkirmishers(EStrategySkirmisherRole::Screen) :
                        SpecialUnit->SkirmisherComponent->RecallSkirmishers();
                break;
            case EAction::Spread:
                if (SpecialUnit->FieldOfficerComponent) SpecialAccepted = SpecialUnit->FieldOfficerComponent->SetManualSpread(bEnable);
                break;
            default: break;
        }
        if (SpecialAccepted && bEnable && Action != EAction::SkirmishScreen)
        {
            SpecialPC->CancelRequestedWaypointRoute(SpecialUnit, SpecialPreviousRoute.WaypointRouteId);
            if (Action == EAction::BuildFieldworks && SpecialUnit->OrderComponent)
            {
                FStrategyOrder SpecialBuildHold;
                SpecialBuildHold.Type = EStrategyOrderType::Hold;
                SpecialBuildHold.TargetLocation = SpecialUnit->GetActorLocation();
                SpecialBuildHold.Authority = EStrategyOrderAuthority::DirectPlayer;
                if (!SpecialUnit->OrderComponent->SetOrder(SpecialBuildHold))
                {
                    SpecialUnit->FieldworksComponent->CancelFieldworks();
                    SpecialAccepted = false;
                }
            }
        }
        if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugSpread")))
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-SPREAD: HUD unit=%s action=%d enable=%d accepted=%d"),
                *SpecialUnit->StableUnitId.ToString(), int32(Action), bEnable, SpecialAccepted);
    }
}

FString AStrategyHUD::SpecialOrderTag(const AStrategyUnit* Unit) const
{
    if (Unit->FieldworksComponent && Unit->FieldworksComponent->bBuilding)
        return FString::Printf(TEXT("BYGGER %.0f%%"), Unit->FieldworksComponent->GetBuildProgress() * 100.f);
    if (Unit->FieldOfficerComponent && Unit->FieldOfficerComponent->IsStandingUpFromFireCover()) return TEXT("REJSER SIG");
    if (Unit->FieldOfficerComponent && Unit->FieldOfficerComponent->IsTakingFireCover()) return TEXT("SPRED");
    return FString();
}
