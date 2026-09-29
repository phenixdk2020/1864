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
			R.Present = FMath::Max(Campaign1851Mobilisation::PeacePresent, R.Present - DeltaDays * Campaign1851Mobilisation::SendHomePerDay);
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
		Men += FMath::Max(0.f, R.Present - Campaign1851Mobilisation::PeacePresent) * R.Men;
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
