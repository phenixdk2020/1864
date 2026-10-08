#include "Campaign1851ConstructionSite.h"

#include "Campaign1851Buildings.h"
#include "Campaign1851Map.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	using Campaign1851Scenery::ESitePiece;

	// Stage boundaries (fraction of a module's time).
	constexpr float StakedEnd = 0.08f, FoundationEnd = 0.2f, WallsEnd = 0.8f, RoofEnd = 0.95f;
	constexpr float PlinthTop = 0.6f;
	constexpr float WagonSpeed = 22.f;   // world units per second
	const FName BuildTopParam(TEXT("BuildTop"));

	float Phase(float P, float A, float B) { return FMath::Clamp((P - A) / (B - A), 0.f, 1.f); }

	FString CardPath(const TCHAR* Name) { return FString::Printf(TEXT("/Game/Campaign1851/Buildings/%s.%s"), Name, Name); }

	FCampaign1851SiteModule Module(const TCHAR* Key, const TCHAR* Name, ESitePiece Piece, float Length, float Width, float Eave, float Top, const TCHAR* Card)
	{
		FCampaign1851SiteModule M;
		M.Key = Key;
		M.Name = Name;
		M.Piece = Piece;
		M.Length = Length;
		M.Width = Width;
		M.Eave = Eave;
		M.Top = Top;
		M.Card = Card && *Card ? CardPath(Card) : FString();
		return M;
	}
}

// ------------------------------------------------------------------ module data

int32 FCampaign1851SiteModule::Cost() const
{
	const FCampaign1851BuildingDef* Data = Campaign1851Buildings::Find(Key);
	if (ACampaign1851Map::ActiveScenario().Id != TEXT("1825")) { return Data ? Data->CostRd : 0; }
	return Data ? FMath::RoundToInt(Data->CostRd * ACampaign1851Map::EconomyValue(TEXT("constructionCost"), 1.0)) : 0;
}

float FCampaign1851SiteModule::Days() const
{
	const FCampaign1851BuildingDef* Data = Campaign1851Buildings::Find(Key);
	return Data ? float(FMath::Max(Data->Days, 1)) : 90.f;
}

int32 FCampaign1851SiteModule::Upkeep() const
{
	const FCampaign1851BuildingDef* Data = Campaign1851Buildings::Find(Key);
	return Data ? Data->UpkeepRdPerYear : 0;
}

FString FCampaign1851SiteModule::Type() const
{
	const FCampaign1851BuildingDef* Data = Campaign1851Buildings::Find(Key);
	return Data ? Data->Type : FString(TEXT("grundmur"));
}

double FCampaign1851SiteModule::CostPerDay() const
{
	return Cost() * (1.0 - Campaign1851Buildings::DownPayment) / FMath::Max(Days(), 1.f);
}

const TArray<FCampaign1851SiteModule>& ACampaign1851ConstructionSite::GarrisonModules()
{
	// The parade ground runs along the barracks front (+Y); stables and depot flank it, the
	// infirmary stands behind the barracks.
	static const TArray<FCampaign1851SiteModule> List = []
	{
		TArray<FCampaign1851SiteModule> L;
		FCampaign1851SiteModule& Barracks = L.Add_GetRef(Module(TEXT("Garrison_Barracks"), TEXT("Infanterikaserne"), ESitePiece::Barracks,
			Campaign1851Scenery::BarracksLength, Campaign1851Scenery::BarracksWidth, Campaign1851Scenery::BarracksEave, Campaign1851Scenery::BarracksTop, TEXT("T_Barracks_Infantry")));
		Barracks.bFlag = true;
		Barracks.FlagPos = FVector2D(0.0, 12.4);
		FCampaign1851SiteModule& Stables = L.Add_GetRef(Module(TEXT("Garrison_Stables"), TEXT("Stalde"), ESitePiece::Stables, 12.f, 4.4f, 3.2f, 6.4f, TEXT("T_Module_Stables")));
		Stables.Slot = FVector2D(-15.0, 7.5);
		Stables.Yaw = 90.f;
		FCampaign1851SiteModule& Depot = L.Add_GetRef(Module(TEXT("Garrison_Depot"), TEXT("Depot og magasin"), ESitePiece::Depot, 7.f, 5.6f, 7.f, 11.6f, TEXT("T_Module_Depot")));
		Depot.Slot = FVector2D(15.0, 7.5);
		Depot.Yaw = 90.f;
		FCampaign1851SiteModule& Infirmary = L.Add_GetRef(Module(TEXT("Garrison_Infirmary"), TEXT("Sygestue"), ESitePiece::Infirmary, 9.f, 4.6f, 4.6f, 8.2f, TEXT("T_Module_Infirmary")));
		Infirmary.Slot = FVector2D(0.0, -10.0);
		return L;
	}();
	return List;
}

const TArray<FCampaign1851SiteModule>& ACampaign1851ConstructionSite::TownBuildings()
{
	static const TArray<FCampaign1851SiteModule> List = []
	{
		TArray<FCampaign1851SiteModule> L;
		auto Add = [&L](FCampaign1851SiteModule M, ECampaign1851PlotRule Rule, float RadiusKm, int32 MinPop, bool bCoast, int32 FromYear) -> FCampaign1851SiteModule&
		{
			M.Rule = Rule;
			M.PlotRadiusKm = RadiusKm;
			M.MinPopulation = MinPop;
			M.bNeedsCoast = bCoast;
			M.FromYear = FromYear;
			return L.Add_GetRef(M);
		};
		FCampaign1851SiteModule& Arsenal = Add(Module(TEXT("Arsenal"), TEXT("Arsenal"), ESitePiece::Arsenal, 18.f, 13.f, 6.f, 10.6f, TEXT("T_Bld_Arsenal")),
			ECampaign1851PlotRule::Shore, 0.34f, 10000, true, 0);
		Arsenal.bFlag = true;
		Arsenal.FlagPos = FVector2D(0.0, 8.0);
		Add(Module(TEXT("Field_Hospital"), TEXT("Lazaret"), ESitePiece::Lazaret, 12.f, 5.2f, 5.6f, 9.8f, TEXT("T_Bld_Field_Hospital")),
			ECampaign1851PlotRule::Edge, 0.2f, 2500, false, 0);
		FCampaign1851SiteModule& Battery = Add(Module(TEXT("Coastal_Battery"), TEXT("Kystbatteri"), ESitePiece::Battery, 16.f, 6.f, 2.2f, 3.0f, TEXT("T_Bld_Coastal_Battery")),
			ECampaign1851PlotRule::Shore, 0.22f, 0, true, 0);
		Battery.bScaffold = Battery.bCrane = false;
		Battery.bFlag = true;
		Battery.FlagPos = FVector2D(-9.0, -3.5);
		FCampaign1851SiteModule& Magazine = Add(Module(TEXT("Powder_Magazine"), TEXT("Krudtmagasin"), ESitePiece::PowderMagazine, 10.f, 9.f, 2.6f, 4.4f, TEXT("T_Bld_Powder_Magazine")),
			ECampaign1851PlotRule::Outside, 0.15f, 0, false, 0);
		Magazine.bScaffold = Magazine.bCrane = false;
		FCampaign1851SiteModule& Fort = Add(Module(TEXT("Star_Fort"), TEXT("Skanse"), ESitePiece::StarFort, 34.f, 34.f, 2.6f, 4.3f, TEXT("T_Bld_Star_Fort")),
			ECampaign1851PlotRule::Strategic, 0.5f, 0, false, 0);
		Fort.bScaffold = Fort.bCrane = false;
		Fort.bFlag = true;
		Fort.FlagPos = FVector2D(0.0, 4.5);
		Add(Module(TEXT("Telegraph_Office"), TEXT("Telegrafstation"), ESitePiece::Telegraph, 9.f, 4.f, 3.6f, 11.f, TEXT("T_Bld_Telegraph_Office")),
			ECampaign1851PlotRule::InTown, 0.12f, 0, false, 1854);
		Add(Module(TEXT("Harbor_Warehouse"), TEXT("Havnepakhus"), ESitePiece::Depot, 7.f, 5.6f, 7.f, 11.6f, TEXT("T_Bld_Harbor_Warehouse")),
			ECampaign1851PlotRule::Shore, 0.15f, 0, true, 0);
		Add(Module(TEXT("Grain_Warehouse"), TEXT("Kornmagasin"), ESitePiece::Granary, 13.f, 5.4f, 6.4f, 10.4f, TEXT("T_Bld_Grain_Warehouse")),
			ECampaign1851PlotRule::Edge, 0.18f, 0, false, 0);
		// Works that fill the state's stores (Campaign1851Materiel::Production).
		Add(Module(TEXT("Rifle_Workshop"), TEXT("Geværværksted"), ESitePiece::Workshop, 20.f, 9.f, 5.f, 15.f, TEXT("T_Bld_Rifle_Workshop")), ECampaign1851PlotRule::Edge, 0.2f, 4000, false, 0);
		Add(Module(TEXT("Cannon_Foundry"), TEXT("Kanonstøberi"), ESitePiece::Factory, 27.f, 16.f, 11.f, 22.f, TEXT("T_Bld_Cannon_Foundry")), ECampaign1851PlotRule::Outside, 0.25f, 8000, false, 0);
		Add(Module(TEXT("Ammunition_Works"), TEXT("Ammunitionsfabrik"), ESitePiece::Workshop, 20.f, 9.f, 5.f, 15.f, TEXT("T_Bld_Ammunition_Works")), ECampaign1851PlotRule::Outside, 0.2f, 4000, false, 0);
		Add(Module(TEXT("Stud_Farm"), TEXT("Stutteri"), ESitePiece::Stables, 12.f, 4.4f, 3.2f, 6.4f, TEXT("T_Bld_Stud_Farm")), ECampaign1851PlotRule::Outside, 0.25f, 0, false, 0);
		Add(Module(TEXT("Remount_Depot"), TEXT("Remontedepot"), ESitePiece::Stables, 12.f, 4.4f, 3.2f, 6.4f, TEXT("T_Bld_Remount_Depot")), ECampaign1851PlotRule::Outside, 0.25f, 3000, false, 0);
		// Civil buildings (growth and income: Campaign1851Nations::CivilEffect).
		Add(Module(TEXT("Schoolhouse"), TEXT("Skole"), ESitePiece::School, 11.f, 7.f, 3.4f, 8.4f, TEXT("T_Bld_Schoolhouse")), ECampaign1851PlotRule::InTown, 0.12f, 800, false, 0);
		Add(Module(TEXT("Town_Hall"), TEXT("Rådhus"), ESitePiece::TownHall, 17.f, 9.f, 7.f, 17.f, TEXT("T_Bld_Town_Hall")), ECampaign1851PlotRule::InTown, 0.15f, 3000, false, 0);
		Add(Module(TEXT("Post_Office"), TEXT("Posthus"), ESitePiece::PostOffice, 13.f, 5.f, 5.4f, 8.6f, TEXT("T_Bld_Post_Office")), ECampaign1851PlotRule::InTown, 0.12f, 1500, false, 0);
		Add(Module(TEXT("Hospital"), TEXT("Sygehus"), ESitePiece::Hospital, 21.f, 13.f, 8.f, 12.f, TEXT("T_Bld_Hospital")), ECampaign1851PlotRule::Edge, 0.2f, 6000, false, 0);
		Add(Module(TEXT("Harbor_Building"), TEXT("Toldbod"), ESitePiece::CustomsHouse, 12.f, 12.f, 6.f, 9.6f, TEXT("T_Bld_Harbor_Building")), ECampaign1851PlotRule::Shore, 0.15f, 1500, true, 0);
		Add(Module(TEXT("Lighthouse"), TEXT("Fyrtårn"), ESitePiece::Lighthouse, 6.f, 12.f, 17.6f, 19.f, TEXT("T_Bld_Lighthouse")), ECampaign1851PlotRule::Shore, 0.3f, 0, true, 0);
		Add(Module(TEXT("Merchant_House"), TEXT("Købmandsgård"), ESitePiece::MerchantYard, 15.f, 14.f, 5.6f, 9.f, TEXT("T_Bld_Merchant_House")), ECampaign1851PlotRule::InTown, 0.15f, 1500, false, 0);
		Add(Module(TEXT("Brewery"), TEXT("Bryggeri og brænderi"), ESitePiece::Brewery, 16.f, 9.f, 6.f, 13.f, TEXT("T_Bld_Brewery")), ECampaign1851PlotRule::Edge, 0.18f, 2500, false, 0);
		Add(Module(TEXT("Brickworks"), TEXT("Teglværk"), ESitePiece::Brickworks, 18.f, 14.f, 3.2f, 16.f, TEXT("T_Bld_Brickworks")), ECampaign1851PlotRule::Outside, 0.2f, 0, false, 0);
		Add(Module(TEXT("Sawmill"), TEXT("Savværk"), ESitePiece::Sawmill, 13.f, 13.f, 3.6f, 9.f, TEXT("T_Bld_Sawmill")), ECampaign1851PlotRule::Outside, 0.2f, 0, false, 0);
		Add(Module(TEXT("Machine_Workshop"), TEXT("Maskinværksted"), ESitePiece::Workshop, 20.f, 9.f, 5.f, 15.f, TEXT("T_Bld_Machine_Workshop")), ECampaign1851PlotRule::Edge, 0.2f, 8000, false, 0);
		Add(Module(TEXT("Textile_Mill"), TEXT("Klædefabrik"), ESitePiece::Factory, 27.f, 16.f, 11.f, 22.f, TEXT("T_Bld_Textile_Mill")), ECampaign1851PlotRule::Edge, 0.25f, 10000, false, 0);
		Add(Module(TEXT("Inn"), TEXT("Kro"), ESitePiece::Inn, 18.f, 12.f, 3.6f, 7.4f, TEXT("T_Bld_Inn")), ECampaign1851PlotRule::Edge, 0.2f, 0, false, 0);
		return L;
	}();
	return List;
}

const FCampaign1851SiteModule* ACampaign1851ConstructionSite::FindTownBuilding(const FString& Key)
{
	return TownBuildings().FindByPredicate([&Key](const FCampaign1851SiteModule& M) { return M.Key == Key; });
}

int32 ACampaign1851ConstructionSite::GetYearlyUpkeep() const
{
	int32 Total = 0;
	for (int32 m = 0; m < NumModules(); ++m)
	{
		Total += IsModuleDone(m) ? Modules[m].Upkeep() : 0;
	}
	return Total;
}

// ------------------------------------------------------------------ site

ACampaign1851ConstructionSite::ACampaign1851ConstructionSite()
{
	PrimaryActorTick.bCanEverTick = false;   // advanced by the map's calendar
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	auto Part = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Root);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		return C;
	};
	Ground = Part(TEXT("Ground"));
	CraneMast = Part(TEXT("CraneMast"));
	CraneJib = Part(TEXT("CraneJib"));
	Wagon = Part(TEXT("Wagon"));
	Flagpole = Part(TEXT("Flagpole"));
	Flag = Part(TEXT("Flag"));
}

void ACampaign1851ConstructionSite::Setup(int32 InCityIndex, const TArray<FVector>& InWagonPath, UMaterialInterface* Material,
	const TArray<FCampaign1851SiteModule>& InModules, bool bInGarrison)
{
	using namespace Campaign1851Scenery;
	CityIndex = InCityIndex;
	Modules = InModules;
	bGarrison = bInGarrison;
	WagonPath = InWagonPath;
	WagonDistance.Reset();
	float Total = 0.f;
	for (int32 i = 0; i < WagonPath.Num(); ++i)
	{
		Total += i > 0 ? float(FVector::Dist(WagonPath[i - 1], WagonPath[i])) : 0.f;
		WagonDistance.Add(Total);
	}

	const FCampaign1851SiteModule& Main = Modules[0];
	Ground->SetStaticMesh(bGarrison ? BuildSitePiece(ESitePiece::Ground, Material) : BuildPlotGround(Main.Length, Main.Width, Material));
	Ground->SetVisibility(bGarrison || Main.bScaffold);   // earthworks (battery, fort, magazine) have their own yard
	CraneMast->SetStaticMesh(BuildSitePiece(ESitePiece::CraneMast, Material));
	CraneJib->SetStaticMesh(BuildSitePiece(ESitePiece::CraneJib, Material));
	Wagon->SetStaticMesh(BuildSitePiece(ESitePiece::Wagon, Material));
	Flagpole->SetStaticMesh(BuildSitePiece(ESitePiece::Flagpole, Material));
	Flag->SetStaticMesh(BuildSitePiece(ESitePiece::Flag, Material));
	FlagSpot = FVector(Main.FlagPos.X, Main.FlagPos.Y, 0.0);
	Flagpole->SetRelativeLocation(FlagSpot);
	Flagpole->SetVisibility(Main.bFlag);

	for (int32 m = 0; m < NumModules(); ++m)
	{
		const FCampaign1851SiteModule& Def = Modules[m];
		auto Make = [&](const TCHAR* Kind, UStaticMesh* Mesh, TArray<TObjectPtr<UStaticMeshComponent>>& Out, TArray<TObjectPtr<UMaterialInstanceDynamic>>& Mids)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("%s_%d"), Kind, m));
			C->SetupAttachment(Root);
			C->SetRelativeLocationAndRotation(FVector(Def.Slot.X, Def.Slot.Y, 0.0), FRotator(0.f, Def.Yaw, 0.f));
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			C->SetStaticMesh(Mesh);
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, this);
			C->SetMaterial(0, Mid);
			C->SetVisibility(false);
			C->RegisterComponent();
			Out.Add(C);
			Mids.Add(Mid);
		};
		Make(TEXT("Building"), BuildSitePiece(Def.Piece, Material), Buildings, BuildingMids);
		Make(TEXT("Scaffold"), BuildScaffold(Def.Length, Def.Width, Def.Top + 1.f, Material), Scaffolds, ScaffoldMids);
	}

	Elapsed.Init(-1.f, NumModules());
	StartModule(0);
	Apply(0.f);
}

bool ACampaign1851ConstructionSite::CanStartModule(int32 Module) const
{
	return bGarrison && Module > 0 && Module < NumModules() && IsBarracksDone() && Active == INDEX_NONE && !IsModuleStarted(Module);
}

void ACampaign1851ConstructionSite::StartModule(int32 Module)
{
	if (Module == 0 ? IsModuleStarted(0) : !CanStartModule(Module))
	{
		return;
	}
	Elapsed[Module] = 0.f;
	Active = Module;
}

float ACampaign1851ConstructionSite::GetModuleProgress(int32 Module) const
{
	return IsModuleStarted(Module) ? FMath::Clamp(Elapsed[Module] / ModuleDays(Module), 0.f, 1.f) : 0.f;
}

FString ACampaign1851ConstructionSite::GetStageName(int32 Module) const
{
	if (!IsModuleStarted(Module)) return TEXT("Ikke bygget");
	const float P = GetModuleProgress(Module);
	if (P >= 1.f) return TEXT("Færdig");
	if (P < StakedEnd) return TEXT("Grunden afstikkes");
	if (!GetModule(Module).bScaffold)
	{
		return P < RoofEnd ? TEXT("Jordarbejde") : TEXT("Indretning");   // earthworks: ramparts rise
	}
	if (P < FoundationEnd) return TEXT("Fundament");
	if (P < WallsEnd) return TEXT("Murværk");
	if (P < RoofEnd) return TEXT("Tag");
	return TEXT("Indretning");
}

void ACampaign1851ConstructionSite::Advance(float DeltaDays, float DeltaSeconds)
{
	Clock += DeltaSeconds;
	if (Active != INDEX_NONE)
	{
		Elapsed[Active] = FMath::Min(Elapsed[Active] + DeltaDays, ModuleDays(Active));
		if (IsModuleDone(Active))
		{
			// Let the last frame show the finished state before the next project can start.
			Apply(DeltaSeconds);
			Active = INDEX_NONE;
			return;
		}
	}
	Apply(DeltaSeconds);
}

void ACampaign1851ConstructionSite::SetClip(UMaterialInstanceDynamic* Mid, float PieceHeight) const
{
	if (Mid)
	{
		Mid->SetScalarParameterValue(BuildTopParam, float(GetActorLocation().Z) + PieceHeight * float(GetActorScale3D().Z));
	}
}

void ACampaign1851ConstructionSite::Apply(float DeltaSeconds)
{
	for (int32 m = 0; m < Buildings.Num(); ++m)
	{
		const FCampaign1851SiteModule& Def = Modules[m];
		const bool bStarted = IsModuleStarted(m);
		const float P = GetModuleProgress(m);
		Buildings[m]->SetVisibility(bStarted);

		// Plinth, then the walls storey by storey, then the roof (and chimneys). Earthworks rise evenly.
		float Top = -1.f;
		if (bStarted && P >= StakedEnd)
		{
			if (Def.bScaffold)
			{
				Top = FMath::Lerp(0.f, PlinthTop, Phase(P, StakedEnd, FoundationEnd));
				Top = FMath::Lerp(Top, Def.Eave, Phase(P, FoundationEnd, WallsEnd));
				Top = FMath::Lerp(Top, Def.Top, Phase(P, WallsEnd, RoofEnd));
			}
			else
			{
				Top = FMath::Lerp(0.f, Def.Top, Phase(P, StakedEnd, RoofEnd));
			}
		}
		SetClip(BuildingMids[m], Top);

		// Scaffold: goes up ahead of the walls, comes down top first while the building is fitted out.
		const bool bScaffold = Def.bScaffold && bStarted && P >= StakedEnd && P < 1.f;
		Scaffolds[m]->SetVisibility(bScaffold);
		const float ScaffoldTop = P < RoofEnd ? FMath::Min(Top + 2.4f, Def.Top + 1.f) : FMath::Lerp(Def.Top + 1.f, 0.f, Phase(P, RoofEnd, 1.f));
		SetClip(ScaffoldMids[m], FMath::Max(ScaffoldTop, 2.2f * Phase(P, StakedEnd, StakedEnd + 0.04f)));
	}

	// Crane and supply wagon work for whichever module is being built.
	const float P = Active != INDEX_NONE ? GetModuleProgress(Active) : 1.f;
	const bool bCrane = Active != INDEX_NONE && Modules[Active].bCrane && P >= FoundationEnd && P < RoofEnd;
	CraneMast->SetVisibility(bCrane);
	CraneJib->SetVisibility(bCrane);
	if (bCrane)
	{
		// Stand the crane at the working module's corner, jib swinging over it.
		const FCampaign1851SiteModule& Def = Modules[Active];
		const FVector Corner = FRotator(0.f, Def.Yaw, 0.f).RotateVector(FVector(-Def.Length * 0.5 - 2.4, -Def.Width * 0.5 - 2.0, 0.0));
		CraneMast->SetRelativeLocation(FVector(Def.Slot.X, Def.Slot.Y, 0.0) + Corner);
		CraneJib->SetRelativeLocation(FVector(Def.Slot.X, Def.Slot.Y, 0.0) + Corner);
		CraneJib->SetRelativeRotation(FRotator(0.f, Def.Yaw + 35.f + 45.f * FMath::Sin(Clock * 0.45f), 0.f));
	}
	const bool bWagon = Active != INDEX_NONE && P >= 0.03f && P < RoofEnd && WagonPath.Num() >= 2;
	Wagon->SetVisibility(bWagon);
	if (bWagon)
	{
		MoveWagon(DeltaSeconds);
	}

	// Dannebrog goes up the pole when the main building is finished, then flies.
	const bool bFlag = Modules[0].bFlag;
	const float Main = GetModuleProgress(0);
	const float Hoist = Phase(Main, RoofEnd, 1.f);
	Flag->SetVisibility(bFlag && Main >= RoofEnd);
	Flag->SetRelativeLocation(FlagSpot + FVector(0.15, 0.0, FMath::Lerp(1.2f, 11.4f, Hoist)));
	Flag->SetRelativeRotation(FRotator(0.f, 8.f * FMath::Sin(Clock * 2.1f) + 4.f * FMath::Sin(Clock * 3.7f), 0.f));
}

void ACampaign1851ConstructionSite::MoveWagon(float DeltaSeconds)
{
	const float Length = WagonDistance.Last();
	if (WagonPause > 0.f)
	{
		WagonPause -= DeltaSeconds;
	}
	else
	{
		WagonAt += WagonDirection * WagonSpeed * DeltaSeconds;
		if (WagonAt >= Length || WagonAt <= 0.f)
		{
			WagonAt = FMath::Clamp(WagonAt, 0.f, Length);
			WagonDirection = -WagonDirection;
			WagonPause = 1.5f;
		}
	}
	int32 i = 1;
	while (i < WagonDistance.Num() - 1 && WagonDistance[i] < WagonAt)
	{
		++i;
	}
	const float Span = FMath::Max(WagonDistance[i] - WagonDistance[i - 1], 1e-3f);
	const FVector At = FMath::Lerp(WagonPath[i - 1], WagonPath[i], (WagonAt - WagonDistance[i - 1]) / Span);
	const FVector Dir = (WagonPath[i] - WagonPath[i - 1]) * WagonDirection;
	Wagon->SetWorldLocationAndRotation(At, FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)), 0.f));
}

void ACampaign1851ConstructionSite::RestoreState(const TArray<float>& InDays, int32 InActive)
{
	for (int32 m = 0; m < NumModules(); ++m)
	{
		Elapsed[m] = InDays.IsValidIndex(m) ? FMath::Min(InDays[m], ModuleDays(m)) : -1.f;
	}
	Active = (Elapsed.IsValidIndex(InActive) && IsModuleStarted(InActive) && !IsModuleDone(InActive)) ? InActive : INDEX_NONE;
	Apply(0.f);
}
