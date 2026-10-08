// Peace footing and mobilisation of the 1851 campaign: most men on leave in peace, called in when war threatens.

#include "Campaign1851Map.h"

#include "Campaign1851ConstructionSite.h"

namespace Campaign1851Mobilisation
{
	const TCHAR* FootingName(ECampaign1851Footing F)
	{
		return F == ECampaign1851Footing::Peace ? TEXT("Fredsfod") : F == ECampaign1851Footing::Mobilising ? TEXT("Mobiliserer") : TEXT("Krigsfod");
	}
}

bool ACampaign1851Map::Mobilise(FString* OutReason)
{
	if (Footing != ECampaign1851Footing::Peace)
	{
		if (OutReason) { *OutReason = TEXT("Hæren er allerede mobiliseret"); }
		return false;
	}
	if (Treasury < Campaign1851Mobilisation::OrderCost)
	{
		if (OutReason) { *OutReason = TEXT("Ikke råd til mobiliseringen"); }
		return false;
	}
	AddTransaction(-Campaign1851Mobilisation::OrderCost, TEXT("Mobilisering: indkaldelse af de hjemsendte"));
	Footing = ECampaign1851Footing::Mobilising;
	News.Add(TEXT("Hæren mobiliseres: de hjemsendte kaldes ind"));
	return true;
}

bool ACampaign1851Map::CanUpgradeToHorseBattery(int32 RegimentIndex, FString* OutWhy) const
{
	auto Fail = [OutWhy](const FString& Why) { if (OutWhy) { *OutWhy = Why; } return false; };
	if (!Regiments.IsValidIndex(RegimentIndex))
	{
		return Fail(TEXT("Ingen enhed"));
	}
	const FCampaign1851Regiment& R = Regiments[RegimentIndex];
	if (R.Arm != ECampaign1851Arm::Artillery || R.Mortars > 0 || R.Guns <= 0)
	{
		return Fail(TEXT("Kun et fodbatteri kan blive ridende"));
	}
	if (R.IsMarching())
	{
		return Fail(TEXT("Batteriet skal stå stille (ikke på march)"));
	}
	const int32 NeedHorses = FMath::Max(0, HorseBatteryHorses - R.Horses);
	if (Horses < NeedHorses)
	{
		return Fail(FString::Printf(TEXT("Mangler heste: %d på lager, %d skal bruges"), Horses, NeedHorses));
	}
	if (Treasury < HorseBatteryCost)
	{
		return Fail(TEXT("Ikke råd"));
	}
	return true;
}

bool ACampaign1851Map::UpgradeToHorseBattery(int32 RegimentIndex, FString* OutWhy)
{
	if (!CanUpgradeToHorseBattery(RegimentIndex, OutWhy))
	{
		return false;
	}
	FCampaign1851Regiment& R = Regiments[RegimentIndex];
	const int32 NeedHorses = FMath::Max(0, HorseBatteryHorses - R.Horses);
	Horses -= NeedHorses;
	R.Horses += NeedHorses;
	R.MaxHorses = HorseBatteryHorses;
	GunStock += FMath::Max(0, R.Guns - 6);
	R.Guns = FMath::Min(R.Guns, 6);
	R.MaxMen = FMath::Max(R.MaxMen, 180);
	R.Arm = ECampaign1851Arm::HorseArtillery;
	R.PaceKmPerDay = Campaign1851Army::MarchKmPerDay(R.Arm);
	if (R.CustomName.IsEmpty()) { R.Name = R.Name.Replace(TEXT("Batteri"), TEXT("Ridende Batteri")); }
	// The gunners must learn to ride with the guns: the drill falls for a while.
	R.Skills[int32(ECampaign1851Skill::Drill)] = FMath::Max(30.f, R.Skills[int32(ECampaign1851Skill::Drill)] - 15.f);
	AddTransaction(-HorseBatteryCost, FString::Printf(TEXT("%s gøres ridende"), *R.Name));
	News.Add(FString::Printf(TEXT("%s er nu et ridende batteri (6 kanoner, alle kanonerer til hest)"), *R.Name));
	return true;
}

void ACampaign1851Map::Demobilise()
{
	if (Footing != ECampaign1851Footing::Peace)
	{
		Footing = ECampaign1851Footing::Peace;
		News.Add(TEXT("Hæren sættes på fredsfod: mandskabet hjemsendes"));
	}
}

void ACampaign1851Map::AdvanceFooting(float DeltaDays)
{
	if (DeltaDays <= 0.f)
	{
		return;
	}
	bool bAllIn = true;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		FCampaign1851Regiment& R = Regiments[i];
		if (Footing == ECampaign1851Footing::Peace)
		{
			R.Present = FMath::Max(ArmyPeacePresent(R.Arm), R.Present - DeltaDays * Campaign1851Mobilisation::SendHomePerDay);
			continue;
		}
		// The men come to their battalion's garrison; a mobilisation depot there hurries them along.
		const ACampaign1851ConstructionSite* Depot = FindBuilding(R.Home, TEXT("Mobilization_Center"));
		const float Rate = Campaign1851Mobilisation::CallInPerDay * CallInFactor() * (Depot && Depot->IsModuleDone(0) ? Campaign1851Mobilisation::DepotBonus : 1.f);
		R.Present = FMath::Min(1.f, R.Present + DeltaDays * Rate);
		bAllIn &= R.Present >= 0.999f;
	}
	if (Footing == ECampaign1851Footing::Mobilising && bAllIn)
	{
		Footing = ECampaign1851Footing::War;
		News.Add(TEXT("Mobiliseringen er gennemført: hæren står på krigsfod"));
	}
}

double ACampaign1851Map::MobilisedPayPerMonth() const
{
	double Men = 0.0;
	for (const FCampaign1851Regiment& R : Regiments)
	{
		Men += FMath::Max(0.f, R.Present - ArmyPeacePresent(R.Arm)) * R.Men;
	}
	return Men * Campaign1851Mobilisation::PayPerManMonth;
}

void ACampaign1851Map::MonthlyFooting()
{
	const double Pay = MobilisedPayPerMonth();
	if (Pay >= 1.0)
	{
		AddTransaction(-Pay, TEXT("De indkaldtes løn og underhold"));
	}
}
