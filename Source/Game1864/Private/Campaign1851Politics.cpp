// Government, public opinion and the great powers (Docs/Politics1851.md, backlog items 7, 8 and 11).
// Three currents divide opinion: the whole-state conservatives (Helstat), the National Liberals of the
// Eider policy (Ejder) and the Scandinavianists. The public mood rises with victories and good years and
// falls with mobilisation, losses and occupied towns; it moves the taxes and the call-in. The government
// follows the leading current: the historical cabinets take office on their dates when opinion allows, else
// a cabinet of the leading current; a government falls when its current or the mood sinks too low.
// The guarantor powers watch the Danish policy: an Eider course and a rising tension cost their goodwill.

#include "Campaign1851Map.h"

namespace
{
	struct FCabinet
	{
		int32 Year, Month, Day;
		const TCHAR* Name;
		ECampaign1851Current Line;
	};
	// The cabinets of the period (dates historical; their line simplified for the game).
	const FCabinet Cabinets[] = {
		{ 1852, 1, 27,  TEXT("C.A. Bluhme"),  ECampaign1851Current::Helstat },
		{ 1853, 4, 21,  TEXT("A.S. Ørsted"),  ECampaign1851Current::Helstat },
		{ 1854, 12, 12, TEXT("P.G. Bang"),    ECampaign1851Current::Helstat },
		{ 1856, 10, 18, TEXT("C.G. Andræ"),   ECampaign1851Current::Helstat },
		{ 1857, 5, 13,  TEXT("C.C. Hall"),    ECampaign1851Current::Ejder },
		{ 1859, 12, 2,  TEXT("C.E. Rotwitt"), ECampaign1851Current::Helstat },
		{ 1860, 2, 24,  TEXT("C.C. Hall"),    ECampaign1851Current::Ejder },
		{ 1863, 12, 31, TEXT("D.G. Monrad"),  ECampaign1851Current::Ejder },
	};
	const TCHAR* Alternates[3][3] = {
		{ TEXT("L.N. Scheele"), TEXT("F. Moltke"), TEXT("C. Moltke") },
		{ TEXT("Orla Lehmann"), TEXT("A.F. Krieger"), TEXT("C.F. Tietgen") },
		{ TEXT("Carl Ploug"), TEXT("J.F. Schouw"), TEXT("C. Hostrup") } };
}

const TCHAR* Campaign1851Politics::CurrentName(ECampaign1851Current C)
{
	return C == ECampaign1851Current::Helstat ? TEXT("Helstaten") : C == ECampaign1851Current::Ejder ? TEXT("Ejderpolitikken") : TEXT("Skandinavismen");
}

const TCHAR* Campaign1851Politics::CurrentEffect(ECampaign1851Current C)
{
	return C == ECampaign1851Current::Helstat ? TEXT("forsigtig udenrigspolitik: spændingen stiger 15 % mindre, stormagterne er venligere")
		: C == ECampaign1851Current::Ejder ? TEXT("Slesvig knyttes til kongeriget: begivenhederne skærper spændingen 20 %, men Krigsministeriet får flere penge")
		: TEXT("nordisk samling: forholdet til Sverige-Norge stiger, alliancen koster det halve");
}

void ACampaign1851Map::ResetPolitics()
{
	// Summer 1851: the whole state restored after the war, the National Liberals strong in Copenhagen.
	FRandomStream Rng(int32(HashCombine(uint32(Seed), 0x9011u)));
	Support[0] = 45.f + 10.f * Deviation * Rng.FRandRange(-1.f, 1.f);
	Support[1] = 40.f + 10.f * Deviation * Rng.FRandRange(-1.f, 1.f);
	Support[2] = 100.f - Support[0] - Support[1];
	Mood = 60.f;
	PrimeMinister = TEXT("A.W. Moltke");
	Government = ECampaign1851Current::Helstat;
	GovernmentSince = 0.0;
	NextCabinet = 0;
}

float ACampaign1851Map::TaxMoodFactor() const
{
	return 0.9f + 0.2f * Mood / 100.f;
}

float ACampaign1851Map::CallInMoodFactor() const
{
	return 0.8f + 0.4f * Mood / 100.f;
}

float ACampaign1851Map::GovernmentTensionFactor() const
{
	return Government == ECampaign1851Current::Helstat ? 0.85f : Government == ECampaign1851Current::Ejder ? 1.2f : 1.f;
}

ECampaign1851Current ACampaign1851Map::LeadingCurrent() const
{
	return Support[1] > Support[0] && Support[1] >= Support[2] ? ECampaign1851Current::Ejder
		: Support[2] > Support[0] && Support[2] > Support[1] ? ECampaign1851Current::Scandinavian : ECampaign1851Current::Helstat;
}

void ACampaign1851Map::FormGovernment(const FString& Name, ECampaign1851Current Line, const FString& Why)
{
	if (Name == PrimeMinister && Line == Government)
	{
		return;
	}
	PrimeMinister = Name;
	Government = Line;
	GovernmentSince = CampaignDays;
	News.Add(FString::Printf(TEXT("Ny regering: %s (%s)"), *Name, Campaign1851Politics::CurrentName(Line)));
	FCampaign1851Decision D;
	D.Day = CampaignDays;
	D.Nation = PlayerNation;
	D.Portfolio = ECampaign1851Portfolio::Interior;
	D.Action = FString::Printf(TEXT("Regeringsskifte: konseilspræsident %s"), *Name);
	D.Reasons = Why;
	D.bDone = true;
	AddDecision(D);
	// The line steers the ministries' priorities (the National Liberals spend on the army).
	if (Nations.IsValidIndex(PlayerNation))
	{
		FCampaign1851Nation& N = Nations[PlayerNation];
		const float WarMul = Line == ECampaign1851Current::Ejder ? 1.15f : Line == ECampaign1851Current::Helstat ? 0.9f : 1.f;
		N.Weights[int32(ECampaign1851Portfolio::War)] = N.BaseWeights[int32(ECampaign1851Portfolio::War)] * WarMul;
	}
}

void ACampaign1851Map::PoliticalShock(float MoodChange, float EjderChange)
{
	Mood = FMath::Clamp(Mood + MoodChange, 0.f, 100.f);
	Support[1] = FMath::Clamp(Support[1] + EjderChange, 5.f, 85.f);
	const float Rest = 100.f - Support[1];
	const float Old = FMath::Max(1.f, Support[0] + Support[2]);
	Support[0] = Rest * Support[0] / Old;
	Support[2] = Rest - Support[0];
}

void ACampaign1851Map::MonthlyPolitics()
{
	// The mood: back towards content, down with mobilisation, war and occupied towns.
	int32 Occupied = 0;
	for (const FCampaign1851City& C : Cities)
	{
		Occupied += !C.Occupier.IsEmpty() && !C.bForeign ? 1 : 0;
	}
	Mood += (60.f - Mood) * 0.05f - (Footing != ECampaign1851Footing::Peace ? 1.5f : 0.f) - (bAtWar ? 0.5f : 0.f) - Occupied * 1.f;
	Mood = FMath::Clamp(Mood, 0.f, 100.f);
	// The currents: tension feeds the Eider policy, a friendly Sweden the Scandinavianists.
	const FCampaign1851Nation* SE = Nations.FindByPredicate([](const FCampaign1851Nation& N) { return N.Id == TEXT("SE"); });
	Support[1] += (Tension - 40.f) / 40.f;
	Support[2] += SE ? (SE->Relation - 40.f) / 60.f + (SE->bAlliance ? 1.f : 0.f) : 0.f;
	for (float& S : Support)
	{
		S = FMath::Clamp(S, 5.f, 85.f);
	}
	const float Sum = Support[0] + Support[1] + Support[2];
	for (float& S : Support)
	{
		S *= 100.f / Sum;
	}
	// The government's line works on the world.
	for (FCampaign1851Nation& N : Nations)
	{
		if (Government == ECampaign1851Current::Helstat && (N.Id == TEXT("GB") || N.Id == TEXT("RU")))
		{
			N.Relation = FMath::Min(100.f, N.Relation + 0.5f);
		}
		if (Government != ECampaign1851Current::Helstat && N.Id == TEXT("SE"))
		{
			N.Relation = FMath::Min(100.f, N.Relation + (Government == ECampaign1851Current::Scandinavian ? 1.f : 0.5f));
		}
	}
	// The great powers watch: an Eider course with a high tension costs their goodwill; a guarantee is
	// withdrawn when the goodwill is gone (backlog item 8).
	for (FCampaign1851Nation& N : Nations)
	{
		if (!N.bCanGuarantee)
		{
			continue;
		}
		if (Government == ECampaign1851Current::Ejder && Tension > 60.f)
		{
			N.Relation = FMath::Max(-100.f, N.Relation - (N.Id == TEXT("RU") ? 3.f : 2.f));
		}
		if (N.bGuarantee && N.Relation < 20.f)
		{
			N.bGuarantee = false;
			News.Add(FString::Printf(TEXT("%s trækker sin garanti tilbage: den danske politik mishager"), *N.Name));
			FCampaign1851Decision D;
			D.Day = CampaignDays;
			D.Nation = Nations.IndexOfByPredicate([&N](const FCampaign1851Nation& X) { return X.Id == N.Id; });
			D.Portfolio = ECampaign1851Portfolio::Interior;
			D.Action = FString::Printf(TEXT("%s trækker garantien for helstaten tilbage"), *N.Name);
			D.Reasons = FString::Printf(TEXT("forholdet faldt til %.0f (regering: %s, spænding %.0f)"), N.Relation, Campaign1851Politics::CurrentName(Government), Tension);
			D.bDone = true;
			AddDecision(D);
		}
		// In war a friendly power offers its mediation early.
		if (bAtWar && PeaceTalksDay < 0.0 && N.Relation >= 60.f && CampaignDays - WarStartDay > 30.0)
		{
			PeaceTalksDay = CampaignDays;
			News.Add(FString::Printf(TEXT("%s tilbyder mægling: en fredskonference samles"), *N.Name));
		}
	}
	// Cabinets: the historical one on its date if its current is strong enough, else the leading current's.
	const FDateTime Now = GetDate();
	while (NextCabinet < int32(UE_ARRAY_COUNT(Cabinets)) && Now >= FDateTime(Cabinets[NextCabinet].Year, Cabinets[NextCabinet].Month, Cabinets[NextCabinet].Day))
	{
		const FCabinet& K = Cabinets[NextCabinet++];
		if (Support[int32(K.Line)] >= 30.f)
		{
			FormGovernment(K.Name, K.Line, FString::Printf(TEXT("%s har %.0f %% af opinionen bag sig"), Campaign1851Politics::CurrentName(K.Line), Support[int32(K.Line)]));
		}
	}
	const float Mine = Support[int32(Government)];
	if ((Mine < 28.f || Mood < 25.f) && CampaignDays - GovernmentSince > 180.0)
	{
		const ECampaign1851Current Lead = LeadingCurrent();
		FRandomStream Rng(int32(HashCombine(uint32(Seed), uint32(Now.GetYear() * 12 + Now.GetMonth()))));
		FormGovernment(Alternates[int32(Lead)][Rng.RandHelper(3)], Lead, Mood < 25.f
			? FString::Printf(TEXT("regeringen faldt: stemningen i landet er nede på %.0f"), Mood)
			: FString::Printf(TEXT("regeringen faldt: %s har kun %.0f %% bag sig"), Campaign1851Politics::CurrentName(Government), Mine));
	}
}

TArray<FString> ACampaign1851Map::SavePolitics() const
{
	TArray<FString> Out;
	Out.Add(FString::Printf(TEXT("state|%.2f|%.2f|%.2f|%.2f|%s|%d|%.2f|%d|%d|%d"), Support[0], Support[1], Support[2], Mood, *PrimeMinister, int32(Government), GovernmentSince, NextCabinet,
		DanishWarLosses, EnemyWarLosses));
	return Out;
}

void ACampaign1851Map::RestorePolitics(const TArray<FString>& Lines)
{
	ResetPolitics();
	for (const FString& Line : Lines)
	{
		TArray<FString> P;
		Line.ParseIntoArray(P, TEXT("|"), false);
		if (P.Num() == 11 && P[0] == TEXT("state"))
		{
			for (int32 s = 0; s < 3; ++s)
			{
				Support[s] = FCString::Atof(*P[1 + s]);
			}
			Mood = FCString::Atof(*P[4]);
			PrimeMinister = P[5];
			Government = ECampaign1851Current(FMath::Clamp(FCString::Atoi(*P[6]), 0, 2));
			GovernmentSince = FCString::Atod(*P[7]);
			NextCabinet = FCString::Atoi(*P[8]);
			DanishWarLosses = FCString::Atoi(*P[9]);
			EnemyWarLosses = FCString::Atoi(*P[10]);
		}
	}
}
