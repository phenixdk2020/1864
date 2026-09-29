// Supply of the 1851 campaign (Docs/Backlog-Forsyning.md, first delivery F-1 .. F-4, F-7, F-11): what each
// unit carries (rations, fodder, ammunition), the depots that refill it, local purchase in the towns, the
// consequences of want, the forts' magazines, and the data the 3D battles read. ACampaign1851Map's supply layer.

#include "Campaign1851Map.h"

#include "Campaign1851ConstructionSite.h"
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
