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
	FString TownBuilds;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignBuildTown="), TownBuilds, false))
	{
		TArray<FString> Orders;
		TownBuilds.ParseIntoArray(Orders, TEXT(";"));
		for (const FString& Order : Orders)
		{
			FString Town, Key;
			if (Order.Split(TEXT(":"), &Town, &Key))
			{
				BuildTownBuilding(Map->FindCity(Town), Key);
			}
		}
	}
	// -CampaignBuildLink=Aalborg:Randers:bane;Odense:Nyborg:chaussee starts link works (paid like any order).
	FString LinkBuilds;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignBuildLink="), LinkBuilds, false))
	{
		TArray<FString> Orders;
		LinkBuilds.ParseIntoArray(Orders, TEXT(";"));
		for (const FString& Order : Orders)
		{
			TArray<FString> Parts;
			Order.ParseIntoArray(Parts, TEXT(":"));
			const int32 A = Parts.Num() == 3 ? Map->FindCity(Parts[0]) : INDEX_NONE, B = Parts.Num() == 3 ? Map->FindCity(Parts[1]) : INDEX_NONE;
			const int32 Link = Map->GetLinks().IndexOfByPredicate([A, B](const FCampaign1851Link& L) { return (L.A == A && L.B == B) || (L.A == B && L.B == A); });
			if (Link != INDEX_NONE)
			{
				BuildLink(Link, Parts[2].StartsWith(TEXT("b")) ? ECampaign1851LinkWork::Railway : ECampaign1851LinkWork::Chaussee);
				FocusLink(Link);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|no link for '%s'"), *Order);
			}
		}
	}
	// -CampaignMarch=B9,D2:Randers;A1:Roskilde sends columns off (and selects the last one).
	FString Marches;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignMarch="), Marches, false))
	{
		TArray<FString> Orders;
		Marches.ParseIntoArray(Orders, TEXT(";"));
		for (const FString& Order : Orders)
		{
			FString Ids, Town;
			if (!Order.Split(TEXT(":"), &Ids, &Town))
			{
				continue;
			}
			TArray<FString> IdList;
			Ids.ParseIntoArray(IdList, TEXT(","));
			TArray<int32> Column;
			for (const FString& Id : IdList)
			{
				const int32 i = Map->FindRegiment(Id);
				if (i != INDEX_NONE)
				{
					Column.Add(i);
				}
			}
			SelectRegiments(Column);
			MarchSelected(Map->FindCity(Town));
		}
	}
	FString StartCity;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignSelectCity="), StartCity) && Overlay.IsValid())
	{
		Overlay->SetSelectedCity(Map->FindCity(StartCity));
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
	if (WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		RightDownAt = Mouse;
		bRightDragged = false;
	}
	bRightDragged |= IsInputKeyDown(EKeys::RightMouseButton) && FVector2D::Distance(Mouse, RightDownAt) > 6.f;
	if (WasInputKeyJustReleased(EKeys::RightMouseButton) && !bRightDragged && Overlay.IsValid() && Overlay->GetSelectedRegiments().Num() > 0)
	{
		MarchSelected(CityUnderCursor());
	}
	if (WasInputKeyJustPressed(EKeys::Escape) && Overlay.IsValid())
	{
		Overlay->SetSelectedRegiments({});
	}

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
	const FKey SpeedKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five };
	for (int32 s = 0; s < UE_ARRAY_COUNT(SpeedKeys); ++s)
	{
		if (WasInputKeyJustPressed(SpeedKeys[s])) { Map->SetSpeed(s + 1); }
	}
	// + / - step the speed (the pause stays on the space bar).
	if (WasInputKeyJustPressed(EKeys::Add) || WasInputKeyJustPressed(EKeys::Equals)) { Map->SetSpeed(FMath::Max(1, Map->GetSpeed() + 1)); }
	if (WasInputKeyJustPressed(EKeys::Subtract) || WasInputKeyJustPressed(EKeys::Hyphen)) { Map->SetSpeed(FMath::Max(1, Map->GetSpeed() - 1)); }
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
		else if (Button == SCampaign1851Overlay::EButton::BuildTown && ACampaign1851ConstructionSite::TownBuildings().IsValidIndex(Module))
		{
			BuildTownBuilding(Overlay->GetSelectedCity(), ACampaign1851ConstructionSite::TownBuildings()[Module].Key);
		}
		else if (Button == SCampaign1851Overlay::EButton::ShowSite && ACampaign1851ConstructionSite::TownBuildings().IsValidIndex(Module))
		{
			FocusSite(Map->FindBuilding(Overlay->GetSelectedCity(), ACampaign1851ConstructionSite::TownBuildings()[Module].Key));
		}
		else if (Button == SCampaign1851Overlay::EButton::Regiment)
		{
			// Click a counter: its whole stack; shift-click adds or removes it.
			TArray<int32> Stack = StackOf(Module);
			if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
			{
				TArray<int32> Sel = Overlay->GetSelectedRegiments();
				const bool bAll = !Stack.ContainsByPredicate([&Sel](int32 i) { return !Sel.Contains(i); });
				for (int32 i : Stack)
				{
					if (bAll) { Sel.Remove(i); } else { Sel.AddUnique(i); }
				}
				Stack = Sel;
			}
			SelectRegiments(Stack);
		}
		else if (Button == SCampaign1851Overlay::EButton::RegimentRow)
		{
			TArray<int32> Sel = Overlay->GetSelectedRegiments();
			if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
			{
				Sel.Remove(Module);
			}
			else
			{
				Sel = { Module };
			}
			SelectRegiments(Sel);
		}
		else if (Button == SCampaign1851Overlay::EButton::ArmyHome)
		{
			// Each regiment home to its own garrison.
			for (int32 i : Overlay->GetSelectedRegiments())
			{
				if (Map->GetRegiments().IsValidIndex(i))
				{
					Map->OrderMarch({ i }, Map->GetRegiments()[i].Home);
				}
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::ArmyHalt)
		{
			for (int32 i : Overlay->GetSelectedRegiments())
			{
				Map->HaltRegiment(i);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::BuildLink)
		{
			BuildLink(Module / 2, Module % 2 ? ECampaign1851LinkWork::Railway : ECampaign1851LinkWork::Chaussee);
		}
		else if (Button == SCampaign1851Overlay::EButton::ShowLink)
		{
			FocusLink(Module);
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

	for (const FString& News : Map->TakeNews())
	{
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|news|%s"), *News);
		if (Overlay.IsValid())
		{
			Overlay->ShowToast(News);
		}
	}
	Map->UpdateMarkers(Camera->GetDistanceKm());
	LastCameraTarget = Camera->GetTarget();
	LastCameraDistanceKm = Camera->GetDistanceKm();
	LastCameraYaw = Camera->GetYaw();
}

int32 ACampaign1851PlayerController::CityUnderCursor() const
{
	float MX, MY;
	if (!Map.IsValid() || !GetMousePosition(MX, MY))
	{
		return INDEX_NONE;
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
	return Best;
}

void ACampaign1851PlayerController::PickCity()
{
	if (!Overlay.IsValid())
	{
		return;
	}
	const int32 Best = CityUnderCursor();
	Overlay->SetSelectedRegiments({});
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
		P.Kind = Site->GetKind();
		P.PlotKm = Site->PlotKm;
		P.Yaw = float(Site->GetActorRotation().Yaw);
		TArray<FString> Built;
		for (int32 m = 0; m < Site->NumModules(); ++m)
		{
			if (Site->IsModuleStarted(m))
			{
				Built.Add(Site->ModuleName(m).ToLower() + (Site->IsModuleDone(m) ? TEXT("") : TEXT(" (under bygning)")));
			}
		}
		Parts.Add(FString::Printf(TEXT("%s: %s"), *P.City, *FString::Join(Built, TEXT(", "))));
	}
	Save->CampaignDays = Map->GetCampaignDays();
	Save->Speed = Map->GetSpeed();
	Save->Treasury = Map->GetTreasury();
	Save->Ledger = Map->GetLedger();
	Save->Links = Map->SaveNetwork();
	Save->Regiments = Map->SaveArmy();
	if (Save->Links.Num() > 0)
	{
		Parts.Add(FString::Printf(TEXT("%d vej-/baneanlæg"), Save->Links.Num()));
	}
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
	Map->ResetNetwork();
	const int32 LinksRestored = Save->SaveVersion >= 5 ? Map->RestoreNetwork(Save->Links) : 0;
	Map->ResetArmy();
	if (Save->SaveVersion >= 6)
	{
		Map->RestoreArmy(Save->Regiments);
	}
	if (Overlay.IsValid())
	{
		Overlay->SetSelectedRegiments({});
	}
	int32 Restored = 0;
	for (const FCampaign1851ProjectSave& P : Save->Projects)
	{
		Restored += Map->RestoreProject(P.City, P.ModuleDays, P.ActiveModule, P.Kind, P.PlotKm, P.Yaw) ? 1 : 0;
	}
	Camera->SetView(Save->CameraTarget, Save->CameraDistanceKm, Save->CameraYaw);
	if (Overlay.IsValid())
	{
		Overlay->SetSelectedCity(Map->FindCity(Save->SelectedCity));
		Overlay->ShowToast(FString::Printf(TEXT("Indlæst  ·  %s  ·  gemt %s"), *SlotLabel(Slot), *Save->SavedAt.ToString(TEXT("%d-%m-%Y %H:%M"))));
	}
	Map->UpdateMarkers(Camera->GetDistanceKm());
	AutosaveTimer = 0.f;
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|load|%s|projects=%d/%d|links=%d/%d|version=%d"), *Slot, Restored, Save->Projects.Num(), LinksRestored, Save->Links.Num(), Save->SaveVersion);
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
	Map->ResetNetwork();
	Map->ResetArmy();
	if (Overlay.IsValid())
	{
		Overlay->SetSelectedRegiments({});
	}
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

void ACampaign1851PlayerController::FocusSite(const ACampaign1851ConstructionSite* Site)
{
	ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	if (!Site || !Camera)
	{
		return;
	}
	Camera->SetView(Site->GetActorLocation(), 6.5f, float(Site->GetActorRotation().Yaw) + 25.f);
	Map->UpdateMarkers(Camera->GetDistanceKm());
}

TArray<int32> ACampaign1851PlayerController::StackOf(int32 Regiment) const
{
	const TArray<FCampaign1851Regiment>& Regs = Map->GetRegiments();
	if (!Regs.IsValidIndex(Regiment))
	{
		return {};
	}
	const FCampaign1851Regiment& R = Regs[Regiment];
	if (!R.IsMarching())
	{
		return Map->RegimentsIn(R.Town);
	}
	TArray<int32> Out;
	for (int32 i = 0; i < Regs.Num(); ++i)
	{
		if (i == Regiment || (R.Group && Regs[i].Group == R.Group && Regs[i].IsMarching()))
		{
			Out.Add(i);
		}
	}
	return Out;
}

void ACampaign1851PlayerController::SelectRegiments(const TArray<int32>& Regiments)
{
	if (Overlay.IsValid())
	{
		Overlay->SetSelectedRegiments(Regiments);
		if (Regiments.Num() > 0)
		{
			Overlay->SetSelectedCity(INDEX_NONE);
			Overlay->SetSelectedAmt(0);
			Map->SetHighlightedAmt(0);
		}
	}
}

void ACampaign1851PlayerController::MarchSelected(int32 CityIndex)
{
	if (!Map.IsValid() || !Overlay.IsValid() || CityIndex == INDEX_NONE)
	{
		return;
	}
	const TArray<int32> Column = Overlay->GetSelectedRegiments();
	FString Why;
	if (Map->OrderMarch(Column, CityIndex, &Why))
	{
		const FCampaign1851Regiment& R = Map->GetRegiments()[Column[0]];
		const FDateTime Arrive = Map->GetDate() + FTimespan::FromDays(R.DaysLeft());
		Overlay->ShowToast(FString::Printf(TEXT("%s mod %s  ·  ankomst %s %s"), Column.Num() > 1 ? *FString::Printf(TEXT("%d enheder marcherer"), Column.Num()) : *FString::Printf(TEXT("%s marcherer"), *R.Name),
			*Map->GetCities()[CityIndex].Name, *ACampaign1851Map::FormatClock(Arrive), *ACampaign1851Map::FormatDate(Arrive, true)));
	}
	else if (!Why.IsEmpty())
	{
		Overlay->ShowToast(Why);
	}
}

void ACampaign1851PlayerController::BuildLink(int32 Link, ECampaign1851LinkWork Work)
{
	if (!Map.IsValid() || !Map->GetLinks().IsValidIndex(Link))
	{
		return;
	}
	FString Why;
	if (Map->StartLinkWork(Link, Work, true, &Why))
	{
		const FCampaign1851Link& L = Map->GetLinks()[Link];
		if (Overlay.IsValid())
		{
			Overlay->ShowToast(FString::Printf(TEXT("%s %s–%s bestilt"), Work == ECampaign1851LinkWork::Railway ? TEXT("Jernbane") : TEXT("Chaussé"),
				*Map->GetCities()[L.A].Name, *Map->GetCities()[L.B].Name));
		}
		SaveToSlot(TEXT("Autosave"), true);
	}
	else if (Overlay.IsValid())
	{
		Overlay->ShowToast(Why);
	}
}

void ACampaign1851PlayerController::FocusLink(int32 Link)
{
	ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	if (!Map.IsValid() || !Camera || !Map->GetLinks().IsValidIndex(Link))
	{
		return;
	}
	const FCampaign1851Link& L = Map->GetLinks()[Link];
	const TArray<FVector2D>& Line = L.Work == ECampaign1851LinkWork::Railway ? L.RailPath : L.Km;
	// Over the head of the works, close enough to see the gang and the new line.
	const double Length = ACampaign1851Map::LineLength(Line);
	const double Head = L.Work == ECampaign1851LinkWork::None ? Length * 0.5 : FMath::Max(0.3, Length * FMath::Min(1.0, L.Progress() / (L.Work == ECampaign1851LinkWork::Railway ? 0.6 : 1.0)) - 0.3);
	FVector2D Dir;
	const FVector Target = Map->WorldAtKm(ACampaign1851Map::AlongLine(Line, Head, &Dir));
	Camera->SetView(Target, L.Work == ECampaign1851LinkWork::None ? FMath::Clamp(float(Length) * 1.4f, 20.f, 120.f) : 7.f, FMath::RadiansToDegrees(FMath::Atan2(-Dir.Y, Dir.X)) + 60.f);
	Map->UpdateMarkers(Camera->GetDistanceKm());
}

void ACampaign1851PlayerController::BuildTownBuilding(int32 CityIndex, const FString& Key)
{
	if (!Map.IsValid() || CityIndex == INDEX_NONE)
	{
		return;
	}
	FString Why;
	if (const ACampaign1851ConstructionSite* Site = Map->StartBuilding(CityIndex, Key, true, nullptr, 0.f, &Why))
	{
		if (Overlay.IsValid())
		{
			Overlay->SetSelectedCity(CityIndex);
			Overlay->ShowToast(FString::Printf(TEXT("%s bestilt i %s"), *Site->ModuleName(0), *Map->GetCities()[CityIndex].Name));
		}
		FocusSite(Site);
		SaveToSlot(TEXT("Autosave"), true);
	}
	else if (Overlay.IsValid())
	{
		Overlay->ShowToast(Why);
	}
}
