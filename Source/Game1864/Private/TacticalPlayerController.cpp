#include "TacticalPlayerController.h"

#include "InfantryCompany.h"
#include "TacticalCameraPawn.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"

ATacticalPlayerController::ATacticalPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void ATacticalPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
}

void ATacticalPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateCamera(DeltaTime);
	UpdateSelection();
}

void ATacticalPlayerController::UpdateCamera(float DeltaTime)
{
	ATacticalCameraPawn* CameraPawn = Cast<ATacticalCameraPawn>(GetPawn());
	if (!CameraPawn)
	{
		return;
	}

	FVector2D Pan = FVector2D::ZeroVector;
	if (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up))    { Pan.Y += 1.f; }
	if (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down))  { Pan.Y -= 1.f; }
	if (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right)) { Pan.X += 1.f; }
	if (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left))  { Pan.X -= 1.f; }

	if (bEdgePan)
	{
		float MouseX, MouseY;
		int32 SizeX, SizeY;
		GetViewportSize(SizeX, SizeY);
		if (GetMousePosition(MouseX, MouseY) && SizeX > 0 && SizeY > 0)
		{
			if (MouseX <= EdgePanMargin)         { Pan.X -= 1.f; }
			if (MouseX >= SizeX - EdgePanMargin) { Pan.X += 1.f; }
			if (MouseY <= EdgePanMargin)         { Pan.Y += 1.f; }
			if (MouseY >= SizeY - EdgePanMargin) { Pan.Y -= 1.f; }
		}
	}
	CameraPawn->Pan(Pan.ClampAxes(-1.f, 1.f), DeltaTime);

	if (WasInputKeyJustPressed(EKeys::MouseScrollUp))   { CameraPawn->Zoom(1.f); }
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown)) { CameraPawn->Zoom(-1.f); }

	if (IsInputKeyDown(EKeys::Q)) { CameraPawn->Rotate(-1.f, DeltaTime); }
	if (IsInputKeyDown(EKeys::E)) { CameraPawn->Rotate(1.f, DeltaTime); }
}

void ATacticalPlayerController::UpdateSelection()
{
	if (!WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		return;
	}

	FHitResult Hit;
	AInfantryCompany* Clicked = nullptr;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		Clicked = Cast<AInfantryCompany>(Hit.GetActor());
	}

	if (Clicked == SelectedCompany.Get())
	{
		return;
	}
	if (AInfantryCompany* Previous = SelectedCompany.Get())
	{
		Previous->SetSelected(false);
	}
	SelectedCompany = Clicked;
	if (Clicked)
	{
		Clicked->SetSelected(true);
	}
}
