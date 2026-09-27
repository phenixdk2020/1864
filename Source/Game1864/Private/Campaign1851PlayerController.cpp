#include "Campaign1851PlayerController.h"

#include "Campaign1851Camera.h"
#include "Campaign1851Map.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "SCampaign1851Overlay.h"

ACampaign1851PlayerController::ACampaign1851PlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
}

void ACampaign1851PlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
}

void ACampaign1851PlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Overlay.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
	}
	Overlay.Reset();
	Super::EndPlay(Reason);
}

void ACampaign1851PlayerController::TryInit()
{
	if (bInitialised)
	{
		return;
	}
	for (TActorIterator<ACampaign1851Map> It(GetWorld()); It; ++It)
	{
		Map = *It;
		break;
	}
	ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	if (!Map.IsValid() || !Map->IsReady() || !Camera)
	{
		return;
	}
	const FVector2D Half = Map->GetSizeKm() * 0.5 * ACampaign1851Map::KmToUnits;
	FVector Home = Map->Project(55.55, 10.45);
	Home.Z = 0.0;
	Camera->Init(Home, 960.f, Half);

	if (GEngine && GEngine->GameViewport)
	{
		Overlay = SNew(SCampaign1851Overlay).Map(Map).Controller(this);
		GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 10);
	}
	bInitialised = true;

	FString BuildCity;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignBuild="), BuildCity, false))
	{
		CampaignBuild(BuildCity);
	}

	FString Start;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignView="), Start, false))
	{
		TArray<FString> Parts;
		Start.ParseIntoArray(Parts, TEXT(","));
		if (Parts.Num() >= 3)
		{
			CampaignView(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]), Parts.Num() > 3 ? FCString::Atof(*Parts[3]) : 0.f);
		}
	}
}

void ACampaign1851PlayerController::CampaignBuild(const FString& CityName)
{
	if (!Map.IsValid())
	{
		return;
	}
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	for (int32 i = 0; i < Cities.Num(); ++i)
	{
		if (Cities[i].Name.Equals(CityName, ESearchCase::IgnoreCase) && Map->StartProject(i))
		{
			if (Overlay.IsValid())
			{
				Overlay->SetSelectedCity(i);
			}
			FocusPlot(i);
			return;
		}
	}
}

void ACampaign1851PlayerController::FocusPlot(int32 CityIndex)
{
	ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	if (!Map.IsValid() || !Camera || !Map->GetCities().IsValidIndex(CityIndex) || !Map->GetCities()[CityIndex].bHasPlot)
	{
		return;
	}
	// Orbit the site itself (at terrain height), close enough to watch the work, looking at the
	// front of the building across the parade ground, a little from the side.
	Camera->SetView(Map->PlotWorld(CityIndex), 7.f, Map->GetCities()[CityIndex].PlotYaw + 25.f);
	Map->UpdateMarkers(Camera->GetDistanceKm());
}

void ACampaign1851PlayerController::CampaignView(float Lat, float Lon, float DistanceKm, float Yaw)
{
	ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	if (!Map.IsValid() || !Camera)
	{
		return;
	}
	FVector Target = Map->Project(Lat, Lon);
	Target.Z = 0.0;
	Camera->SetView(Target, DistanceKm, Yaw);
	Map->UpdateMarkers(Camera->GetDistanceKm());
}

bool ACampaign1851PlayerController::ScreenGround(const FVector2D& Screen, FVector& Out) const
{
	FVector Origin, Dir;
	if (!DeprojectScreenPositionToWorld(Screen.X, Screen.Y, Origin, Dir) || Dir.Z >= -1e-4)
	{
		return false;
	}
	Out = Origin + Dir * (-Origin.Z / Dir.Z);
	return true;
}

bool ACampaign1851PlayerController::CursorGround(FVector& Out) const
{
	float X, Y;
	return GetMousePosition(X, Y) && ScreenGround(FVector2D(X, Y), Out);
}

void ACampaign1851PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	TryInit();
	ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	if (!bInitialised || !Camera)
	{
		return;
	}

	FVector2D Pan = FVector2D::ZeroVector;
	if (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up))    { Pan.Y += 1.f; }
	if (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down))  { Pan.Y -= 1.f; }
	if (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right)) { Pan.X += 1.f; }
	if (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left))  { Pan.X -= 1.f; }
	Camera->Pan(Pan, DeltaTime);

	// Drag pan: keep the ground point under the cursor.
	float MX = 0.f, MY = 0.f;
	GetMousePosition(MX, MY);
	const FVector2D Mouse(MX, MY);
	const bool bDragging = IsInputKeyDown(EKeys::RightMouseButton) || IsInputKeyDown(EKeys::MiddleMouseButton);
	if (bDragging && !(WasInputKeyJustPressed(EKeys::RightMouseButton) || WasInputKeyJustPressed(EKeys::MiddleMouseButton)))
	{
		FVector A, B;
		if (ScreenGround(LastMouse, A) && ScreenGround(Mouse, B))
		{
			Camera->PanByWorld(A - B);
		}
	}
	LastMouse = Mouse;

	FVector Focus;
	const bool bFocus = CursorGround(Focus);
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp))   { Camera->Zoom(1.f, bFocus ? &Focus : nullptr); }
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown)) { Camera->Zoom(-1.f, bFocus ? &Focus : nullptr); }
	if (IsInputKeyDown(EKeys::Q)) { Camera->Rotate(-1.f, DeltaTime); }
	if (IsInputKeyDown(EKeys::E)) { Camera->Rotate(1.f, DeltaTime); }
	if (WasInputKeyJustPressed(EKeys::Home)) { Camera->ResetView(); }

	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		int32 Module = INDEX_NONE;
		const SCampaign1851Overlay::EButton Button = Overlay.IsValid() ? Overlay->HitButton(Mouse, &Module) : SCampaign1851Overlay::EButton::None;
		if (Button == SCampaign1851Overlay::EButton::BuildModule)
		{
			Map->StartModule(Overlay->GetSelectedCity(), Module);
			FocusPlot(Overlay->GetSelectedCity());
		}
		else if (Button == SCampaign1851Overlay::EButton::Build)
		{
			Map->StartProject(Overlay->GetSelectedCity());
			FocusPlot(Overlay->GetSelectedCity());
		}
		else if (Button == SCampaign1851Overlay::EButton::ShowOnMap)
		{
			FocusPlot(Overlay->GetSelectedCity());
		}
		else
		{
			PickCity();
		}
	}

	Map->UpdateMarkers(Camera->GetDistanceKm());
}

void ACampaign1851PlayerController::PickCity()
{
	float MX, MY;
	if (!Overlay.IsValid() || !GetMousePosition(MX, MY))
	{
		return;
	}
	// Nearest visible marker within ~16 px of the cursor.
	int32 Best = INDEX_NONE;
	float BestDist = 16.f;
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	for (int32 i = 0; i < Cities.Num(); ++i)
	{
		FVector2D Screen;
		if (Cities[i].bBornholm || !ProjectWorldLocationToScreen(Cities[i].World, Screen, false))
		{
			continue;
		}
		const float Dist = FVector2D::Distance(Screen, FVector2D(MX, MY));
		if (Dist < BestDist)
		{
			BestDist = Dist;
			Best = i;
		}
	}
	Overlay->SetSelectedCity(Best);
}
