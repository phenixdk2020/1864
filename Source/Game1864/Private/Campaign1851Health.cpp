// Sickness, the wounded and the prisoners (Docs/Health1851.md, backlog items 5 and 15). Men fall sick in the
// field (more in wet and cold weather and when hungry, few in garrison; the cholera of 1853 in Copenhagen),
// the wounded of a battle join them, and the lazaret sends most of them back to the ranks within weeks.
// A battle's losses are the killed, the wounded and the prisoners; prisoners are exchanged at the peace.

#include "Campaign1851Map.h"
#include "Campaign1851ConstructionSite.h"

void ACampaign1851Map::DailyHealth()
{
	const int32 Day = FMath::FloorToInt(CampaignDays);
	const FDateTime Now = GetDate();
	const ECampaign1851Weather W = GetWeather();
	const bool bWet = W == ECampaign1851Weather::Rain || W == ECampaign1851Weather::Snow || W == ECampaign1851Weather::Thaw;
	const bool bSanitation = HasResearch(TEXT("sanitation"));
	// The cholera in Copenhagen, summer 1853 (some 4,700 dead in the city).
	const bool bCholera = Now.GetYear() == 1853 && Now.GetMonth() >= 6 && Now.GetMonth() <= 9;
	const int32 Copenhagen = FindCity(TEXT("København"));
	int32 FellSick = 0, Returned = 0, Died = 0;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		FCampaign1851Regiment& R = Regiments[i];
		FRandomStream Rng(int32(HashCombine(uint32(Day) * 31u, uint32(i))));
		auto Round = [&Rng](float X) { const int32 N = FMath::FloorToInt(X); return N + (Rng.FRand() < X - N ? 1 : 0); };
		// Falling sick: the men present, in the field or in garrison.
		const bool bGarrison = !R.IsMarching() && Cities.IsValidIndex(R.Town) && !Cities[R.Town].bForeign;
		float Rate = bGarrison ? 0.0002f : 0.001f;
		Rate *= !bGarrison && bWet ? 2.f : 1.f;
		Rate *= R.Food <= 0.f ? 2.f : 1.f;
		Rate *= bCholera && bGarrison && R.Town == Copenhagen ? 10.f : 1.f;
		const int32 Sick = FMath::Min(R.Men, Round(R.PresentMen() * Rate));
		R.Men -= Sick;
		R.Sick += Sick;
		FellSick += Sick;
		// The lazaret: back to the ranks within weeks, fewer die with a lazaret near and a sanitary service.
		if (R.Sick > 0)
		{
			const ACampaign1851ConstructionSite* Lazaret = FindBuilding(R.Home, TEXT("Field_Hospital"));
			const bool bCare = bSanitation || (Lazaret && !Lazaret->IsDemolishing() && Lazaret->IsModuleDone(0));
			const int32 Back = FMath::Min(R.Sick, Round(R.Sick * (bCare ? 0.05f : 0.03f) * (HasResearch(TEXT("hospitals")) ? 1.4f : 1.f)));
			const int32 Dead = FMath::Min(R.Sick - Back, Round(R.Sick * (bCare ? 0.002f : 0.004f)));
			R.Sick -= Back + Dead;
			R.Men = FMath::Min(R.MaxMen, R.Men + Back);   // beyond the establishment: discharged
			Returned += Back;
			Died += Dead;
		}
	}
	if (bCholera && Now.GetMonth() == 6 && Now.GetDay() == 12)
	{
		News.Add(TEXT("Kolera i København: garnisonen rammes hårdt"));
	}
	if (Day % 30 == 0 && FellSick + Returned + Died > 0)
	{
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|health|sick %d today|back %d|dead %d|in lazaret %d"), FellSick, Returned, Died, SickTotal());
	}
}

int32 ACampaign1851Map::SickTotal() const
{
	int32 Total = 0;
	for (const FCampaign1851Regiment& R : Regiments)
	{
		Total += R.Sick;
	}
	return Total;
}

void ACampaign1851Map::SplitLosses(int32 RegimentIndex, int32 Lost, bool bDefeat, int32& OutPrisoners)
{
	if (!Regiments.IsValidIndex(RegimentIndex) || Lost <= 0)
	{
		return;
	}
	// Of those lost: killed a fifth to a quarter, wounded (to the lazaret) a third or more, the rest taken.
	FCampaign1851Regiment& R = Regiments[RegimentIndex];
	const float WoundedShare = bDefeat ? 0.35f : 0.5f;
	const float PrisonerShare = bDefeat ? 0.45f : 0.25f;
	const int32 Wounded = FMath::RoundToInt(Lost * WoundedShare);
	const int32 Prisoners = FMath::RoundToInt(Lost * PrisonerShare);
	R.Sick += Wounded;
	OutPrisoners += Prisoners;
	DanesCaptured += Prisoners;
}

void ACampaign1851Map::ExchangePrisoners()
{
	if (DanesCaptured <= 0 && EnemyCaptured <= 0)
	{
		return;
	}
	// The Danes come home to their amter (manpower for the replacements).
	if (AmtManpower.Num() > 0)
	{
		for (float& M : AmtManpower)
		{
			M += float(DanesCaptured) / AmtManpower.Num();
		}
	}
	News.Add(FString::Printf(TEXT("Fangeudveksling: %s danske vender hjem, %s fjendtlige frigives"), *FString::FromInt(DanesCaptured), *FString::FromInt(EnemyCaptured)));
	DanesCaptured = 0;
	EnemyCaptured = 0;
}
