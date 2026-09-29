// Pulling down forts and buildings, and using what comes of it: the fortress guns go to the state's gun
// store (for any fort), bricks, stone and timber to a materials store at the nearest town, which pays part
// of the next works within 30 km (or is sold off at half after a year). ACampaign1851Map's salvage layer.

#include "Campaign1851Map.h"

#include "Campaign1851Buildings.h"
#include "Campaign1851ConstructionSite.h"

namespace
{
	/** Share of a price that comes back as materials, by the kind of construction. */
	double SalvageShare(const FString& Type)
	{
		if (Type == TEXT("grundmur") || Type == TEXT("stenværk") || Type == TEXT("industri"))
		{
			return 0.35;   // bricks, stone, roof tiles, iron
		}
		if (Type == TEXT("bindingsværk"))
		{
			return 0.2;    // timber
		}
		return 0.1;        // earthworks: little but the planks
	}
}

void ACampaign1851Map::AddMaterials(int32 CityIndex, double Rd, const FString& From, bool bNews)
{
	if (Rd < 1.0 || !Cities.IsValidIndex(CityIndex))
	{
		return;
	}
	MaterialLots.Add(FVector(double(CityIndex), Rd, CampaignDays));
	if (!bNews)
	{
		return;
	}
	News.Add(FString::Printf(TEXT("Materialer for %s rd. fra %s ligger nu på lager i %s"), *FString::FromInt(FMath::RoundToInt(Rd)), *From, *Cities[CityIndex].Name));
}

double ACampaign1851Map::MaterialsNear(const FVector2D& Km) const
{
	double Total = 0.0;
	for (const FVector& Lot : MaterialLots)
	{
		const int32 C = int32(Lot.X);
		Total += Cities.IsValidIndex(C) && FVector2D::Distance(TownKm(C), Km) <= MaterialReachKm ? Lot.Y : 0.0;
	}
	return Total;
}

void ACampaign1851Map::UseMaterials(const FVector2D& Km, double Cost, const FString& For)
{
	// Up to half the price is paid in materials already on hand nearby (the oldest lots first).
	double Want = Cost * 0.5, Used = 0.0;
	for (int32 i = 0; i < MaterialLots.Num() && Want > 0.5; ++i)
	{
		FVector& Lot = MaterialLots[i];
		const int32 C = int32(Lot.X);
		if (!Cities.IsValidIndex(C) || FVector2D::Distance(TownKm(C), Km) > MaterialReachKm)
		{
			continue;
		}
		const double Take = FMath::Min(Lot.Y, Want);
		Lot.Y -= Take;
		Want -= Take;
		Used += Take;
	}
	MaterialLots.RemoveAll([](const FVector& Lot) { return Lot.Y < 1.0; });
	if (Used >= 1.0)
	{
		AddTransaction(Used, FString::Printf(TEXT("Genbrugte materialer: %s"), *For));
	}
}

void ACampaign1851Map::MonthlySalvage()
{
	// Materials lying unused for a year are sold off at half their worth.
	double Sold = 0.0;
	for (FVector& Lot : MaterialLots)
	{
		if (CampaignDays - Lot.Z > 365.0)
		{
			Sold += Lot.Y * 0.5;
			Lot.Y = 0.0;
		}
	}
	MaterialLots.RemoveAll([](const FVector& Lot) { return Lot.Y < 1.0; });
	if (Sold >= 1.0)
	{
		AddTransaction(Sold, TEXT("Overskydende byggematerialer solgt"));
	}
}

double ACampaign1851Map::MaterialsIn(int32 CityIndex) const
{
	double Total = 0.0;
	for (const FVector& Lot : MaterialLots)
	{
		Total += int32(Lot.X) == CityIndex ? Lot.Y : 0.0;
	}
	return Total;
}

void ACampaign1851Map::MonthlyBuildingMaterials()
{
	// Brickworks and sawmills (the state's or private) deliver bricks and timber to their town's store, which
	// pays part of the works within 30 km; the store of a town holds up to a few thousand rigsdaler.
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		if (!Site || Site->IsGarrison() || Site->IsDemolishing() || !Site->IsModuleDone(0))
		{
			continue;
		}
		const double Made = Site->GetKind() == TEXT("Brickworks") ? 600.0 : Site->GetKind() == TEXT("Sawmill") ? 300.0 : 0.0;
		const int32 City = Site->GetCityIndex();
		if (Made > 0.0 && MaterialsIn(City) < MaterialStoreCap)
		{
			AddMaterials(City, FMath::Min(Made, MaterialStoreCap - MaterialsIn(City)), Site->ModuleName(0).ToLower(), false);
		}
	}
}

// ------------------------------------------------------------------ buildings

FString ACampaign1851Map::DemolishText(const ACampaign1851ConstructionSite* Site) const
{
	if (!Site)
	{
		return FString();
	}
	double Salvage = 0.0, Cost = 0.0;
	for (int32 m = 0; m < Site->NumModules(); ++m)
	{
		const double Share = Site->IsModuleStarted(m) ? FMath::Clamp(Site->GetModuleProgress(m), 0.f, 1.f) : 0.0;
		Salvage += Site->ModuleCost(m) * Share * SalvageShare(Site->ModuleType(m));
		Cost += Site->ModuleCost(m) * Share;
	}
	return FString::Printf(TEXT("materialer ca. %d rd.%s"), FMath::RoundToInt(Salvage), Site->IsPrivate() ? *FString::Printf(TEXT("  ·  erstatning til ejeren %d rd."), FMath::RoundToInt(Cost * 0.5)) : TEXT(""));
}

bool ACampaign1851Map::DemolishSite(ACampaign1851ConstructionSite* Site, FString* OutReason)
{
	if (!Site || !Projects.Contains(Site) || Site->IsDemolishing())
	{
		if (OutReason) { *OutReason = TEXT("Kan ikke rives ned"); }
		return false;
	}
	double Built = 0.0, BuildDays = 0.0;
	for (int32 m = 0; m < Site->NumModules(); ++m)
	{
		const double Share = Site->IsModuleStarted(m) ? FMath::Clamp(Site->GetModuleProgress(m), 0.f, 1.f) : 0.0;
		Built += Site->ModuleCost(m) * Share;
		BuildDays += Site->ModuleDays(m) * Share;
	}
	if (Site->IsPrivate())
	{
		const double Compensation = Built * 0.5;
		if (Compensation > Treasury)
		{
			if (OutReason) { *OutReason = TEXT("Ikke råd til erstatningen til ejeren"); }
			return false;
		}
		AddTransaction(-Compensation, FString::Printf(TEXT("Erstatning for nedrevet %s i %s"), *Site->ModuleName(0).ToLower(), *Cities[Site->GetCityIndex()].Name));
	}
	// A fifth of the building time; the labourers' wages a tenth of the price, paid day by day.
	Site->StartDemolition(FMath::Max(5.f, float(BuildDays) * 0.2f), float(Built) * 0.1f);
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|salvage|%s|%s pulled down"), *Cities[Site->GetCityIndex()].Name, *Site->GetKind());
	return true;
}

void ACampaign1851Map::AdvanceDemolitions(float DeltaDays)
{
	if (DeltaDays <= 0.f)
	{
		return;
	}
	for (int32 i = Projects.Num() - 1; i >= 0; --i)
	{
		ACampaign1851ConstructionSite* Site = Projects[i];
		if (!Site || !Site->IsDemolishing())
		{
			continue;
		}
		const float Work = DeltaDays * Campaign1851Buildings::WorkRate(Site->ModuleType(0), GetDate());
		const double PerDay = Site->GetDemolishWages() / FMath::Max(Site->GetDemolishDays(), 1.f);
		if (Work * PerDay > Treasury)
		{
			continue;   // no wages, no work
		}
		Treasury -= Work * PerDay;
		MonthSpend.FindOrAdd(FString::Printf(TEXT("Nedrivning: %s, %s"), *Cities[Site->GetCityIndex()].Name, *Site->ModuleName(0).ToLower())) += Work * PerDay;
		if (Site->AdvanceDemolition(Work))
		{
			// Down: what stood goes to the town's materials store.
			double Salvage = 0.0;
			const TArray<float>& Before = Site->GetDemolishFrom();
			for (int32 m = 0; m < Site->NumModules(); ++m)
			{
				const double Share = Before.IsValidIndex(m) && Before[m] > 0.f ? FMath::Clamp(Before[m] / FMath::Max(Site->ModuleDays(m), 1.f), 0.f, 1.f) : 0.0;
				Salvage += Site->ModuleCost(m) * Share * SalvageShare(Site->ModuleType(m));
			}
			const int32 City = Site->GetCityIndex();
			AddMaterials(City, Salvage, Site->GetKind() == TEXT("Garrison") ? FString(TEXT("garnisonen")) : Site->ModuleName(0).ToLower());
			News.Add(FString::Printf(TEXT("%s i %s er revet ned"), *Site->ModuleName(0), *Cities[City].Name));
			Projects.RemoveAt(i);
			Site->Destroy();
		}
	}
}

// ------------------------------------------------------------------ forts

bool ACampaign1851Map::DemolishFort(int32 Id, FString* OutReason)
{
	const int32 Index = FortIndex(Id);
	if (Index == INDEX_NONE || Forts[Index].Work == ECampaign1851FortWork::Demolish)
	{
		if (OutReason) { *OutReason = TEXT("Kan ikke rives ned"); }
		return false;
	}
	FCampaign1851Fort& F = Forts[Index];
	// The companies leave for their battalions; the works stop.
	for (const FCampaign1851FortCompany& C : F.Companies)
	{
		if (Regiments.IsValidIndex(C.Regiment))
		{
			Regiments[C.Regiment].Men += C.Men;
			if (Regiments[C.Regiment].CompanyFort.IsValidIndex(C.Company))
			{
				Regiments[C.Regiment].CompanyFort[C.Company] = 0;
			}
		}
	}
	F.Companies.Reset();
	F.Garrison = 0;
	F.Work = ECampaign1851FortWork::Demolish;
	F.WorkDays = Campaign1851Forts::BuildDays(F.bLarge) * 0.2f;
	F.WorkCost = FMath::RoundToInt(Campaign1851Forts::BuildCost(F.bLarge) * 0.1f / (1.f - Campaign1851Buildings::DownPayment));   // wages only
	F.DaysBuilt = 0.f;
	ExportForts();
	return true;
}

void ACampaign1851Map::FinishFortDemolition(int32 Index)
{
	FCampaign1851Fort& F = Forts[Index];
	// Anyone still in it goes back to his battalion.
	for (const FCampaign1851FortCompany& C : F.Companies)
	{
		if (Regiments.IsValidIndex(C.Regiment))
		{
			Regiments[C.Regiment].Men += C.Men;
			if (Regiments[C.Regiment].CompanyFort.IsValidIndex(C.Company))
			{
				Regiments[C.Regiment].CompanyFort[C.Company] = 0;
			}
		}
	}
	GunStock += F.Guns;
	const int32 Near = NearestTown(F.Km);
	AddMaterials(Near, F.Invested * 0.1, F.Name);
	News.Add(FString::Printf(TEXT("%s er sløjfet: %d kanoner på lager"), *F.Name, F.Guns));
	for (int32 p = FortParts.Num() - 1; p >= 0; --p)
	{
		if (FortPartOwner[p] == F.Id)
		{
			if (FortParts[p])
			{
				FortParts[p]->DestroyComponent();
			}
			FortParts.RemoveAt(p);
			FortPartOwner.RemoveAt(p);
		}
	}
	Forts.RemoveAt(Index);
	UpdateTrenches();
	ExportForts();
}

int32 ACampaign1851Map::TakeGunsFromStock(int32 Wanted)
{
	const int32 Taken = FMath::Clamp(Wanted, 0, GunStock);
	GunStock -= Taken;
	return Taken;
}
