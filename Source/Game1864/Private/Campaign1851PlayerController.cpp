#include "Campaign1851PlayerController.h"

#include "Campaign1851Camera.h"
#include "Campaign1851Map.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "SCampaign1851Overlay.h"
#include "Campaign1851ConstructionSite.h"
#include "Campaign1851SaveGame.h"
#include "Campaign1851Buildings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

ACampaign1851PlayerController::ACampaign1851PlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
}

namespace
{
	// Survives an OpenLevel when a manual save needs another scenario's data.
	FString GCampaignOfficerScenarioPendingSlot;
	bool GCampaignOfficerScenarioPendingBattle = false;

	// The player's settings live in GameUserSettings (the battle's section); a save carries them along and a load puts them back.
	const TCHAR* const GSettingsSection = TEXT("/Script/Strategy1864.Settings");

	void CaptureSettings(TMap<FString, FString>& Out)
	{
		Out.Reset();
		TArray<FString> Lines;
		if (GConfig && GConfig->GetSection(GSettingsSection, Lines, GGameUserSettingsIni))
		{
			for (const FString& Line : Lines)
			{
				FString Key, Value;
				if (Line.Split(TEXT("="), &Key, &Value)) { Out.Add(Key, Value); }
			}
		}
	}

	void RestoreSettings(const TMap<FString, FString>& In)
	{
		if (!GConfig || In.Num() == 0) { return; }
		for (const TPair<FString, FString>& Pair : In)
		{
			GConfig->SetString(GSettingsSection, *Pair.Key, *Pair.Value, GGameUserSettingsIni);
		}
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	FString PriceText(double Amount)
	{
		return FString::FormatAsNumber(FMath::RoundToInt(Amount)) + TEXT(" rd.");
	}

	/**
	 * What a costly or lasting step does and what it costs, for the confirmation dialog (JA / NEJ) before it is
	 * taken. False for the buttons that only look or select (they act at once).
	 */
	bool DescribeAction(const ACampaign1851Map& Map, const SCampaign1851Overlay& Overlay, SCampaign1851Overlay::EButton Button, int32 Module, FString& Title, FString& Text)
	{
		using EB = SCampaign1851Overlay::EButton;
		const TArray<FCampaign1851City>& Cities = Map.GetCities();
		const int32 City = Overlay.GetSelectedCity();
		const FString CityName = Cities.IsValidIndex(City) ? Cities[City].Name : FString(TEXT("byen"));
		const FString Down = FString::Printf(TEXT("%d %%"), FMath::RoundToInt(Campaign1851Buildings::DownPayment * 100.f));
		switch (Button)
		{
		case EB::BuildTown:
		{
			if (!ACampaign1851ConstructionSite::TownBuildings().IsValidIndex(Module))
			{
				return false;
			}
			const FCampaign1851SiteModule& D = ACampaign1851ConstructionSite::TownBuildings()[Module];
			Title = FString::Printf(TEXT("Byg %s i %s?"), *D.Name.ToLower(), *CityName);
			Text = FString::Printf(TEXT("%s (%s) opføres i %s på omkring %d dage. Det koster %s i alt: %s nu til materialerne, resten løbende mens der bygges. Bagefter koster bygningen %s om året i drift."),
				*D.Name, *D.Type(), *CityName, FMath::RoundToInt(D.Days()), *PriceText(D.Cost()), *Down, *PriceText(D.Upkeep()));
			return true;
		}
		case EB::Build:
			Title = FString::Printf(TEXT("Anlæg garnison i %s?"), *CityName);
			Text = FString::Printf(TEXT("Byggepladsen ryddes, og den første infanterikaserne påbegyndes. Materialerne betales med %s med det samme, resten løbende mens der bygges. Senere kan flere bygninger føjes til garnisonen."), *Down);
			return true;
		case EB::BuildModule:
		{
			const ACampaign1851ConstructionSite* Site = Map.FindProject(City);
			if (!Site)
			{
				return false;
			}
			Title = FString::Printf(TEXT("Byg %s?"), *Site->ModuleName(Module).ToLower());
			Text = FString::Printf(TEXT("%s føjes til garnisonen i %s på omkring %d dage. Det koster %s: %s nu til materialerne, resten løbende."),
				*Site->ModuleName(Module), *CityName, FMath::RoundToInt(Site->ModuleDays(Module)), *PriceText(Site->ModuleCost(Module)), *Down);
			return true;
		}
		case EB::BuildLink:
		{
			const int32 Link = Module / 2;
			if (!Map.GetLinks().IsValidIndex(Link))
			{
				return false;
			}
			const ECampaign1851LinkWork Work = Module % 2 ? ECampaign1851LinkWork::Railway : ECampaign1851LinkWork::Chaussee;
			const FCampaign1851Link& L = Map.GetLinks()[Link];
			const bool bRail = Work == ECampaign1851LinkWork::Railway;
			Title = FString::Printf(TEXT("Anlæg %s %s–%s?"), bRail ? TEXT("jernbane") : TEXT("chaussé"), *Cities[L.A].Name, *Cities[L.B].Name);
			Text = FString::Printf(TEXT("%s Arbejdet tager omkring %d dage og koster %s, betalt efterhånden som det skrider frem."),
				bRail ? TEXT("Tog kører tropper og forsyninger mange gange hurtigere end til fods, og handlen langs banen vokser.") : TEXT("En fast landevej: hæren og trænet marcherer hurtigere og slider mindre, også i tøbrud."),
				FMath::RoundToInt(Map.LinkWorkDays(Link, Work)), *PriceText(Map.LinkWorkCost(Link, Work)));
			return true;
		}
		case EB::RaiseBattalion:
			Title = FString::Printf(TEXT("Opret en bataljon i %s?"), *CityName);
			Text = FString::Printf(TEXT("%d rekrutter indkaldes med en major og fire kaptajner. Det koster %s nu (hvervning og udrustning), og bataljonen skal derefter have sold og forplejning. Rekrutterne er grønne og skal eksercere, før de duer i felten."),
				Campaign1851Army::RaiseMen, *PriceText(Campaign1851Army::RaiseCost()));
			return true;
		case EB::UnitRaise:
		{
			const Campaign1851Resources::FUnitType T = Campaign1851Resources::SizedType(Overlay.RaiseType, Overlay.RaiseSize);
			Title = FString::Printf(TEXT("Opret %s?"), T.Name);
			Text = FString::Printf(TEXT("%d mand%s%s indkaldes og udrustes (%d geværer, %d uniformer). Det koster omkring %s inklusive indkøb af manglende geværer, heste og vogne. Træningsprogram: %s."),
				T.Men, T.Guns > 0 ? *FString::Printf(TEXT(", %d kanoner"), T.Guns) : TEXT(""), T.Horses > 0 ? *FString::Printf(TEXT(", %d heste"), T.Horses) : TEXT(""),
				T.Rifles, T.Uniforms, *PriceText(Map.UnitCost(Overlay.RaiseType, Overlay.RaiseSize)), Campaign1851Army::ProgramName(ECampaign1851Program(Overlay.RaiseProgram)));
			return true;
		}
		case EB::ResearchStart:
		{
			if (!Campaign1851Research::Topics().IsValidIndex(Module))
			{
				return false;
			}
			const FCampaign1851ResearchTopic& T = Campaign1851Research::Topics()[Module];
			Title = FString::Printf(TEXT("Forsk i %s?"), T.Name);
			Text = FString::Printf(TEXT("%s\nDet tager %d måneder og koster %s om måneden, i alt %s."), T.Effect, T.Months, *PriceText(T.CostPerMonth), *PriceText(T.CostPerMonth * T.Months));
			return true;
		}
		case EB::DoctrineSet:
			Title = FString::Printf(TEXT("Skift doktrin til %s?"), Campaign1851Research::DoctrineName(Module / 10, Module % 10));
			Text = FString::Printf(TEXT("Hæren omskoles efter den nye doktrin. Det koster %s, og i omkring %d dage, mens officerer og mænd lærer det nye, kæmper hæren dårligere."),
				*PriceText(ACampaign1851Map::DoctrineChangeCost), FMath::RoundToInt(ACampaign1851Map::DoctrineChangeDays));
			return true;
		case EB::Loan:
			if (Module == 2)
			{
				Title = TEXT("Afdrag på statsgælden?");
				Text = FString::Printf(TEXT("%s betales tilbage af statskassen. Renterne falder tilsvarende."), *PriceText(FMath::Min(100000.0, Map.GetDebt())));
			}
			else
			{
				const double Amount = Module == 0 ? 100000.0 : 250000.0;
				Title = FString::Printf(TEXT("Optag et lån på %s?"), *PriceText(Amount));
				Text = FString::Printf(TEXT("Pengene kommer i statskassen med det samme. Renten er nu %.1f %%, dvs. omkring %s om året, indtil lånet er betalt tilbage. Mere gæld gør de næste lån dyrere."),
					Map.CreditRate() * 100.f, *PriceText(Amount * Map.CreditRate()));
			}
			return true;
		case EB::ShipOrder:
		{
			if (!Campaign1851Navy::Classes().IsValidIndex(Module))
			{
				return false;
			}
			const FCampaign1851ShipClass& C = Campaign1851Navy::Classes()[Module];
			Title = FString::Printf(TEXT("Bestil en %s?"), C.Name);
			Text = FString::Printf(TEXT("Skibet bygges på Holmen på %d måneder og koster %s. Når det er i tjeneste, koster det %s om året og styrker flåden med %.0f."),
				C.Months, *PriceText(C.Cost), *PriceText(C.UpkeepPerYear), C.Strength);
			return true;
		}
		case EB::Blockade:
			Title = Module == 1 ? TEXT("Blokér fjendens havne?") : TEXT("Hæv blokaden?");
			Text = Module == 1 ? TEXT("Flåden lægger sig ud for fjendens havne. Det kvæler hans handel og trækker krigen mod os, men skibene slides og kan møde fjendens eskadrer.")
				: TEXT("Flåden går hjem. Fjendens handel kommer i gang igen.");
			return true;
		case EB::SupplyBuy:
			Title = TEXT("Køb en trænkolonne?");
			Text = FString::Printf(TEXT("20 vogne og 80 heste med kuske, som Intendanturen kan sende forsyninger med til hæren i felten. Det koster %s."), *PriceText(Campaign1851Supply::ColumnCost));
			return true;
		case EB::KitBuy:
			Title = Module == 1 ? TEXT("Køb 2 mortérer?") : TEXT("Køb 10 vogne?");
			Text = Module == 1 ? FString::Printf(TEXT("To mortérer købes i udlandet og lægges på lager. Det koster %s."), *PriceText(2 * Campaign1851Resources::MortarPrice))
				: FString::Printf(TEXT("Ti vogne købes i landet og lægges på lager. Det koster %s."), *PriceText(10 * Campaign1851Resources::WagonPrice));
			return true;
		case EB::RawBuy:
		{
			if (Module % 10 == 0)
			{
				return false;   // the small step goes at once
			}
			const ECampaign1851Raw R = ECampaign1851Raw(Module / 10);
			const float Amount = (R == ECampaign1851Raw::Cloth || R == ECampaign1851Raw::Leather ? 100.f : 10.f) * 10.f;
			Title = TEXT("Køb råvarer?");
			Text = FString::Printf(TEXT("%.0f enheder købes og lægges på lager. Det koster omkring %s."), Amount, *PriceText(Amount * Map.RawPrice(R)));
			return true;
		}
		case EB::OfficerRecruit:
			Title = Module == 1 ? TEXT("Ansæt en general?") : TEXT("Ansæt en officer?");
			Text = FString::Printf(TEXT("Han ansættes og venter på en post. Ansættelsen koster %s, og derefter får han sold, også mens han er uden post."), *PriceText(Map.OfficerCost(Module == 1)));
			return true;
		case EB::OfficerPromote:
			if (!Map.GetOfficers().IsValidIndex(Module))
			{
				return false;
			}
			Title = FString::Printf(TEXT("Forfrem %s?"), *Map.GetOfficers()[Module].Name);
			Text = TEXT("Han rykker en grad op og får højere gage. Bliver han general, må han forlade sin post, og regimentet skal have en ny chef.");
			return true;
		case EB::OfficerDismiss:
			if (!Map.GetOfficers().IsValidIndex(Module))
			{
				return false;
			}
			Title = FString::Printf(TEXT("Afsked %s?"), *Map.GetOfficers()[Module].Name);
			Text = TEXT("Han forlader hæren for altid, og hans post bliver ledig. Hans erfaring går tabt, men soldet spares.");
			return true;
		case EB::MinisterAppoint:
			Title = TEXT("Udnævn en ny minister?");
			Text = TEXT("Den nuværende minister går af, og den nye overtager ministeriet med sine egne evner og sin egen politik. Et skifte kan koste ro i Rigsdagen.");
			return true;
		case EB::MakePeace:
		{
			const TArray<FCampaign1851PeaceOffer> Offers = Map.PeaceOffers();
			if (!Offers.IsValidIndex(Module))
			{
				return false;
			}
			const FCampaign1851PeaceOffer& P = Offers[Module];
			FString Ceded;
			for (int32 T : P.Ceded)
			{
				if (Cities.IsValidIndex(T))
				{
					Ceded += (Ceded.IsEmpty() ? TEXT("") : TEXT(", ")) + Cities[T].Name;
				}
			}
			Title = FString::Printf(TEXT("Tilbyd fred: %s?"), *P.Name);
			Text = FString::Printf(TEXT("%s Krigen slutter, hvis fjenden tager imod; ellers taber vi ansigt. Kan ikke gøres om."),
				Ceded.IsEmpty() ? TEXT("Ingen byer afstås.") : *FString::Printf(TEXT("Disse byer afstås: %s."), *Ceded));
			return true;
		}
		case EB::Diplomacy:
		{
			const int32 Nation = Module / 10;
			const FString Who = Map.GetNations().IsValidIndex(Nation) ? Map.GetNations()[Nation].Name : FString(TEXT("landet"));
			switch (ACampaign1851Map::EDiplomacyAction(Module % 10))
			{
			case ACampaign1851Map::EDiplomacyAction::Envoy:
				Title = FString::Printf(TEXT("Send en gesandt til %s?"), *Who);
				Text = FString::Printf(TEXT("Forholdet til %s bedres. Det koster %s."), *Who, *PriceText(ACampaign1851Map::EnvoyCost));
				break;
			case ACampaign1851Map::EDiplomacyAction::Trade:
				Title = FString::Printf(TEXT("Handelstraktat med %s?"), *Who);
				Text = FString::Printf(TEXT("Toldsatserne sænkes gensidigt; handlen giver statskassen mere hvert år. Forhandlingerne koster %s."), *PriceText(ACampaign1851Map::TreatyCost));
				break;
			case ACampaign1851Map::EDiplomacyAction::Alliance:
				Title = FString::Printf(TEXT("Alliance med %s?"), *Who);
				Text = FString::Printf(TEXT("%s lover at stå os bi i krig, og vi det samme. Det koster %s, og en alliance kan trække os ind i andres krige."), *Who, *PriceText(Map.AllianceCostNow()));
				break;
			default:
				Title = FString::Printf(TEXT("Søg garanti fra %s?"), *Who);
				Text = FString::Printf(TEXT("%s garanterer vores grænser; spændingen stiger langsommere. Det koster %s."), *Who, *PriceText(ACampaign1851Map::GuaranteeCost));
				break;
			}
			return true;
		}
		case EB::FortGuns:
		case EB::FortDefence:
		case EB::FortTrenches:
		{
			const FCampaign1851Fort* F = Map.GetForts().FindByPredicate([Module](const FCampaign1851Fort& X) { return X.Id == Module; });
			if (!F)
			{
				return false;
			}
			if (Button == EB::FortGuns)
			{
				Title = TEXT("To kanoner mere i skansen?");
				Text = FString::Printf(TEXT("To nye kanonbænke med fæstningskanoner, på %d dage. Det koster op til %s (kanoner fra statens lager sparer %s stykket)."),
					FMath::RoundToInt(Campaign1851Forts::GunsDays), *PriceText(Campaign1851Forts::GunsCost), *PriceText(Campaign1851Forts::GunPrice));
			}
			else if (Button == EB::FortTrenches)
			{
				Title = TEXT("Grav løbegrave?");
				Text = FString::Printf(TEXT("Løbegrave til skanserne omkring giver reserven dækning, når den skal frem. %d dage, %s."),
					FMath::RoundToInt(Campaign1851Forts::TrenchesDays), *PriceText(Campaign1851Forts::TrenchesCost(F->bLarge)));
			}
			else
			{
				const int32 Next = FMath::Min(F->Defence + 1, Campaign1851Forts::MaxDefence);
				Title = FString::Printf(TEXT("Udbyg: %s?"), Campaign1851Forts::DefenceName(Next));
				Text = FString::Printf(TEXT("%s Dækning for besætningen %d %%. %d dage, %s."), Campaign1851Forts::DefenceNote(Next), Campaign1851Forts::CoverPercent(Next),
					FMath::RoundToInt(Campaign1851Forts::DefenceDays(Next)), *PriceText(Campaign1851Forts::DefenceCost(Next, F->bLarge)));
			}
			return true;
		}
		case EB::BridgeDo:
			switch (EBridgeAction(Module % 10))
			{
			case EBridgeAction::Blow:
				Title = TEXT("Spræng broen?");
				Text = TEXT("Broen sprænges, og fjenden kan ikke gå over her. Vi kan heller ikke selv, før den er bygget op igen, hvilket tager tid og penge.");
				break;
			case EBridgeAction::Rebuild:
				Title = TEXT("Genopbyg broen?");
				Text = TEXT("Pionererne bygger broen op igen. Det koster penge og tager en rum tid.");
				break;
			default:
				Title = TEXT("Slå en pontonbro?");
				Text = FString::Printf(TEXT("Pionererne slår en pontonbro over vandet på %d dage. Det koster %s."), FMath::RoundToInt(Map.PontoonDays()), *PriceText(Map.PontoonCost()));
				break;
			}
			return true;
		case EB::TrainOrder:
			Title = TEXT("Bestil et togsæt?");
			Text = FString::Printf(TEXT("Et lokomotiv med vogne bestilles i England og leveres til %s. Det koster %s."),
				Cities.IsValidIndex(Module) ? *Cities[Module].Name : TEXT("København"), *PriceText(ACampaign1851Map::TroopTrainCost));
			return true;
		case EB::TrainMove:
		{
			const int32 To = Module % 1000;
			Title = TEXT("Skib toget?");
			Text = FString::Printf(TEXT("Toget skibes til %s og kan køre på banerne der. Overførslen koster %s."),
				Cities.IsValidIndex(To) ? *Cities[To].Name : TEXT("den anden bane"), *PriceText(ACampaign1851Map::TrainTransferCost));
			return true;
		}
		case EB::MergeUnit:
		{
			const int32 Keep = Module / 10000, Absorb = Module % 10000;
			if (!Map.GetRegiments().IsValidIndex(Keep) || !Map.GetRegiments().IsValidIndex(Absorb))
			{
				return false;
			}
			const FCampaign1851Regiment& K = Map.GetRegiments()[Keep];
			const FCampaign1851Regiment& A = Map.GetRegiments()[Absorb];
			Title = FString::Printf(TEXT("Saml %s og %s?"), *K.Name, *A.Name);
			Text = FString::Printf(TEXT("De to halvdele bliver én enhed igen: %d kompagnier, %d mand. Erfaring og øvelse blandes efter mandtal, samhørigheden falder lidt en tid. %s Det koster intet."),
				K.Captains.Num() + A.Captains.Num(), K.Men + A.Men, Map.GetOfficers().IsValidIndex(A.Chief) ? *FString::Printf(TEXT("%s bliver ledig til en anden post."), *Map.GetOfficers()[A.Chief].Name) : TEXT(""));
			return true;
		}
		case EB::BattleAuto:
			Title = TEXT("Afgør slaget automatisk?");
			Text = TEXT("Slaget udregnes straks ud fra styrke, erfaring, terræn og ledelse, uden at du fører tropperne selv. Udfaldet kan ikke gøres om.");
			return true;
		case EB::BattleRetreat:
			Title = TEXT("Træk hæren tilbage?");
			Text = TEXT("Hæren opgiver stillingen og trækker sig tilbage. Det koster færre tab end et tabt slag, men fjenden tager feltet, og humøret falder.");
			return true;
		default:
			return false;
		}
	}
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
	if (bInitialised && bCampaignStarted)
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
	if (!Overlay.IsValid()) { return; }
	bInitialised = true;

	FString BuildCity;
	bool bSiegeTestNewCampaign = false;
	FString SiegeTestRequest;
	const bool bSiegeTestStart = FParse::Value(FCommandLine::Get(), TEXT("CampaignTestSiege="), SiegeTestRequest, false);
	const bool bTestStart = FParse::Value(FCommandLine::Get(), TEXT("CampaignBuild="), BuildCity, false) || bSiegeTestStart;
	// Back from a 3D battle: the campaign as it was left (whatever the command line says); the result is read in.
	const FString ReturnFlag = FPaths::ProjectSavedDir() / TEXT("Battle/ReturnToCampaign.flag");
	const bool bBackFromBattle = IFileManager::Get().FileExists(*ReturnFlag) && UGameplayStatics::DoesSaveGameExist(TEXT("Autosave"), 0);
	if (!GCampaignOfficerScenarioPendingSlot.IsEmpty())
	{
		const FString PendingSlot = GCampaignOfficerScenarioPendingSlot;
		GCampaignOfficerScenarioPendingSlot.Reset();
		bResumedFromBattle = GCampaignOfficerScenarioPendingBattle;
		GCampaignOfficerScenarioPendingBattle = false;
		LoadFromSlot(PendingSlot);
		if (bResumedFromBattle) { Map->PollBattleResults(); }
	}
	else if (bBackFromBattle)
	{
		IFileManager::Get().Delete(*ReturnFlag);
		bResumedFromBattle = true;
		LoadFromSlot(TEXT("Autosave"));
		if (!GCampaignOfficerScenarioPendingSlot.IsEmpty()) { return; }
		Map->PollBattleResults();
	}
	else if (!bTestStart && IFileManager::Get().FileExists(*(FPaths::ProjectSavedDir() / TEXT("Campaign/NewGame.flag"))))
	{
		FString StartNation;
		FFileHelper::LoadFileToString(StartNation, *(FPaths::ProjectSavedDir() / TEXT("Campaign/NewGame.flag")));
		FString StartDeviation;
		if (StartNation.Split(TEXT("|"), &Map->NewGameNation, &StartDeviation))
		{ Map->NewGameDeviation = FCString::Atof(*StartDeviation); }
		else { Map->NewGameNation = StartNation == TEXT("SE") ? TEXT("SE") : TEXT("DK"); }
		IFileManager::Get().Delete(*(FPaths::ProjectSavedDir() / TEXT("Campaign/NewGame.flag")));
		CampaignNewGame();   // the new game of the scenario chosen in the menu
	}
	else if (!bTestStart)
	{
		Map->SetSpeed(0);
		OpenGameMenu();
		Overlay->ShowStartMenu();
		return;
	}
	if (bTestStart && (!bSiegeTestStart || !bCampaignStarted))
	{
		bCampaignStarted = true;
		if (bSiegeTestStart) { CampaignNewGame(); bSiegeTestNewCampaign = true; }
		else { CampaignBuild(BuildCity); }
	}
	// Test starts: -CampaignSpeed=0..3, -CampaignDate=1852-01-20 (e.g. to see the winter).
	int32 StartSpeed = 0;
	Map->SetSpeed(0);   // the game starts paused
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
	// -CampaignFocusBuilding=Odense:Textile_Mill (later, when the scene is up) puts the camera on a town building.
	FParse::Value(FCommandLine::Get(), TEXT("CampaignFocusBuilding="), FocusBuildingOrder, false);
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
	// -CampaignDiplomacy=SE:0;SE:1;GB:3 takes foreign actions (0 envoy, 1 trade, 2 alliance, 3 guarantee) with test relations.
	FString DiplomacyTest;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignDiplomacy="), DiplomacyTest, false))
	{
		TArray<FString> Items;
		DiplomacyTest.ParseIntoArray(Items, TEXT(";"));
		for (const FString& Item : Items)
		{
			FString Id, Action;
			Item.Split(TEXT(":"), &Id, &Action);
			const int32 n = Map->GetNations().IndexOfByPredicate([&Id](const FCampaign1851Nation& N) { return N.Id == Id; });
			FString Why;
			const bool bDone = Map->DoDiplomacy(n, ACampaign1851Map::EDiplomacyAction(FCString::Atoi(*Action)), &Why);
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|diplomacy|%s|%s|%s|relation %.0f|income %.0f"), *Id, *Action, bDone ? TEXT("done") : *Why,
				Map->GetNations().IsValidIndex(n) ? Map->GetNations()[n].Relation : 0.f, Map->ForeignIncomePerYear());
		}
	}
	// -CampaignResearchDone=conserves,sanitation grants topics; -CampaignResearch=staff starts one; -CampaignDoctrine=0:1,2:2 sets doctrines.
	FString ResearchTest;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignResearchDone="), ResearchTest, false))
	{
		TArray<FString> Ids;
		ResearchTest.ParseIntoArray(Ids, TEXT(","));
		for (const FString& Id : Ids)
		{
			Map->GrantResearch(Id);
		}
	}
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignResearch="), ResearchTest))
	{
		FString Why;
		const bool bDone = Map->StartResearch(Campaign1851Research::FindTopic(ResearchTest), &Why);
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|research|%s|%s"), *ResearchTest, bDone ? TEXT("started") : *Why);
	}
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignDoctrine="), ResearchTest, false))
	{
		TArray<FString> Items;
		ResearchTest.ParseIntoArray(Items, TEXT(","));
		for (const FString& Item : Items)
		{
			FString Level, Choice, Why;
			Item.Split(TEXT(":"), &Level, &Choice);
			const bool bDone = Map->SetDoctrine(FCString::Atoi(*Level), FCString::Atoi(*Choice), &Why);
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|doctrine|%s|%s"), *Item, bDone ? TEXT("set") : *Why);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|weather|%s"), *Map->GetSeasonAndWeather());
	// -CampaignWorks=0,1 carries out the historical works now (0 Dannevirke, 1 Dybbøl, 2 Fredericia).
	FString WorksTest;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignWorks="), WorksTest, false))
	{
		TArray<FString> Items;
		WorksTest.ParseIntoArray(Items, TEXT(","));
		for (const FString& Item : Items)
		{
			Map->BuildProgramme(FCString::Atoi(*Item));
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("CampaignFortsComplete")))
		{
			Map->CompleteForts();
		}
	}
	// -CampaignFormCommand=0 drags a whole general command into the field army (test).
	int32 FormCommand = -1;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignFormCommand="), FormCommand))
	{
		const int32 Id = Map->FormFromCommand(FormCommand, 0);
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|oob|command %d -> formation %d with %d units"), FormCommand, Id, Map->FormationRegiments(Id).Num());
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CampaignYearlyOfficers")))
	{
		Map->YearlyOfficers();
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|research-effects|loss %.2f|cover +%.0f|guns %.2f|infantry %.2f|food %.0f|call-in %.2f"),
		Map->DanishLossFactor(), Map->FortCoverBonus(), Map->DanishGunFactor(), Map->InfantryFactor(), Map->FoodCap(), Map->CallInFactor());
	FString WindowName;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignOpenWindow="), WindowName) && Overlay.IsValid())
	{
		Overlay->OpenWindow(WindowName == TEXT("army") ? SCampaign1851Overlay::EWindow::Army : WindowName == TEXT("officers") ? SCampaign1851Overlay::EWindow::Officers
			: WindowName == TEXT("budget") ? SCampaign1851Overlay::EWindow::Budget : WindowName == TEXT("trains") ? SCampaign1851Overlay::EWindow::Trains
			: WindowName == TEXT("chart") ? SCampaign1851Overlay::EWindow::Chart : WindowName == TEXT("council") ? SCampaign1851Overlay::EWindow::Council
			: WindowName == TEXT("supply") ? SCampaign1851Overlay::EWindow::Supply : WindowName == TEXT("foreign") ? SCampaign1851Overlay::EWindow::Foreign
			: WindowName == TEXT("research") ? SCampaign1851Overlay::EWindow::Research : WindowName == TEXT("navy") ? SCampaign1851Overlay::EWindow::Navy
			: WindowName == TEXT("battlefield") ? SCampaign1851Overlay::EWindow::Battlefield
			: WindowName == TEXT("materiel") ? SCampaign1851Overlay::EWindow::Materiel
			: WindowName == TEXT("nations") ? SCampaign1851Overlay::EWindow::Nations
			: WindowName == TEXT("gazette") ? SCampaign1851Overlay::EWindow::Gazette : WindowName == TEXT("end") ? SCampaign1851Overlay::EWindow::End
			: SCampaign1851Overlay::EWindow::Towns);
	// -CampaignBattlefield=lat,lon,km builds the ground there (test of the generator).
	FString FieldAt;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignBattlefield="), FieldAt, false))
	{
		TArray<FString> P;
		FieldAt.ParseIntoArray(P, TEXT(","));
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|battlefield|test request %s (%d parts)"), *FieldAt, P.Num());
		if (P.Num() >= 2)
		{
			Map->GenerateBattlefield(Map->KmAtWorld(Map->Project(FCString::Atod(*P[0]), FCString::Atod(*P[1]))), P.Num() > 2 ? FCString::Atof(*P[2]) : 8.f, TEXT("Test"));
		}
		// -CampaignBattleView: and go onto it (test of the 3D model).
		if (FParse::Param(FCommandLine::Get(), TEXT("CampaignBattleView")))
		{
			SetBattleView(true);
		}
	}
	if (Overlay.IsValid())
	{
		FParse::Value(FCommandLine::Get(), TEXT("CampaignResearchPick="), Overlay->ResearchPick);
		FParse::Value(FCommandLine::Get(), TEXT("CampaignForeignTab="), Overlay->ForeignTab);
	}
	int32 GazetteTab = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignGazetteTab="), GazetteTab) && Overlay.IsValid())
	{
		Overlay->SetGazetteTab(GazetteTab);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CampaignSupplyMap")) && Overlay.IsValid())
	{
		Overlay->ToggleSupplyMap();
	}
	}
	// -CampaignBuildFort=54.91,9.75,stor,135;54.90,9.72,lille,160 starts forts (lat, lon, size, front bearing);
	// -CampaignFortsComplete finishes them fully armed and strengthened (to see them).
	FString FortOrders;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignBuildFort="), FortOrders, false))
	{
		TArray<FString> Orders;
		FortOrders.ParseIntoArray(Orders, TEXT(";"));
		for (const FString& Order : Orders)
		{
			TArray<FString> P;
			Order.ParseIntoArray(P, TEXT(","));
			if (P.Num() >= 2)
			{
				FString Why;
				const float Bearing = P.Num() > 3 ? FCString::Atof(*P[3]) : 180.f;
				const int32 Id = Map->StartFort(Map->KmAtWorld(Map->Project(FCString::Atod(*P[0]), FCString::Atod(*P[1]))), P.Num() > 2 && P[2] == TEXT("stor"), Bearing - 90.f, &Why);
				UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|fort|%s -> %d %s"), *Order, Id, *Why);
				if (Id != INDEX_NONE && Overlay.IsValid())
				{
					Overlay->SelectFort(Id);
				}
			}
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("CampaignFortsComplete")))
		{
			Map->CompleteForts();
		}
		// -CampaignDemolishFort=1 slights a fort (test of salvage).
		int32 SlightId = 0;
		if (FParse::Value(FCommandLine::Get(), TEXT("CampaignDemolishFort="), SlightId))
		{
			FString Why;
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|fort|demolish %d: %s"), SlightId, Map->DemolishFort(SlightId, &Why) ? TEXT("ok") : *Why);
		}
		// -CampaignFortGarrison=1:B12:0,B12:1;2:B12:2 puts companies (battalion id : company) in forts (id).
		FString Garrisons;
		if (FParse::Value(FCommandLine::Get(), TEXT("CampaignFortGarrison="), Garrisons, false))
		{
			TArray<FString> PerFort;
			Garrisons.ParseIntoArray(PerFort, TEXT(";"));
			for (const FString& G : PerFort)
			{
				FString FortId, List;
				G.Split(TEXT(":"), &FortId, &List);
				TArray<FString> Items;
				List.ParseIntoArray(Items, TEXT(","));
				for (const FString& Item : Items)
				{
					FString Reg, K;
					Item.Split(TEXT(":"), &Reg, &K);
					FString Why;
					const bool bOk = Map->AddFortCompany(FCString::Atoi(*FortId), Map->FindRegiment(Reg), FCString::Atoi(*K), &Why);
					UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|fort|garrison %s <- %s: %s"), *FortId, *Item, bOk ? TEXT("ok") : *Why);
				}
			}
		}
	}
	// -CampaignWarTest brings the ultimatum forward (test of the war).
	if (!bResumedFromBattle && FParse::Param(FCommandLine::Get(), TEXT("CampaignWarTest")))
	{
		Map->ForceWar();
	}
	// -CampaignAutoBattles resolves each battle at once (test).
	bAutoBattles = FParse::Param(FCommandLine::Get(), TEXT("CampaignAutoBattles"));
	if (bSiegeTestNewCampaign && Map->StartSiegeTest(SiegeTestRequest))
	{
		bAutoBattles = true;
	}
	// -CampaignMobilise calls the army in at once (test).
	if (!bResumedFromBattle && FParse::Param(FCommandLine::Get(), TEXT("CampaignMobilise")))
	{
		Map->Mobilise();
	}
	// -CampaignDelegate=auto|advisory hands every portfolio to the ministries (to watch the AI).
	FString DelegateMode;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignDelegate="), DelegateMode))
	{
		for (int32 p = 0; p < int32(ECampaign1851Portfolio::Count); ++p)
		{
			Map->SetDelegation(ECampaign1851Portfolio(p), DelegateMode == TEXT("auto") ? ECampaign1851Delegation::Auto : ECampaign1851Delegation::Advisory);
		}
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
	if (Map.IsValid() && Map->IsBattleView())
	{
		return false;   // the battlefield model: no map under the cursor
	}
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
	TickAutoClicks();
	if (bCampaignStarted && Map.IsValid()) { Map->PruneEmptyFormations(); }
	if (Overlay.IsValid() && Overlay->IsStartMenu())
	{
		Map->SetSpeed(0);
		// -CampaignMenuShot: saves a screenshot of the start menu after a few seconds and quits (QA).
		static bool bMenuShot = false;
		if (!bMenuShot && FParse::Param(FCommandLine::Get(), TEXT("CampaignMenuShot")) && GetWorld()->GetRealTimeSeconds() > 10.f)
		{
			bMenuShot = true;
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/menu_shot.png"), true, false);
			FTimerHandle Quit;
			GetWorldTimerManager().SetTimer(Quit, []() { FPlatformMisc::RequestExit(false); }, 3.f, false);
		}
		float StartX = 0.f, StartY = 0.f;
		if (PointerPosition(StartX, StartY) && LeftJustPressed())
		{
			int32 StartRow = INDEX_NONE;
			auto Button = Overlay->HitButton(FVector2D(StartX, StartY), &StartRow);
			using EButton = SCampaign1851Overlay::EButton;
			if (Button == EButton::Scenario) { Overlay->SetMenuScenario(StartRow); Button = EButton::Block; }
			if (Button == EButton::NewGameNation) { Map->NewGameNation = StartRow == 1 ? TEXT("SE") : TEXT("DK"); Button = EButton::Block; }
			if (Button == EButton::Deviation) { Map->NewGameDeviation = StartRow / 100.f; Button = EButton::Block; }
			if (Button == EButton::StartLoad) { Overlay->ShowStartLoad(true); Button = EButton::Block; }
			if (Button == EButton::StartTest)
			{
				// Temporary test battles: the flags the Start-Test-*.bat files give (1 or 4 Danish companies against one Swedish).
				FCommandLine::Append(TEXT(" -Strategy1864Skirmish=1 -Strategy1864SkirmishSwedes -Strategy1864EnemyDefends -Strategy1864SkirmishPassive"));
				if (StartRow == 9)
				{
					FCommandLine::Append(TEXT(" -Strategy1864SkirmishDanes=2 -Strategy1864SkirmishCavalry"));
				}
				else
				{
					FCommandLine::Append(StartRow >= 4 ? TEXT(" -Strategy1864SkirmishDanes=4 -Strategy1864HoldReserve") : TEXT(" -Strategy1864SkirmishDanes=1"));
				}
				UGameplayStatics::OpenLevel(this, FName(TEXT("Strategy1864_Skirmish")));
				Button = EButton::Block;
			}
			if (Button == EButton::CloseMenu) { Overlay->ShowStartLoad(false); Button = EButton::Block; }
			if (Button == EButton::LoadSlot && SaveSlots().IsValidIndex(StartRow))
			{
				if (LoadFromSlot(SaveSlots()[StartRow])) { Overlay->CloseMenu(); }
				Button = EButton::Block;
			}
			if (Button == EButton::NewGame)
			{
				const int32 StartScenario = Overlay->GetMenuScenario();
				if (StartScenario != ACampaign1851Map::ScenarioIndex())
				{
					IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("Campaign")), true);
					if (FFileHelper::SaveStringToFile(Map->NewGameNation + TEXT("|") + FString::SanitizeFloat(Map->NewGameDeviation), *(FPaths::ProjectSavedDir() / TEXT("Campaign/NewGame.flag"))))
					{ ACampaign1851Map::SetScenarioIndex(StartScenario); UGameplayStatics::OpenLevel(this, FName(TEXT("Campaign1851"))); }
					else { Overlay->ShowToast(TEXT("Kunne ikke starte det valgte scenarie")); }
				}
				else { CampaignNewGame(); }
				Button = EButton::Block;
			}
			if (Button == EButton::ExitGame) { UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false); Button = EButton::Block; }
		}
		return;
	}
	if (!bCampaignStarted) { return; }
	// -CampaignDebugClicks: logs every left click (what it hits and the state) and takes -CampaignShotAt=T1,T2 screenshots (QA of the clicks).
	if (FParse::Param(FCommandLine::Get(), TEXT("CampaignDebugClicks")) && Overlay.IsValid())
	{
		{
			static int32 LastSelected = -1;
			const int32 NowSelected = Overlay->GetSelectedRegiments().Num();
			if (NowSelected != LastSelected)
			{
				UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-CLICK: the selection is now %d regiments (was %d)"), NowSelected, LastSelected);
				LastSelected = NowSelected;
			}
		}
		float DX = 0.f, DY = 0.f;
		if (LeftJustPressed() && PointerPosition(DX, DY))
		{
			int32 DRow = INDEX_NONE;
			const SCampaign1851Overlay::EButton DHit = Overlay->HitButton(FVector2D(DX, DY), &DRow);
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-CLICK: at %.0f,%.0f hit=%d row=%d menuOpen=%d startMenu=%d selected=%d window=%d"), DX, DY, int32(DHit), DRow,
				Overlay->IsMenuOpen() ? 1 : 0, Overlay->IsStartMenu() ? 1 : 0, Overlay->GetSelectedRegiments().Num(), int32(Overlay->GetWindow()));
		}
		FString ShotTimes;
		static TArray<float> Times;
		static int32 NextShot = 0;
		if (Times.Num() == 0 && FParse::Value(FCommandLine::Get(), TEXT("CampaignShotAt="), ShotTimes, false))
		{
			TArray<FString> Parts;
			ShotTimes.ParseIntoArray(Parts, TEXT(","));
			for (const FString& P : Parts) { Times.Add(FCString::Atof(*P)); }
		}
		if (NextShot < Times.Num() && GetWorld()->GetRealTimeSeconds() >= Times[NextShot])
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/timed_shot_%d.png"), NextShot), true, false);
			++NextShot;
		}
	}
	// -CampaignUiShots=sec:cmd;cmd,sec:cmd,...: opens the windows and cards by itself and saves a screenshot of each step
	// (QA of the screens). cmd: window=army|officers|budget|towns|council|foreign|..., minister=N, officer=N, select=N (unit),
	// unitcard, oob, city=Name, tab=N, civil=0/1, info=N (building type), clear.
	if (Overlay.IsValid())
	{
		static TArray<FString> Steps;
		static int32 NextStep = 0;
		static float StepAt = -1.f;
		static bool bParsed = false;
		if (!bParsed)
		{
			bParsed = true;
			FString Plan;
			if (FParse::Value(FCommandLine::Get(), TEXT("CampaignUiShots="), Plan, false))
			{
				Plan.ParseIntoArray(Steps, TEXT(","));
			}
		}
		const float Real = GetWorld()->GetRealTimeSeconds();
		if (NextStep < Steps.Num())
		{
			FString Time, Cmds;
			Steps[NextStep].Split(TEXT(":"), &Time, &Cmds);
			if (StepAt < 0.f && Real >= FCString::Atof(*Time))
			{
				using W = SCampaign1851Overlay::EWindow;
				TArray<FString> List;
				Cmds.ParseIntoArray(List, TEXT(";"));
				for (const FString& C : List)
				{
					FString Key, Value;
					if (!C.Split(TEXT("="), &Key, &Value)) { Key = C; }
					if (Key == TEXT("window"))
					{
						static const TMap<FString, W> Names = { { TEXT("none"), W::None }, { TEXT("army"), W::Army }, { TEXT("officers"), W::Officers }, { TEXT("budget"), W::Budget }, { TEXT("towns"), W::Towns },
							{ TEXT("trains"), W::Trains }, { TEXT("chart"), W::Chart }, { TEXT("council"), W::Council }, { TEXT("supply"), W::Supply }, { TEXT("foreign"), W::Foreign },
							{ TEXT("research"), W::Research }, { TEXT("navy"), W::Navy }, { TEXT("status"), W::ArmyStatus } };
						Overlay->OpenWindow(Names.Contains(Value) ? Names[Value] : W::None);
					}
					else if (Key == TEXT("minister")) { Overlay->SetMinisterInfo(FCString::Atoi(*Value)); }
					else if (Key == TEXT("officer")) { Overlay->InspectOfficer(FCString::Atoi(*Value)); }
					else if (Key == TEXT("select")) { Overlay->SetSelectedRegiments({ FCString::Atoi(*Value) }); }
					else if (Key == TEXT("focus")) { Overlay->FocusOOB(FCString::Atoi(*Value)); Overlay->OpenWindow(W::Chart); }
					else if (Key == TEXT("kamporden")) { Overlay->OpenWindow(W::Chart); const TArray<int32>& KSel = Overlay->GetSelectedRegiments(); const int32 KTown = KSel.Num() > 0 && Map->GetRegiments().IsValidIndex(KSel[0]) ? Map->GetRegiments()[KSel[0]].Town : INDEX_NONE; if (KTown != INDEX_NONE) { Overlay->SetOOBPlace(KTown); } else { Overlay->FilterOOB(KSel); } }
					else if (Key == TEXT("selectmany")) { TArray<FString> Parts; Value.ParseIntoArray(Parts, TEXT("+")); TArray<int32> Sel; for (const FString& P : Parts) { Sel.Add(FCString::Atoi(*P)); } Overlay->SetSelectedRegiments(Sel); }
					else if (Key == TEXT("genfield")) { TArray<FString> P; Value.ParseIntoArray(P, TEXT("+")); if (P.Num() >= 2) { Map->GenerateBattlefield(Map->KmAtWorld(Map->Project(FCString::Atod(*P[0]), FCString::Atod(*P[1]))), 8.f, TEXT("Test")); } }
					else if (Key == TEXT("split")) { Map->SplitRegiment(FCString::Atoi(*Value)); }
					else if (Key == TEXT("unitcard")) { Overlay->ToggleUnitCard(); }
					else if (Key == TEXT("rtab")) { Overlay->SetResearchTab(FCString::Atoi(*Value)); }
					else if (Key == TEXT("pool")) { Overlay->TogglePoolOpen(FCString::Atoi(*Value)); }
					else if (Key == TEXT("unitco")) { Overlay->SetUnitCardCompany(FCString::Atoi(*Value)); }
					else if (Key == TEXT("oob")) { Overlay->ToggleOOB(); }
					else if (Key == TEXT("city")) { Overlay->SetSelectedCity(Map->FindCity(Value)); }
					else if (Key == TEXT("tab")) { Overlay->SetTownTab(FCString::Atoi(*Value)); }
					else if (Key == TEXT("civil")) { Overlay->SetCivilTab(Value == TEXT("1")); }
					else if (Key == TEXT("info")) { Overlay->ToggleBuildingInfo(FCString::Atoi(*Value)); }
					else if (Key == TEXT("clear")) { Overlay->SetSelectedRegiments({}); Overlay->InspectOfficer(INDEX_NONE); Overlay->SetMinisterInfo(-1); Overlay->OpenWindow(W::None); }
				}
				StepAt = Real + 1.5f;
			}
			if (StepAt > 0.f && Real >= StepAt)
			{
				const FString File = FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString::Printf(TEXT("ui_shot_%02d.png"), NextStep);
				FScreenshotRequest::RequestScreenshot(File, true, false);
				UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-UISHOT: %s (%s)"), *File, *Steps[NextStep]);
				StepAt = -1.f;
				++NextStep;
				if (NextStep >= Steps.Num() && FParse::Param(FCommandLine::Get(), TEXT("CampaignUiShotsQuit")))
				{
					FTimerHandle Quit;
					GetWorldTimerManager().SetTimer(Quit, []() { FPlatformMisc::RequestExit(false); }, 3.f, false);
				}
			}
		}
	}
	// -CampaignFight3D: the first battle goes to 3D at once (test of the way to the battle and back).
	static bool bFight3DDone = false;
	if (!bFight3DDone && FParse::Param(FCommandLine::Get(), TEXT("CampaignFight3D")) && Map.IsValid() && Map->GetBattles().Num() > 0 &&
		!Map->GetBattles()[0].bWaiting && Overlay.IsValid())
	{
		bFight3DDone = true;
		const int32 BattleId = Map->GetBattles()[0].Id;
		if (Map->FightBattleIn3D(BattleId))
		{
			SaveToSlot(TEXT("Autosave"), true);
			GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
			UGameplayStatics::OpenLevel(this, FName(TEXT("Strategy1864_Field")), true, FString::Printf(TEXT("Battle=%d"), BattleId));
			return;
		}
	}
	if (bAutoBattles && Map.IsValid() && Map->GetBattles().Num() > 0)
	{
		Map->AutoResolveBattle(Map->GetBattles()[0].Id);
		Map->SetSpeed(6);
	}
	if (!FocusBuildingOrder.IsEmpty() && Map.IsValid())
	{
		FString Town, Key;
		FocusBuildingOrder.Split(TEXT(":"), &Town, &Key);
		if (const ACampaign1851ConstructionSite* Site = Map->FindBuilding(Map->FindCity(Town), Key))
		{
			if (Site->IsModuleDone(0))
			{
				FocusSite(Site);
				FocusBuildingOrder.Reset();
			}
		}
	}

	// Slate owns the name field; letter keys must not pan the campaign map.
	if (Overlay.IsValid() && Overlay->bEditingUnitName) { return; }
	FVector2D Pan = FVector2D::ZeroVector;
	if (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up))    { Pan.Y += 1.f; }
	if (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down))  { Pan.Y -= 1.f; }
	if (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right)) { Pan.X += 1.f; }
	if (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left))  { Pan.X -= 1.f; }
	Camera->Pan(Pan, DeltaTime);

	// Drag pan: keep the ground point under the cursor.
	float MX = 0.f, MY = 0.f;
	PointerPosition(MX, MY);
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
		if (Overlay->bEditingUnitUniform) { Overlay->CloseUnitCustomisation(); }
		else if (Overlay->GetWindow() != SCampaign1851Overlay::EWindow::None)
		{
			Overlay->OpenWindow(SCampaign1851Overlay::EWindow::None);
		}
		else if (Overlay->GetFortPlacing() != 0)
		{
			Overlay->SetFortPlacing(0);
		}
		else
		{
			Overlay->SetSelectedRegiments({});
			Overlay->SelectFort(0);
		}
	}

	FVector Focus;
	const bool bFocus = CursorGround(Focus);
	const bool bOverTree = Overlay.IsValid() && Overlay->IsOverTree(Mouse);
	const bool bOverChart = Overlay.IsValid() && Overlay->IsOverChart(Mouse);
	const bool bShift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
	if (bOverChart && WasInputKeyJustPressed(EKeys::MouseScrollUp))   { Overlay->ScrollChart(-2, bShift); }
	if (bOverChart && WasInputKeyJustPressed(EKeys::MouseScrollDown)) { Overlay->ScrollChart(2, bShift); }
	const bool bOverBuildings = Overlay.IsValid() && Overlay->IsOverBuildings(Mouse);
	const bool bOverCard = Overlay.IsValid() && Overlay->IsOverUnitCard(Mouse);
	if (bOverCard && WasInputKeyJustPressed(EKeys::MouseScrollUp))   { Overlay->ScrollStack(-1); }
	if (bOverCard && WasInputKeyJustPressed(EKeys::MouseScrollDown)) { Overlay->ScrollStack(1); }
	if (!bOverChart && !bOverCard && WasInputKeyJustPressed(EKeys::MouseScrollUp))   { if (bOverBuildings) { Overlay->ScrollBuildings(-1); } else if (bOverTree) { Overlay->ScrollTree(-3); } else { Camera->Zoom(1.f, bFocus ? &Focus : nullptr); } }
	if (!bOverChart && !bOverCard && WasInputKeyJustPressed(EKeys::MouseScrollDown)) { if (bOverBuildings) { Overlay->ScrollBuildings(1); } else if (bOverTree) { Overlay->ScrollTree(3); } else { Camera->Zoom(-1.f, bFocus ? &Focus : nullptr); } }
	if (WasInputKeyJustPressed(EKeys::K) && Overlay.IsValid()) { Overlay->ToggleOOB(); }
	if (WasInputKeyJustPressed(EKeys::F) && Overlay.IsValid()) { Overlay->ToggleSupplyMap(); }
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

	if (Overlay.IsValid() && Overlay->IsScrollDragging())
	{
		if (IsInputKeyDown(EKeys::LeftMouseButton)) { Overlay->DragScrollTo(Mouse); } else { Overlay->EndScrollDrag(); }
	}
	if (LeftJustPressed() && Overlay.IsValid() && Overlay->IsMenuOpen())
	{
		int32 Row = INDEX_NONE;
		SCampaign1851Overlay::EButton Button = Overlay->HitButton(Mouse, &Row);
		if (Button == SCampaign1851Overlay::EButton::StartMenu)
		{
			SaveToSlot(TEXT("Autosave"), true);
			bCampaignStarted = false;
			Map->SetSpeed(0);
			OpenGameMenu();
			Overlay->ShowStartMenu();
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::SaveSlot && SaveSlots().IsValidIndex(Row))
		{
			SaveToSlot(SaveSlots()[Row]);
			OpenGameMenu();   // refresh the rows
		}
		else if (Button == SCampaign1851Overlay::EButton::LoadSlot && SaveSlots().IsValidIndex(Row))
		{
			if (LoadFromSlot(SaveSlots()[Row])) { Overlay->CloseMenu(); }
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
	else if (LeftJustPressed())
	{
		int32 Module = INDEX_NONE;
		SCampaign1851Overlay::EButton Button = Overlay.IsValid() ? Overlay->HitButton(Mouse, &Module) : SCampaign1851Overlay::EButton::None;
		// The confirmation dialog: JA carries out the step asked about, NEJ drops it.
		bool bConfirmed = false;
		if (Button == SCampaign1851Overlay::EButton::ConfirmYes)
		{
			Button = Overlay->TakeConfirm(Module);
			bConfirmed = true;
		}
		else if (Button == SCampaign1851Overlay::EButton::ConfirmNo)
		{
			Overlay->CloseConfirm();
			Button = SCampaign1851Overlay::EButton::Block;
		}
		// A step that costs money or cannot be undone: first what it does and what it costs, then JA / NEJ.
		if (!bConfirmed && Map.IsValid() && Overlay.IsValid())
		{
			FString Title, Text;
			if (DescribeAction(*Map, *Overlay, Button, Module, Title, Text))
			{
				Overlay->AskConfirm(Title, Text, Button, Module);
				Button = SCampaign1851Overlay::EButton::Block;
			}
		}
		if (Button == SCampaign1851Overlay::EButton::UnitDeployEarly)
		{
			if (Map->DeployRaisedUnit(Module)) { Overlay->ShowToast(TEXT("Enheden kan nu marchere med sin nuværende uddannelse")); }
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::RaisingPage)
		{
			Overlay->RaisingPageIndex += Module;
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::TransferAdj)
		{
			const int32 Cur = Overlay->GetTransferCount();
			const int32 Reg = Overlay->GetTransferReg(), From = Overlay->GetTransferFrom(), To = Overlay->GetTransferTo();
			if (Module == 5000) { Overlay->SetTransferCount(Overlay->GetTransferMax()); }
			else if (Module == 5001) { Overlay->SetTransferCount((Map->CompanyMen(Reg, From) - Map->CompanyMen(Overlay->GetTransferToReg(), To)) / 2); }
			else { Overlay->SetTransferCount(Cur + (Module - 1000)); }
			Button = SCampaign1851Overlay::EButton::Block;   // handled: the click must not fall through to the map
		}
		if (Button == SCampaign1851Overlay::EButton::TransferYes)
		{
			FString Why;
			Map->TransferCompanyMen(Overlay->GetTransferReg(), Overlay->GetTransferFrom(), Overlay->GetTransferToReg(), Overlay->GetTransferTo(), Overlay->GetTransferCount(), &Why);
			Overlay->CloseTransfer();
			Overlay->ShowToast(Why);
			Button = SCampaign1851Overlay::EButton::Block;   // handled: the click must not fall through to the map
		}
		if (Button == SCampaign1851Overlay::EButton::TransferGun)
		{
			FString Why;
			Map->TransferSectionGuns(Overlay->GetTransferReg(), Overlay->GetTransferFrom(), Overlay->GetTransferToReg(), Overlay->GetTransferTo(), 1, &Why);
			Overlay->ShowToast(Why);
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::TransferWhole)
		{
			FString Why;
			int32 NewTo = Overlay->GetTransferToReg();
			const bool bMoved = Map->MoveCompany(Overlay->GetTransferReg(), Overlay->GetTransferFrom(), Overlay->GetTransferToReg(), &Why, &NewTo);
			Overlay->CloseTransfer();
			Overlay->ShowToast(bMoved ? FString::Printf(TEXT("Hele enheden går over til %s"), *Map->GetRegiments()[NewTo].Name) : Why);
			Button = SCampaign1851Overlay::EButton::Block;   // handled: the click must not fall through to the map
		}
		if (Button == SCampaign1851Overlay::EButton::TransferNo)
		{
			Overlay->CloseTransfer();
			Button = SCampaign1851Overlay::EButton::Block;   // handled: the click must not fall through to the map
		}
		if (Button == SCampaign1851Overlay::EButton::Ransom)
		{
			FString Why;
			Map->RansomOfficer(Module, &Why);
			Overlay->ShowToast(Why);
			Button = SCampaign1851Overlay::EButton::Block;   // handled: the click must not fall through to the map
		}
		if (Button == SCampaign1851Overlay::EButton::StackList)
		{
			Overlay->ToggleStackList();
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::ScrollBarV || Button == SCampaign1851Overlay::EButton::ScrollBarH)
		{
			Overlay->BeginScrollDrag(Button == SCampaign1851Overlay::EButton::ScrollBarV ? (Module == 1 ? 3 : 1) : 2, Mouse);
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::ResearchTab)
		{
			Overlay->SetResearchTab(Module);
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::UnitRename)
		{
			Overlay->BeginUnitRename(Module);
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::UnitUniform)
		{
			Overlay->OpenUnitUniform(Module);
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::UnitUniformSwatch)
		{
			Overlay->ChooseUnitUniform(Module);
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::UnitCustomClose)
		{
			Overlay->CloseUnitCustomisation(Module == 1);
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::UnitUpgrade)
		{
			Map->UpgradeUnitWeapon(Module);
			Overlay->ShowToast(Map->UnitUpgradeDescription(Module));
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::UnitCardPart)
		{
			Overlay->SetUnitCardCompany(Module - 1);
			Button = SCampaign1851Overlay::EButton::Block;   // handled: the click must not fall through to the map
		}
		if (Button == SCampaign1851Overlay::EButton::PoolFold)
		{
			Overlay->TogglePoolOpen(Module);
			Button = SCampaign1851Overlay::EButton::Block;   // handled: the click must not fall through to the map
		}
		if (Button == SCampaign1851Overlay::EButton::FormationInsertHQ)
		{
			const int32 OOBNewHQ = Map->InsertFormationHQ(Module / 10, ECampaign1851Echelon(Module % 10));
			const int32 OOBNewIndex = Map->FormationIndex(OOBNewHQ);
			Overlay->ShowToast(OOBNewIndex != INDEX_NONE ? FString::Printf(TEXT("%s oprettet; underordnede er flyttet ind under staben"), *Map->GetFormations()[OOBNewIndex].Name) : FString(TEXT("Dette HQ passer ikke ind i hierarkiet")));
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::FormationChiefRemove)
		{
			if (Map->RemoveFormationCommander(Module))
			{
				Overlay->ShowToast(TEXT("Chefen er tilbage i officerspuljen"));
			}
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::FormationDissolve)
		{
			Map->DissolveFormation(Module);
			Overlay->ShowToast(TEXT("Hovedkvarteret er opløst; underordnede enheder er flyttet et niveau op"));
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::EqualizeUnit)
		{
			FString Why;
			Map->EqualizeCompanies(Module, &Why);
			Overlay->ShowToast(Why);
			Button = SCampaign1851Overlay::EButton::Block;   // handled: the click must not fall through to the map
		}
		if (Button == SCampaign1851Overlay::EButton::TestBattle)
		{
			// A test battle on the generated field: the campaign is saved, the battle map opens on that field file and
			// comes back here (the result is not used by the campaign).
			if (Map->GetBattlefield().IsValid())
			{
				SaveToSlot(TEXT("Autosave"), true);
				if (GEngine && GEngine->GameViewport && Overlay.IsValid())
				{
					GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
				}
				UGameplayStatics::OpenLevel(this, FName(TEXT("Strategy1864_Field")), true, FString::Printf(TEXT("Field=%s"), *FPaths::GetCleanFilename(Map->BattlefieldFile())));
				return;
			}
			Button = SCampaign1851Overlay::EButton::Block;
		}
		if (Button == SCampaign1851Overlay::EButton::Block)
		{
			// A card in the front: the click stays on it.
		}
		else if (Button == SCampaign1851Overlay::EButton::MinisterInfo)
		{
			Overlay->SetMinisterInfo(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::Menu)
		{
			OpenGameMenu();
		}
		else if (Button == SCampaign1851Overlay::EButton::MergeUnit)
		{
			FString Why;
			const int32 Joined = Map->MergeRegiments(Module / 10000, Module % 10000, &Why);
			if (Joined != INDEX_NONE)
			{
				SelectRegiments({ Joined });
				Overlay->OpenWindow(SCampaign1851Overlay::EWindow::Chart);
				Overlay->FilterOOB({ Joined });
				Overlay->SetOOBBuilding(INDEX_NONE);
				Overlay->ShowToast(FString::Printf(TEXT("%s er samlet igen"), *Map->GetRegiments()[Joined].Name));
				SaveToSlot(TEXT("Autosave"), true);
			}
			else
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::HorseBattery)
		{
			if (!bConfirmed)
			{
				Overlay->AskConfirm(TEXT("Gør batteriet ridende?"), FString::Printf(TEXT("Alle kanonerer kommer til hest, og batteriet kan følge rytteriet. Det får 6 kanoner (2 går tilbage på lager), 180 mand og %d heste (de manglende tages fra lageret). Det koster %s rd., og eksercitsen falder en tid, mens mændene lærer at ride med kanonerne."),
					ACampaign1851Map::HorseBatteryHorses, *FString::FormatAsNumber(int32(ACampaign1851Map::HorseBatteryCost))), Button, Module);
			}
			else
			{
				FString Why;
				Overlay->ShowToast(Map->UpgradeToHorseBattery(Module, &Why) ? FString(TEXT("Batteriet er nu ridende")) : Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::UnitCard)
		{
			Overlay->ToggleUnitCard();
		}
		else if (Button == SCampaign1851Overlay::EButton::BuildingInfo)
		{
			Overlay->ToggleBuildingInfo(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::BuildingScroll)
		{
			Overlay->ScrollBuildings(Module - 1);
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
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-CLICK: counter of regiment %d, stack of %d"), Module, Stack.Num());
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
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-CLICK: after SelectRegiments the selection is %d"), Overlay->GetSelectedRegiments().Num());
		}
		else if (Button == SCampaign1851Overlay::EButton::RegimentPiece)
		{
			// Close in: the miniature is the regiment itself; shift-click adds or removes it.
			TArray<int32> Sel = Overlay->GetSelectedRegiments();
			if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
			{
				if (Sel.Contains(Module)) { Sel.Remove(Module); } else { Sel.Add(Module); }
			}
			else
			{
				Sel = { Module };
			}
			SelectRegiments(Sel);
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
				if (Map->CanAssignFormationPost(Module, Overlay->GetPickerFormation(), Overlay->GetPickerPost()) && Map->AssignFormationStaff(Module, Overlay->GetPickerFormation(), Overlay->GetPickerPost()))
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
			const bool OOBFormationPicker = Overlay->GetPicker() == SCampaign1851Overlay::EPicker::FormationGeneral || Overlay->GetPicker() == SCampaign1851Overlay::EPicker::FormationOfficer;
			const int32 New = Map->RecruitOfficer(Module == 1, OOBFormationPicker ? Map->FormationPostRank(Overlay->GetPickerFormation(), Overlay->GetPickerPost()) : nullptr);
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
			case SCampaign1851Overlay::CloseFortPanel: Overlay->HideFortTool(); Overlay->SetFortPlacing(0); break;
			case SCampaign1851Overlay::CloseFort: Overlay->SelectFort(0); break;
			default:
				Overlay->SelectBridge(0);
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
		else if (Button == SCampaign1851Overlay::EButton::Demolish)
		{
			// First click arms it (the button asks to confirm), the second pulls down.
			if (Overlay->GetDemolishArmed() != Module)
			{
				Overlay->ArmDemolish(Module);
			}
			else
			{
				Overlay->ArmDemolish(0);
				FString Why;
				bool bOk = false;
				if (Module >= 1000000)
				{
					bOk = Map->DemolishFort(Module - 1000000, &Why);
				}
				else
				{
					const int32 City = Module / 100, Kind = Module % 100;
					ACampaign1851ConstructionSite* Site = Kind == 99 ? Map->FindProject(City)
						: ACampaign1851ConstructionSite::TownBuildings().IsValidIndex(Kind) ? Map->FindBuilding(City, ACampaign1851ConstructionSite::TownBuildings()[Kind].Key) : nullptr;
					bOk = Map->DemolishSite(Site, &Why);
				}
				Overlay->ShowToast(bOk ? FString(TEXT("Nedrivningen er begyndt")) : Why);
				if (bOk)
				{
					SaveToSlot(TEXT("Autosave"), true);
				}
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::SupplySend)
		{
			FString Why;
			const bool bFort = Module >= 1000000;
			if (Map->SendSupplyColumn(bFort, bFort ? Module - 1000000 : Module, &Why))
			{
				Overlay->ShowToast(TEXT("Trænkolonnen kører fra depotet"));
			}
			else
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::BattleFight3D)
		{
			// The battle in 3D (the Strategy1864 battle in this project): the request, the units and the field are
			// written, the campaign saved; the battle map opens and comes back here with BattleResult_N.json.
			if (Map->FightBattleIn3D(Module))
			{
				SaveToSlot(TEXT("Autosave"), true);
				if (GEngine && GEngine->GameViewport && Overlay.IsValid())
				{
					GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
				}
				UGameplayStatics::OpenLevel(this, FName(TEXT("Strategy1864_Field")), true, FString::Printf(TEXT("Battle=%d"), Module));
				return;
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::BattleAuto)
		{
			Map->AutoResolveBattle(Module);
			SaveToSlot(TEXT("Autosave"), true);
		}
		else if (Button == SCampaign1851Overlay::EButton::BattleRetreat)
		{
			Map->RetreatFromBattle(Module);
			SaveToSlot(TEXT("Autosave"), true);
		}
		else if (Button == SCampaign1851Overlay::EButton::Diplomacy)
		{
			FString Why;
			const bool bDone = Map->DoDiplomacy(Module / 10, ACampaign1851Map::EDiplomacyAction(Module % 10), &Why);
			Overlay->ShowToast(bDone ? FString(TEXT("Udført")) : Why);
		}
		else if (Button == SCampaign1851Overlay::EButton::Loan)
		{
			FString Why;
			if (Module == 2)
			{
				Overlay->ShowToast(Map->RepayLoan(100000.0) ? FString(TEXT("Afdrag betalt")) : FString(TEXT("Ingen gæld eller ikke råd")));
			}
			else
			{
				Overlay->ShowToast(Map->TakeLoan(Module == 0 ? 100000.0 : 250000.0, &Why) ? FString(TEXT("Lånet er optaget")) : Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::OpenGazette)
		{
			Overlay->OpenWindow(Overlay->GetWindow() == SCampaign1851Overlay::EWindow::Gazette ? SCampaign1851Overlay::EWindow::None : SCampaign1851Overlay::EWindow::Gazette);
		}
		else if (Button == SCampaign1851Overlay::EButton::GazetteTab)
		{
			Overlay->SetGazetteTab(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::ShipOrder)
		{
			FString Why;
			Overlay->ShowToast(Map->OrderShip(Module, &Why) ? FString::Printf(TEXT("%s bestilt"), Campaign1851Navy::Classes()[Module].Name) : Why);
		}
		else if (Button == SCampaign1851Overlay::EButton::Blockade)
		{
			Map->SetBlockade(Module == 1);
		}
		else if (Button == SCampaign1851Overlay::EButton::ResearchStart)
		{
			FString Why;
			Overlay->ShowToast(Map->StartResearch(Module, &Why) ? FString::Printf(TEXT("Forskning påbegyndt: %s"), Campaign1851Research::Topics()[Module].Name) : Why);
		}
		else if (Button == SCampaign1851Overlay::EButton::DoctrineSet)
		{
			FString Why;
			if (Map->SetDoctrine(Module / 10, Module % 10, &Why))
			{
				Overlay->ShowToast(FString::Printf(TEXT("Ny doktrin: %s"), Campaign1851Research::DoctrineName(Module / 10, Module % 10)));
			}
			else if (!Why.IsEmpty())
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::MakePeace)
		{
			FString Why;
			if (Map->MakePeace(Module, &Why))
			{
				Overlay->ShowToast(TEXT("Fred sluttet"));
				SaveToSlot(TEXT("Autosave"), true);
			}
			else
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::Footing && Map->GetFooting() == ECampaign1851Footing::Peace && !bConfirmed)
		{
			// First the question: what it costs and what it does.
			int32 Away = 0;
			for (const FCampaign1851Regiment& R : Map->GetRegiments())
			{
				Away += FMath::RoundToInt(R.Men * (1.f - R.Present));
			}
			Overlay->AskConfirm(TEXT("Mobilisér hæren?"), FString::Printf(
				TEXT("Indkaldelsen koster %s rd. nu. %s hjemsendte kaldes ind over 2-3 uger (hurtigere med mobiliseringsdepot i garnisonsbyen), og lønnen stiger med ca. %s rd. om måneden. Skatterne falder 10 %% mens mændene er væk fra gårde og værksteder; stemningen falder, og spændingen med Det tyske forbund stiger. Kassen har %s rd."),
				*FString::FormatAsNumber(int32(Campaign1851Mobilisation::OrderCost)), *FString::FormatAsNumber(Away),
				*FString::FormatAsNumber(FMath::RoundToInt(Away * Campaign1851Mobilisation::PayPerManMonth)), *FString::FormatAsNumber(FMath::RoundToInt(Map->GetTreasury()))),
				SCampaign1851Overlay::EButton::Footing, 0);
		}
		else if (Button == SCampaign1851Overlay::EButton::Footing)
		{
			FString Why;
			if (Map->GetFooting() == ECampaign1851Footing::Peace)
			{
				Overlay->ShowToast(Map->Mobilise(&Why) ? FString(TEXT("Mobilisering beordret: de hjemsendte kaldes ind")) : Why);
			}
			else
			{
				Map->Demobilise();
				Overlay->ShowToast(TEXT("Hjemsendelse beordret"));
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::SupplyMap)
		{
			Overlay->ToggleSupplyMap();
			if (Overlay->IsSupplyMap())
			{
				Overlay->OpenWindow(SCampaign1851Overlay::EWindow::None);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::SupplyBuy)
		{
			Overlay->ShowToast(Map->BuySupplyColumn() ? FString::Printf(TEXT("Trænkolonne købt: %d i alt"), Map->GetSupplyColumnCount()) : FString(TEXT("Ikke råd")));
		}
		else if (Button == SCampaign1851Overlay::EButton::RaiseBattalion)
		{
			FString Why;
			const int32 New = Map->RaiseBattalion(Overlay->GetSelectedCity(), &Why);
			if (New != INDEX_NONE)
			{
				Overlay->ShowToast(FString::Printf(TEXT("%s oprettet: 760 rekrutter, en major og fire kaptajner"), *Map->GetRegiments()[New].Name));
				SaveToSlot(TEXT("Autosave"), true);
			}
			else
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::FortTool)
		{
			Overlay->ToggleFortTool();
			Overlay->SetFortPlacing(0);
		}
		else if (Button == SCampaign1851Overlay::EButton::FortChoose)
		{
			Overlay->SetFortPlacing(Overlay->GetFortPlacing() == Module ? 0 : Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::FortSelect || Button == SCampaign1851Overlay::EButton::FortShow)
		{
			Overlay->SelectFort(Module);
			const int32 Index = Map->FortIndex(Module);
			if (Index != INDEX_NONE)
			{
				Camera->SetView(Map->FortWorld(Index), 3.f, Camera->GetYaw());
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::FortPickCompany)
		{
			Overlay->ToggleFortPickCompany();
		}
		else if (Button == SCampaign1851Overlay::EButton::FortAddCompany || Button == SCampaign1851Overlay::EButton::FortReturn)
		{
			FString Why;
			const bool bAdd = Button == SCampaign1851Overlay::EButton::FortAddCompany;
			if (bAdd ? Map->AddFortCompany(Overlay->GetSelectedFort(), Module / 10, Module % 10, &Why) : Map->ReturnFortCompany(Overlay->GetSelectedFort(), Module, &Why))
			{
				SaveToSlot(TEXT("Autosave"), true);
			}
			else
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::FortGuns || Button == SCampaign1851Overlay::EButton::FortDefence || Button == SCampaign1851Overlay::EButton::FortTrenches)
		{
			FString Why;
			if (Map->UpgradeFort(Module, Button == SCampaign1851Overlay::EButton::FortGuns ? ECampaign1851FortWork::Guns
				: Button == SCampaign1851Overlay::EButton::FortTrenches ? ECampaign1851FortWork::Trenches : ECampaign1851FortWork::Defence, &Why))
			{
				SaveToSlot(TEXT("Autosave"), true);
			}
			else
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::FortTurn)
		{
			Map->TurnFort(Overlay->GetSelectedFort(), Module * 22.5f);
		}
		else if (Button == SCampaign1851Overlay::EButton::TownBuildingsTab)
		{
			Overlay->SetCivilTab(Module == 1);
		}
		else if (Button == SCampaign1851Overlay::EButton::Delegate)
		{
			Map->SetDelegation(ECampaign1851Portfolio(Module / 3), ECampaign1851Delegation(Module % 3));
			Overlay->ShowToast(FString::Printf(TEXT("%s: %s"), Campaign1851Nations::PortfolioName(ECampaign1851Portfolio(Module / 3)), Campaign1851Nations::DelegationName(ECampaign1851Delegation(Module % 3))));
		}
		else if (Button == SCampaign1851Overlay::EButton::Reserve && Map->GetNations().IsValidIndex(Map->GetPlayerNation()))
		{
			Map->SetReserve(Map->GetNations()[Map->GetPlayerNation()].Reserve + (Module == 1 ? 50000.0 : -50000.0));
		}
		else if (Button == SCampaign1851Overlay::EButton::DecisionExecute)
		{
			Overlay->ShowToast(Map->ExecuteDecision(Module) ? FString::Printf(TEXT("Udført: %s"), *Map->GetDecisions()[Module].Action) : FString(TEXT("Kan ikke udføres længere")));
		}
		else if (Button == SCampaign1851Overlay::EButton::Deviation)
		{
			Map->NewGameDeviation = Module / 100.f;
		}
		else if (Button == SCampaign1851Overlay::EButton::NewGameNation)
		{
			Map->NewGameNation = Module == 1 ? TEXT("SE") : TEXT("DK");
		}
		else if (Button == SCampaign1851Overlay::EButton::BridgeSelect)
		{
			Overlay->SelectBridge(Overlay->GetSelectedBridge() == Module ? 0 : Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::BridgeDo)
		{
			FString Why;
			const bool bDone = Map->BridgeAction(Module / 10, EBridgeAction(Module % 10), &Why);
			if (!bDone)
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::OpenMateriel)
		{
			Overlay->OpenWindow(Overlay->GetWindow() == SCampaign1851Overlay::EWindow::Materiel ? SCampaign1851Overlay::EWindow::None : SCampaign1851Overlay::EWindow::Materiel);
		}
		else if (Button == SCampaign1851Overlay::EButton::RawBuy)
		{
			const ECampaign1851Raw R = ECampaign1851Raw(Module / 10);
			const float Step = R == ECampaign1851Raw::Cloth || R == ECampaign1851Raw::Leather ? 100.f : 10.f;
			FString Why;
			if (!Map->BuyRaw(R, Step * (Module % 10 == 0 ? 1.f : 10.f), &Why))
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::ForeignTab)
		{
			Overlay->ForeignTab = Module;
		}
		else if (Button == SCampaign1851Overlay::EButton::ResearchPick)
		{
			Overlay->ResearchPick = Overlay->ResearchPick == Module ? -1 : Module;
		}
		else if (Button == SCampaign1851Overlay::EButton::KitBuy)
		{
			FString Why;
			if (!Map->BuyKit(Module == 1, Module == 1 ? 2 : 10, &Why))
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::UnitSize)
		{
			Overlay->RaiseSize = (Overlay->RaiseSize + Module + 3) % 3;
		}
		else if (Button == SCampaign1851Overlay::EButton::UnitType)
		{
			Overlay->RaiseType = Module;
		}
		else if (Button == SCampaign1851Overlay::EButton::UnitTown)
		{
			Overlay->RaiseTownPick += Module;
		}
		else if (Button == SCampaign1851Overlay::EButton::UnitCommand)
		{
			Overlay->RaiseCommand += Module;
		}
		else if (Button == SCampaign1851Overlay::EButton::UnitProgram)
		{
			const int32 N = int32(ECampaign1851Program::Count);
			Overlay->RaiseProgram = ((Overlay->RaiseProgram + Module) % N + N) % N;
		}
		else if (Button == SCampaign1851Overlay::EButton::UnitRaise)
		{
			const TArray<int32> Towns = Map->RaiseTowns();
			const int32 Town = Towns.Num() > 0 ? Towns[((Overlay->RaiseTownPick % Towns.Num()) + Towns.Num()) % Towns.Num()] : INDEX_NONE;
			const int32 NC = Map->GetCommands().Num();
			const int32 Command = NC > 0 ? ((Overlay->RaiseCommand % NC) + NC) % NC : INDEX_NONE;
			FString Why;
			const int32 New = Map->RaiseUnit(Overlay->RaiseType, Town, Command, ECampaign1851Program(Overlay->RaiseProgram), &Why, Overlay->RaiseSize);
			Overlay->ShowToast(New != INDEX_NONE ? FString::Printf(TEXT("%s er oprettet"), *Map->GetRegiments()[New].Name) : Why);
		}
		else if (Button == SCampaign1851Overlay::EButton::OpenBattlefield)
		{
			const bool bOpen = Overlay->GetWindow() == SCampaign1851Overlay::EWindow::Battlefield;
			if (!bOpen && !Map->GetBattlefield().IsValid())
			{
				Map->GenerateBattlefield(Map->KmAtWorld(Camera->GetTarget()), Map->BattlefieldSizeKm, TEXT("Kort"));
			}
			Overlay->OpenWindow(bOpen ? SCampaign1851Overlay::EWindow::None : SCampaign1851Overlay::EWindow::Battlefield);
		}
		else if (Button == SCampaign1851Overlay::EButton::BattlefieldSize)
		{
			Map->BattlefieldSizeKm = float(Module);
			if (Map->GetBattlefield().IsValid())
			{
				Map->GenerateBattlefield(Map->GetBattlefield().CentreKm, Map->BattlefieldSizeKm, Map->GetBattlefield().Name);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::BattlefieldHere)
		{
			Map->GenerateBattlefield(Map->KmAtWorld(Camera->GetTarget()), Map->BattlefieldSizeKm, TEXT("Kort"));
			Overlay->ShowToast(FString::Printf(TEXT("Slagmarken ved %s er bygget: Saved/Battle/%s"), *Map->GetBattlefield().Place, *Map->BattlefieldFile()));
		}
		else if (Button == SCampaign1851Overlay::EButton::BattlefieldAtBattle)
		{
			const FCampaign1851Battle* Bt = Map->GetBattles().FindByPredicate([Module](const FCampaign1851Battle& X) { return X.Id == Module; });
			if (Bt)
			{
				Map->GenerateBattlefield(Bt->Km, Map->BattlefieldSizeKm, FString::Printf(TEXT("Battle_%d"), Bt->Id));
				Overlay->OpenWindow(SCampaign1851Overlay::EWindow::Battlefield);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::DelegateAll)
		{
			Map->SetAllDelegation(ECampaign1851Delegation(Module));
			Overlay->ShowToast(FString::Printf(TEXT("Alle ressorter: %s"), Campaign1851Nations::DelegationName(ECampaign1851Delegation(Module))));
		}
		else if (Button == SCampaign1851Overlay::EButton::MinisterDismiss)
		{
			Overlay->SetMinisterPick(Overlay->GetMinisterPick() == Module ? -1 : Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::MinisterAppoint)
		{
			if (Map->AppointMinister(Module / 10, Module % 10))
			{
				Overlay->ShowToast(FString::Printf(TEXT("Ny minister: %s"), *Map->GetMinister(ECampaign1851Portfolio(Module / 10)).Name));
			}
			Overlay->SetMinisterPick(-1);
		}
		else if (Button == SCampaign1851Overlay::EButton::MinisterPickClose)
		{
			Overlay->SetMinisterPick(-1);
		}
		else if (Button == SCampaign1851Overlay::EButton::NationWeight)
		{
			Map->AdjustNationWeight(Module / 2, Module % 2 == 1 ? 0.1f : -0.1f);
		}
		else if (Button == SCampaign1851Overlay::EButton::FormationDeputy || Button == SCampaign1851Overlay::EButton::FormationStaff)
		{
			// Staff posts are filled by officers (a division's deputy is typically its senior colonel).
			Overlay->OpenFormationPicker(Module, false, Button == SCampaign1851Overlay::EButton::FormationDeputy ? 1 : 2);
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
			// The big order-of-battle window with only the selected units (splitting is done in there).
			// The garrison and the army where the chosen unit stands (a unit on the march: just its own tree); never the previous view.
			Overlay->OpenWindow(SCampaign1851Overlay::EWindow::Chart);
			const TArray<int32>& OOBChosen = Overlay->GetSelectedRegiments();
			const int32 OOBTown = OOBChosen.Num() > 0 && Map->GetRegiments().IsValidIndex(OOBChosen[0]) ? Map->GetRegiments()[OOBChosen[0]].Town : INDEX_NONE;
			if (OOBTown != INDEX_NONE) { Overlay->SetOOBPlace(OOBTown); } else { Overlay->FilterOOB(OOBChosen); }
		}
		else if (Button == SCampaign1851Overlay::EButton::Engage)
		{
			FString Why;
			if (!Map->EngageCorps(Module, Overlay->GetSelectedRegiments(), &Why))
			{
				Overlay->ShowToast(Why);
			}
		}
		else if (Button == SCampaign1851Overlay::EButton::MapView)
		{
			Overlay->SetMapView(Module);
		}
		else if (Button == SCampaign1851Overlay::EButton::OOBFocusClear)
		{
			Overlay->ClearOOBView();
		}
		else if (Button == SCampaign1851Overlay::EButton::SplitUnit)
		{
			if (!bConfirmed)
			{
				Overlay->AskConfirm(TEXT("Del enheden i to?"), TEXT("Halvdelen af kompagnierne med deres kaptajner, mænd, heste og kanoner bliver en halvbataljon for sig, der hvor enheden står. Begge dele mister lidt samhørighed, og den nye enhed skal have en chef. Delingen kan gøres om igen: SAML IGEN i kamporden."),
					Button, Module);
			}
			else
			{
				FString Why;
				const int32 New = Map->SplitRegiment(Module, &Why);
				Overlay->ShowToast(New != INDEX_NONE ? FString::Printf(TEXT("Enheden er delt: %s"), *Map->GetRegiments()[New].Name) : Why);
				if (New != INDEX_NONE)
				{
					// Both halves side by side in the big order of battle, to move companies between.
					Overlay->OpenWindow(SCampaign1851Overlay::EWindow::Chart);
					TArray<int32> Both = Overlay->GetOOBFilter();
					Both.AddUnique(Module);
					Both.AddUnique(New);
					Overlay->FilterOOB(Both);
				}
			}
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
			Overlay->ShowToast(Map->OrderTroopTrain(Module) ? FString::Printf(TEXT("Togsæt bestilt i England til %s"), Map->GetCities().IsValidIndex(Module) ? *Map->GetCities()[Module].Name : TEXT("København"))
				: FString(TEXT("Ikke råd i statskassen")));
		}
		else if (Button == SCampaign1851Overlay::EButton::BattleViewEnter)
		{
			SetBattleView(true);
		}
		else if (Button == SCampaign1851Overlay::EButton::BattleViewLeave)
		{
			SetBattleView(false);
		}
		else if (Button == SCampaign1851Overlay::EButton::MinistryBudget)
		{
			Map->StepMinistryBudget(ECampaign1851Portfolio(Module / 2), Module % 2 == 1 ? 1 : -1);
		}
		else if (Button == SCampaign1851Overlay::EButton::TrainMove)
		{
			FString Why;
			const int32 Train = Module / 1000, To = Module % 1000;
			Overlay->ShowToast(Map->TransferTrain(Train, To, &Why) ? FString::Printf(TEXT("Toget skibes til %s"), *Map->GetCities()[To].Name) : FString::Printf(TEXT("Kan ikke flyttes: %s"), *Why));
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
		else if (Overlay.IsValid() && Overlay->GetWindow() == SCampaign1851Overlay::EWindow::None && Overlay->GetFortPlacing() != 0)
		{
			// Placing a fort: its front faces south, towards the Eider and the German lands (turn it afterwards).
			FVector Ground;
			if (CursorGround(Ground))
			{
				FString Why;
				const int32 Id = Map->StartFort(Map->KmAtWorld(Ground), Overlay->GetFortPlacing() == 2, 90.f, &Why);
				if (Id != INDEX_NONE)
				{
					Overlay->SetFortPlacing(0);
					Overlay->SelectFort(Id);
					Overlay->ShowToast(FString::Printf(TEXT("%s påbegyndt"), *Map->GetForts()[Map->FortIndex(Id)].Name));
					SaveToSlot(TEXT("Autosave"), true);
				}
				else
				{
					Overlay->ShowToast(Why);
				}
			}
		}
		else if (Overlay.IsValid() && Overlay->GetWindow() == SCampaign1851Overlay::EWindow::None && FortUnderCursor() != 0)
		{
			Overlay->SelectFort(FortUnderCursor());
		}
		else if (Overlay.IsValid() && Overlay->GetWindow() == SCampaign1851Overlay::EWindow::None)
		{
			Overlay->SelectFort(0);
			PickCity();
		}
	}

	if (Map->TakeEndPending() && Overlay.IsValid())
	{
		Map->SetSpeed(0);
		Overlay->OpenWindow(SCampaign1851Overlay::EWindow::End);
	}
	for (const FString& News : Map->TakeNews())
	{
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|news|%s"), *News);
		if (Overlay.IsValid())
		{
			Overlay->ShowToast(News);
		}
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CampaignDebugClicks")) && Overlay.IsValid() && bAutoClickFrame) { UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-CLICK: at the end of the tick the selection is %d"), Overlay->GetSelectedRegiments().Num()); }
	Map->UpdateMarkers(Camera->GetDistanceKm());
	LastCameraTarget = Camera->GetTarget();
	LastCameraDistanceKm = Camera->GetDistanceKm();
	LastCameraYaw = Camera->GetYaw();
}

int32 ACampaign1851PlayerController::FortUnderCursor() const
{
	float MX, MY;
	if (!Map.IsValid() || !GetMousePosition(MX, MY))
	{
		return 0;
	}
	// A fort under the cursor (only where the scenery shows): within its size on screen, at least 20 px.
	const TArray<FCampaign1851Fort>& Forts = Map->GetForts();
	int32 Best = 0;
	float BestScore = 1.f;
	for (int32 i = 0; i < Forts.Num(); ++i)
	{
		const FVector World = Map->FortWorld(i);
		FVector2D Screen, Edge;
		if (!ProjectWorldLocationToScreen(World, Screen, false))
		{
			continue;
		}
		float Radius = 20.f;
		if (ProjectWorldLocationToScreen(World + FVector((Forts[i].bLarge ? 14.0 : 9.0) * ACampaign1851Map::PieceScale, 0.0, 0.0), Edge, false))
		{
			Radius = FMath::Max(Radius, float(FVector2D::Distance(Screen, Edge)));
		}
		const float Score = FVector2D::Distance(Screen, FVector2D(MX, MY)) / Radius;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Forts[i].Id;
		}
	}
	return Best;
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
	if (!Map.IsValid() || !bCampaignStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|save|%s|skipped: no active campaign"), *Slot);
		return false;
	}
	const ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	UCampaign1851SaveGame* Save = Cast<UCampaign1851SaveGame>(UGameplayStatics::CreateSaveGameObject(UCampaign1851SaveGame::StaticClass()));
	if (Save) { Save->Scenario = ACampaign1851Map::ScenarioIndex(); }
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
		P.bPrivate = Site->IsPrivate();
		P.bHistoric = Site->IsHistoric();
		P.bDemolishing = Site->IsDemolishing();
		if (P.bDemolishing)
		{
			P.DemolishDays = Site->GetDemolishDays();
			P.DemolishWages = Site->GetDemolishWages();
			P.DemolishDone = Site->GetDemolishDone();
			P.DemolishFrom = Site->GetDemolishFrom();
		}
		if (Site->IsHistoric())
		{
			continue;   // the towns' own buildings of 1851 are not news in the summary
		}
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
	CaptureSettings(Save->Settings);
	Map->SaveWorld(Save);
	Save->Forts = Map->SaveForts();
	Save->AmtManpower = Map->GetAmtManpower();
	Save->GunStock = Map->GetGunStock();
	Save->Supply = Map->SaveSupply();
	Save->SupplyColumns = Map->SaveSupplyColumns();
	Save->Rifles = Map->GetRifles();
	Save->HorseStock = Map->GetHorseStock();
	Save->Footing = uint8(Map->GetFooting());
	Save->War = Map->SaveWar();
	Save->Diplomacy = Map->SaveDiplomacy();
	Save->Research = Map->SaveResearch();
	Save->Navy = Map->SaveNavy();
	Save->Politics = Map->SavePolitics();
	Save->Economy = Map->SaveEconomy();
	Save->Bridges = Map->SaveBridges();
	Map->ExportUnits();
	Save->MaterialLots = Map->GetMaterialLots();
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
	if (Save->Scenario != ACampaign1851Map::ScenarioIndex() && Save->Scenario >= 0 && Save->Scenario < ACampaign1851Map::Scenarios().Num())
	{
		GCampaignOfficerScenarioPendingSlot = Slot;
		GCampaignOfficerScenarioPendingBattle = bResumedFromBattle;
		// Do not overwrite the slot being loaded with the outgoing scenario in EndPlay.
		bCampaignStarted = false;
		ACampaign1851Map::SetScenarioIndex(Save->Scenario);
		UGameplayStatics::OpenLevel(this, FName(TEXT("Campaign1851")));
		return true; // restoration continues after the scenario's map data have loaded
	}
	bCampaignStarted = true;
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
	// The world from the save's seed (older saves: the 1851 world without deviation).
	Map->ResetWorld(Save->SaveVersion >= 12 ? Save->Seed : 1851, Save->SaveVersion >= 12 ? Save->Deviation : 0.f);
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
	if (Save->SaveVersion >= 30)
	{
		RestoreSettings(Save->Settings);
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
		ACampaign1851ConstructionSite* RestoredSite = P.Kind == TEXT("Garrison") || P.Kind.IsEmpty() ? Map->FindProject(Map->FindCity(P.City)) : Map->FindBuilding(Map->FindCity(P.City), P.Kind);
		if (RestoredSite && P.bPrivate)
		{
			RestoredSite->SetPrivate(true);
		}
		if (RestoredSite && P.bHistoric)
		{
			RestoredSite->SetHistoric(true);
		}
		if (RestoredSite && P.bDemolishing)
		{
			RestoredSite->RestoreDemolition(P.DemolishDays, P.DemolishWages, P.DemolishDone, P.DemolishFrom);
		}
	}
	if (Save->SaveVersion >= 12)
	{
		Map->RestoreWorld(Save);
	}
	Map->RestoreForts(Save->SaveVersion >= 13 ? Save->Forts : TArray<FCampaign1851FortSave>());
	if (Save->SaveVersion >= 15)
	{
		Map->SetAmtManpower(Save->AmtManpower);
	}
	Map->SetGunStock(Save->SaveVersion >= 16 ? Save->GunStock : 0);
	Map->ResetSupply();
	if (Save->SaveVersion >= 17)
	{
		Map->RestoreSupply(Save->Supply);
	}
	if (Save->SaveVersion >= 19)
	{
		Map->RestoreSupplyColumns(Save->SupplyColumns);
	}
	if (Save->SaveVersion >= 20)
	{
		Map->SetMateriel(Save->Rifles, Save->HorseStock);
	}
	else
	{
		const int32 Guns = Map->GetGunStock();
		Map->ResetMateriel();
		Map->SetGunStock(FMath::Max(Guns, Map->GetGunStock()));
	}
	Map->SetFooting(Save->SaveVersion >= 21 ? ECampaign1851Footing(FMath::Min<uint8>(Save->Footing, 2)) : ECampaign1851Footing::Peace);
	if (Save->SaveVersion >= 22)
	{
		Map->RestoreWar(Save->War);
	}
	if (Save->SaveVersion >= 23)
	{
		Map->RestoreDiplomacy(Save->Diplomacy);
		Map->RestoreResearch(Save->Research);
	}
	else
	{
		Map->ResetDiplomacy();
		Map->ResetResearch();
	}
	Map->RestoreNavy(Save->SaveVersion >= 24 ? Save->Navy : TArray<FString>());
	Map->RestorePolitics(Save->SaveVersion >= 25 ? Save->Politics : TArray<FString>());
	Map->RestoreEconomy(Save->SaveVersion >= 26 ? Save->Economy : TArray<FString>());
	Map->RestoreBridges(Save->SaveVersion >= 27 ? Save->Bridges : TArray<FString>());
	Map->SetMaterialLots(Save->SaveVersion >= 16 ? Save->MaterialLots : TArray<FVector>());
	Camera->SetView(Save->CameraTarget, Save->CameraDistanceKm, Save->CameraYaw);
	if (Overlay.IsValid())
	{
		Overlay->CloseMenu();
		Overlay->SetSelectedCity(Map->FindCity(Save->SelectedCity));
		Overlay->ShowToast(FString::Printf(TEXT("Indlæst  ·  %s  ·  gemt %s"), *SlotLabel(Slot), *Save->SavedAt.ToString(TEXT("%d-%m-%Y %H:%M"))));
	}
	Map->UpdateMarkers(Camera->GetDistanceKm());
	AutosaveTimer = 0.f;
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|load|%s|projects=%d/%d|links=%d/%d|version=%d"), *Slot, Restored, Save->Projects.Num(), LinksRestored, Save->Links.Num(), Save->SaveVersion);
	return true;
}

bool ACampaign1851PlayerController::PointerPosition(float& X, float& Y) const
{
	if (bAutoMouse)
	{
		X = float(AutoMouse.X);
		Y = float(AutoMouse.Y);
		return true;
	}
	return GetMousePosition(X, Y);
}

void ACampaign1851PlayerController::TickAutoClicks()
{
	if (!bAutoClicksParsed)
	{
		bAutoClicksParsed = true;
		FString Plan;
		if (FParse::Value(FCommandLine::Get(), TEXT("CampaignAutoClick="), Plan, false))
		{
			TArray<FString> Items;
			Plan.ParseIntoArray(Items, TEXT(";"));
			for (const FString& Item : Items)
			{
				TArray<FString> P;
				Item.ParseIntoArray(P, TEXT(":"));
				if (P.Num() == 3) { AutoClicks.Add(FVector(FCString::Atof(*P[0]), FCString::Atof(*P[1]), FCString::Atof(*P[2]))); }
			}
		}
	}
	const float Real = GetWorld()->GetRealTimeSeconds();
	bAutoClickFrame = false;
	if (AutoClicks.IsValidIndex(NextAutoClick) && Real >= AutoClicks[NextAutoClick].X)
	{
		AutoMouse = FVector2D(AutoClicks[NextAutoClick].Y, AutoClicks[NextAutoClick].Z);
		bAutoMouse = true;
		bAutoClickFrame = true;
		++NextAutoClick;
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-AUTOCLICK: click %d at %.0f,%.0f"), NextAutoClick, AutoMouse.X, AutoMouse.Y);
	}
}

void ACampaign1851PlayerController::CampaignNewGame()
{
	ACampaign1851Camera* Camera = Cast<ACampaign1851Camera>(GetPawn());
	if (!Map.IsValid() || !Camera)
	{
		return;
	}
	bCampaignStarted = true;
	AutosaveTimer = 0.f;
	Map->ClearProjects();
	Map->SetCampaignDays(0.0);
	Map->SetSpeed(0);
	Map->ResetEconomy();
	Map->ResetNetwork();
	// A new world: its own seed (or -CampaignSeed=N) and the chosen deviation from history (-CampaignDeviation=20).
	int32 NewSeed = int32(FPlatformTime::Cycles() & 0x7fffffff);
	FParse::Value(FCommandLine::Get(), TEXT("CampaignSeed="), NewSeed);
	float DeviationPct = Map->NewGameDeviation * 100.f;
	FParse::Value(FCommandLine::Get(), TEXT("CampaignDeviation="), DeviationPct);
	FString NationFlag;
	if (FParse::Value(FCommandLine::Get(), TEXT("CampaignNation="), NationFlag))
	{
		Map->NewGameNation = NationFlag;
	}
	Map->ResetWorld(NewSeed, DeviationPct / 100.f);
	Map->SeedHistoricBuildings();
	Map->ResetArmy();
	Map->ResetForts();
	Map->SetMaterialLots({});
	Map->ResetSupply();
	Map->ResetMateriel();
	Map->SetFooting(ECampaign1851Footing::Peace);
	Map->ExportForts();
	if (Overlay.IsValid())
	{
		Overlay->SetSelectedRegiments({});
	}
	Camera->ResetView();
	if (Overlay.IsValid())
	{
		Overlay->SetSelectedCity(INDEX_NONE);
		Overlay->CloseMenu();
		Overlay->ShowToast(FString::Printf(TEXT("Nyt spil  ·  %s"), *ACampaign1851Map::ActiveScenario().Name));
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
		FString SavedNation = TEXT("Danmark");
		if (Save)
		{
			for (const auto& SavedNationRow : Save->Nations)
			{
				if (SavedNationRow.bPlayer)
				{
					const auto& MenuNations = Map->GetNations();
					const int32 MenuNationIndex = MenuNations.IndexOfByPredicate([&SavedNationRow](const FCampaign1851Nation& MenuNation) { return MenuNation.Id == SavedNationRow.Id; });
					SavedNation = MenuNations.IsValidIndex(MenuNationIndex) ? MenuNations[MenuNationIndex].Name : SavedNationRow.Id;
				}
			}
		}
		const FString SavedScenario = Save && ACampaign1851Map::Scenarios().IsValidIndex(Save->Scenario) ? ACampaign1851Map::Scenarios()[Save->Scenario].Id : TEXT("ukendt");
		Row.Info = Save ? FString::Printf(TEXT("%s · %s · %s · %s"), *Save->SavedAt.ToString(TEXT("%d-%m-%Y %H:%M")), *SavedScenario, *SavedNation, *Summary) : TEXT("Tom");
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
	const K OOBSourceKind = SCampaign1851Overlay::TreeKind(Source);
	auto OOBDetachPart = [&](int32 OOBPartKey, FString& OOBWhy) -> int32
	{
		const int32 OOBUnit = OOBPartKey / 10;
		if (Map->GetRegiments().IsValidIndex(OOBUnit) && OOBPartKey % 10 < Map->SubUnitCount(OOBUnit) && Map->SubUnitCount(OOBUnit) == 1) { return OOBUnit; }
		return Map->SplitOffCompany(OOBUnit, OOBPartKey % 10, &OOBWhy);
	};
	// The permanent right target always starts an independent army, without intermediate HQs.
	if (TargetKind == K::NewFormation && TargetId == 99999)
	{
		TArray<int32> OOBUnits;
		FString OOBWhy;
		if (OOBSourceKind == K::Company)
		{
			const int32 OOBDetached = OOBDetachPart(SourceId, OOBWhy);
			if (OOBDetached == INDEX_NONE) { Overlay->ShowToast(OOBWhy); return; }
			OOBUnits.Add(OOBDetached);
		}
		else if (OOBSourceKind == K::Regiment && Map->GetRegiments().IsValidIndex(SourceId))
		{
			OOBUnits = Overlay->GetSelectedRegiments().Contains(SourceId) ? Overlay->GetSelectedRegiments() : TArray<int32> { SourceId };
		}
		else if (OOBSourceKind == K::Formation)
		{
			const int32 OOBSourceIndex = Map->FormationIndex(SourceId);
			if (OOBSourceIndex == INDEX_NONE || Map->GetFormations()[OOBSourceIndex].Echelon == ECampaign1851Echelon::Army) { Overlay->ShowToast(TEXT("En felthær kan ikke ligge under en anden felthær")); return; }
			const int32 OOBArmy = Map->CreateFormation(ECampaign1851Echelon::Army, 0);
			Map->MoveFormation(SourceId, OOBArmy);
			Overlay->RevealOOBArmy(OOBArmy, Map->FormationRegiments(OOBArmy));
			Overlay->ShowToast(TEXT("Ny felthær med den trukne formation direkte under sig"));
			return;
		}
		else if (OOBSourceKind == K::Command)
		{
			for (int32 OOBUnit = 0; OOBUnit < Map->GetRegiments().Num(); ++OOBUnit)
			{
				if (Map->GetRegiments()[OOBUnit].Command == SourceId && Map->GetRegiments()[OOBUnit].Formation == 0) { OOBUnits.Add(OOBUnit); }
			}
		}
		if (OOBUnits.Num() == 0) { Overlay->ShowToast(TEXT("Træk en enhed eller et kompagni herover")); return; }
		const int32 OOBArmy = Map->CreateFormation(ECampaign1851Echelon::Army, 0);
		for (int32 OOBUnit : OOBUnits) { Map->MoveRegimentToFormation(OOBUnit, OOBArmy); }
		Overlay->RevealOOBArmy(OOBArmy, OOBUnits);
		Overlay->ShowToast(FString::Printf(TEXT("Ny felthær med %d enheder direkte under sig"), OOBUnits.Num()));
		return;
	}
	// Validate all units before changing anything, including a company's source before splitting.
	if (TargetKind == K::Garrisons || TargetKind == K::Command || TargetKind == K::ArmGroup)
	{
		TArray<int32> OOBReturning;
		if (OOBSourceKind == K::Regiment) { OOBReturning.Add(SourceId); }
		else if (OOBSourceKind == K::Company) { OOBReturning.Add(SourceId / 10); }
		else if (OOBSourceKind == K::Formation) { OOBReturning = Map->FormationRegiments(SourceId); }
		FString OOBWhy;
		for (int32 OOBUnit : OOBReturning)
		{
			if (!Map->CanReturnToGarrison(OOBUnit, &OOBWhy)) { Overlay->ShowToast(OOBWhy); return; }
		}
		if (OOBSourceKind == K::Company)
		{
			const int32 OOBDetached = OOBDetachPart(SourceId, OOBWhy);
			if (OOBDetached != INDEX_NONE && Map->MoveRegimentToFormation(OOBDetached, 0)) { Overlay->ShowToast(TEXT("Underenheden er tilbage i garnison")); }
			else { Overlay->ShowToast(OOBWhy); }
			return;
		}
	}
	// The last company (squadron) of a half onto the other half of the same unit: the halves are one unit again.
	if (SCampaign1851Overlay::TreeKind(Source) == K::Company && (TargetKind == K::Regiment || TargetKind == K::Company))
	{
		const int32 From = SourceId / 10, To = TargetKind == K::Regiment ? TargetId : TargetId / 10;
		if (From != To && Map->SubUnitCount(From) <= 1 && Map->IsSplitPair(From, To))
		{
			const bool bKeepTo = !(Map->GetRegiments()[To].bDetached && !Map->GetRegiments()[From].bDetached);
			const int32 Keep = bKeepTo ? To : From, Absorb = bKeepTo ? From : To;
			FString Why;
			const int32 Joined = Map->MergeRegiments(Keep, Absorb, &Why);
			if (Joined != INDEX_NONE)
			{
				Overlay->FilterOOB({ Joined });
				Overlay->SetOOBBuilding(INDEX_NONE);
				SelectRegiments({ Joined });
				Overlay->ShowToast(FString::Printf(TEXT("%s er samlet igen"), *Map->GetRegiments()[Joined].Name));
			}
			else
			{
				Overlay->ShowToast(Why);
			}
			return;
		}
	}
	// A company onto another battalion (or one of its companies): it goes over with its captain and men.
	if (SCampaign1851Overlay::TreeKind(Source) == K::Company && TargetKind == K::Company && SourceId != TargetId
		&& Map->GetRegiments().IsValidIndex(SourceId / 10) && Map->GetRegiments().IsValidIndex(TargetId / 10)
		&& Map->GetRegiments()[SourceId / 10].Arm == Map->GetRegiments()[TargetId / 10].Arm
		&& (Map->GetRegiments()[SourceId / 10].Captains.Num() > 0) == (Map->GetRegiments()[TargetId / 10].Captains.Num() > 0))
	{
		// Two companies (or squadrons), in the same unit or in two units standing together: a window asks how many men go over
		// (and for two units it can move the whole company instead).
		const int32 Reg = SourceId / 10, From = SourceId % 10, ToReg = TargetId / 10, To = TargetId % 10;
		const int32 Ma = Map->CompanyMen(Reg, From), Mb = Map->CompanyMen(ToReg, To);
		const int32 Max = FMath::Min(Ma, Map->CompanyCapacity(ToReg) - Mb);
		if (Max <= 0 && Map->GetRegiments()[Reg].Arm != ECampaign1851Arm::Artillery)
		{
			Overlay->ShowToast(Ma <= 0 ? TEXT("Kompagniet har ingen mænd at give") : TEXT("Det andet kompagni er fuldt"));
		}
		else
		{
			Overlay->OpenTransfer(Reg, From, ToReg, To, FMath::Max(0, Max), FMath::Clamp((Ma - Mb) / 2, 1, FMath::Max(1, Max)));
		}
		return;
	}
	if (SCampaign1851Overlay::TreeKind(Source) == K::Company && (TargetKind == K::Regiment || TargetKind == K::Company))
	{
		const int32 From = SourceId / 10, To = TargetKind == K::Regiment ? TargetId : TargetId / 10;
		if (From != To)
		{
			FString Why;
			const FString What = Overlay->TreeKeyText(Source);
			int32 NewTo = To;
			const bool bMoved = Map->MoveCompany(From, SourceId % 10, To, &Why, &NewTo);
			Overlay->ShowToast(bMoved ? FString::Printf(TEXT("%s går over til %s"), *What, *Map->GetRegiments()[NewTo].Name) : Why);
		}
		return;
	}
	// One half of a split unit onto the other: joined again (asked first).
	if (SCampaign1851Overlay::TreeKind(Source) == K::Regiment && TargetKind == K::Regiment && Map->IsSplitPair(SourceId, TargetId))
	{
		const int32 Keep = Map->GetRegiments()[TargetId].bDetached && !Map->GetRegiments()[SourceId].bDetached ? SourceId : TargetId;
		const int32 Absorb = Keep == TargetId ? SourceId : TargetId;
		FString Why, Title, Text;
		if (!Map->CanMerge(Keep, Absorb, &Why))
		{
			Overlay->ShowToast(Why);
		}
		else if (DescribeAction(*Map, *Overlay, SCampaign1851Overlay::EButton::MergeUnit, Keep * 10000 + Absorb, Title, Text))
		{
			Overlay->AskConfirm(Title, Text, SCampaign1851Overlay::EButton::MergeUnit, Keep * 10000 + Absorb);
		}
		return;
	}
	// Whole commands add their units directly, without generating a division/brigade.
	if (OOBSourceKind == K::Command && (TargetKind == K::Formation || TargetKind == K::FieldArmy))
	{
		TArray<int32> OOBCommandUnits;
		for (int32 OOBUnit = 0; OOBUnit < Map->GetRegiments().Num(); ++OOBUnit)
		{
			if (Map->GetRegiments()[OOBUnit].Command == SourceId && Map->GetRegiments()[OOBUnit].Formation == 0) { OOBCommandUnits.Add(OOBUnit); }
		}
		if (OOBCommandUnits.Num() == 0) { Overlay->ShowToast(TEXT("Kommandoen har ingen enheder i garnison")); return; }
		const int32 OOBInto = TargetKind == K::FieldArmy ? Map->CreateFormation(ECampaign1851Echelon::Army, 0) : TargetId;
		for (int32 OOBUnit : OOBCommandUnits) { Map->MoveRegimentToFormation(OOBUnit, OOBInto); }
		Overlay->ShowToast(TEXT("Kommandoens enheder er flyttet direkte ind under hovedkvarteret"));
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
		Into = -1;
		if (OOBSourceKind == K::Regiment || OOBSourceKind == K::Company)
		{
			FString OOBWhy;
			const int32 OOBUnit = OOBSourceKind == K::Company ? OOBDetachPart(SourceId, OOBWhy) : SourceId;
			if (!Map->GetRegiments().IsValidIndex(OOBUnit)) { Overlay->ShowToast(OOBWhy); return; }
			Into = Map->CreateFormation(ECampaign1851Echelon::Army, 0);
			Map->MoveRegimentToFormation(OOBUnit, Into);
			Overlay->ShowToast(TEXT("Enheden står direkte under den nye felthær"));
			return;
		}
	}
	if (OOBSourceKind == K::Company && Into > 0)
	{
		FString OOBWhy;
		const int32 OOBDetached = OOBDetachPart(SourceId, OOBWhy);
		if (OOBDetached != INDEX_NONE && Map->MoveRegimentToFormation(OOBDetached, Into)) { Overlay->ShowToast(TEXT("Underenheden er flyttet direkte ind under hovedkvarteret")); }
		else { Overlay->ShowToast(OOBWhy); }
		return;
	}
	bool bDone = false;
	FString What;
	if (SCampaign1851Overlay::TreeKind(Source) == K::Regiment && Into >= 0)
	{
		bDone = Map->MoveRegimentToFormation(SourceId, Into);
		What = Into == 0 ? TEXT("tilbage i garnison") : FString::Printf(TEXT("ind i %s"), *Map->GetFormations()[Map->FormationIndex(Into)].Name);
	}
	else if (SCampaign1851Overlay::TreeKind(Source) == K::Formation && Into == 0)
	{
		// A whole formation back to the garrisons: its units go home to their commands, it is dissolved.
		const FString Name = Overlay->TreeKeyText(Source);
		const int32 Units = Map->ReturnFormationToGarrison(SourceId);
		Overlay->ShowToast(FString::Printf(TEXT("%s opløst: %d enheder tilbage i garnison"), *Name, Units));
		return;
	}
	else if (SCampaign1851Overlay::TreeKind(Source) == K::Formation && Into != 0)
	{
		const int32 Parent = Into < 0 ? 0 : Into;
		bDone = Map->MoveFormation(SourceId, Parent);
		What = Parent == 0 ? FString(TEXT("direkte under felthæren")) : FString::Printf(TEXT("under %s"), *Map->GetFormations()[Map->FormationIndex(Parent)].Name);
	}
	Overlay->ShowToast(bDone ? FString::Printf(TEXT("%s flyttet %s"), *Overlay->TreeKeyText(Source), *What) : FString(TEXT("Kan ikke flyttes dertil")));
}

void ACampaign1851PlayerController::SetBattleView(bool bEnter)
{
	ACampaign1851Camera* Cam = Cast<ACampaign1851Camera>(GetPawn());
	if (!Map.IsValid() || !Cam)
	{
		return;
	}
	if (bEnter && !Map->IsBattleView() && Map->EnterBattleView())
	{
		// Remember the map view, then look at the model from above, its whole width in sight.
		SavedMapTarget = Cam->GetTarget();
		SavedMapDistance = Cam->GetDistanceKm();
		SavedMapYaw = Cam->GetYaw();
		SavedMapCentre = Cam->GetBoundsCentre();
		SavedMapHalf = Cam->GetHalfExtent();
		const FVector O = Map->GetActorTransform().TransformPosition(Map->BattleViewOrigin());
		Cam->SetBounds(FVector2D(O.X, O.Y), Map->BattleViewHalfExtent());
		Cam->SetView(FVector(O.X, O.Y, O.Z + Map->BattleViewGroundZ()), float(Map->BattleViewHalfExtent().X / ACampaign1851Map::KmToUnits) * 1.6f, 0.f);
		if (Overlay.IsValid()) { Overlay->OpenWindow(SCampaign1851Overlay::EWindow::None); }
		Map->SetSpeed(0);
	}
	else if (!bEnter && Map->IsBattleView())
	{
		Map->LeaveBattleView();
		Cam->SetBounds(SavedMapCentre, SavedMapHalf);
		Cam->SetView(SavedMapTarget, SavedMapDistance, SavedMapYaw);
	}
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
