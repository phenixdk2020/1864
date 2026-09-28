#include "Campaign1851PlayerController.h"

#include "Campaign1851Camera.h"
#include "Campaign1851Map.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "SCampaign1851Overlay.h"
#include "Campaign1851ConstructionSite.h"
#include "Campaign1851SaveGame.h"
#include "Kismet/GameplayStatics.h"

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
	// The campaign survives a restart: always leave an autosave behind.
	if (bInitialised)
	{
		SaveToSlot(TEXT("Autosave"), true);
	}
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
	const bool bTestStart = FParse::Value(FCommandLine::Get(), TEXT("CampaignBuild="), BuildCity, false);
	if (!bTestStart && !FParse::Param(FCommandLine::Get(), TEXT("CampaignNew")) && UGameplayStatics::DoesSaveGameExist(TEXT("Autosave"), 0))
	{
		LoadFromSlot(TEXT("Autosave"));
	}
	if (bTestStart)
	{
		CampaignBuild(BuildCity);
	}
	// Test starts: -CampaignSpeed=0..3, -CampaignDate=1852-01-20 (e.g. to see the winter).
	int32 StartSpeed = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignSpeed="), StartSpeed))
	{
		Map->SetSpeed(StartSpeed);
	}
	FString StartDate;
	FDateTime Parsed;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignDate="), StartDate) && FDateTime::ParseIso8601(*StartDate, Parsed))
	{
		Map->SetCampaignDays((Parsed - ACampaign1851Map::StartDate()).GetTotalDays());
	}
	int32 StartAmt = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignSelectAmt="), StartAmt) && Overlay.IsValid())
	{
		Overlay->SetSelectedCity(INDEX_NONE);
		Overlay->SetSelectedAmt(StartAmt);
		Map->SetHighlightedAmt(StartAmt);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CampaignOpenLedger")) && Overlay.IsValid())
	{
		Overlay->ToggleLedger();
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CampaignOpenMenu")))
	{
		OpenGameMenu();
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

	if (WasInputKeyJustPressed(EKeys::M))
	{
		if (Overlay.IsValid() && Overlay->IsMenuOpen()) { Overlay->CloseMenu(); } else { OpenGameMenu(); }
	}
	// Calendar: the game stands still while the menu is open.
	Map->AdvanceTime(Overlay.IsValid() && Overlay->IsMenuOpen() ? 0.f : DeltaTime);
	if (WasInputKeyJustPressed(EKeys::SpaceBar))
	{
		if (Map->GetSpeed() > 0) { SpeedBeforePause = Map->GetSpeed(); Map->SetSpeed(0); } else { Map->SetSpeed(SpeedBeforePause); }
	}
	if (WasInputKeyJustPressed(EKeys::One)) { Map->SetSpeed(1); }
	if (WasInputKeyJustPressed(EKeys::Two)) { Map->SetSpeed(2); }
	if (WasInputKeyJustPressed(EKeys::Three)) { Map->SetSpeed(3); }
	if (WasInputKeyJustPressed(EKeys::F5)) { SaveToSlot(TEXT("Quicksave")); }
	if (WasInputKeyJustPressed(EKeys::F9)) { LoadFromSlot(TEXT("Quicksave")); }
	AutosaveTimer += DeltaTime;
	if (AutosaveTimer > 30.f)
	{
		AutosaveTimer = 0.f;
		SaveToSlot(TEXT("Autosave"), true);
	}

	if (WasInputKeyJustPressed(EKeys::LeftMouseButton) && Overlay.IsValid() && Overlay->IsMenuOpen())
	{
		int32 Row = INDEX_NONE;
		const SCampaign1851Overlay::EButton Button = Overlay->HitButton(Mouse, &Row);
		if (Button == SCampaign1851Overlay::EButton::SaveSlot && SaveSlots().IsValidIndex(Row))
		{
			SaveToSlot(SaveSlots()[Row]);
			OpenGameMenu();   // refresh the rows
		}
		else if (Button == SCampaign1851Overlay::EButton::LoadSlot && SaveSlots().IsValidIndex(Row))
		{
			LoadFromSlot(SaveSlots()[Row]);
			Overlay->CloseMenu();
		}
		else if (Button == SCampaign1851Overlay::EButton::NewGame)
		{
			if (Overlay->IsConfirmingNewGame())
			{
				CampaignNewGame();
			}
			else
			{
				Overlay->SetConfirmNewGame(true);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::CloseMenu)
		{
			Overlay->CloseMenu();
		}
	}
	else if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		int32 Module = INDEX_NONE;
		const SCampaign1851Overlay::EButton Button = Overlay.IsValid() ? Overlay->HitButton(Mouse, &Module) : SCampaign1851Overlay::EButton::None;
		if (Button == SCampaign1851Overlay::EButton::Menu)
		{
			OpenGameMenu();
		}
		else if (Button == SCampaign1851Overlay::EButton::Speed)
		{
			Map->SetSpeed(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::Treasury)
		{
			Overlay->ToggleLedger();
		}
		else if (Button == SCampaign1851Overlay::EButton::BuildModule)
		{
			if (Map->StartModule(Overlay->GetSelectedCity(), Module))
			{
				FocusPlot(Overlay->GetSelectedCity());
				SaveToSlot(TEXT("Autosave"), true);
			}
			else
			{
				Overlay->ShowToast(TEXT("Ikke råd til materialerne endnu"));
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::Build)
		{
			if (Map->StartProject(Overlay->GetSelectedCity()))
			{
				FocusPlot(Overlay->GetSelectedCity());
				SaveToSlot(TEXT("Autosave"), true);
			}
			else
			{
				Overlay->ShowToast(TEXT("Ikke råd til materialerne endnu"));
			}
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
	LastCameraTarget = Camera->GetTarget();
	LastCameraDistanceKm = Camera->GetDistanceKm();
	LastCameraYaw = Camera->GetYaw();
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
	// No town under the cursor: select the amt there instead (and light it up).
	FVector Ground;
	const int32 Amt = Best == INDEX_NONE && CursorGround(Ground) ? Map->AmtAtWorld(Ground) : 0;
	Overlay->SetSelectedAmt(Amt);
	Map->SetHighlightedAmt(Best != INDEX_NONE ? Map->GetCities()[Best].AmtId : Amt);
}

// ------------------------------------------------------------------ save / load

const TArray<FString>& ACampaign1851PlayerController::SaveSlots()
{
	static const TArray<FString> Slots = { TEXT("Autosave"), TEXT("Quicksave"), TEXT("Slot1"), TEXT("Slot2"), TEXT("Slot3") };
	return Slots;
}

FString ACampaign1851PlayerController::SlotLabel(const FString& Slot) const
{
	if (Slot == TEXT("Autosave")) return TEXT("Autogem");
	if (Slot == TEXT("Quicksave")) return TEXT("Hurtiggem (F5)");
	if (Slot.StartsWith(TEXT("Slot"))) return FString::Printf(TEXT("Plads %s"), *Slot.RightChop(4));
	return Slot;
}

void ACampaign1851PlayerController::CampaignSave(const FString& Slot)
{
	SaveToSlot(Slot.IsEmpty() ? TEXT("Quicksave") : Slot);
}

void ACampaign1851PlayerController::CampaignLoad(const FString& Slot)
{
	LoadFromSlot(Slot.IsEmpty() ? TEXT("Quicksave") : Slot);
}

bool ACampaign1851PlayerController::SaveToSlot(const FString& Slot, bool bQuiet)
{
	if (!Map.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|save|%s|skipped: the map is gone"), *Slot);
		return false;
	}
	const ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	UCampaign1851SaveGame* Save = Cast<UCampaign1851SaveGame>(UGameplayStatics::CreateSaveGameObject(UCampaign1851SaveGame::StaticClass()));
	Save->SavedAt = FDateTime::Now();
	Save->CameraTarget = Camera ? Camera->GetTarget() : LastCameraTarget;
	Save->CameraDistanceKm = Camera ? Camera->GetDistanceKm() : LastCameraDistanceKm;
	Save->CameraYaw = Camera ? Camera->GetYaw() : LastCameraYaw;
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	if (Overlay.IsValid() && Cities.IsValidIndex(Overlay->GetSelectedCity()))
	{
		Save->SelectedCity = Cities[Overlay->GetSelectedCity()].Name;
	}
	TArray<FString> Parts;
	for (const ACampaign1851ConstructionSite* Site : Map->GetProjects())
	{
		if (!Site || !Cities.IsValidIndex(Site->GetCityIndex()))
		{
			continue;
		}
		FCampaign1851ProjectSave& P = Save->Projects.AddDefaulted_GetRef();
		P.City = Cities[Site->GetCityIndex()].Name;
		P.ModuleDays = Site->GetModuleDaysBuilt();
		P.ActiveModule = Site->GetActiveModule();
		TArray<FString> Built;
		for (int32 m = 0; m < ACampaign1851ConstructionSite::NumModules(); ++m)
		{
			if (Site->IsModuleStarted(m))
			{
				Built.Add(ACampaign1851ConstructionSite::ModuleName(m).ToLower() + (Site->IsModuleDone(m) ? TEXT("") : TEXT(" (under bygning)")));
			}
		}
		Parts.Add(FString::Printf(TEXT("%s: %s"), *P.City, *FString::Join(Built, TEXT(", "))));
	}
	Save->CampaignDays = Map->GetCampaignDays();
	Save->Speed = Map->GetSpeed();
	Save->Treasury = Map->GetTreasury();
	Save->Ledger = Map->GetLedger();
	Save->Summary = FString::Printf(TEXT("%s  ·  %s"), *ACampaign1851Map::FormatDate(Map->GetDate(), true),
		Parts.Num() > 0 ? *FString::Join(Parts, TEXT("  ·  ")) : TEXT("ingen byggerier"));
	const bool bOk = UGameplayStatics::SaveGameToSlot(Save, Slot, 0);
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|save|%s|%s|%s"), *Slot, bOk ? TEXT("ok") : TEXT("FAILED"), *Save->Summary);
	if (!bQuiet && Overlay.IsValid())
	{
		Overlay->ShowToast(bOk ? FString::Printf(TEXT("Spillet er gemt  ·  %s"), *SlotLabel(Slot)) : TEXT("Spillet kunne ikke gemmes"));
	}
	return bOk;
}

bool ACampaign1851PlayerController::LoadFromSlot(const FString& Slot)
{
	ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	UCampaign1851SaveGame* Save = UGameplayStatics::DoesSaveGameExist(Slot, 0) ? Cast<UCampaign1851SaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0)) : nullptr;
	if (!Map.IsValid() || !Camera || !Save)
	{
		if (Overlay.IsValid())
		{
			Overlay->ShowToast(FString::Printf(TEXT("Intet gemt spil i %s"), *SlotLabel(Slot)));
		}
		return false;
	}
	Map->ClearProjects();
	// v1 saves had no calendar: they start on 1 July 1851 at normal speed.
	Map->SetCampaignDays(Save->SaveVersion >= 2 ? Save->CampaignDays : 0.0);
	Map->SetSpeed(Save->SaveVersion >= 2 ? Save->Speed : 1);
	// v1-2 saves had no treasury: they start with the opening cash.
	if (Save->SaveVersion >= 3)
	{
		Map->RestoreEconomy(Save->Treasury, Save->Ledger);
	}
	else
	{
		Map->ResetEconomy();
	}
	int32 Restored = 0;
	for (const FCampaign1851ProjectSave& P : Save->Projects)
	{
		Restored += Map->RestoreProject(P.City, P.ModuleDays, P.ActiveModule) ? 1 : 0;
	}
	Camera->SetView(Save->CameraTarget, Save->CameraDistanceKm, Save->CameraYaw);
	if (Overlay.IsValid())
	{
		Overlay->SetSelectedCity(Map->FindCity(Save->SelectedCity));
		Overlay->ShowToast(FString::Printf(TEXT("Indlæst  ·  %s  ·  gemt %s"), *SlotLabel(Slot), *Save->SavedAt.ToString(TEXT("%d-%m-%Y %H:%M"))));
	}
	Map->UpdateMarkers(Camera->GetDistanceKm());
	AutosaveTimer = 0.f;
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|load|%s|projects=%d/%d|version=%d"), *Slot, Restored, Save->Projects.Num(), Save->SaveVersion);
	return true;
}

void ACampaign1851PlayerController::CampaignNewGame()
{
	ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	if (!Map.IsValid() || !Camera)
	{
		return;
	}
	Map->ClearProjects();
	Map->SetCampaignDays(0.0);
	Map->SetSpeed(1);
	Map->ResetEconomy();
	Camera->ResetView();
	if (Overlay.IsValid())
	{
		Overlay->SetSelectedCity(INDEX_NONE);
		Overlay->CloseMenu();
		Overlay->ShowToast(TEXT("Nyt spil  ·  Danmark 1851"));
	}
	// The blank campaign replaces the autosave, so a restart does not bring the old game back.
	SaveToSlot(TEXT("Autosave"), true);
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|new game"));
}

void ACampaign1851PlayerController::OpenGameMenu()
{
	if (!Overlay.IsValid())
	{
		return;
	}
	TArray<SCampaign1851Overlay::FSlotInfo> Rows;
	for (const FString& Slot : SaveSlots())
	{
		SCampaign1851Overlay::FSlotInfo& Row = Rows.AddDefaulted_GetRef();
		Row.Label = SlotLabel(Slot);
		Row.bCanSave = Slot != TEXT("Autosave");
		const UCampaign1851SaveGame* Save = UGameplayStatics::DoesSaveGameExist(Slot, 0) ? Cast<UCampaign1851SaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0)) : nullptr;
		Row.bExists = Save != nullptr;
		// One line under the slot name, clear of the buttons.
		const FString Summary = Save && Save->Summary.Len() > 64 ? Save->Summary.Left(62) + TEXT(" …") : Save ? Save->Summary : FString();
		Row.Info = Save ? FString::Printf(TEXT("%s  ·  %s"), *Save->SavedAt.ToString(TEXT("%d-%m-%Y %H:%M")), *Summary) : TEXT("Tom");
	}
	Overlay->OpenMenu(Rows);
}
