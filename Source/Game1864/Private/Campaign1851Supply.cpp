// Supply of the 1851 campaign (Docs/Backlog-Forsyning.md, first delivery F-1 .. F-4, F-7, F-11): what each
// unit carries (rations, fodder, ammunition), the depots that refill it, local purchase in the towns, the
// consequences of want, the forts' magazines, and the data the 3D battles read. ACampaign1851Map's supply layer.

#include "Campaign1851Map.h"

#include "Campaign1851ConstructionSite.h"
#include "Campaign1851Scenery.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace Campaign1851Supply
{
	bool NeedsFodder(ECampaign1851Arm Arm)
	{
		return Arm == ECampaign1851Arm::Cavalry || Arm == ECampaign1851Arm::Artillery || Arm == ECampaign1851Arm::HorseArtillery;
	}

	FString Describe(const FCampaign1851Regiment& R)
	{
		return FString::Printf(TEXT("proviant %.1f d.%s  ·  ammunition %.0f %%"), R.Food,
			NeedsFodder(R.Arm) ? *FString::Printf(TEXT("  ·  foder %.1f d."), R.Fodder) : TEXT(""), R.Ammo * 100.f);
	}
}

FCampaign1851DepotCapacity ACampaign1851Map::DepotCapacity(int32 CityIndex) const
{
	// A garrison's depot and magazine, a grain store, an arsenal: each adds room.
	FCampaign1851DepotCapacity C;
	if (const ACampaign1851ConstructionSite* Garrison = FindProject(CityIndex))
	{
		if (!Garrison->IsDemolishing() && Garrison->NumModules() > 2 && Garrison->IsModuleDone(2))
		{
			C.Food += 40000.f;
			C.Fodder += 20000.f;
			C.Ammo += 30.f;
		}
	}
	if (const ACampaign1851ConstructionSite* Granary = FindBuilding(CityIndex, TEXT("Grain_Warehouse")))
	{
		if (!Granary->IsDemolishing() && Granary->IsModuleDone(0))
		{
			C.Food += 60000.f;
			C.Fodder += 60000.f;
		}
	}
	if (const ACampaign1851ConstructionSite* Arsenal = FindBuilding(CityIndex, TEXT("Arsenal")))
	{
		if (!Arsenal->IsDemolishing() && Arsenal->IsModuleDone(0))
		{
			C.Ammo += 200.f;
		}
	}
	return C;
}

FCampaign1851DepotStock ACampaign1851Map::DepotStock(int32 CityIndex) const
{
	const FCampaign1851DepotStock* S = Depots.Find(CityIndex);
	return S ? *S : FCampaign1851DepotStock();
}

int32 ACampaign1851Map::DepotFor(const FVector2D& Km) const
{
	// The nearest depot with anything in it, within a day's march.
	int32 Best = INDEX_NONE;
	double BestKm = Campaign1851Supply::DepotReachKm;
	for (const TPair<int32, FCampaign1851DepotStock>& D : Depots)
	{
		const double Dist = FVector2D::Distance(TownKm(D.Key), Km);
		if (Dist <= BestKm && (D.Value.Food > 1.f || D.Value.Fodder > 1.f || D.Value.Ammo > 0.01f))
		{
			BestKm = Dist;
			Best = D.Key;
		}
	}
	return Best;
}

int32 ACampaign1851Map::TownForPurchase(const FVector2D& Km) const
{
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		if (!Cities[c].bForeign && FVector2D::Distance(TownKm(c), Km) <= TownRadiusKm(Cities[c].Population) + Campaign1851Supply::PurchaseReachKm)
		{
			return c;
		}
	}
	return INDEX_NONE;
}

void ACampaign1851Map::ResetSupply()
{
	Depots.Reset();
	SupplyColumns.Reset();
	SupplyColumnCount = Campaign1851Supply::ColumnsAtStart;
	UpdateSupplyColumnPieces();
	for (FCampaign1851Regiment& R : Regiments)
	{
		R.Food = Campaign1851Supply::FoodCarried;
		R.Fodder = Campaign1851Supply::FodderCarried;
		R.Ammo = 1.f;
	}
}

void ACampaign1851Map::AdvanceSupply(float DeltaDays)
{
	if (DeltaDays <= 0.f)
	{
		return;
	}
	using namespace Campaign1851Supply;
	double Bought = 0.0;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		FCampaign1851Regiment& R = Regiments[i];
		const bool bFodder = NeedsFodder(R.Arm);
		// In garrison (a town of the monarchy, halted) the army's regular budget feeds it: full.
		const bool bGarrison = !R.IsMarching() && Cities.IsValidIndex(R.Town) && !Cities[R.Town].bForeign;
		if (bGarrison)
		{
			R.Food = FoodCarried;
			R.Fodder = FodderCarried;
			// Ammunition only where a depot or arsenal holds it (the garrison's own store).
			FCampaign1851DepotStock* Own = Depots.Find(R.Town);
			if (Own && R.Ammo < 1.f && Own->Ammo > 0.f)
			{
				const float Take = FMath::Min(1.f - R.Ammo, Own->Ammo);
				Own->Ammo -= Take;
				R.Ammo += Take;
			}
			continue;
		}
		// In the field: they eat what they carry...
		R.Food -= DeltaDays;
		if (bFodder)
		{
			R.Fodder -= DeltaDays;
		}
		// ... and refill from a depot within a day's march, or buy in a town they pass.
		const FVector2D Km = R.Km;
		if (const int32 Depot = DepotFor(Km); Depot != INDEX_NONE)
		{
			FCampaign1851DepotStock& S = Depots[Depot];
			const float NeedFood = FMath::Max(0.f, FoodCarried - R.Food) * R.Men;
			const float TakeFood = FMath::Min(NeedFood, S.Food);
			S.Food -= TakeFood;
			R.Food += R.Men > 0 ? TakeFood / R.Men : 0.f;
			if (bFodder)
			{
				const float NeedFodder = FMath::Max(0.f, FodderCarried - R.Fodder) * R.Horses;
				const float TakeFodder = FMath::Min(NeedFodder, S.Fodder);
				S.Fodder -= TakeFodder;
				R.Fodder += R.Horses > 0 ? TakeFodder / R.Horses : 0.f;
			}
			if (R.Ammo < 1.f && S.Ammo > 0.f)
			{
				const float Take = FMath::Min(1.f - R.Ammo, S.Ammo);
				S.Ammo -= Take;
				R.Ammo += Take;
			}
		}
		else if (const int32 Town = TownForPurchase(Km); Town != INDEX_NONE)
		{
			// The town sells what it can spare in a day (about a ration for every tenth inhabitant).
			const float Spare = Cities[Town].Population * 0.1f * DeltaDays;
			const float TakeFood = FMath::Min(FMath::Max(0.f, FoodCarried - R.Food) * R.Men, Spare);
			R.Food += R.Men > 0 ? TakeFood / R.Men : 0.f;
			Bought += TakeFood * PurchasePricePerRation;
			if (bFodder)
			{
				const float TakeFodder = FMath::Min(FMath::Max(0.f, FodderCarried - R.Fodder) * R.Horses, Spare);
				R.Fodder += R.Horses > 0 ? TakeFodder / R.Horses : 0.f;
				Bought += TakeFodder * FodderPrice;
			}
		}
		// Want: hunger breaks cohesion and spirit, sickness and desertion take men; horses fail without fodder.
		if (R.Food < 0.f)
		{
			const float Days = FMath::Min(DeltaDays, -R.Food);
			R.Cohesion = FMath::Max(0.f, R.Cohesion - 4.f * Days);
			R.Morale = FMath::Max(0.05f, R.Morale - 0.02f * Days);
			R.Men = FMath::Max(0, R.Men - FMath::CeilToInt(R.Men * 0.004f * Days));
			R.Food = 0.f;
			bHungerNews |= true;
		}
		if (bFodder && R.Fodder < 0.f)
		{
			const float Days = FMath::Min(DeltaDays, -R.Fodder);
			R.Cohesion = FMath::Max(0.f, R.Cohesion - 3.f * Days);
			R.Horses = FMath::Max(0, R.Horses - FMath::CeilToInt(R.Horses * 0.01f * Days));
			R.Fodder = 0.f;
		}
	}
	if (Bought > 0.0)
	{
		Treasury -= Bought;
		MonthSpend.FindOrAdd(TEXT("Opkøb af proviant og foder i byerne")) += Bought;
	}
	// The forts' garrisons eat from the fort's store, refilled like a column's.
	for (FCampaign1851Fort& F : Forts)
	{
		if (!F.bBuilt || F.Companies.Num() == 0)
		{
			continue;
		}
		F.FoodDays -= DeltaDays;
		if (const int32 Depot = DepotFor(F.Km); Depot != INDEX_NONE)
		{
			FCampaign1851DepotStock& S = Depots[Depot];
			const float Need = FMath::Max(0.f, FortFoodDays - F.FoodDays) * F.Garrison;
			const float Take = FMath::Min(Need, S.Food);
			S.Food -= Take;
			F.FoodDays += F.Garrison > 0 ? Take / F.Garrison : 0.f;
		}
		else if (TownForPurchase(F.Km) != INDEX_NONE)
		{
			const float Need = FMath::Max(0.f, FortFoodDays - F.FoodDays) * F.Garrison;
			F.FoodDays = FortFoodDays;
			Treasury -= Need * PurchasePricePerRation;
			MonthSpend.FindOrAdd(TEXT("Opkøb af proviant og foder i byerne")) += Need * PurchasePricePerRation;
		}
		F.FoodDays = FMath::Max(F.FoodDays, 0.f);
	}
}

double ACampaign1851Map::StockingCostPerMonth() const
{
	double Total = 0.0;
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		const FCampaign1851DepotCapacity Cap = DepotCapacity(c);
		if (Cap.Food + Cap.Fodder + Cap.Ammo <= 0.f)
		{
			continue;
		}
		const FCampaign1851DepotStock S = DepotStock(c);
		Total += FMath::Max(0.f, Cap.Food - S.Food) * Campaign1851Supply::DepotPricePerRation * Campaign1851Supply::RefillShare
			+ FMath::Max(0.f, Cap.Fodder - S.Fodder) * Campaign1851Supply::FodderPrice * Campaign1851Supply::RefillShare
			+ FMath::Max(0.f, Cap.Ammo - S.Ammo) * AmmoLoadPrice() * Campaign1851Supply::RefillShare;
	}
	return Total;
}

double ACampaign1851Map::AmmoLoadPrice() const
{
	// Own ammunition works make it cheaper than buying abroad.
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site && !Site->IsGarrison() && (Site->GetKind() == TEXT("Arsenal") || Site->GetKind() == TEXT("Ammunition_Works")) && Site->IsModuleDone(0))
		{
			return Campaign1851Supply::AmmoLoadPrice * 0.6;
		}
	}
	return Campaign1851Supply::AmmoLoadPrice;
}

void ACampaign1851Map::MonthlySupply()
{
	// The intendance buys a share of what the depots lack each month (bread, oats and hay from the amter).
	double Cost = 0.0;
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		const FCampaign1851DepotCapacity Cap = DepotCapacity(c);
		if (Cap.Food + Cap.Fodder + Cap.Ammo <= 0.f)
		{
			Depots.Remove(c);
			continue;
		}
		FCampaign1851DepotStock& S = Depots.FindOrAdd(c);
		const float Food = FMath::Max(0.f, Cap.Food - S.Food) * Campaign1851Supply::RefillShare;
		const float Fodder = FMath::Max(0.f, Cap.Fodder - S.Fodder) * Campaign1851Supply::RefillShare;
		const float Ammo = FMath::Max(0.f, Cap.Ammo - S.Ammo) * Campaign1851Supply::RefillShare;
		const double Price = Food * Campaign1851Supply::DepotPricePerRation + Fodder * Campaign1851Supply::FodderPrice + Ammo * AmmoLoadPrice();
		if (Price > Treasury)
		{
			continue;
		}
		S.Food += Food;
		S.Fodder += Fodder;
		S.Ammo += Ammo;
		Cost += Price;
	}
	if (Cost >= 1.0)
	{
		AddTransaction(-Cost, TEXT("Forråd til depoterne"));
	}
	if (bHungerNews)
	{
		News.Add(TEXT("Tropper i felten mangler proviant: samhørighed og moral falder, folk bliver syge"));
		bHungerNews = false;
	}
	ExportUnits();
}

TArray<FString> ACampaign1851Map::SaveSupply() const
{
	TArray<FString> Out;
	for (const TPair<int32, FCampaign1851DepotStock>& D : Depots)
	{
		if (Cities.IsValidIndex(D.Key))
		{
			Out.Add(FString::Printf(TEXT("depot|%s|%.0f|%.0f|%.2f"), *Cities[D.Key].Name, D.Value.Food, D.Value.Fodder, D.Value.Ammo));
		}
	}
	for (const FCampaign1851Regiment& R : Regiments)
	{
		Out.Add(FString::Printf(TEXT("unit|%s|%.2f|%.2f|%.3f"), *R.Id, R.Food, R.Fodder, R.Ammo));
	}
	for (const FCampaign1851Fort& F : Forts)
	{
		Out.Add(FString::Printf(TEXT("fort|%d|%.2f|%.0f|%.0f"), F.Id, F.FoodDays, F.RoundsPerGun, F.CartridgesPerMan));
	}
	return Out;
}

void ACampaign1851Map::RestoreSupply(const TArray<FString>& Lines)
{
	Depots.Reset();
	for (const FString& Line : Lines)
	{
		TArray<FString> P;
		Line.ParseIntoArray(P, TEXT("|"), false);
		if (P.Num() == 5 && P[0] == TEXT("depot") && FindCity(P[1]) != INDEX_NONE)
		{
			Depots.Add(FindCity(P[1]), { FCString::Atof(*P[2]), FCString::Atof(*P[3]), FCString::Atof(*P[4]) });
		}
		else if (P.Num() == 5 && P[0] == TEXT("unit") && FindRegiment(P[1]) != INDEX_NONE)
		{
			FCampaign1851Regiment& R = Regiments[FindRegiment(P[1])];
			R.Food = FCString::Atof(*P[2]);
			R.Fodder = FCString::Atof(*P[3]);
			R.Ammo = FCString::Atof(*P[4]);
		}
		else if (P.Num() == 5 && P[0] == TEXT("fort") && FortIndex(FCString::Atoi(*P[1])) != INDEX_NONE)
		{
			FCampaign1851Fort& F = Forts[FortIndex(FCString::Atoi(*P[1]))];
			F.FoodDays = FCString::Atof(*P[2]);
			F.RoundsPerGun = FCString::Atof(*P[3]);
			F.CartridgesPerMan = FCString::Atof(*P[4]);
		}
	}
}

void ACampaign1851Map::ExportUnits() const
{
	// Every regiment with its place, strength, supply and battle factors, for the 3D battles (Docs/Supply1851.md).
	TSharedRef<FJsonObject> Doc = MakeShared<FJsonObject>();
	Doc->SetStringField(TEXT("format"), TEXT("PROJECT1864-Units-1"));
	Doc->SetStringField(TEXT("date"), GetDate().ToIso8601());
	TArray<TSharedPtr<FJsonValue>> List;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		const FCampaign1851Regiment& R = Regiments[i];
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		const FVector2D LatLon = Extent.Projection.Inverse(R.Km);
		O->SetStringField(TEXT("id"), R.Id);
		O->SetStringField(TEXT("name"), R.Name);
		O->SetStringField(TEXT("arm"), Campaign1851Army::ArmName(R.Arm));
		O->SetNumberField(TEXT("lat"), LatLon.X);
		O->SetNumberField(TEXT("lon"), LatLon.Y);
		O->SetStringField(TEXT("place"), DescribePlace(R.Town, R.Km));
		O->SetNumberField(TEXT("men"), R.Men);
		O->SetNumberField(TEXT("maxMen"), R.MaxMen);
		O->SetNumberField(TEXT("horses"), R.Horses);
		O->SetNumberField(TEXT("guns"), R.Guns);
		O->SetNumberField(TEXT("foodDays"), R.Food);
		O->SetNumberField(TEXT("fodderDays"), R.Fodder);
		O->SetNumberField(TEXT("ammoFraction"), R.Ammo);
		O->SetNumberField(TEXT("cartridgesPerMan"), R.Guns > 0 ? 0.0 : R.Ammo * Campaign1851Supply::CartridgesCarried);
		O->SetNumberField(TEXT("roundsPerGun"), R.Guns > 0 ? R.Ammo * Campaign1851Supply::RoundsPerGun : 0.0);
		const Campaign1851Army::FBattleFactors B = Campaign1851Army::BattleFactors(R);
		TSharedRef<FJsonObject> Bf = MakeShared<FJsonObject>();
		Bf->SetNumberField(TEXT("reloadTime"), B.ReloadTime);
		Bf->SetNumberField(TEXT("accuracy"), B.Accuracy);
		Bf->SetNumberField(TEXT("deploySpeed"), B.DeploySpeed);
		Bf->SetNumberField(TEXT("skirmish"), B.Skirmish);
		Bf->SetNumberField(TEXT("fatigueRate"), B.FatigueRate);
		Bf->SetNumberField(TEXT("assault"), B.Assault);
		Bf->SetNumberField(TEXT("morale"), B.Morale);
		Bf->SetNumberField(TEXT("cohesion"), B.Cohesion);
		Bf->SetNumberField(TEXT("experience"), B.Experience);
		O->SetObjectField(TEXT("battleFactors"), Bf);
		List.Add(MakeShared<FJsonValueObject>(O));
	}
	Doc->SetArrayField(TEXT("units"), List);
	FString Text;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
	FJsonSerializer::Serialize(Doc, Writer);
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Battle/Units.json");
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

// ------------------------------------------------------------------ supply columns (F-5)

namespace
{
	/** A column's load in rations, fodder and ammunition loads (20 wagons of 800 kg). */
	constexpr float ColumnFood = 12000.f;
	constexpr float ColumnFodder = 4000.f;
	constexpr float ColumnAmmo = 6.f;
}

FVector2D ACampaign1851Map::SupplyTargetKm(const FCampaign1851SupplyColumn& C) const
{
	if (C.bFort)
	{
		const int32 F = FortIndex(C.Target);
		return F != INDEX_NONE ? Forts[F].Km : C.Km;
	}
	return Regiments.IsValidIndex(C.Target) ? Regiments[C.Target].Km : C.Km;
}

bool ACampaign1851Map::PlanColumnRoute(FCampaign1851SupplyColumn& C, const FVector2D& To, int32 ToTown)
{
	C.Route.Reset();
	C.Leg = 0;
	C.LegElapsed = 0.f;
	TArray<FCampaign1851Leg> Legs;
	if (!PlanMarch(INDEX_NONE, C.Km, ToTown, To, Campaign1851Supply::ColumnKmPerDay, ECampaign1851RouteMode::RoadsOnly, Legs))
	{
		if (!PlanMarch(INDEX_NONE, C.Km, ToTown, To, Campaign1851Supply::ColumnKmPerDay, ECampaign1851RouteMode::Direct, Legs))
		{
			return FVector2D::Distance(C.Km, To) < 0.5;   // already there
		}
	}
	C.Route = MoveTemp(Legs);
	return true;
}

bool ACampaign1851Map::SendSupplyColumn(bool bFort, int32 Target, FString* OutReason)
{
	auto Fail = [OutReason](const FString& Why) { if (OutReason) { *OutReason = Why; } return false; };
	const FVector2D To = bFort ? (FortIndex(Target) != INDEX_NONE ? Forts[FortIndex(Target)].Km : FVector2D::ZeroVector)
		: (Regiments.IsValidIndex(Target) ? Regiments[Target].Km : FVector2D::ZeroVector);
	if (To.IsZero())
	{
		return Fail(TEXT("-"));
	}
	if (SupplyColumns.ContainsByPredicate([&](const FCampaign1851SupplyColumn& C) { return C.bFort == bFort && C.Target == Target && C.State != ESupplyColumnState::Returning; }))
	{
		return Fail(TEXT("En trænkolonne er allerede på vej"));
	}
	if (FreeSupplyColumns() <= 0)
	{
		return Fail(TEXT("Ingen ledige trænkolonner: køb flere"));
	}
	// The nearest depot with stock (as the crow flies; the road decides the time).
	int32 Best = INDEX_NONE;
	double BestKm = 1e9;
	for (const TPair<int32, FCampaign1851DepotStock>& D : Depots)
	{
		const double Dist = FVector2D::Distance(TownKm(D.Key), To);
		if (D.Value.Food > 1000.f && Dist < BestKm)
		{
			BestKm = Dist;
			Best = D.Key;
		}
	}
	if (Best == INDEX_NONE)
	{
		return Fail(TEXT("Intet depot med forråd: byg Depot og magasin eller Kornmagasin"));
	}
	FCampaign1851DepotStock& S = Depots[Best];
	FCampaign1851SupplyColumn C;
	C.Id = NextSupplyColumnId++;
	C.Depot = Best;
	C.bFort = bFort;
	C.Target = Target;
	C.Km = TownKm(Best);
	C.Food = FMath::Min(ColumnFood, S.Food);
	C.Fodder = FMath::Min(ColumnFodder, S.Fodder);
	C.Ammo = FMath::Min(ColumnAmmo, S.Ammo);
	S.Food -= C.Food;
	S.Fodder -= C.Fodder;
	S.Ammo -= C.Ammo;
	C.State = ESupplyColumnState::Outbound;
	if (!PlanColumnRoute(C, To, INDEX_NONE))
	{
		S.Food += C.Food;
		S.Fodder += C.Fodder;
		S.Ammo += C.Ammo;
		return Fail(TEXT("Ingen vej derhen"));
	}
	SupplyColumns.Add(C);
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|supply|column %d from %s to %s"), C.Id, *Cities[Best].Name,
		bFort ? *Forts[FortIndex(Target)].Name : *Regiments[Target].Name);
	return true;
}

int32 ACampaign1851Map::FreeSupplyColumns() const
{
	return SupplyColumnCount - SupplyColumns.Num();
}

bool ACampaign1851Map::BuySupplyColumn()
{
	if (Treasury < Campaign1851Supply::ColumnCost)
	{
		return false;
	}
	AddTransaction(-Campaign1851Supply::ColumnCost, TEXT("Trænkolonne købt (20 vogne, 80 heste)"));
	TakeHorses(80, TEXT("trænkolonne"));
	++SupplyColumnCount;
	return true;
}

void ACampaign1851Map::AdvanceSupplyColumns(float DeltaDays)
{
	if (DeltaDays <= 0.f)
	{
		return;
	}
	// Frost and thaw: the wagons crawl in winter.
	const int32 Month = GetDate().GetMonth();
	const float Weather = (Month == 12 || Month <= 2) ? 0.6f : Month == 3 ? 0.8f : 1.f;
	for (int32 i = SupplyColumns.Num() - 1; i >= 0; --i)
	{
		FCampaign1851SupplyColumn& C = SupplyColumns[i];
		C.LegElapsed += DeltaDays * Weather;
		while (C.Route.IsValidIndex(C.Leg) && C.LegElapsed >= C.Route[C.Leg].Days)
		{
			C.LegElapsed -= C.Route[C.Leg].Days;
			C.Km = C.Route[C.Leg].ToKm;
			++C.Leg;
		}
		if (C.Route.IsValidIndex(C.Leg))
		{
			const FCampaign1851Leg& L = C.Route[C.Leg];
			const TArray<FVector2D> Line = LegLine(L);
			C.Km = AlongLine(Line, LineLength(Line) * FMath::Clamp(C.LegElapsed / FMath::Max(L.Days, 0.001f), 0.f, 1.f));
			continue;
		}
		// Arrived.
		if (C.State == ESupplyColumnState::Returning)
		{
			FCampaign1851DepotStock& S = Depots.FindOrAdd(C.Depot);
			S.Food += C.Food;
			S.Fodder += C.Fodder;
			S.Ammo += C.Ammo;
			SupplyColumns.RemoveAt(i);
			continue;
		}
		const FVector2D To = SupplyTargetKm(C);
		if (FVector2D::Distance(C.Km, To) > 3.0)
		{
			// The unit has marched on: follow it.
			if (!PlanColumnRoute(C, To, INDEX_NONE))
			{
				C.State = ESupplyColumnState::Returning;
				PlanColumnRoute(C, TownKm(C.Depot), C.Depot);
			}
			continue;
		}
		// Hand over what they need, then drive home with the rest.
		if (C.bFort)
		{
			const int32 F = FortIndex(C.Target);
			if (F != INDEX_NONE)
			{
				FCampaign1851Fort& Fort = Forts[F];
				const float Men = float(FMath::Max(Fort.Garrison, 1));
				const float Food = FMath::Min(C.Food, FMath::Max(0.f, Campaign1851Supply::FortFoodDays - Fort.FoodDays) * Men);
				Fort.FoodDays += Food / Men;
				C.Food -= Food;
				const float Ammo = FMath::Min(C.Ammo, 1.f);
				if (Ammo > 0.f)
				{
					Fort.RoundsPerGun = FMath::Min(120.f, Fort.RoundsPerGun + 60.f * Ammo);
					Fort.CartridgesPerMan = FMath::Min(100.f, Fort.CartridgesPerMan + 50.f * Ammo);
					C.Ammo -= Ammo;
				}
				News.Add(FString::Printf(TEXT("Trænkolonnen har forsynet %s"), *Fort.Name));
			}
		}
		else if (Regiments.IsValidIndex(C.Target))
		{
			FCampaign1851Regiment& R = Regiments[C.Target];
			const float Food = FMath::Min(C.Food, FMath::Max(0.f, Campaign1851Supply::FoodCarried - R.Food) * R.Men);
			R.Food += R.Men > 0 ? Food / R.Men : 0.f;
			C.Food -= Food;
			const float Fodder = FMath::Min(C.Fodder, FMath::Max(0.f, Campaign1851Supply::FodderCarried - R.Fodder) * R.Horses);
			R.Fodder += R.Horses > 0 ? Fodder / R.Horses : 0.f;
			C.Fodder -= Fodder;
			const float Ammo = FMath::Min(C.Ammo, 1.f - R.Ammo);
			R.Ammo += Ammo;
			C.Ammo -= Ammo;
			News.Add(FString::Printf(TEXT("Trænkolonnen har forsynet %s"), *R.Name));
		}
		C.State = ESupplyColumnState::Returning;
		if (!PlanColumnRoute(C, TownKm(C.Depot), C.Depot))
		{
			SupplyColumns.RemoveAt(i);
		}
	}
	// Units far from any depot and running short call for a column (the intendance's standing order).
	for (int32 r = 0; r < Regiments.Num(); ++r)
	{
		const FCampaign1851Regiment& R = Regiments[r];
		if (!(R.IsMarching() || R.IsInField()) || DepotFor(R.Km) != INDEX_NONE || FreeSupplyColumns() <= 0
			|| SupplyColumns.ContainsByPredicate([r](const FCampaign1851SupplyColumn& C) { return !C.bFort && C.Target == r && C.State == ESupplyColumnState::Outbound; }))
		{
			continue;
		}
		// Order in time: the drive from the nearest stocked depot (roads wind, winter slows) and a day to spare.
		double Nearest = 1e9;
		for (const TPair<int32, FCampaign1851DepotStock>& D : Depots)
		{
			Nearest = D.Value.Food > 1000.f ? FMath::Min(Nearest, FVector2D::Distance(TownKm(D.Key), R.Km)) : Nearest;
		}
		const float Drive = Nearest < 1e8 ? float(Nearest * 1.4 / (Campaign1851Supply::ColumnKmPerDay * Weather)) : 0.f;
		if (Nearest < 1e8 && R.Food < FMath::Min(Drive + 1.f, Campaign1851Supply::FoodCarried - 0.5f))
		{
			SendSupplyColumn(false, r);
		}
	}
	UpdateSupplyColumnPieces();
}

void ACampaign1851Map::UpdateSupplyColumnPieces()
{
	if (!ColumnMesh)
	{
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery"));
		if (!Material)
		{
			return;
		}
		ColumnMesh = Campaign1851Scenery::BuildSitePiece(Campaign1851Scenery::ESitePiece::Wagon, Material);
	}
	while (SupplyColumnPieces.Num() < SupplyColumns.Num())
	{
		UStaticMeshComponent* P = NewObject<UStaticMeshComponent>(this);
		P->SetupAttachment(Root);
		P->SetStaticMesh(ColumnMesh);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->RegisterComponent();
		SupplyColumnPieces.Add(P);
	}
	for (int32 i = 0; i < SupplyColumnPieces.Num(); ++i)
	{
		UStaticMeshComponent* P = SupplyColumnPieces[i];
		if (!P)
		{
			continue;
		}
		if (!SupplyColumns.IsValidIndex(i))
		{
			P->SetVisibility(false);
			continue;
		}
		const FCampaign1851SupplyColumn& C = SupplyColumns[i];
		FVector2D Dir(1.0, 0.0);
		if (C.Route.IsValidIndex(C.Leg))
		{
			const TArray<FVector2D> Line = LegLine(C.Route[C.Leg]);
			AlongLine(Line, LineLength(Line) * FMath::Clamp(C.LegElapsed / FMath::Max(C.Route[C.Leg].Days, 0.001f), 0.f, 1.f), &Dir);
		}
		// Km north is -Y in the world.
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-Dir.Y, Dir.X));
		P->SetWorldTransform(FTransform(FRotator(0.f, Yaw, 0.f), WorldAtKm(C.Km), FVector(PieceScale * 3.f)));
		P->SetVisibility(LastCameraDistanceKm < 60.f);
	}
}

TArray<FString> ACampaign1851Map::SaveSupplyColumns() const
{
	TArray<FString> Out;
	Out.Add(FString::Printf(TEXT("count|%d"), SupplyColumnCount));
	for (const FCampaign1851SupplyColumn& C : SupplyColumns)
	{
		// Columns under way are saved as their loads going home to their depot (a simple, safe restore).
		if (Cities.IsValidIndex(C.Depot))
		{
			Out.Add(FString::Printf(TEXT("home|%s|%.0f|%.0f|%.2f"), *Cities[C.Depot].Name, C.Food, C.Fodder, C.Ammo));
		}
	}
	return Out;
}

void ACampaign1851Map::RestoreSupplyColumns(const TArray<FString>& Lines)
{
	SupplyColumns.Reset();
	SupplyColumnCount = Campaign1851Supply::ColumnsAtStart;
	for (const FString& Line : Lines)
	{
		TArray<FString> P;
		Line.ParseIntoArray(P, TEXT("|"), false);
		if (P.Num() == 2 && P[0] == TEXT("count"))
		{
			SupplyColumnCount = FCString::Atoi(*P[1]);
		}
		else if (P.Num() == 5 && P[0] == TEXT("home") && FindCity(P[1]) != INDEX_NONE)
		{
			FCampaign1851DepotStock& S = Depots.FindOrAdd(FindCity(P[1]));
			S.Food += FCString::Atof(*P[2]);
			S.Fodder += FCString::Atof(*P[3]);
			S.Ammo += FCString::Atof(*P[4]);
		}
	}
	UpdateSupplyColumnPieces();
}
