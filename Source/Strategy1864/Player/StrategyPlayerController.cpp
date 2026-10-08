#include "StrategyPlayerController.h"

#include "StrategyHUD.h"
#include "StrategyCameraPawn.h"
#include "StrategyGameMode.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "../Units/StrategyUnit.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyHQUnit.h"
#include "../AI/StrategyOfficerProfileComponent.h"
#include "DrawDebugHelpers.h"
#include "../Visual/StrategyCourierRider.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../AI/StrategyOfficerAIComponent.h"
#include "../AI/StrategyAIDifficultyComponent.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryProjectilePresentation.h"
#include "../Artillery/StrategyArtilleryProjectilePresentationComponent.h"

AStrategyPlayerController::AStrategyPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Default;
}

void AStrategyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    check(InputComponent);
    // Clicks work while paused too (the pause button on the screen is itself a click).
    InputComponent->BindAction(TEXT("Select"), IE_Pressed, this, &AStrategyPlayerController::SelectionPressed).bExecuteWhenPaused = true;
    InputComponent->BindAction(TEXT("Select"), IE_Released, this, &AStrategyPlayerController::SelectionReleased).bExecuteWhenPaused = true;

    FInputActionBinding& PauseBinding =
        InputComponent->BindAction(
            TEXT("PauseSimulation"),
            IE_Pressed,
            this,
            &AStrategyPlayerController::TogglePauseSimulation);
    PauseBinding.bExecuteWhenPaused = true;

    FInputActionBinding& Speed1Binding =
        InputComponent->BindAction(TEXT("Time1x"), IE_Pressed, this, &AStrategyPlayerController::SetSpeed1x);
    Speed1Binding.bExecuteWhenPaused = true;

    FInputActionBinding& Speed2Binding =
        InputComponent->BindAction(TEXT("Time2x"), IE_Pressed, this, &AStrategyPlayerController::SetSpeed2x);
    Speed2Binding.bExecuteWhenPaused = true;

    FInputActionBinding& Speed3Binding =
        InputComponent->BindAction(TEXT("Time3x"), IE_Pressed, this, &AStrategyPlayerController::SetSpeed3x);
    Speed3Binding.bExecuteWhenPaused = true;

    FInputActionBinding& ResetBinding =
        InputComponent->BindAction(TEXT("ResetQAScenario"), IE_Pressed, this, &AStrategyPlayerController::ResetQAScenario);
    ResetBinding.bExecuteWhenPaused = true;

    InputComponent->BindAction(
        TEXT("ToggleOfficerAI"),
        IE_Pressed,
        this,
        &AStrategyPlayerController::ToggleSelectedOfficerAI);

    InputComponent->BindAction(
        TEXT("AIDifficultyEasy"),
        IE_Pressed,
        this,
        &AStrategyPlayerController::SetEnemyDifficultyEasy);

    InputComponent->BindAction(
        TEXT("AIDifficultyNormal"),
        IE_Pressed,
        this,
        &AStrategyPlayerController::SetEnemyDifficultyNormal);

    InputComponent->BindAction(
        TEXT("AIDifficultyHard"),
        IE_Pressed,
        this,
        &AStrategyPlayerController::SetEnemyDifficultyHard);

    InputComponent->BindAction(
        TEXT("FollowLatestProjectile"),
        IE_Pressed,
        this,
        &AStrategyPlayerController::ToggleFollowLatestProjectile);

    InputComponent->BindAction(
        TEXT("ToggleProjectileTrajectoryDebug"),
        IE_Pressed,
        this,
        &AStrategyPlayerController::ToggleProjectileTrajectoryDebug);
}

void AStrategyPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    TickCouriers(DeltaTime);
    TickRightMouse();
    // Test (-Strategy1864TestCourier): after 25 s the Danish company farthest from the staff is ordered 200 m forward.
    static bool bTestCourier = false;
    if (!bTestCourier && GetWorld() && GetWorld()->GetTimeSeconds() > 25.0f && FParse::Param(FCommandLine::Get(), TEXT("Strategy1864TestCourier")))
    {
        bTestCourier = true;
        FVector Staff;
        AStrategyUnit* Far = nullptr;
        float FarDistance = 0.0f;
        if (FindArmyStaff(Staff))
        {
            for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
            {
                if (IsValid(*It) && It->Side == EStrategySide::Denmark && It->Echelon == EStrategyEchelon::Company && FVector::Dist2D(Staff, It->GetActorLocation()) > FarDistance)
                {
                    Far = *It;
                    FarDistance = FVector::Dist2D(Staff, It->GetActorLocation());
                }
            }
        }
        if (Far)
        {
            SelectedUnitObjects.Reset();
            SelectedUnitObjects.Add(Far);
            IssueOrderToSelection(EStrategyOrderType::Move, Far->GetActorLocation() + Far->GetActorForwardVector() * 20000.0f, 0.0f, false);
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-COURIER: order to %s, %.0f m from the staff, %d riders"), *Far->DisplayName.ToString(), FarDistance / 100.0f, Couriers.Num());
        }
    }

    if (!bSelectionInputDown)
    {
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionCurrent = FVector2D(MouseX, MouseY);
        if (AStrategyHUD* HUD = GetStrategyHUD())
        {
            HUD->UpdateSelectionBox(SelectionCurrent);
        }
    }
}

void AStrategyPlayerController::SelectionPressed()
{
    // A click on the screen's panels (order of battle, command panel) stays there.
    {
        float UIX = 0.0f, UIY = 0.0f;
        AStrategyHUD* HUD = GetStrategyHUD();
        if (HUD && GetMousePosition(UIX, UIY) && HUD->HandleClick(FVector2D(UIX, UIY)))
        {
            bUIClickConsumed = true;
            return;
        }
    }

    if (bOrderPlacementPending)
    {
        FVector GroundPoint;
        if (ResolveGroundPointUnderCursor(GroundPoint))
        {
            PendingOrderTarget = GroundPoint;
            bOrderFacingDragActive = true;
        }
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return;
    }

    bSelectionInputDown = true;
    SelectionStart = FVector2D(MouseX, MouseY);
    SelectionCurrent = SelectionStart;

    if (AStrategyHUD* HUD = GetStrategyHUD())
    {
        HUD->BeginSelectionBox(SelectionStart);
    }
}

void AStrategyPlayerController::SelectionReleased()
{
    if (bUIClickConsumed)
    {
        bUIClickConsumed = false;
        return;
    }

    if (bOrderPlacementPending && bOrderFacingDragActive)
    {
        FVector FacingPoint;
        const bool bHasFacingPoint = ResolveGroundPointUnderCursor(FacingPoint);

        const FVector FlatFacingDelta(
            FacingPoint.X - PendingOrderTarget.X,
            FacingPoint.Y - PendingOrderTarget.Y,
            0.0f);

        const bool bHasFacing =
            bHasFacingPoint &&
            FlatFacingDelta.SizeSquared() >= FMath::Square(100.0f);

        const float FacingYaw =
            bHasFacing
            ? FlatFacingDelta.Rotation().Yaw
            : 0.0f;

        const EStrategyOrderType OrderType = PendingOrderType;
        const FVector Target = PendingOrderTarget;

        CancelOrderPlacement();

        IssueOrderToSelection(
            OrderType,
            Target,
            FacingYaw,
            bHasFacing);

        return;
    }

    if (!bSelectionInputDown)
    {
        return;
    }

    bSelectionInputDown = false;

    if (AStrategyHUD* HUD = GetStrategyHUD())
    {
        HUD->EndSelectionBox();
    }

    const float DragDistance = FVector2D::Distance(SelectionStart, SelectionCurrent);
    if (DragDistance >= BoxSelectionThresholdPixels)
    {
        SelectUnitsInScreenRectangle(SelectionStart, SelectionCurrent);
    }
    else
    {
        SelectSingleUnderCursor();
    }
}

void AStrategyPlayerController::ReadModifierState(bool& bOutAdd, bool& bOutRemove) const
{
    bOutAdd =
        IsInputKeyDown(EKeys::LeftShift) ||
        IsInputKeyDown(EKeys::RightShift);

    bOutRemove =
        IsInputKeyDown(EKeys::LeftControl) ||
        IsInputKeyDown(EKeys::RightControl);
}

void AStrategyPlayerController::SelectSingleUnderCursor()
{
    bool bAdd = false;
    bool bRemove = false;
    ReadModifierState(bAdd, bRemove);

    FHitResult Hit;
    AStrategyUnit* HitUnit = nullptr;
    if (GetHitResultUnderCursor(ECC_Visibility, true, Hit))
    {
        HitUnit = Cast<AStrategyUnit>(Hit.GetActor());
    }

    // Formation models are made from many instanced soldiers and do not carry
    // a collision shape.  Use a generous screen-space fallback so clicking
    // anywhere on the visible formation still selects its owning unit.
    if (!HitUnit)
    {
        FVector2D Cursor;
        if (GetMousePosition(Cursor.X, Cursor.Y))
        {
            float BestDistance = 140.0f;
            UWorld* World = GetWorld();
            if (World)
            {
                for (TActorIterator<AStrategyUnit> It(World); It; ++It)
                {
                    AStrategyUnit* Candidate = *It;
                    if (!IsValid(Candidate) || !Candidate->bPlayerControllable)
                    {
                        continue;
                    }

                    FVector2D ScreenPoint;
                    if (!ProjectWorldLocationToScreen(
                        Candidate->GetActorLocation(), ScreenPoint, false))
                    {
                        continue;
                    }

                    const float Distance = FVector2D::Distance(Cursor, ScreenPoint);
                    if (Distance < BestDistance)
                    {
                        BestDistance = Distance;
                        HitUnit = Candidate;
                    }
                }
            }
        }
    }

    if (!bAdd && !bRemove)
    {
        ClearSelection();
    }

    if (HitUnit && HitUnit->bPlayerControllable)
    {
        ApplySelection(HitUnit, bAdd, bRemove);
    }
}

void AStrategyPlayerController::SelectUnitsInScreenRectangle(const FVector2D& Start, const FVector2D& End)
{
    bool bAdd = false;
    bool bRemove = false;
    ReadModifierState(bAdd, bRemove);

    if (!bAdd && !bRemove)
    {
        ClearSelection();
    }

    const float Left = FMath::Min(Start.X, End.X);
    const float Top = FMath::Min(Start.Y, End.Y);
    const float Right = FMath::Max(Start.X, End.X);
    const float Bottom = FMath::Max(Start.Y, End.Y);

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<AStrategyUnit> It(World); It; ++It)
    {
        AStrategyUnit* Unit = *It;
        if (!IsValid(Unit) || !Unit->bPlayerControllable)
        {
            continue;
        }

        FVector2D ScreenPoint;
        if (!ProjectWorldLocationToScreen(Unit->GetActorLocation(), ScreenPoint, false))
        {
            continue;
        }

        const bool bInside =
            ScreenPoint.X >= Left &&
            ScreenPoint.X <= Right &&
            ScreenPoint.Y >= Top &&
            ScreenPoint.Y <= Bottom;

        if (bInside)
        {
            ApplySelection(Unit, bAdd, bRemove);
        }
    }
}

void AStrategyPlayerController::ApplySelection(AStrategyUnit* Unit, bool bAdd, bool bRemove)
{
    if (!IsValid(Unit))
    {
        return;
    }

    if (bRemove)
    {
        if (SelectedUnitObjects.Remove(Unit) > 0)
        {
            Unit->SetSelected(false);
        }
        return;
    }

    if (!SelectedUnitObjects.Contains(Unit))
    {
        SelectedUnitObjects.Add(Unit);
        Unit->SetSelected(true);
    }
}

void AStrategyPlayerController::ClearSelection()
{
    for (AStrategyUnit* Unit : SelectedUnitObjects)
    {
        if (IsValid(Unit))
        {
            Unit->SetSelected(false);
        }
    }

    SelectedUnitObjects.Reset();
}

TArray<AStrategyUnit*> AStrategyPlayerController::GetSelectedUnits() const
{
    TArray<AStrategyUnit*> Result;
    Result.Reserve(SelectedUnitObjects.Num());

    for (AStrategyUnit* Unit : SelectedUnitObjects)
    {
        if (IsValid(Unit))
        {
            Result.Add(Unit);
        }
    }

    return Result;
}

AStrategyHUD* AStrategyPlayerController::GetStrategyHUD() const
{
    return Cast<AStrategyHUD>(GetHUD());
}


void AStrategyPlayerController::BeginOrderPlacement(EStrategyOrderType OrderType)
{
    if (OrderType == EStrategyOrderType::None || SelectedUnitObjects.Num() == 0)
    {
        return;
    }

    if (OrderType == EStrategyOrderType::Hold)
    {
        IssueHoldToSelection();
        return;
    }

    PendingOrderType = OrderType;
    bOrderPlacementPending = true;
}

void AStrategyPlayerController::CancelOrderPlacement()
{
    PendingOrderType = EStrategyOrderType::None;
    PendingOrderTarget = FVector::ZeroVector;
    bOrderFacingDragActive = false;
    bOrderPlacementPending = false;
}

bool AStrategyPlayerController::CommitPendingOrderUnderCursor()
{
    if (!bOrderPlacementPending || PendingOrderType == EStrategyOrderType::None)
    {
        return false;
    }

    FVector GroundPoint;
    if (!ResolveGroundPointUnderCursor(GroundPoint))
    {
        return false;
    }

    const EStrategyOrderType OrderType = PendingOrderType;
    CancelOrderPlacement();

    return IssueOrderToSelection(
        OrderType,
        GroundPoint,
        0.0f,
        false);
}

bool AStrategyPlayerController::IssueOrderToSelection(
    EStrategyOrderType OrderType,
    const FVector& TargetLocation,
    float FacingYaw,
    bool bHasFacing,
    float SpreadCm)
{
    if (OrderType == EStrategyOrderType::None)
    {
        return false;
    }

    bool bIssuedAny = false;
    FVector Staff = FVector::ZeroVector;
    const bool bStaff = AreCouriersOn() && FindArmyStaff(Staff);

    // Several units to one place: side by side across the front they are to face (or the way they come).
    FVector Across = FVector::ZeroVector;
    if (SpreadCm > 0.0f && SelectedUnitObjects.Num() > 1)
    {
        FVector Centre = FVector::ZeroVector;
        int32 Count = 0;
        for (const AStrategyUnit* U : SelectedUnitObjects) { if (IsValid(U)) { Centre += U->GetActorLocation(); ++Count; } }
        FVector Front = bHasFacing ? FRotator(0.0f, FacingYaw, 0.0f).Vector() : (TargetLocation - Centre / FMath::Max(1, Count));
        Front.Z = 0.0f;
        Front = Front.GetSafeNormal();
        Across = FVector(-Front.Y, Front.X, 0.0f);
    }
    int32 UnitNumber = 0;
    const int32 UnitTotal = SelectedUnitObjects.Num();
    for (AStrategyUnit* Unit : SelectedUnitObjects)
    {
        if (!IsValid(Unit) || !Unit->OrderComponent)
        {
            continue;
        }

        FStrategyOrder Order;
        Order.Type = OrderType;
        Order.TargetLocation = TargetLocation + Across * SpreadCm * (float(UnitNumber++) - 0.5f * float(UnitTotal - 1));
        Order.FacingYaw = FacingYaw;
        Order.bHasFacing = bHasFacing;
        Order.Authority = EStrategyOrderAuthority::DirectPlayer;

        // Far from the army's staff the order goes by a rider (within the staff's own circle it is called out).
        const float Distance = FVector::Dist2D(Staff, Unit->GetActorLocation());
        const bool bIsStaff = Unit->Echelon == EStrategyEchelon::Headquarters && (!Unit->CommandComponent || !IsValid(Unit->CommandComponent->CurrentCommandParent));
        if (bStaff && !bIsStaff && Distance > 32000.0f)
        {
            for (const FCourier& Old : Couriers)
            {
                if (Old.Unit.Get() == Unit && Old.Horseman.IsValid()) { Old.Horseman->Dismiss(); }
            }
            Couriers.RemoveAll([Unit](const FCourier& C) { return C.Unit.Get() == Unit; });
            FCourier Rider;
            if (AStrategyCourierRider* Horseman = GetWorld()->SpawnActor<AStrategyCourierRider>(AStrategyCourierRider::StaticClass(), Staff, FRotator::ZeroRotator))
            {
                Horseman->Send(Staff, Unit);
                Rider.Horseman = Horseman;
            }
            Rider.Unit = Unit;
            Rider.Order = Order;
            Rider.From = Staff;
            Rider.Travel = 2.0f + Distance / 1200.0f;   // a rider at a hard trot over open ground: about 43 km/h
            Couriers.Add(Rider);
            bIssuedAny = true;
            continue;
        }

        // An order given here and now replaces one still on its way to the same unit.
        for (const FCourier& Old : Couriers)
        {
            if (Old.Unit.Get() == Unit && Old.Horseman.IsValid()) { Old.Horseman->Dismiss(); }
        }
        Couriers.RemoveAll([Unit](const FCourier& C) { return C.Unit.Get() == Unit; });

        if (Unit->OrderComponent->SetOrder(Order))
        {
            bIssuedAny = true;
        }
    }

    return bIssuedAny;
}

void AStrategyPlayerController::IssueHoldToSelection()
{
    IssueOrderToSelection(
        EStrategyOrderType::Hold,
        FVector::ZeroVector,
        0.0f,
        false);

    CancelOrderPlacement();
}

bool AStrategyPlayerController::ResolveGroundPointUnderCursor(FVector& OutWorldPoint) const
{
    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit))
    {
        return false;
    }

    OutWorldPoint = Hit.ImpactPoint;
    return true;
}


void AStrategyPlayerController::SelectUnitFromOOB(
    AStrategyUnit* Unit,
    bool bFocusCamera)
{
    if (!IsValid(Unit) || !Unit->bPlayerControllable)
    {
        return;
    }

    ClearSelection();
    SelectedUnitObjects.Add(Unit);
    Unit->SetSelected(true);

    if (bFocusCamera)
    {
        if (AStrategyCameraPawn* CameraPawn =
            Cast<AStrategyCameraPawn>(GetPawn()))
        {
            // A double click in the order of battle: the camera behind the unit, looking the way it faces.
            CameraPawn->SetActorRotation(FRotator(0.0f, Unit->GetActorRotation().Yaw, 0.0f));
            CameraPawn->ApplyPreset(4500.0f, -28.0f);
        }
    }
}


void AStrategyPlayerController::TogglePauseSimulation()
{
    const bool bNewPaused = !UGameplayStatics::IsGamePaused(this);
    UGameplayStatics::SetGamePaused(this, bNewPaused);
}

void AStrategyPlayerController::SetSimulationSpeed(float NewSpeed)
{
    SimulationSpeed = FMath::Clamp(NewSpeed, 0.5f, 5.0f);
    UGameplayStatics::SetGlobalTimeDilation(this, SimulationSpeed);

    if (UGameplayStatics::IsGamePaused(this))
    {
        UGameplayStatics::SetGamePaused(this, false);
    }
}

void AStrategyPlayerController::SetSpeed1x()
{
    SetSimulationSpeed(1.0f);
}

void AStrategyPlayerController::SetSpeed2x()
{
    SetSimulationSpeed(2.0f);
}

void AStrategyPlayerController::SetSpeed3x()
{
    SetSimulationSpeed(3.0f);
}


void AStrategyPlayerController::ResetQAScenario()
{
    ClearSelection();

    if (AStrategyGameMode* StrategyGameMode =
        GetWorld() ? GetWorld()->GetAuthGameMode<AStrategyGameMode>() : nullptr)
    {
        StrategyGameMode->ResetQAScenario();
    }
}


bool AStrategyPlayerController::RequestTacticalAttachment(
    AStrategyUnit* Unit,
    AStrategyUnit* NewParent)
{
    ACavalryUnit* Cavalry = Cast<ACavalryUnit>(Unit);
    AStrategyHQUnit* ParentHQ = Cast<AStrategyHQUnit>(NewParent);

    if (!IsValid(Cavalry) ||
        !IsValid(ParentHQ) ||
        Unit == NewParent ||
        Cavalry->Side != ParentHQ->Side ||
        !Cavalry->CommandComponent ||
        !Cavalry->OrderComponent)
    {
        return false;
    }

    const FStrategyOrder CurrentOrder =
        Cavalry->OrderComponent->GetCurrentOrder();

    if (CurrentOrder.IsValidOrder() &&
        CurrentOrder.Authority == EStrategyOrderAuthority::DirectPlayer &&
        Cavalry->OrderComponent->IsPhysicallyExecuting())
    {
        return false;
    }

    Cavalry->CommandComponent->SetCurrentCommandParent(ParentHQ);
    return Cavalry->CommandComponent->CurrentCommandParent == ParentHQ;
}

bool AStrategyPlayerController::RestoreOrganicAttachment(
    AStrategyUnit* Unit)
{
    if (!IsValid(Unit) || !Unit->CommandComponent)
    {
        return false;
    }

    Unit->CommandComponent->RestoreOrganicCommandParent();

    return Unit->CommandComponent->CurrentCommandParent ==
        Unit->CommandComponent->OrganicParent;
}


void AStrategyPlayerController::ToggleSelectedOfficerAI()
{
    for (AStrategyUnit* Unit : SelectedUnitObjects)
    {
        if (!IsValid(Unit) ||
            !Unit->bPlayerControllable ||
            !Unit->OfficerAIComponent)
        {
            continue;
        }

        Unit->OfficerAIComponent->SetAIEnabled(
            !Unit->OfficerAIComponent->IsAIEnabled(),
            true);
    }
}

void AStrategyPlayerController::SetEnemyDifficultyEasy()
{
    SetEnemyDifficultyByValue(
        static_cast<uint8>(EStrategyAIDifficulty::Easy));
}

void AStrategyPlayerController::SetEnemyDifficultyNormal()
{
    SetEnemyDifficultyByValue(
        static_cast<uint8>(EStrategyAIDifficulty::Normal));
}

void AStrategyPlayerController::SetEnemyDifficultyHard()
{
    SetEnemyDifficultyByValue(
        static_cast<uint8>(EStrategyAIDifficulty::Hard));
}

void AStrategyPlayerController::SetEnemyDifficultyByValue(
    uint8 DifficultyValue)
{
    if (!GetWorld())
    {
        return;
    }

    const EStrategyAIDifficulty Difficulty =
        static_cast<EStrategyAIDifficulty>(DifficultyValue);

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        AStrategyUnit* Unit = *It;

        if (!IsValid(Unit) ||
            !Unit->AIDifficultyComponent ||
            (Unit->Side != EStrategySide::Prussia &&
             Unit->Side != EStrategySide::Austria &&
             Unit->Side != EStrategySide::Enemy))
        {
            continue;
        }

        Unit->AIDifficultyComponent->Difficulty = Difficulty;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("PROJECT1864-AI: enemy difficulty=%s"),
        *StaticEnum<EStrategyAIDifficulty>()->GetNameStringByValue(
            static_cast<int64>(Difficulty)));
}


void AStrategyPlayerController::ToggleFollowLatestProjectile()
{
    AStrategyCameraPawn* CameraPawn =
        Cast<AStrategyCameraPawn>(GetPawn());

    if (!CameraPawn || !GetWorld())
    {
        return;
    }

    if (CameraPawn->bFollowingProjectile)
    {
        CameraPawn->StopProjectileFollow(true);
        return;
    }

    AStrategyArtilleryProjectilePresentation* Best = nullptr;
    int32 BestSerial = TNumericLimits<int32>::Lowest();

    for (TActorIterator<AStrategyArtilleryProjectilePresentation> It(GetWorld()); It; ++It)
    {
        AStrategyArtilleryProjectilePresentation* Projectile = *It;

        if (!IsValid(Projectile) ||
            !Projectile->IsFollowable() ||
            Projectile->bImpacted)
        {
            continue;
        }

        if (Projectile->Spec.ShotSerial > BestSerial)
        {
            BestSerial = Projectile->Spec.ShotSerial;
            Best = Projectile;
        }
    }

    if (Best)
    {
        CameraPawn->BeginProjectileFollow(Best);
    }
}

void AStrategyPlayerController::ToggleProjectileTrajectoryDebug()
{
    if (!GetWorld())
    {
        return;
    }

    bool bAnyEnabled = false;

    for (TActorIterator<AStrategyArtilleryBatteryUnit> It(GetWorld()); It; ++It)
    {
        AStrategyArtilleryBatteryUnit* Battery = *It;

        if (IsValid(Battery) &&
            Battery->ProjectilePresentationComponent &&
            Battery->ProjectilePresentationComponent->IsDebugTrajectoryEnabled())
        {
            bAnyEnabled = true;
            break;
        }
    }

    const bool bNewEnabled = !bAnyEnabled;

    for (TActorIterator<AStrategyArtilleryBatteryUnit> It(GetWorld()); It; ++It)
    {
        AStrategyArtilleryBatteryUnit* Battery = *It;

        if (IsValid(Battery) &&
            Battery->ProjectilePresentationComponent)
        {
            Battery->ProjectilePresentationComponent
                ->SetDebugTrajectoryEnabled(bNewEnabled);
        }
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("PROJECT1864-PROJECTILE: trajectory debug=%s"),
        bNewEnabled ? TEXT("ON") : TEXT("OFF"));
}

// ------------------------------------------------------------------ couriers

namespace
{
    int32 GCouriersOn = -1;   // -1: not read from the settings yet
}

bool AStrategyPlayerController::AreCouriersOn()
{
    if (GCouriersOn < 0)
    {
        bool bOn = true;
        if (GConfig)
        {
            GConfig->GetBool(TEXT("/Script/Strategy1864.Settings"), TEXT("CouriersOn"), bOn, GGameUserSettingsIni);
        }
        GCouriersOn = bOn && !FParse::Param(FCommandLine::Get(), TEXT("Strategy1864NoCouriers")) ? 1 : 0;
    }
    return GCouriersOn == 1;
}

void AStrategyPlayerController::SetCouriersOn(bool bOn)
{
    GCouriersOn = bOn ? 1 : 0;
    if (GConfig)
    {
        GConfig->SetBool(TEXT("/Script/Strategy1864.Settings"), TEXT("CouriersOn"), bOn, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }
}

bool AStrategyPlayerController::FindArmyStaff(FVector& OutLocation) const
{
    // The army's staff: the highest Danish headquarters that has none above it (the one with the most units under it when there are several).
    const AStrategyHQUnit* Best = nullptr;
    int32 BestLevel = -1;
    for (TActorIterator<AStrategyHQUnit> It(GetWorld()); It; ++It)
    {
        const AStrategyHQUnit* Hq = *It;
        if (!IsValid(Hq) || Hq->Side != EStrategySide::Denmark || (Hq->CommandComponent && IsValid(Hq->CommandComponent->CurrentCommandParent)))
        {
            continue;
        }
        const int32 Level = static_cast<int32>(Hq->HQLevel);
        if (Level > BestLevel)
        {
            Best = Hq;
            BestLevel = Level;
        }
    }
    if (!Best)
    {
        return false;
    }
    OutLocation = Best->GetActorLocation();
    return true;
}

TArray<AStrategyPlayerController::FCourierInfo> AStrategyPlayerController::GetPendingCouriers() const
{
    TArray<FCourierInfo> Out;
    for (const FCourier& C : Couriers)
    {
        if (C.Unit.IsValid())
        {
            FCourierInfo I;
            I.Unit = C.Unit;
            I.SecondsLeft = C.Horseman.IsValid() ? C.Horseman->SecondsLeft() : FMath::Max(0.0f, C.Travel - C.Elapsed);
            Out.Add(I);
        }
    }
    return Out;
}

void AStrategyPlayerController::TickCouriers(float DeltaTime)
{
    if (Couriers.Num() == 0 || !GetWorld())
    {
        return;
    }
    for (int32 i = Couriers.Num() - 1; i >= 0; --i)
    {
        FCourier& C = Couriers[i];
        AStrategyUnit* Unit = C.Unit.Get();
        if (!IsValid(Unit))
        {
            Couriers.RemoveAt(i);
            continue;
        }
        C.Elapsed += DeltaTime;
        // The order is handed over when the rider is there (without one: when the time is up).
        const bool bHere = C.Horseman.IsValid() ? C.Horseman->HasArrived() : C.Elapsed >= C.Travel;
        if (C.Horseman.IsValid())
        {
            DrawDebugLine(GetWorld(), C.From + FVector(0.0f, 0.0f, 120.0f), C.Horseman->GetActorLocation() + FVector(0.0f, 0.0f, 150.0f), FColor(220, 190, 90), false, 0.0f, SDPG_World, 6.0f);
        }
        if (bHere)
        {
            if (C.Horseman.IsValid()) { C.Horseman->Dismiss(); }
            const FStrategyOrder Order = C.Order;
            Couriers.RemoveAt(i);
            DeliverOrder(Unit, Order);
        }
    }
}

void AStrategyPlayerController::DeliverOrder(AStrategyUnit* Unit, FStrategyOrder Order)
{
    if (!IsValid(Unit) || !Unit->OrderComponent)
    {
        return;
    }
    // The officer reads the order: a reaction time by the staff work, discipline and calm; and the one who is unsure of himself
    // and poor at the craft may take the place a little wrong (a movement, an attack, a place to hold).
    const UStrategyOfficerProfileComponent* Profile = Unit->OfficerProfileComponent;
    const float Efficiency = Profile ? Profile->GetCommandEfficiency() : 0.5f;
    const float Stability = Profile ? Profile->GetDecisionStability() : 0.5f;
    const float Skill = Profile ? Profile->TacticalSkill / 100.0f : 0.5f;
    const float Reaction = 2.0f + 12.0f * (1.0f - Efficiency);
    FString Remark;
    if (!Order.TargetLocation.IsNearlyZero() && FMath::FRand() < FMath::Clamp(0.45f * (1.0f - Stability) - 0.05f, 0.0f, 0.35f))
    {
        const FVector Here = Unit->GetActorLocation();
        const float Distance = FVector::Dist2D(Here, Order.TargetLocation);
        const float Off = Distance * (0.05f + 0.15f * (1.0f - Skill));
        const float Angle = FMath::FRand() * 2.0f * PI;
        Order.TargetLocation += FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Off;
        Remark = FString::Printf(TEXT("%s har forstået ordren løst: stedet ligger ca. %.0f m fra det ønskede"), *Unit->DisplayName.ToString(), Off / 100.0f);
    }
    float ExtraDelay = 0.0f;
    // A cautious, unaggressive officer will not attack an enemy far stronger than his own men: he takes a position where he is and says so.
    if (Profile && (Order.Type == EStrategyOrderType::AttackHere || Order.Type == EStrategyOrderType::Charge || Order.Type == EStrategyOrderType::Advance) &&
        Profile->Caution > 70.0f && Profile->Aggression < 45.0f)
    {
        float Enemy = 0.0f;
        const FVector Goal = Order.TargetLocation.IsNearlyZero() ? Unit->GetActorLocation() : Order.TargetLocation;
        for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
        {
            if (IsValid(*It) && It->Side != Unit->Side && It->Side != EStrategySide::Neutral && It->Echelon != EStrategyEchelon::Headquarters && It->Echelon != EStrategyEchelon::Supply &&
                It->IsCombatEffective() && FVector::Dist2D(It->GetActorLocation(), Goal) < 60000.0f)
            {
                Enemy += FMath::Max(0, It->CurrentStrength);
            }
        }
        if (Enemy > 1.6f * FMath::Max(1, Unit->CurrentStrength))
        {
            Order.Type = EStrategyOrderType::DefendHere;
            Order.TargetLocation = Unit->GetActorLocation();
            Remark = FString::Printf(TEXT("%s tøver: fjenden er for stærk (%.0f mand mod hans %d), han holder sin stilling i stedet"), *Unit->DisplayName.ToString(), Enemy, Unit->CurrentStrength);
        }
    }
    // A poorly disciplined officer is now and then slow to carry an order out.
    if (Profile && Profile->Discipline < 35.0f && FMath::FRand() < 0.15f)
    {
        ExtraDelay = 30.0f + FMath::FRand() * 30.0f;
        if (Remark.IsEmpty()) { Remark = FString::Printf(TEXT("%s er langsom til at følge ordren (%.0f s)"), *Unit->DisplayName.ToString(), Reaction + ExtraDelay); }
    }
    Unit->OrderComponent->QueueDelayedOrder(Order, Reaction + ExtraDelay);
    if (AStrategyHUD* Hud = Cast<AStrategyHUD>(GetHUD()))
    {
        Hud->AddNotice(Remark.IsEmpty() ? FString::Printf(TEXT("Ordren er fremme hos %s (%.0f s til den udføres)"), *Unit->DisplayName.ToString(), Reaction) : Remark);
    }
}

// ------------------------------------------------------------------ right mouse: move, and the front to end with

void AStrategyPlayerController::TickRightMouse()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    float MouseX = 0.0f, MouseY = 0.0f;
    const bool bMouse = GetMousePosition(MouseX, MouseY);
    if (!bRmbCommand && bMouse && !bOrderPlacementPending && WasInputKeyJustPressed(EKeys::RightMouseButton))
    {
        AStrategyHUD* Hud = GetStrategyHUD();
        bool bAny = false;
        for (const AStrategyUnit* U : SelectedUnitObjects) { bAny |= IsValid(U) && U->bPlayerControllable; }
        FVector Ground;
        if (bAny && !(Hud && Hud->IsOverPanel(FVector2D(MouseX, MouseY))) && ResolveGroundPointUnderCursor(Ground))
        {
            bRmbCommand = true;
            bRmbDragged = false;
            RmbStart = Ground;
            RmbStartScreen = FVector2D(MouseX, MouseY);
        }
    }
    if (!bRmbCommand)
    {
        return;
    }
    FVector Now = RmbStart;
    ResolveGroundPointUnderCursor(Now);
    if (bMouse && FVector2D::Distance(FVector2D(MouseX, MouseY), RmbStartScreen) > 14.0f)
    {
        bRmbDragged = true;
    }
    // The place, and while the button is held and the mouse moved, the arrow for the front.
    DrawDebugCircle(World, RmbStart + FVector(0.0f, 0.0f, 40.0f), 350.0f, 32, FColor(255, 215, 90), false, 0.0f, SDPG_World, 14.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
    FVector Delta = Now - RmbStart;
    Delta.Z = 0.0f;
    const bool bArrow = bRmbDragged && Delta.SizeSquared() > FMath::Square(150.0f);
    if (bArrow)
    {
        const FVector Dir = Delta.GetSafeNormal();
        const FVector End = RmbStart + Dir * FMath::Max(900.0f, Delta.Size());
        DrawDebugDirectionalArrow(World, RmbStart + FVector(0.0f, 0.0f, 60.0f), End + FVector(0.0f, 0.0f, 60.0f), 500.0f, FColor(255, 215, 90), false, 0.0f, SDPG_World, 16.0f);
    }
    if (WasInputKeyJustReleased(EKeys::RightMouseButton) || !IsInputKeyDown(EKeys::RightMouseButton))
    {
        bRmbCommand = false;
        IssueOrderToSelection(EStrategyOrderType::Move, RmbStart, bArrow ? Delta.Rotation().Yaw : 0.0f, bArrow, 7200.0f);
        bRmbDragged = false;
    }
}
