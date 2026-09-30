// The choice of nation and the end of the campaign (Docs/Endgame1851.md, backlog items 19 and 20).
// A new game may be played as Denmark (the map) or, as a first step towards more nations, as Sweden-Norway on
// the abstract model while the AI governs Denmark. The campaign ends on 1 January 1867 or with the peace after
// a war; the result is scored on the monarchy kept, growth, the finances, the war and the mood.

#include "Campaign1851Map.h"

void ACampaign1851Map::ApplyNewGameNation()
{
	PlayedNation = PlayerNation;
	for (int32 n = 0; n < Nations.Num(); ++n)
	{
		FCampaign1851Nation& N = Nations[n];
		const bool bPlayer = N.Id == NewGameNation;
		N.Controller = bPlayer ? ECampaign1851Controller::Player : ECampaign1851Controller::AI;
		for (ECampaign1851Delegation& M : N.Modes)
		{
			M = bPlayer && N.bOnMap ? ECampaign1851Delegation::Manual : ECampaign1851Delegation::Auto;
		}
		PlayedNation = bPlayer ? n : PlayedNation;
	}
	if (PlayedNation != PlayerNation)
	{
		News.Add(FString::Printf(TEXT("Du spiller %s (abstrakt model); Danmark styres af AI"), *Nations[PlayedNation].Name));
	}
}

int32 ACampaign1851Map::GetPlayedNation() const
{
	const int32 n = Nations.IndexOfByPredicate([](const FCampaign1851Nation& N) { return N.IsPlayer(); });
	return n == INDEX_NONE ? PlayerNation : n;
}

void ACampaign1851Map::AdjustNationWeight(int32 Portfolio, float Delta)
{
	const int32 n = GetPlayedNation();
	if (Nations.IsValidIndex(n) && Portfolio >= 0 && Portfolio < int32(ECampaign1851Portfolio::Count))
	{
		Nations[n].Weights[Portfolio] = FMath::Clamp(Nations[n].Weights[Portfolio] + Delta, 0.1f, 3.f);
	}
}

TArray<FCampaign1851ScoreLine> ACampaign1851Map::FinalScore() const
{
	TArray<FCampaign1851ScoreLine> Lines;
	// The monarchy kept: towns of 1851 still Danish.
	int32 Own = 0, Kept = 0;
	for (const FCampaign1851City& C : Cities)
	{
		if (C.bForeign && !C.bCeded)
		{
			continue;
		}
		++Own;
		Kept += C.bCeded ? 0 : 1;
	}
	const float Territory = Own > 0 ? 40.f * Kept / Own : 0.f;
	Lines.Add({ FString::Printf(TEXT("Monarkiet bevaret: %d af %d byer"), Kept, Own), Territory });
	// Growth of the people since 1851.
	double Now = 0.0, Then = 0.0;
	for (int32 c = 0; c < Cities.Num() && c < CityBasePopulation.Num(); ++c)
	{
		if (Cities[c].bForeign && !Cities[c].bCeded)
		{
			continue;   // never the monarchy's
		}
		Now += Cities[c].bCeded ? 0.0 : Cities[c].Population;
		Then += CityBasePopulation[c];
	}
	for (int32 a = 0; a < Amter.Num() && a < AmtBase.Num(); ++a)
	{
		Now += IsAmtOccupied(Amter[a]) ? 0.0 : Amter[a].Rural;
		Then += AmtBase[a].Y;
	}
	const float Growth = FMath::Clamp(float((Now / FMath::Max(Then, 1.0) - 1.0) * 100.0), -10.f, 20.f);
	Lines.Add({ FString::Printf(TEXT("Befolkningens vækst: %+.1f %%"), float((Now / FMath::Max(Then, 1.0) - 1.0) * 100.0)), Growth });
	// The finances: cash less debt.
	const float Money = FMath::Clamp(float((Treasury - Debt) / 100000.0), -15.f, 15.f);
	Lines.Add({ FString::Printf(TEXT("Finanserne: kasse %s, gæld %s rd."), *FString::FromInt(int32(Treasury)), *FString::FromInt(int32(Debt))), Money });
	// The war, or the peace kept.
	const bool bHadWar = EventsFired.Contains(TEXT("ultimatum")) || DanishWarLosses + EnemyWarLosses > 0;
	const float War = bHadWar ? WarScore() * 15.f : 10.f;
	Lines.Add({ bHadWar ? FString::Printf(TEXT("Krigen: krigsstilling %+.2f"), WarScore()) : FString(TEXT("Freden bevaret")), War });
	Lines.Add({ FString::Printf(TEXT("Stemningen i landet: %.0f"), Mood), Mood / 10.f });
	return Lines;
}

FString ACampaign1851Map::FinalGrade(float Total)
{
	return Total >= 80.f ? TEXT("Storslået: helstaten står stærkere end nogensinde")
		: Total >= 60.f ? TEXT("Hæderligt: monarkiet har klaret sig gennem stormene")
		: Total >= 40.f ? TEXT("Tåleligt: tabene kunne være større")
		: TEXT("Katastrofe: som i 1864");
}

void ACampaign1851Map::CheckCampaignEnd()
{
	if (!bEndShown && GetDate() >= FDateTime(1867, 1, 1))
	{
		bEndPending = true;
	}
}

bool ACampaign1851Map::TakeEndPending()
{
	const bool b = bEndPending && !bEndShown;
	if (b)
	{
		bEndShown = true;
		bEndPending = false;
		float Total = 0.f;
		for (const FCampaign1851ScoreLine& L : FinalScore())
		{
			Total += L.Points;
		}
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|end|%.0f|%s"), Total, *FinalGrade(Total));
	}
	return b;
}
