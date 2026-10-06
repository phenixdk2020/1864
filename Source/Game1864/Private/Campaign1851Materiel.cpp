// Materiel of the 1851 campaign: the state's stores of rifles, guns and horses, the works that make them
// (rifle workshop, cannon foundry, arsenal, stud and remount depot), and buying abroad or in the amter
// what the stores lack. ACampaign1851Map's materiel layer (Docs/Backlog.md items 2 and 3).

#include "Campaign1851Map.h"

#include "Campaign1851ConstructionSite.h"

namespace Campaign1851Materiel
{
	FMateriel Production(const FString& Key)
	{
		// A month's output of a finished works (estimates).
		if (Key == TEXT("Rifle_Workshop")) return { 150, 0, 0 };
		if (Key == TEXT("Arsenal")) return { 60, 1, 0 };
		if (Key == TEXT("Cannon_Foundry")) return { 0, 2, 0 };
		if (Key == TEXT("Stud_Farm")) return { 0, 0, 20 };
		if (Key == TEXT("Remount_Depot")) return { 0, 0, 40 };
		return {};
	}
}

void ACampaign1851Map::ResetMateriel()
{
	// After the war: the depots hold spare rifles, the arsenal field guns, the army a remount reserve.
	Rifles = Campaign1851Materiel::RiflesAtStart;
	Horses = Campaign1851Materiel::HorsesAtStart;
	GunStock = Campaign1851Materiel::GunsAtStart;
}

void ACampaign1851Map::MonthlyMateriel()
{
	// The arms works make as much as the iron, coal and timber in store allow.
	const float Share = MonthlyRawMaterials() * WorksOutputFactor();   // the smithies and the steam engines
	Campaign1851Materiel::FMateriel Made;
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site && !Site->IsGarrison() && !Site->IsDemolishing() && Site->IsModuleDone(0))
		{
			const Campaign1851Materiel::FMateriel P = Campaign1851Materiel::Production(Site->GetKind());
			Made.Rifles += FMath::RoundToInt(P.Rifles * Share);
			Made.Guns += FMath::FloorToInt(P.Guns * Share + 0.5f);
			Made.Horses += P.Horses;
		}
	}
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site && !Site->IsGarrison() && !Site->IsDemolishing() && Site->IsModuleDone(0))
		{
			MortarStock += Site->GetKind() == TEXT("Arsenal") && Share > 0.5f ? 1 : 0;
			WagonStock += Site->GetKind() == TEXT("Wagon_Works") ? FMath::RoundToInt(20.f * Share) : 0;
		}
	}
	Rifles += Made.Rifles;
	GunStock += Made.Guns;
	Horses += Made.Horses;
	if (Made.Rifles + Made.Guns + Made.Horses > 0)
	{
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|materiel|made %d rifles, %d guns, %d horses|store %d / %d / %d"), Made.Rifles, Made.Guns, Made.Horses, Rifles, GunStock, Horses);
	}
	// Mounted units and batteries are brought back to their horses from the store, or by buying in the amter.
	int32 Bought = 0;
	for (FCampaign1851Regiment& R : Regiments)
	{
		const int32 Need = FMath::Max(0, R.MaxHorses - R.Horses);
		if (Need <= 0 || R.IsMarching())
		{
			continue;
		}
		const int32 FromStore = FMath::Min(Need, Horses);
		Horses -= FromStore;
		int32 Buy = Need - FromStore;
		Buy = FMath::Min(Buy, FMath::FloorToInt(float(Treasury) / Campaign1851Materiel::HorsePrice));
		R.Horses += FromStore + Buy;
		Bought += Buy;
	}
	if (Bought > 0)
	{
		AddTransaction(-Bought * double(Campaign1851Materiel::HorsePrice), FString::Printf(TEXT("%d heste opkøbt i amterne"), Bought));
	}
}

double ACampaign1851Map::TakeRifles(int32 Wanted, const FString& For)
{
	// From the store first; what is missing is bought abroad (dearer), paid at once.
	const int32 FromStore = FMath::Min(Wanted, Rifles);
	Rifles -= FromStore;
	const int32 Import = Wanted - FromStore;
	const double Cost = Import * double(Campaign1851Materiel::RifleImportPrice);
	if (Import > 0)
	{
		AddTransaction(-Cost, FString::Printf(TEXT("%d geværer købt i udlandet: %s"), Import, *For));
	}
	return Cost;
}

int32 ACampaign1851Map::TakeHorses(int32 Wanted, const FString& For)
{
	const int32 FromStore = FMath::Min(Wanted, Horses);
	Horses -= FromStore;
	const int32 Buy = Wanted - FromStore;
	if (Buy > 0)
	{
		AddTransaction(-Buy * double(Campaign1851Materiel::HorsePrice), FString::Printf(TEXT("%d heste opkøbt: %s"), Buy, *For));
	}
	return Wanted;
}
