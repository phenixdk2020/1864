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
#include "Kismet/KismetSystemLibrary.h"

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
			// Ids:Town, Ids:@lat,lon (a point in the field), optionally :road|roadonly|direct.
			TArray<FString> Parts;
			Order.ParseIntoArray(Parts, TEXT(":"));
			if (Parts.Num() < 2)
			{
				continue;
			}
			const FString Ids = Parts[0], Town = Parts[1];
			if (Parts.Num() > 2 && Overlay.IsValid())
			{
				Overlay->SetRouteMode(Parts[2] == TEXT("direct") ? ECampaign1851RouteMode::Direct : Parts[2] == TEXT("roadonly") ? ECampaign1851RouteMode::RoadsOnly : ECampaign1851RouteMode::RoadsAndRail);
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
			if (Town.StartsWith(TEXT("@")))
			{
				FString Lat, Lon;
				Town.RightChop(1).Split(TEXT(","), &Lat, &Lon);
				MarchSelected(INDEX_NONE, Map->KmAtWorld(Map->Project(FCString::Atod(*Lat), FCString::Atod(*Lon))));
			}
			else
			{
				MarchSelected(Map->FindCity(Town), Map->TownKm(Map->FindCity(Town)));
			}
		}
	}
	// -CampaignSelectRegiments=B9,D2 selects regiments; -CampaignOpenPicker=chief|general opens the officer list.
	FString SelectIds;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignSelectRegiments="), SelectIds, false))
	{
		TArray<FString> IdList;
		SelectIds.ParseIntoArray(IdList, TEXT(","));
		TArray<int32> Sel;
		for (const FString& Id : IdList)
		{
			if (Map->FindRegiment(Id) != INDEX_NONE)
			{
				Sel.Add(Map->FindRegiment(Id));
			}
		}
		SelectRegiments(Sel);
	}
	FString PickerRole;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignOpenPicker="), PickerRole) && Overlay.IsValid())
	{
		Overlay->OpenPicker(PickerRole == TEXT("general") ? SCampaign1851Overlay::EPicker::General : SCampaign1851Overlay::EPicker::Chief);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CampaignOpenTraining")) && Overlay.IsValid())
	{
		Overlay->ToggleTrainingMenu();
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|ui|training menu open=%d selected=%d"), Overlay->IsTrainingMenuOpen() ? 1 : 0, Overlay->GetSelectedRegiments().Num());
	}
	FString WindowName;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignOpenWindow="), WindowName) && Overlay.IsValid())
	{
		Overlay->OpenWindow(WindowName == TEXT("army") ? SCampaign1851Overlay::EWindow::Army : WindowName == TEXT("officers") ? SCampaign1851Overlay::EWindow::Officers
			: WindowName == TEXT("budget") ? SCampaign1851Overlay::EWindow::Budget : WindowName == TEXT("trains") ? SCampaign1851Overlay::EWindow::Trains
			: WindowName == TEXT("chart") ? SCampaign1851Overlay::EWindow::Chart : SCampaign1851Overlay::EWindow::Towns);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CampaignTestFieldArmy")))
	{
		Map->BuildTestFieldArmy();
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CampaignOpenOOB")) && Overlay.IsValid())
	{
		Overlay->ToggleOOB();
	}
	// -CampaignSplitTo=Vejle:B7 opens the march order for the selection to a town with the listed units staying
	// behind (OPDEL) and carries it out; with -CampaignSplitShow it only opens the dialog.
	FString SplitTo;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignSplitTo="), SplitTo, false) && Overlay.IsValid())
	{
		FString Town, Stay;
		SplitTo.Split(TEXT(":"), &Town, &Stay);
		const int32 City = Map->FindCity(Town);
		OpenOrderDialog(City, Map->TownKm(City));
		TArray<FString> StayIds;
		Stay.ParseIntoArray(StayIds, TEXT(","));
		SCampaign1851Overlay::FOrderDialog& D = Overlay->EditOrder();
		for (int32 u = 0; u < D.Units.Num(); ++u)
		{
			D.Ways[u] = StayIds.Contains(Map->GetRegiments()[D.Units[u]].Id) ? 3 : D.Ways[u];
		}
		RefreshOrderDialog();
		if (!FParse::Param(FCommandLine::Get(), TEXT("CampaignSplitShow")))
		{
			ExecuteOrderDialog();
		}
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|order|split to %s: %s"), *Town, *FString::Join(D.Columns, TEXT(" | ")));
	}
	FString InspectId;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignInspectOfficer="), InspectId) && Overlay.IsValid())
	{
		Overlay->InspectOfficer(Map->GetOfficers().IndexOfByPredicate([&InspectId](const FCampaign1851Officer& O) { return O.Id == InspectId; }));
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

void ACampaign1851PlayerController::CampaignMoney(float Amount)
{
	if (Map.IsValid())
	{
		Map->AddTransaction(Amount - Map->GetTreasury(), TEXT("Testpenge"));
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
	if (WasInputKeyJustReleased(EKeys::RightMouseButton) && !bRightDragged && Overlay.IsValid() && Overlay->GetSelectedRegiments().Num() > 0
		&& Overlay->GetWindow() == SCampaign1851Overlay::EWindow::None)
	{
		// A town, or any point on the ground; Ctrl sends it straight across country whatever the setting.
		// Shift: straight away the panel's way; otherwise the order dialog with the times each way.
		const int32 Town = CityUnderCursor();
		FVector Ground;
		const bool bNow = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
		if (Town != INDEX_NONE || CursorGround(Ground))
		{
			const FVector2D Km = Town != INDEX_NONE ? Map->TownKm(Town) : Map->KmAtWorld(Ground);
			if (bNow)
			{
				MarchSelected(Town, Km);
			}
			else
			{
				OpenOrderDialog(Town, Km);
			}
		}
	}
	if (WasInputKeyJustPressed(EKeys::Escape) && Overlay.IsValid())
	{
		if (Overlay->GetWindow() != SCampaign1851Overlay::EWindow::None)
		{
			Overlay->OpenWindow(SCampaign1851Overlay::EWindow::None);
		}
		else
		{
			Overlay->SetSelectedRegiments({});
		}
	}

	FVector Focus;
	const bool bFocus = CursorGround(Focus);
	const bool bOverTree = Overlay.IsValid() && Overlay->IsOverTree(Mouse);
	const bool bOverChart = Overlay.IsValid() && Overlay->IsOverChart(Mouse);
	const bool bShift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
	if (bOverChart && WasInputKeyJustPressed(EKeys::MouseScrollUp))   { Overlay->ScrollChart(-2, bShift); }
	if (bOverChart && WasInputKeyJustPressed(EKeys::MouseScrollDown)) { Overlay->ScrollChart(2, bShift); }
	if (!bOverChart && WasInputKeyJustPressed(EKeys::MouseScrollUp))   { if (bOverTree) { Overlay->ScrollTree(-3); } else { Camera->Zoom(1.f, bFocus ? &Focus : nullptr); } }
	if (!bOverChart && WasInputKeyJustPressed(EKeys::MouseScrollDown)) { if (bOverTree) { Overlay->ScrollTree(3); } else { Camera->Zoom(-1.f, bFocus ? &Focus : nullptr); } }
	if (WasInputKeyJustPressed(EKeys::K) && Overlay.IsValid()) { Overlay->ToggleOOB(); }
	// Tree drag and drop: pressed on a row, moved a little -> dragging; released -> drop (or a click).
	if (TreePressKey != INDEX_NONE && Overlay.IsValid())
	{
		int32 Hover = INDEX_NONE;
		const SCampaign1851Overlay::EButton Under = Overlay->HitButton(Mouse, &Hover);
		bTreeDragging |= FVector2D::Distance(Mouse, TreePressAt) > 6.f;
		Overlay->SetDrag(bTreeDragging, TreePressKey, Mouse, Under == SCampaign1851Overlay::EButton::TreeRow ? Hover : INDEX_NONE);
		if (WasInputKeyJustReleased(EKeys::LeftMouseButton))
		{
			if (bTreeDragging)
			{
				TreeDrop(TreePressKey, Under == SCampaign1851Overlay::EButton::TreeRow ? Hover : INDEX_NONE);
			}
			else
			{
				TreeClick(TreePressKey);
			}
			TreePressKey = INDEX_NONE;
			bTreeDragging = false;
			Overlay->SetDrag(false, 0, Mouse, INDEX_NONE);
		}
	}
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
	const FKey SpeedKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six };
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
		else if (Button == SCampaign1851Overlay::EButton::ExitGame)
		{
			// Save first, then quit (in the editor this only ends the play session).
			SaveToSlot(TEXT("Autosave"), true);
			UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
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
		else if (Button == SCampaign1851Overlay::EButton::RouteMode)
		{
			const ECampaign1851RouteMode Modes[] = { ECampaign1851RouteMode::RoadsAndRail, ECampaign1851RouteMode::RoadsOnly, ECampaign1851RouteMode::Direct };
			Overlay->SetRouteMode(Modes[FMath::Clamp(Module, 0, 2)]);
		}
		else if (Button == SCampaign1851Overlay::EButton::TrainingProgram)
		{
			Overlay->ToggleTrainingMenu();
		}
		else if (Button == SCampaign1851Overlay::EButton::ProgramPick)
		{
			// The chosen programme for everything selected.
			for (int32 i : Overlay->GetSelectedRegiments())
			{
				Map->SetProgram(i, ECampaign1851Program(Module));
			}
			Overlay->CloseTrainingMenu();
			Overlay->ShowToast(FString::Printf(TEXT("Øvelser: %s"), Campaign1851Army::ProgramName(ECampaign1851Program(Module))));
		}
		else if (Button == SCampaign1851Overlay::EButton::OfficerChange || Button == SCampaign1851Overlay::EButton::GeneralChange)
		{
			const SCampaign1851Overlay::EPicker Want = Button == SCampaign1851Overlay::EButton::GeneralChange ? SCampaign1851Overlay::EPicker::General : SCampaign1851Overlay::EPicker::Chief;
			Overlay->OpenPicker(Overlay->GetPicker() == Want ? SCampaign1851Overlay::EPicker::None : Want);
		}
		else if (Button == SCampaign1851Overlay::EButton::OfficerInfo)
		{
			Overlay->InspectOfficer(Overlay->GetInspectedOfficer() == Module ? INDEX_NONE : Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::OfficerCardClose)
		{
			Overlay->InspectOfficer(INDEX_NONE);
		}
		else if (Button == SCampaign1851Overlay::EButton::PickerClose)
		{
			Overlay->CloseTrainingMenu();
			Overlay->OpenPicker(SCampaign1851Overlay::EPicker::None);
		}
		else if (Button == SCampaign1851Overlay::EButton::OfficerPick)
		{
			// A chief takes the (single) selected regiment; a general goes with the first regiment of the stack,
			// or takes over a general command or a formation when that is the post being filled.
			const int32 Regiment = Overlay->GetSelectedRegiments().Num() > 0 ? Overlay->GetSelectedRegiments()[0] : INDEX_NONE;
			if (Overlay->GetPicker() == SCampaign1851Overlay::EPicker::FormationGeneral || Overlay->GetPicker() == SCampaign1851Overlay::EPicker::FormationOfficer)
			{
				if (Map->AssignFormationStaff(Module, Overlay->GetPickerFormation(), Overlay->GetPickerPost()))
				{
					Overlay->ShowToast(FString::Printf(TEXT("%s: %s"), *Map->GetOfficers()[Module].Name, *Map->OfficerRole(Module)));
					Overlay->OpenPicker(SCampaign1851Overlay::EPicker::None);
					Overlay->InspectOfficer(INDEX_NONE);
				}
			}
			else if (Overlay->GetPicker() == SCampaign1851Overlay::EPicker::CommandGeneral)
			{
				if (Map->AssignCommandGeneral(Module, Overlay->GetPickerCommand()))
				{
					Overlay->ShowToast(FString::Printf(TEXT("%s har overtaget %s"), *Map->GetOfficers()[Module].Name, *Map->GetCommands()[Overlay->GetPickerCommand()].Name));
					Overlay->OpenPicker(SCampaign1851Overlay::EPicker::None);
					Overlay->InspectOfficer(INDEX_NONE);
				}
			}
			else if (Map->AssignOfficer(Module, Regiment) && Map->GetOfficers().IsValidIndex(Module))
			{
				const FCampaign1851Officer& O = Map->GetOfficers()[Module];
				Overlay->ShowToast(FString::Printf(TEXT("%s %s %s"), *O.Rank, *O.Name,
					O.bGeneral ? TEXT("har overtaget kommandoen") : *FString::Printf(TEXT("er ny chef for %s"), *Map->GetRegiments()[Regiment].Name)));
				Overlay->OpenPicker(SCampaign1851Overlay::EPicker::None);
				Overlay->InspectOfficer(INDEX_NONE);
				SaveToSlot(TEXT("Autosave"), true);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::OfficerRecruit)
		{
			const int32 New = Map->RecruitOfficer(Module == 1);
			if (New != INDEX_NONE)
			{
				const FCampaign1851Officer& O = Map->GetOfficers()[New];
				Overlay->ShowToast(FString::Printf(TEXT("%s %s er ansat og venter på en post"), *O.Rank, *O.Name));
			}
			else
			{
				Overlay->ShowToast(TEXT("Ikke råd i statskassen"));
			}
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
				Map->StopRegiment(i);
			}
			Overlay->ShowToast(TEXT("Holdt!"));
		}
		else if (Button == SCampaign1851Overlay::EButton::ArmyCancel)
		{
			for (int32 i : Overlay->GetSelectedRegiments())
			{
				Map->CancelOrder(i);
			}
			Overlay->ShowToast(TEXT("Ordren er slettet: tilbage til udgangspunktet"));
		}
		else if (Button == SCampaign1851Overlay::EButton::ClosePanel)
		{
			switch (Module)
			{
			case SCampaign1851Overlay::CloseTownTab: Overlay->SetTownTab(0); break;
			case SCampaign1851Overlay::CloseTraining: Overlay->CloseTrainingMenu(); break;
			case SCampaign1851Overlay::ClosePicker: Overlay->OpenPicker(SCampaign1851Overlay::EPicker::None); break;
			case SCampaign1851Overlay::CloseOfficerCard: Overlay->InspectOfficer(INDEX_NONE); break;
			case SCampaign1851Overlay::CloseWindow: Overlay->OpenWindow(SCampaign1851Overlay::EWindow::None); break;
			case SCampaign1851Overlay::CloseLedger: Overlay->ToggleLedger(); break;
			case SCampaign1851Overlay::CloseOOB: Overlay->ToggleOOB(); break;
			case SCampaign1851Overlay::CloseOrder: Overlay->EditOrder().bOpen = false; break;
			default:
				Overlay->SetSelectedRegiments({});
				Overlay->SetSelectedCity(INDEX_NONE);
				Overlay->SetSelectedAmt(0);
				Map->SetHighlightedAmt(0);
				break;
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::TreeRow)
		{
			TreePressKey = Module;   // click or drag: decided on release
			TreePressAt = Mouse;
			bTreeDragging = false;
		}
		else if (Button == SCampaign1851Overlay::EButton::TreeToggle)
		{
			Overlay->ToggleCollapsed(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::TreeNew)
		{
			const int32 Id = Map->CreateFormation(ECampaign1851Echelon(Module), 0);
			const int32 Index = Map->FormationIndex(Id);
			Overlay->ShowToast(FString::Printf(TEXT("%s oprettet: træk enheder hen på den"), Index != INDEX_NONE ? *Map->GetFormations()[Index].Name : TEXT("")));
		}
		else if (Button == SCampaign1851Overlay::EButton::FormationChief)
		{
			const int32 Index = Map->FormationIndex(Module);
			Overlay->OpenFormationPicker(Module, Index != INDEX_NONE && (Map->GetFormations()[Index].Echelon == ECampaign1851Echelon::Division || Map->GetFormations()[Index].Echelon == ECampaign1851Echelon::Army));
		}
		else if (Button == SCampaign1851Overlay::EButton::FormationDeputy || Button == SCampaign1851Overlay::EButton::FormationStaff)
		{
			// Staff posts are filled by officers (a division's deputy is typically its senior colonel).
			Overlay->OpenFormationPicker(Module, false, Button == SCampaign1851Overlay::EButton::FormationDeputy ? 1 : 2);
		}
		else if (Button == SCampaign1851Overlay::EButton::FormationDissolve)
		{
			Map->DissolveFormation(Module);
			Overlay->ShowToast(TEXT("Formationen er opløst"));
		}
		else if (Button == SCampaign1851Overlay::EButton::OrderAll || Button == SCampaign1851Overlay::EButton::OrderUnit)
		{
			SCampaign1851Overlay::FOrderDialog& D = Overlay->EditOrder();
			if (Button == SCampaign1851Overlay::EButton::OrderAll)
			{
				for (uint8& W : D.Ways) { W = uint8(Module); }
			}
			else if (D.Ways.IsValidIndex(Module / 4))
			{
				D.Ways[Module / 4] = uint8(Module % 4);
			}
			RefreshOrderDialog();
		}
		else if (Button == SCampaign1851Overlay::EButton::OrderExecute)
		{
			ExecuteOrderDialog();
		}
		else if (Button == SCampaign1851Overlay::EButton::OrderCancel)
		{
			Overlay->EditOrder().bOpen = false;
		}
		else if (Button == SCampaign1851Overlay::EButton::OfficerPromote)
		{
			if (Map->PromoteOfficer(Module))
			{
				const FCampaign1851Officer& O = Map->GetOfficers()[Module];
				Overlay->ShowToast(FString::Printf(TEXT("%s er forfremmet til %s%s"), *O.Name, *O.Rank, O.bGeneral && O.IsFree() ? TEXT(": ledig general, regimentet mangler en chef") : TEXT("")));
				SaveToSlot(TEXT("Autosave"), true);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::OpenOOB)
		{
			Overlay->ToggleOOB();
		}
		else if (Button == SCampaign1851Overlay::EButton::OOBCommand)
		{
			Overlay->ExpandOOB(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::CommandGeneralChange)
		{
			Overlay->OpenCommandPicker(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::TrainOrder)
		{
			Overlay->ShowToast(Map->OrderTroopTrain() ? TEXT("Togsæt bestilt i England") : TEXT("Ikke råd i statskassen"));
		}
		else if (Button == SCampaign1851Overlay::EButton::MainMenu)
		{
			const SCampaign1851Overlay::EWindow Want = SCampaign1851Overlay::EWindow(Module);
			Overlay->OpenWindow(Overlay->GetWindow() == Want ? SCampaign1851Overlay::EWindow::None : Want);
		}
		else if (Button == SCampaign1851Overlay::EButton::WindowClose)
		{
			Overlay->OpenWindow(SCampaign1851Overlay::EWindow::None);
		}
		else if (Button == SCampaign1851Overlay::EButton::TableSort)
		{
			Overlay->SetSort(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::TablePage)
		{
			Overlay->TurnPage(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::OfficerFilter)
		{
			Overlay->SetOfficerFilter(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::OfficerDismiss)
		{
			if (Map->DismissOfficer(Module))
			{
				Overlay->InspectOfficer(INDEX_NONE);
				Overlay->ShowToast(TEXT("Officeren er afskediget"));
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::TableRow)
		{
			// A row: the unit or town is picked and the camera goes there; an officer shows his card.
			const SCampaign1851Overlay::EWindow Open = Overlay->GetWindow();
			if (Open == SCampaign1851Overlay::EWindow::Officers)
			{
				Overlay->InspectOfficer(Overlay->GetInspectedOfficer() == Module ? INDEX_NONE : Module);
			}
			else if (Open == SCampaign1851Overlay::EWindow::Army && Map->GetRegiments().IsValidIndex(Module))
			{
				SelectRegiments({ Module });
				Camera->SetView(Map->RegimentWorld(Module), 30.f, Camera->GetYaw());
				Overlay->OpenWindow(SCampaign1851Overlay::EWindow::None);
			}
			else if (Open == SCampaign1851Overlay::EWindow::Towns && Map->GetCities().IsValidIndex(Module))
			{
				Overlay->SetSelectedRegiments({});
				Overlay->SetSelectedCity(Module);
				Map->SetHighlightedAmt(Map->GetCities()[Module].AmtId);
				Camera->SetView(Map->GetCities()[Module].World, 40.f, Camera->GetYaw());
				Overlay->OpenWindow(SCampaign1851Overlay::EWindow::None);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::TownTab)
		{
			Overlay->SetTownTab(Overlay->GetTownTab() == Module ? 0 : Module);
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
		else if (Overlay.IsValid() && Overlay->GetWindow() == SCampaign1851Overlay::EWindow::None)
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
	// The whole town is the target: its built-up area on screen, and at least ~16 px round the dot.
	int32 Best = INDEX_NONE;
	float BestScore = 1.f;
	const TArray<FCampaign1851City>& Cities = Map->GetCities();
	for (int32 i = 0; i < Cities.Num(); ++i)
	{
		FVector2D Screen, Edge;
		if (Cities[i].bBornholm || !ProjectWorldLocationToScreen(Cities[i].World, Screen, false))
		{
			continue;
		}
		float Radius = 16.f;
		const float RadiusUnits = ACampaign1851Map::TownRadiusKm(Cities[i].Population) * float(ACampaign1851Map::KmToUnits);
		if (ProjectWorldLocationToScreen(Cities[i].World + FVector(RadiusUnits, 0.0, 0.0), Edge, false))
		{
			Radius = FMath::Max(Radius, float(FVector2D::Distance(Screen, Edge)));
		}
		const float Score = FVector2D::Distance(Screen, FVector2D(MX, MY)) / Radius;
		if (Score < BestScore)
		{
			BestScore = Score;
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
	Save->Officers = Map->SaveOfficers();
	Save->Trains = Map->SaveTrains();
	Save->Formations = Map->SaveFormations();
	Save->TrainOrders = Map->GetTrainOrders();
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
	if (Save->SaveVersion >= 7)
	{
		Map->RestoreOfficers(Save->Officers);
	}
	if (Save->SaveVersion >= 10)
	{
		Map->RestoreTrains(Save->Trains, Save->TrainOrders);
	}
	if (Save->SaveVersion >= 11)
	{
		Map->RestoreFormations(Save->Formations);
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
	if (R.IsInField())
	{
		// Halted together in the field: everything within ~200 m.
		TArray<int32> Here;
		for (int32 i = 0; i < Regs.Num(); ++i)
		{
			if (Regs[i].IsInField() && FVector2D::Distance(Regs[i].Km, R.Km) < 0.2)
			{
				Here.Add(i);
			}
		}
		return Here;
	}
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

void ACampaign1851PlayerController::MarchSelected(int32 CityIndex, const FVector2D& TargetKm)
{
	if (!Map.IsValid() || !Overlay.IsValid())
	{
		return;
	}
	const TArray<int32> Column = Overlay->GetSelectedRegiments();
	const bool bCtrl = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
	const ECampaign1851RouteMode Mode = bCtrl ? ECampaign1851RouteMode::Direct : Overlay->GetRouteMode();
	FString Why;
	if (Map->OrderMarchTo(Column, CityIndex, TargetKm, Mode, &Why))
	{
		const FString Note = Map->TakeOrderNote();
		if (!Note.IsEmpty())
		{
			Overlay->ShowToast(Note);
			return;
		}
		const FCampaign1851Regiment& R = Map->GetRegiments()[Column[0]];
		const FDateTime Arrive = Map->GetDate() + FTimespan::FromDays(R.DaysLeft());
		Overlay->ShowToast(FString::Printf(TEXT("%s mod %s (%s)  ·  ankomst %s %s"), Column.Num() > 1 ? *FString::Printf(TEXT("%d enheder marcherer"), Column.Num()) : *FString::Printf(TEXT("%s marcherer"), *R.Name),
			*Map->DescribePlace(CityIndex, TargetKm), Campaign1851Army::RouteModeName(Mode), *ACampaign1851Map::FormatClock(Arrive), *ACampaign1851Map::FormatDate(Arrive, true)));
	}
	else if (!Why.IsEmpty())
	{
		Overlay->ShowToast(Why);
	}
}

namespace
{
	/** The dialog's ways (0 on foot, 1 by train, 2 straight across) as route modes. */
	ECampaign1851RouteMode WayMode(int32 Way)
	{
		return Way == 1 ? ECampaign1851RouteMode::RoadsAndRail : Way == 2 ? ECampaign1851RouteMode::Direct : ECampaign1851RouteMode::RoadsOnly;
	}
}

void ACampaign1851PlayerController::OpenOrderDialog(int32 CityIndex, const FVector2D& TargetKm)
{
	SCampaign1851Overlay::FOrderDialog& D = Overlay->EditOrder();
	D = SCampaign1851Overlay::FOrderDialog();
	D.bOpen = true;
	D.Town = CityIndex;
	D.Km = TargetKm;
	D.Goal = Map->DescribePlace(CityIndex, TargetKm);
	D.Units = Overlay->GetSelectedRegiments();
	const ECampaign1851RouteMode Default = Overlay->GetRouteMode();
	const uint8 Way = Default == ECampaign1851RouteMode::RoadsAndRail ? 1 : Default == ECampaign1851RouteMode::Direct ? 2 : 0;
	D.Ways.Init(Way, D.Units.Num());
	RefreshOrderDialog();
}

void ACampaign1851PlayerController::RefreshOrderDialog()
{
	SCampaign1851Overlay::FOrderDialog& D = Overlay->EditOrder();
	const FDateTime Now = Map->GetDate();
	for (int32 w = 0; w < 3; ++w)
	{
		const FCampaign1851MarchPlan P = Map->PlanColumn(D.Units, D.Town, D.Km, WayMode(w));
		D.AllTimes[w] = !P.bOk ? FString(TEXT("umuligt")) : (w == 1 && P.Trains.Num() == 0) ? FString(TEXT("ingen tog")) : ACampaign1851Map::FormatDuration(P.Days);
	}
	// One column per way chosen.
	D.Columns.Reset();
	const TCHAR* Names[] = { TEXT("Til fods"), TEXT("Med tog"), TEXT("Lige linje"), TEXT("Opdel") };
	for (int32 w = 0; w < 4; ++w)
	{
		TArray<int32> Column;
		for (int32 u = 0; u < D.Units.Num(); ++u)
		{
			if (D.Ways[u] == w)
			{
				Column.Add(D.Units[u]);
			}
		}
		if (Column.Num() == 0)
		{
			continue;
		}
		if (w == 3)
		{
			// Split off: they halt (or stay) where they are, as a column of their own.
			const FCampaign1851Regiment& R = Map->GetRegiments()[Column[0]];
			D.Columns.Add(FString::Printf(TEXT("Opdel (%d): udskilles og holder stand %s"), Column.Num(),
				R.IsMarching() ? TEXT("hvor de er nu (standser marchen)") : *FString::Printf(TEXT("ved %s"), *Map->DescribePlace(R.Town, R.Km))));
			continue;
		}
		const FCampaign1851MarchPlan P = Map->PlanColumn(Column, D.Town, D.Km, WayMode(w));
		if (!P.bOk)
		{
			D.Columns.Add(FString::Printf(TEXT("%s (%d): %s"), Names[w], Column.Num(), *P.Note));
			continue;
		}
		const FDateTime Arrive = Now + FTimespan::FromDays(P.Days);
		FString Line = FString::Printf(TEXT("%s (%d): %s  ·  ankomst %s %s"), Names[w], Column.Num(), *ACampaign1851Map::FormatDuration(P.Days),
			*ACampaign1851Map::FormatClock(Arrive), *ACampaign1851Map::FormatDate(Arrive, true));
		if (P.Trains.Num() > 0)
		{
			Line += FString::Printf(TEXT("  ·  %s%s"), *P.TrainSource, P.WaitDays > 0.01f ? *FString::Printf(TEXT(", venter %s"), *ACampaign1851Map::FormatDuration(P.WaitDays)) : TEXT(""));
		}
		else if (!P.Note.IsEmpty())
		{
			Line += FString::Printf(TEXT("  ·  %s"), *P.Note);
		}
		D.Columns.Add(Line);
	}
}

void ACampaign1851PlayerController::ExecuteOrderDialog()
{
	SCampaign1851Overlay::FOrderDialog D = Overlay->EditOrder();
	Overlay->EditOrder().bOpen = false;
	TArray<FString> Notes;
	// Those that stay halt first (a marching column splits where it is), then the others march off.
	int32 Staying = 0;
	for (int32 u = 0; u < D.Units.Num(); ++u)
	{
		if (D.Ways[u] == 3)
		{
			Map->StopRegiment(D.Units[u]);
			++Staying;
		}
	}
	if (Staying > 0)
	{
		Notes.Add(FString::Printf(TEXT("Styrken er opdelt: %d %s udskilt og holder stand"), Staying, Staying == 1 ? TEXT("enhed") : TEXT("enheder")));
		// The force is split: keep only those that march selected (the rest are a column of their own now).
		TArray<int32> Going;
		for (int32 u = 0; u < D.Units.Num(); ++u)
		{
			if (D.Ways[u] != 3)
			{
				Going.Add(D.Units[u]);
			}
		}
		SelectRegiments(Going);
	}
	for (int32 w = 0; w < 3; ++w)
	{
		TArray<int32> Column;
		for (int32 u = 0; u < D.Units.Num(); ++u)
		{
			if (D.Ways[u] == w)
			{
				Column.Add(D.Units[u]);
			}
		}
		FString Why;
		if (Column.Num() > 0 && !Map->OrderMarchTo(Column, D.Town, D.Km, WayMode(w), &Why))
		{
			Notes.Add(Why);
		}
		const FString Note = Map->TakeOrderNote();
		if (!Note.IsEmpty())
		{
			Notes.Add(Note);
		}
	}
	Overlay->ShowToast(Notes.Num() > 0 ? FString::Join(Notes, TEXT("  ·  ")) : FString::Printf(TEXT("Marcherer mod %s"), *D.Goal));
	SaveToSlot(TEXT("Autosave"), true);
}

void ACampaign1851PlayerController::TreeClick(int32 Key)
{
	using K = SCampaign1851Overlay::ETreeKind;
	const int32 Id = SCampaign1851Overlay::TreeId(Key);
	switch (SCampaign1851Overlay::TreeKind(Key))
	{
	case K::Regiment:
		if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
		{
			TArray<int32> Sel = Overlay->GetSelectedRegiments();
			if (Sel.Contains(Id)) { Sel.Remove(Id); } else { Sel.Add(Id); }
			SelectRegiments(Sel);
		}
		else
		{
			SelectRegiments({ Id });
		}
		break;
	case K::Formation:
		SelectRegiments(Map->FormationRegiments(Id));   // the whole formation, to order it as one
		break;
	case K::Company:
		SelectRegiments({ Id / 10 });   // a company moves with its battalion
		break;
	case K::NewFormation:
		break;
	default:
		Overlay->ToggleCollapsed(Key);
		break;
	}
}

void ACampaign1851PlayerController::TreeDrop(int32 Source, int32 Target)
{
	using K = SCampaign1851Overlay::ETreeKind;
	if (Target == INDEX_NONE || Target == Source)
	{
		return;
	}
	const int32 SourceId = SCampaign1851Overlay::TreeId(Source), TargetId = SCampaign1851Overlay::TreeId(Target);
	const K TargetKind = SCampaign1851Overlay::TreeKind(Target);
	if (TargetKind == K::NewFormation)
	{
		// "Drag here for a new unit": a new formation one level below the one it stands by, with the unit in it.
		if (SCampaign1851Overlay::TreeKind(Source) != K::Regiment)
		{
			Overlay->ShowToast(TEXT("Træk en bataljon, eskadron eller et batteri herover"));
			return;
		}
		const int32 ParentIndex = Map->FormationIndex(TargetId);
		const ECampaign1851Echelon Above = ParentIndex == INDEX_NONE ? ECampaign1851Echelon::Army : Map->GetFormations()[ParentIndex].Echelon;
		const ECampaign1851Echelon Echelon = Above == ECampaign1851Echelon::Army ? ECampaign1851Echelon::Division : Above == ECampaign1851Echelon::Division ? ECampaign1851Echelon::Brigade : ECampaign1851Echelon::Regiment;
		const int32 Id = Map->CreateFormation(Echelon, ParentIndex == INDEX_NONE ? 0 : TargetId);
		Map->MoveRegimentToFormation(SourceId, Id);
		Overlay->ShowToast(FString::Printf(TEXT("%s oprettet med %s"), *Map->GetFormations()[Map->FormationIndex(Id)].Name, *Overlay->TreeKeyText(Source)));
		return;
	}
	// Where the drop puts things: a formation, the formation of a regiment dropped on, or the garrisons (0).
	int32 Into = INDEX_NONE;
	if (TargetKind == K::Formation)
	{
		Into = TargetId;
	}
	else if (TargetKind == K::Regiment && Map->GetRegiments().IsValidIndex(TargetId))
	{
		Into = Map->GetRegiments()[TargetId].Formation;
	}
	else if (TargetKind == K::Garrisons || TargetKind == K::Command || TargetKind == K::ArmGroup)
	{
		Into = 0;
	}
	else if (TargetKind == K::FieldArmy)
	{
		Into = -1;   // the top of the field army: only formations can stand there
	}
	bool bDone = false;
	FString What;
	if (SCampaign1851Overlay::TreeKind(Source) == K::Regiment && Into >= 0)
	{
		bDone = Map->MoveRegimentToFormation(SourceId, Into);
		What = Into == 0 ? TEXT("tilbage i garnison") : FString::Printf(TEXT("ind i %s"), *Map->GetFormations()[Map->FormationIndex(Into)].Name);
	}
	else if (SCampaign1851Overlay::TreeKind(Source) == K::Formation && Into != 0)
	{
		const int32 Parent = Into < 0 ? 0 : Into;
		bDone = Map->MoveFormation(SourceId, Parent);
		What = Parent == 0 ? FString(TEXT("direkte under felthæren")) : FString::Printf(TEXT("under %s"), *Map->GetFormations()[Map->FormationIndex(Parent)].Name);
	}
	Overlay->ShowToast(bDone ? FString::Printf(TEXT("%s flyttet %s"), *Overlay->TreeKeyText(Source), *What) : FString(TEXT("Kan ikke flyttes dertil")));
}

void ACampaign1851PlayerController::CampaignTestFieldArmy()
{
	if (Map.IsValid())
	{
		Map->BuildTestFieldArmy();
		if (Overlay.IsValid() && !Overlay->IsOOBOpen())
		{
			Overlay->ToggleOOB();
		}
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
