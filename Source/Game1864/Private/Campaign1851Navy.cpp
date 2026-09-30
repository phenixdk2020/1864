// The navy (Docs/Navy1851.md, backlog item 3): the Danish fleet of 1851, new ships from the naval dockyard,
// upkeep, and the war at sea. While the Danish ships outnumber the enemy's, the enemy corps cannot cross the
// Belts and sounds (only the narrowest, like Alssund, after days of gathering boats); a blockade of the German
// ports costs the enemy its drafts and brings the great powers to the table sooner.

#include "Campaign1851Map.h"

namespace Campaign1851Navy
{
	const TArray<FCampaign1851ShipClass>& Classes()
	{
		// Strength in the game's balance (a ship of the line 10); prices and build times are estimates.
		static const TArray<FCampaign1851ShipClass> List = {
			{ TEXT("Linjeskib"),          10.f, 0,    0.0,      0,  3000.0 },
			{ TEXT("Fregat"),             6.f,  0,    0.0,      0,  1800.0 },
			{ TEXT("Korvet"),             3.f,  0,    0.0,      0,  900.0 },
			{ TEXT("Hjuldamper"),         2.f,  1851, 60000.0,  8,  800.0 },
			{ TEXT("Kanonbådsflotille"),  2.f,  1851, 25000.0,  4,  500.0 },
			{ TEXT("Skruefregat"),        9.f,  1855, 300000.0, 24, 3500.0 },
			{ TEXT("Panserskib"),         12.f, 1862, 450000.0, 18, 4500.0 },
		};
		return List;
	}
}

void ACampaign1851Map::ResetNavy()
{
	// The fleet after 1848-50 (names historical, the balance the game's estimate).
	Ships.Reset();
	auto Add = [this](const TCHAR* Name, int32 Class, int32 Year)
	{
		FCampaign1851Ship S;
		S.Name = Name;
		S.Class = Class;
		S.Built = Year;
		S.ReadyDay = 0.0;
		Ships.Add(S);
	};
	Add(TEXT("Skjold"), 0, 1833);
	Add(TEXT("Frederik VI"), 0, 1831);
	Add(TEXT("Bellona"), 1, 1830);
	Add(TEXT("Havfruen"), 1, 1825);
	Add(TEXT("Thetis"), 1, 1840);
	Add(TEXT("Galathea"), 2, 1831);
	Add(TEXT("Valkyrien"), 2, 1846);
	Add(TEXT("Najaden"), 2, 1820);
	Add(TEXT("Hekla"), 3, 1842);
	Add(TEXT("Geiser"), 3, 1844);
	Add(TEXT("Skirner"), 3, 1847);
	Add(TEXT("1. kanonbådsdivision"), 4, 1848);
	Add(TEXT("2. kanonbådsdivision"), 4, 1848);
	bBlockade = false;
	AustrianSquadronDay = -1.0;
}

float ACampaign1851Map::DanishSeaStrength() const
{
	// Ready ships; the old sailing ships lose a little each decade (rot, obsolete in the age of steam).
	float S = 0.f;
	const int32 Year = GetDate().GetYear();
	for (const FCampaign1851Ship& Ship : Ships)
	{
		if (CampaignDays < Ship.ReadyDay || !Campaign1851Navy::Classes().IsValidIndex(Ship.Class))
		{
			continue;
		}
		const bool bSail = Ship.Class <= 2;
		S += Campaign1851Navy::Classes()[Ship.Class].Strength * (bSail ? FMath::Clamp(1.f - (Year - Ship.Built - 15) * 0.02f, 0.4f, 1.f) : 1.f);
	}
	return S * (bBlockade ? 0.8f : 1.f);
}

float ACampaign1851Map::EnemySeaStrength() const
{
	// Prussia builds a navy from almost nothing; an Austrian squadron comes into the North Sea in war.
	const float Years = float(CampaignDays / 365.0);
	float S = 4.f + 1.2f * Years;
	if (bAtWar && AustrianSquadronDay >= 0.0 && CampaignDays >= AustrianSquadronDay)
	{
		S += 22.f;
	}
	return bAtWar ? S : 0.f;
}

bool ACampaign1851Map::HasSeaControl() const
{
	return DanishSeaStrength() > EnemySeaStrength();
}

double ACampaign1851Map::NavyUpkeepPerYear() const
{
	double Total = 0.0;
	for (const FCampaign1851Ship& Ship : Ships)
	{
		Total += Campaign1851Navy::Classes().IsValidIndex(Ship.Class) ? Campaign1851Navy::Classes()[Ship.Class].UpkeepPerYear : 0.0;
	}
	return Total;
}

FString ACampaign1851Map::ShipBlockReason(int32 Class) const
{
	const TArray<FCampaign1851ShipClass>& List = Campaign1851Navy::Classes();
	if (!List.IsValidIndex(Class) || List[Class].Cost <= 0.0)
	{
		return TEXT("-");
	}
	if (GetDate().GetYear() < List[Class].Year) return FString::Printf(TEXT("fra %d"), List[Class].Year);
	if (Treasury < List[Class].Cost) return TEXT("ikke råd");
	return FString();
}

bool ACampaign1851Map::OrderShip(int32 Class, FString* OutReason)
{
	const FString Why = ShipBlockReason(Class);
	if (!Why.IsEmpty())
	{
		if (OutReason) { *OutReason = Why; }
		return false;
	}
	const FCampaign1851ShipClass& K = Campaign1851Navy::Classes()[Class];
	static const TCHAR* Names[] = { TEXT("Niels Juel"), TEXT("Jylland"), TEXT("Rolf Krake"), TEXT("Esbern Snare"), TEXT("Peder Skram"), TEXT("Dagmar"), TEXT("Heimdal"),
		TEXT("Absalon"), TEXT("Tordenskjold"), TEXT("Sjælland"), TEXT("Danmark"), TEXT("Hvidbjørnen"), TEXT("Diana"), TEXT("Fylla"), TEXT("Willemoes") };
	FCampaign1851Ship S;
	S.Class = Class;
	S.Built = GetDate().GetYear() + K.Months / 12;
	S.ReadyDay = CampaignDays + K.Months * 30.4;
	S.Name = Class == 4 ? FString::Printf(TEXT("%d. kanonbådsdivision"), Ships.FilterByPredicate([](const FCampaign1851Ship& X) { return X.Class == 4; }).Num() + 1)
		: FString(Names[Ships.Num() % UE_ARRAY_COUNT(Names)]);
	Ships.Add(S);
	AddTransaction(-K.Cost, FString::Printf(TEXT("Orlogsværftet: %s %s"), K.Name, *S.Name));
	News.Add(FString::Printf(TEXT("Orlogsværftet lægger %s %s på stabelen (klar om %d måneder)"), K.Name, *S.Name, K.Months));
	return true;
}

void ACampaign1851Map::SetBlockade(bool bOn)
{
	bBlockade = bOn;
	News.Add(bOn ? TEXT("Flåden blokerer de tyske havne") : TEXT("Flåden samles til forsvar af farvandene"));
}

void ACampaign1851Map::MonthlyNavy()
{
	AddTransaction(-NavyUpkeepPerYear() / 12.0, TEXT("Flåden: vedligehold og besætninger"));
	if (!bAtWar)
	{
		return;
	}
	if (AustrianSquadronDay < 0.0)
	{
		AustrianSquadronDay = WarStartDay + 90.0;
	}
	// A blockade with the sea held: prizes for the treasury, the enemy's trade and drafts suffer.
	if (bBlockade && HasSeaControl())
	{
		AddTransaction(2500.0, TEXT("Prisepenge fra blokaden"));
		for (FCampaign1851Nation& N : Nations)
		{
			if (N.Id == TEXT("GB"))
			{
				N.Relation = FMath::Max(-100.f, N.Relation - 2.f);   // neutral shipping stopped
			}
		}
	}
}

int32 ACampaign1851Map::WaterCrossing(const FCampaign1851EnemyCorps& C, const TArray<FCampaign1851Leg>& Legs, FString& OutFerry) const
{
	if (!HasSeaControl())
	{
		return 0;
	}
	int32 Result = 0;
	for (const FCampaign1851Leg& L : Legs)
	{
		if (Links.IsValidIndex(L.Link) && Links[L.Link].HasFerry())
		{
			OutFerry = Links[L.Link].Ferry;
			// A narrow sound can be forced in boats gathered for a week, or walked over in a hard frost; the Belts cannot.
			if (Links[L.Link].FerryKm > 1.5f)
			{
				return 2;
			}
			if (IsIceWinter())
			{
				continue;
			}
			if (C.CrossingReadyDay < 0.0 || CampaignDays < C.CrossingReadyDay)
			{
				Result = 1;
			}
		}
	}
	return Result;
}

TArray<FString> ACampaign1851Map::SaveNavy() const
{
	TArray<FString> Out;
	Out.Add(FString::Printf(TEXT("state|%d|%.2f"), bBlockade ? 1 : 0, AustrianSquadronDay));
	for (const FCampaign1851Ship& S : Ships)
	{
		Out.Add(FString::Printf(TEXT("ship|%s|%d|%d|%.2f"), *S.Name, S.Class, S.Built, S.ReadyDay));
	}
	return Out;
}

void ACampaign1851Map::RestoreNavy(const TArray<FString>& Lines)
{
	if (Lines.Num() == 0)
	{
		ResetNavy();
		return;
	}
	Ships.Reset();
	for (const FString& Line : Lines)
	{
		TArray<FString> P;
		Line.ParseIntoArray(P, TEXT("|"), false);
		if (P.Num() == 3 && P[0] == TEXT("state"))
		{
			bBlockade = P[1] == TEXT("1");
			AustrianSquadronDay = FCString::Atod(*P[2]);
		}
		else if (P.Num() == 5 && P[0] == TEXT("ship"))
		{
			FCampaign1851Ship S;
			S.Name = P[1];
			S.Class = FCString::Atoi(*P[2]);
			S.Built = FCString::Atoi(*P[3]);
			S.ReadyDay = FCString::Atod(*P[4]);
			Ships.Add(S);
		}
	}
}
