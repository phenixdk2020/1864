// Trade goods, state loans and the record of the years (Docs/Economy1851.md, backlog items 10, 12, 16, 17).
// Denmark lives by exporting grain, cattle and butter: their prices follow the harvests and the times (the
// Crimean War's dear grain 1853-56, the crash of 1857, a war that closes Hamburg), and the export duty and the
// farmers' contentment follow the prices. The state may borrow in London and Hamburg at a rate set by its
// credit. Each month the state of the country is written down for the newspaper and the statistics.

#include "Campaign1851Map.h"

namespace
{
	/** Exported per rural inhabitant and year at the price of 1851 (rd., estimate). */
	const float GoodPerHead[int32(ECampaign1851Good::Count)] = { 0.6f, 0.3f, 0.15f };
	constexpr double ExportDuty = 0.02;

	float EcoHash01(uint32 A, uint32 B)
	{
		return FRandomStream(int32(HashCombine(A * 40503u, B))).FRand();
	}
}

const TCHAR* Campaign1851Economy::GoodName(ECampaign1851Good G)
{
	return G == ECampaign1851Good::Grain ? TEXT("Korn") : G == ECampaign1851Good::Cattle ? TEXT("Kvæg") : TEXT("Smør");
}

float ACampaign1851Map::PriceIndex(ECampaign1851Good G) const
{
	const FDateTime Now = GetDate();
	const int32 Month = (Now.GetYear() - 1851) * 12 + Now.GetMonth();
	// A slow walk of the market, and the year's harvest for the grain.
	const float Walk = 1.f + 0.12f * FMath::Sin(Month * UE_TWO_PI / 29.f + EcoHash01(uint32(Seed), uint32(G)) * 6.f) + 0.06f * (EcoHash01(uint32(Seed) + uint32(G), uint32(Month)) - 0.5f);
	const int32 HarvestYear = Now.GetMonth() >= 8 ? Now.GetYear() : Now.GetYear() - 1;
	const float Harvest = G == ECampaign1851Good::Grain ? 0.85f + 0.3f * EcoHash01(uint32(Seed) + 99u, uint32(HarvestYear)) : 1.f;
	float Times = 1.f;
	// The Crimean War: Russian grain gone from the market (October 1853 - March 1856).
	if (Now >= FDateTime(1853, 10, 1) && Now < FDateTime(1856, 4, 1))
	{
		Times *= G == ECampaign1851Good::Grain ? 1.5f : 1.15f;
	}
	// The crash of 1857 (Hamburg, November 1857 - 1858).
	if (Now >= FDateTime(1857, 11, 1) && Now < FDateTime(1859, 1, 1))
	{
		Times *= 0.7f;
	}
	// Harvest scarcity lifts the price as the harvest lowers the quantity.
	return Walk * Times / FMath::Sqrt(Harvest);
}

double ACampaign1851Map::ExportValuePerYear(ECampaign1851Good G) const
{
	double Rural = 0.0;
	for (const FCampaign1851Amt& A : Amter)
	{
		Rural += IsAmtOccupied(A) ? 0.0 : A.Rural;
	}
	const int32 HarvestYear = GetDate().GetMonth() >= 8 ? GetDate().GetYear() : GetDate().GetYear() - 1;
	const float Harvest = G == ECampaign1851Good::Grain ? 0.85f + 0.3f * EcoHash01(uint32(Seed) + 99u, uint32(HarvestYear)) : 1.f;
	// War: Hamburg and the Elbe closed to cattle; without the sea every export suffers.
	float War = 1.f;
	if (bAtWar)
	{
		War = G == ECampaign1851Good::Cattle ? 0.5f : 0.8f;
		War *= HasSeaControl() ? 1.f : 0.5f;
	}
	return Rural * GoodPerHead[int32(G)] * Harvest * PriceIndex(G) * War;
}

double ACampaign1851Map::ExportDutyPerYear() const
{
	double Total = 0.0;
	for (int32 g = 0; g < int32(ECampaign1851Good::Count); ++g)
	{
		Total += ExportValuePerYear(ECampaign1851Good(g));
	}
	return Total * ExportDuty;
}

float ACampaign1851Map::CreditRate() const
{
	// 4 % for a sound state; more with debt against revenue, war and a discontented country.
	const double Revenue = FMath::Max(1.0, YearlyTax() + ExportDutyPerYear() + ForeignIncomePerYear());
	return FMath::Min(0.09f, 0.04f + 0.02f * float(Debt / Revenue) + (bAtWar ? 0.015f : 0.f) + (Mood < 30.f ? 0.01f : 0.f));
}

double ACampaign1851Map::LoanLimit() const
{
	return FMath::Max(0.0, 3.0 * (YearlyTax() + ExportDutyPerYear() + ForeignIncomePerYear()));
}

FString ACampaign1851Map::LoanBlockReason(double Amount) const
{
	if (!FMath::IsFinite(Amount) || Amount <= 0.0)
	{
		return TEXT("lånebeløbet skal være positivt og endeligt");
	}
	if (Debt + Amount > LoanLimit())
	{
		return TEXT("kreditten slår ikke til (højst 3 års indtægter)");
	}
	return FString();
}

bool ACampaign1851Map::TakeLoan(double Amount, FString* OutReason)
{
	if (OutReason) { OutReason->Reset(); }
	const FString Why = LoanBlockReason(Amount);
	if (!Why.IsEmpty())
	{
		if (OutReason) { *OutReason = Why; }
		return false;
	}
	// The new loan at today's rate; the debt's rate is the average.
	const float Rate = CreditRate();
	DebtRate = Debt + Amount > 0.0 ? float((Debt * DebtRate + Amount * Rate) / (Debt + Amount)) : Rate;
	Debt += Amount;
	AddTransaction(Amount, FString::Printf(TEXT("Statslån i %s: %.1f %%"), Amount >= 250000.0 ? TEXT("London") : TEXT("Hamborg"), Rate * 100.f));
	News.Add(FString::Printf(TEXT("Statslån på %s rd. til %.1f %%"), *FString::FromInt(int32(Amount)), Rate * 100.f));
	return true;
}

bool ACampaign1851Map::RepayLoan(double Amount)
{
	if (!FMath::IsFinite(Amount) || Amount <= 0.0)
	{
		return false;
	}
	Amount = FMath::Min(Amount, Debt);
	if (Amount <= 0.0 || Treasury < Amount)
	{
		return false;
	}
	Debt -= Amount;
	if (Debt == 0.0) { DebtRate = 0.04f; }
	AddTransaction(-Amount, TEXT("Afdrag på statsgælden"));
	return true;
}

void ACampaign1851Map::MonthlyEconomy()
{
	AddTransaction(ExportDutyPerYear() / 12.0, TEXT("Told på udførsel (korn, kvæg, smør)"));
	if (Debt > 0.5)
	{
		AddTransaction(-Debt * DebtRate / 12.0, TEXT("Renter af statsgælden"));
	}
	// Good prices content the farmers; bad ones sour them.
	const float Grain = PriceIndex(ECampaign1851Good::Grain);
	Mood = FMath::Clamp(Mood + (Grain - 1.f) * 1.5f, 0.f, 100.f);
	// The year's record for the statistics.
	FCampaign1851Record R;
	R.Day = CampaignDays;
	const FCampaign1851NationFigures F = NationFigures(PlayerNation);
	R.Population = F.Population;
	R.Treasury = Treasury;
	R.ArmyMen = F.ArmyMen;
	R.RailKm = F.RailKm;
	R.Tension = Tension;
	R.Mood = Mood;
	R.Grain = Grain;
	R.Debt = Debt;
	History.Add(R);
}

TArray<FString> ACampaign1851Map::TakeNews()
{
	TArray<FString> Out = MoveTemp(News);
	News.Reset();
	for (const FString& N : Out)
	{
		NewsLog.Add(TPair<double, FString>(CampaignDays, N));
	}
	if (NewsLog.Num() > 400)
	{
		NewsLog.RemoveAt(0, NewsLog.Num() - 400);
	}
	return Out;
}

TArray<FString> ACampaign1851Map::SaveEconomy() const
{
	TArray<FString> Out;
	Out.Add(FString::Printf(TEXT("debt|%.17g|%.9g"), Debt, DebtRate));
	for (const FCampaign1851Record& R : History)
	{
		Out.Add(FString::Printf(TEXT("h|%.1f|%.0f|%.0f|%.0f|%.1f|%.2f|%.2f|%.3f|%.0f"), R.Day, R.Population, R.Treasury, R.ArmyMen, R.RailKm, R.Tension, R.Mood, R.Grain, R.Debt));
	}
	Out.Append(SaveFortProgrammes());
	Out.Append(SaveResources());
	Out.Add(FString::Printf(TEXT("end|%d|%.3f"), bEndShown ? 1 : 0, LastWarScore));
	const int32 From = FMath::Max(0, NewsLog.Num() - 120);
	for (int32 n = From; n < NewsLog.Num(); ++n)
	{
		Out.Add(FString::Printf(TEXT("n|%.2f|%s"), NewsLog[n].Key, *NewsLog[n].Value.Replace(TEXT("|"), TEXT("/"))));
	}
	return Out;
}

void ACampaign1851Map::RestoreEconomy(const TArray<FString>& Lines)
{
	Debt = 0.0;
	DebtRate = 0.04f;
	TArray<FString> ResourceLines;
	History.Reset();
	NewsLog.Reset();
	for (const FString& Line : Lines)
	{
		TArray<FString> P;
		Line.ParseIntoArray(P, TEXT("|"), false);
		if (P.Num() == 3 && P[0] == TEXT("debt"))
		{
			const double SavedDebt = FCString::Atod(*P[1]);
			const float SavedRate = FCString::Atof(*P[2]);
			Debt = FMath::IsFinite(SavedDebt) ? FMath::Max(0.0, SavedDebt) : 0.0;
			DebtRate = Debt > 0.0 && FMath::IsFinite(SavedRate) ? FMath::Clamp(SavedRate, 0.04f, 0.09f) : 0.04f;
		}
		else if (P.Num() == 10 && P[0] == TEXT("h"))
		{
			FCampaign1851Record R;
			R.Day = FCString::Atod(*P[1]);
			R.Population = FCString::Atod(*P[2]);
			R.Treasury = FCString::Atod(*P[3]);
			R.ArmyMen = FCString::Atod(*P[4]);
			R.RailKm = FCString::Atod(*P[5]);
			R.Tension = FCString::Atof(*P[6]);
			R.Mood = FCString::Atof(*P[7]);
			R.Grain = FCString::Atof(*P[8]);
			R.Debt = FCString::Atod(*P[9]);
			History.Add(R);
		}
		else if (P.Num() > 0 && (P[0] == TEXT("raw") || P[0] == TEXT("kit") || P[0] == TEXT("mortars")))
		{
			ResourceLines.Add(Line);
		}
		else if (P.Num() >= 2 && P[0] == TEXT("end"))
		{
			bEndShown = P[1] == TEXT("1");
			LastWarScore = P.Num() > 2 ? FCString::Atof(*P[2]) : 0.f;
		}
		else if (P.Num() == 4 && P[0] == TEXT("prog"))
		{
			RestoreFortProgramme(FCString::Atoi(*P[1]), FCString::Atoi(*P[2]), FCString::Atod(*P[3]));
		}
		else if (P.Num() == 3 && P[0] == TEXT("n"))
		{
			NewsLog.Add(TPair<double, FString>(FCString::Atod(*P[1]), P[2]));
		}
	}
	RestoreResources(ResourceLines);
}
