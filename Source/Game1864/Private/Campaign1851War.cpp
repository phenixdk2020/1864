// War and peace of the 1851 campaign (Docs/Backlog.md item 8): the tension with the German Confederation,
// the historical events that raise or lower it (each varied by the campaign seed: earlier, later, stronger,
// weaker, or not at all), the outbreak of war, the enemy corps marching north, and the towns they occupy.

#include "Campaign1851Map.h"

namespace
{
	struct FHistoricEvent
	{
		const TCHAR* Id;
		int32 Year, Month, Day;
		float Tension;
		const TCHAR* Text;
	};
	// The road to 1864 (dates historical; the effect on the tension is the game's estimate).
	const FHistoricEvent Events[] = {
		{ TEXT("london"),      1852, 5, 8,   -10.f, TEXT("London-protokollen: stormagterne garanterer helstaten") },
		{ TEXT("helstat"),     1855, 10, 2,   5.f,  TEXT("Helstatsforfatningen vedtages; Holsten og Lauenborg protesterer") },
		{ TEXT("suspension"),  1858, 11, 6,   8.f,  TEXT("Helstatsforfatningen ophæves for Holsten og Lauenborg efter krav fra Forbundet") },
		{ TEXT("marts"),       1863, 3, 30,  12.f,  TEXT("Martskundgørelsen: Holsten får særstilling, Slesvig knyttes tættere til kongeriget") },
		{ TEXT("november"),    1863, 11, 18, 22.f,  TEXT("Novemberforfatningen: fælles forfatning for Danmark og Slesvig") },
		{ TEXT("execution"),   1863, 12, 23, 12.f,  TEXT("Forbundseksekutionen: saksiske og hannoveranske tropper rykker ind i Holsten") },
		{ TEXT("ultimatum"),   1864, 1, 16,  15.f,  TEXT("Preussen og Østrig stiller ultimatum: Novemberforfatningen skal ophæves på 48 timer") },
	};
	// The road to 1848 from a start in 1825 (dates historical; the effect on the tension is the game's estimate).
	const FHistoricEvent Events1825[] = {
		{ TEXT("julirevolution"), 1830, 7, 29,  3.f,  TEXT("Julirevolutionen i Paris: uro og forfatningskrav i Europa") },
		{ TEXT("stander"),        1834, 5, 28,  3.f,  TEXT("Stænderforsamlingerne åbnes: sprogstriden og kravet om en fælles forfatning for Slesvig-Holsten") },
		{ TEXT("christian8"),     1839, 12, 3,  2.f,  TEXT("Christian VIII bliver konge: forhåbninger og frygt for helstaten") },
		{ TEXT("abent"),          1846, 7, 8,   15.f, TEXT("Det åbne brev: Kongen erklærer arveretten i Slesvig, og Holsten og Slesvig vil høre sammen") },
		{ TEXT("kiel"),           1848, 3, 24,  28.f, TEXT("Kiel: den provisoriske regering for Slesvig-Holsten rejser sig") },
		{ TEXT("preussen"),       1848, 4, 10,  22.f, TEXT("Preussiske tropper rykker ind i Slesvig til hjælp for oprørerne") },
	};
	constexpr float WarThreshold = 80.f;
}

void ACampaign1851Map::ResetWar()
{
	Tension = ActiveScenario().StartTension;   // 1851: after the war of 1848-50 an uneasy peace; 1825: a quiet one
	bAtWar = false;
	EventsFired.Reset();
	EnemyCorps.Reset();
	Battles.Reset();
	for (FCampaign1851City& C : Cities)
	{
		C.Occupier.Reset();
	}
	// Each event's date and weight vary with the campaign; some may never come.
	EventPlan.Reset();
	FRandomStream Rng(int32(HashCombine(uint32(Seed), 0x1864u)));
	const bool bEarly = ActiveScenario().Year < 1850;
	const FHistoricEvent* List = bEarly ? Events1825 : Events;
	const int32 ListCount = bEarly ? int32(UE_ARRAY_COUNT(Events1825)) : int32(UE_ARRAY_COUNT(Events));
	for (int32 e = 0; e < ListCount; ++e)
	{
		const FHistoricEvent& E = List[e];
		FPlannedEvent P;
		P.Id = E.Id;
		P.Text = E.Text;
		P.Day = (FDateTime(E.Year, E.Month, E.Day) - StartDate()).GetTotalDays() + Deviation * 365.0 * Rng.FRandRange(-1.f, 1.f);
		P.Tension = E.Tension * (1.f + Deviation * Rng.FRandRange(-1.f, 1.f));
		P.bSkip = Rng.FRand() < Deviation * 0.5f;
		EventPlan.Add(P);
	}
}

void ACampaign1851Map::DailyWar()
{
	// Events due today.
	for (const FPlannedEvent& P : EventPlan)
	{
		if (P.bSkip || EventsFired.Contains(P.Id) || CampaignDays < P.Day)
		{
			continue;
		}
		EventsFired.Add(P.Id);
		Tension = FMath::Clamp(Tension + P.Tension * (P.Tension > 0.f ? GuaranteeDamping() * GovernmentTensionFactor() : 1.f), 0.f, 100.f);
		News.Add(FString::Printf(TEXT("%s (spænding %.0f)"), *P.Text, Tension));
		FCampaign1851Decision D;
		D.Day = CampaignDays;
		D.Nation = Nations.IndexOfByPredicate([](const FCampaign1851Nation& N) { return N.Id == TEXT("PR"); });
		D.Portfolio = ECampaign1851Portfolio::War;
		D.Action = P.Text;
		D.Reasons = FString::Printf(TEXT("spændingen med Det tyske forbund %+.0f til %.0f"), P.Tension, Tension);
		D.bDone = true;
		AddDecision(D);
		if (P.Id == TEXT("execution") && !bAtWar)
		{
			SpawnCorps(TEXT("Forbundskorpset (Sachsen, Hannover)"), TEXT("DE"), 0.09f, TEXT("Altona"), { TEXT("Rendsborg") }, 0.f);
		}
	}
	if (!bAtWar && Tension >= WarThreshold)
	{
		DeclareWar();
	}
	// Liberation: an occupied town (not ceded) is free again when 300 Danish soldiers or more have stood within
	// 3 km of it for two days with no enemy corps within 10 km; its amt then pays again.
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		FCampaign1851City& City = Cities[c];
		if (City.Occupier.IsEmpty() || City.bCeded)
		{
			City.LiberationDays = 0.f;
			continue;
		}
		const FVector2D At = TownKm(c);
		int32 Men = 0;
		for (const FCampaign1851Regiment& R : Regiments)
		{
			Men += !R.IsMarching() && FVector2D::Distance(R.Km, At) < 3.0 ? R.PresentMen() : 0;
		}
		const bool bEnemyNear = EnemyCorps.ContainsByPredicate([&](const FCampaign1851EnemyCorps& E) { return E.Men > 0 && FVector2D::Distance(E.Km, At) < 10.0; });
		City.LiberationDays = Men >= LiberationMen && !bEnemyNear ? City.LiberationDays + 1.f : 0.f;
		if (City.LiberationDays >= 2.f)
		{
			City.Occupier.Reset();
			City.LiberationDays = 0.f;
			News.Add(FString::Printf(TEXT("%s er befriet: de danske tropper har holdt byen i to døgn"), *City.Name));
		}
	}
}

void ACampaign1851Map::MonthlyWar()
{
	// The tension drifts back towards an uneasy peace; a mobilised Danish army raises it.
	if (!bAtWar)
	{
		Tension += Footing != ECampaign1851Footing::Peace ? 3.f * GuaranteeDamping() : (25.f - Tension) * 0.03f;
		Tension = FMath::Clamp(Tension, 0.f, 100.f);
	}
}

void ACampaign1851Map::DeclareWar()
{
	bAtWar = true;
	WarStartDay = CampaignDays;
	DanishWarLosses = EnemyWarLosses = 0;
	News.Add(TEXT("KRIG: Preussen og Østrig erklærer Danmark krig"));
	FCampaign1851Decision D;
	D.Day = CampaignDays;
	D.Nation = Nations.IndexOfByPredicate([](const FCampaign1851Nation& N) { return N.Id == TEXT("PR"); });
	D.Portfolio = ECampaign1851Portfolio::War;
	D.Action = TEXT("Preussen og Østrig erklærer krig");
	D.Reasons = FString::Printf(TEXT("spændingen nåede %.0f"), Tension);
	D.bDone = true;
	AddDecision(D);
	// The allied corps (a share of each nation's army on the abstract model) come over the Eider.
	SpawnCorps(TEXT("Preussisk I. Korps"), TEXT("PR"), 0.2f, TEXT("Kiel"), { TEXT("Egernførde"), TEXT("Slesvig"), TEXT("Flensborg"), TEXT("Sønderborg") }, 0.f);
	SpawnCorps(TEXT("Østrigsk VI. Korps"), TEXT("AT"), 0.06f, TEXT("Neumünster"), { TEXT("Rendsborg"), TEXT("Slesvig"), TEXT("Flensborg"), TEXT("Kolding"), TEXT("Fredericia") }, 0.f);
}

void ACampaign1851Map::SpawnCorps(const FString& Name, const FString& NationId, float ShareOfArmy, const FString& From, const TArray<FString>& Objectives, float Delay)
{
	const int32 Town = FindCity(From);
	if (!Cities.IsValidIndex(Town))
	{
		return;
	}
	const FCampaign1851Nation* N = Nations.FindByPredicate([&NationId](const FCampaign1851Nation& X) { return X.Id == NationId; });
	FCampaign1851EnemyCorps C;
	C.Id = NextCorpsId++;
	C.Name = Name;
	C.Nation = NationId;
	C.Men = N ? FMath::RoundToInt(N->ArmyMen * ShareOfArmy) : 12000;
	C.StartMen = C.Men;
	C.SeenKm = TownKm(Town);
	C.SeenDay = CampaignDays;
	C.SeenMen = C.Men;
	C.Guns = FMath::Max(12, C.Men / 250);
	C.Km = TownKm(Town);
	C.Town = Town;
	for (const FString& O : Objectives)
	{
		if (FindCity(O) != INDEX_NONE)
		{
			C.Objectives.Add(FindCity(O));
		}
	}
	EnemyCorps.Add(C);
	News.Add(FString::Printf(TEXT("%s (%d mand, %d kanoner) står ved %s"), *C.Name, C.Men, C.Guns, *Cities[Town].Name));
}

void ACampaign1851Map::AdvanceWar(float DeltaDays)
{
	if (DeltaDays <= 0.f)
	{
		return;
	}
	if (FMath::FloorToInt(CampaignDays) != LastWarDay)
	{
		LastWarDay = FMath::FloorToInt(CampaignDays);
		DailyWar();
		DailyWeather();
		DailyHealth();
		DailyOfficers();
		DailySieges();
		DailyBridges();
		EnemyReinforcements();
	}
	UpdateIntel();
	if (Battles.ContainsByPredicate([](const FCampaign1851Battle& B) { return B.bWaiting; }) && FPlatformTime::Seconds() - LastBattlePoll > 1.0)
	{
		LastBattlePoll = FPlatformTime::Seconds();
		PollBattleResults();
	}
	for (int32 k = 0; k < EnemyCorps.Num(); ++k)
	{
		FCampaign1851EnemyCorps& C = EnemyCorps[k];
		if (C.bEngaged || CampaignDays < C.RestUntil)
		{
			continue;
		}
		// Come before the position it means to besiege: it digs in there.
		if (bAtWar && Cities.IsValidIndex(C.SiegeTown) && !C.bSieging && FVector2D::Distance(C.Km, TownKm(C.SiegeTown)) < 9.0)
		{
			C.bSieging = true;
			C.SiegeStart = CampaignDays;
			C.Route.Reset();
			C.Town = INDEX_NONE;
			News.Add(FString::Printf(TEXT("%s belejrer stillingen ved %s"), *C.Name, *Cities[C.SiegeTown].Name));
		}
		// Contact: Danish troops or a fort within 5 km stop the corps; a battle is at hand. A besieging corps
		// fights only a relief from outside the position.
		FString Contact;
		const FVector2D SiegeAt = C.bSieging && Cities.IsValidIndex(C.SiegeTown) ? TownKm(C.SiegeTown) : FVector2D(1e9, 1e9);
		for (const FCampaign1851Regiment& R : Regiments)
		{
			if (FVector2D::Distance(R.Km, C.Km) < 5.0 && R.Men > 0 && FVector2D::Distance(R.Km, SiegeAt) > 10.0)
			{
				Contact = R.Name;
				break;
			}
		}
		for (const FCampaign1851Fort& F : Forts)
		{
			if (Contact.IsEmpty() && FVector2D::Distance(F.Km, C.Km) < 5.0 && F.bBuilt && !C.bSieging)
			{
				Contact = F.Name;
			}
		}
		if (C.bSieging && Contact.IsEmpty())
		{
			continue;   // the siege goes on (DailySieges)
		}
		if (!Contact.IsEmpty() && bAtWar)
		{
			C.bEngaged = true;
			const int32 Near = NearestTown(C.Km);
			News.Add(FString::Printf(TEXT("%s møder %s ved %s: slag forestår"), *C.Name, *Contact, Cities.IsValidIndex(Near) ? *Cities[Near].Name : TEXT("")));
			CreateBattle(k);
			continue;
		}
		if (!Contact.IsEmpty())
		{
			continue;   // not at war yet: the federal corps faces the Danes and waits
		}
		// Marching: along the roads to the next objective (only while at war; the federal corps waits at the Eider).
		if (!bAtWar && C.Nation == TEXT("DE"))
		{
			if (C.Objectives.Num() > 0 && C.Route.Num() == 0 && C.Town == C.Objectives[0])
			{
				continue;
			}
		}
		if (!bAtWar && C.Nation != TEXT("DE"))
		{
			continue;
		}
		if (C.Route.Num() == 0)
		{
			// The enemy chooses its next objective by what it believes of the Danish defence.
			if (bAtWar && C.Nation != TEXT("DE") && !ChooseCorpsObjective(k))
			{
				continue;
			}
			if (C.Objectives.Num() == 0)
			{
				continue;
			}
			const int32 Goal = C.Objectives[0];
			TArray<FCampaign1851Leg> Legs;
			if (C.Town == Goal || !PlanMarch(C.Town, C.Km, Goal, TownKm(Goal), 16.f, ECampaign1851RouteMode::RoadsOnly, Legs))
			{
				C.Objectives.RemoveAt(0);
				continue;
			}
			// Over water: the Danish fleet bars the Belts; a narrow sound takes a week of gathering boats.
			FString Ferry;
			const int32 Water = WaterCrossing(C, Legs, Ferry);
			if (Water == 2)
			{
				News.Add(FString::Printf(TEXT("%s kan ikke gå over %s: den danske flåde behersker farvandet"), *C.Name, *Ferry));
				C.Barred.AddUnique(Goal);
				C.Objectives.RemoveAt(0);
				C.RestUntil = CampaignDays + 2.0;
				continue;
			}
			if (Water == 1)
			{
				if (C.CrossingReadyDay < 0.0)
				{
					C.CrossingReadyDay = CampaignDays + 7.0;
					News.Add(FString::Printf(TEXT("Efterretning: %s samler både ved %s"), *C.Name, *Ferry));
				}
				continue;
			}
			C.CrossingReadyDay = -1.0;
			C.Route = MoveTemp(Legs);
			C.Leg = 0;
			C.LegElapsed = 0.f;
			C.Town = INDEX_NONE;
		}
		C.LegElapsed += DeltaDays * (C.Route.IsValidIndex(C.Leg) ? LegPace(C.Route[C.Leg]) : 1.f);
		while (C.Route.IsValidIndex(C.Leg) && C.LegElapsed >= C.Route[C.Leg].Days)
		{
			C.LegElapsed -= C.Route[C.Leg].Days;
			C.Km = C.Route[C.Leg].ToKm;
			++C.Leg;
		}
		if (C.Route.IsValidIndex(C.Leg))
		{
			const TArray<FVector2D> Line = LegLine(C.Route[C.Leg]);
			C.Km = AlongLine(Line, LineLength(Line) * FMath::Clamp(C.LegElapsed / FMath::Max(C.Route[C.Leg].Days, 0.001f), 0.f, 1.f));
		}
		else
		{
			// Arrived at the objective: an undefended town is occupied.
			const int32 Goal = C.Objectives.Num() > 0 ? C.Objectives[0] : INDEX_NONE;
			C.Route.Reset();
			C.Town = Goal;
			const bool bDefended = Cities.IsValidIndex(Goal) && Regiments.ContainsByPredicate([&](const FCampaign1851Regiment& R) { return R.Men > 0 && FVector2D::Distance(R.Km, TownKm(Goal)) < 5.0; });
			if (Cities.IsValidIndex(Goal) && bDefended)
			{
				C.Km = TownKm(Goal);   // the defenders are met at the town: contact next day
			}
			else if (Cities.IsValidIndex(Goal))
			{
				C.Km = TownKm(Goal);
				if (Cities[Goal].Occupier.IsEmpty() && !Cities[Goal].bForeign)
				{
					Cities[Goal].Occupier = C.Nation == TEXT("DE") ? TEXT("PR") : C.Nation;
					News.Add(FString::Printf(TEXT("%s er besat af %s"), *Cities[Goal].Name, *C.Name));
				}
			}
			if (C.Objectives.Num() > 0 && (bAtWar || C.Nation != TEXT("DE")))
			{
				C.Objectives.RemoveAt(0);
			}
		}
	}
}

bool ACampaign1851Map::IsAmtOccupied(const FCampaign1851Amt& A) const
{
	const int32 Seat = FindCity(A.Seat);
	return Cities.IsValidIndex(Seat) && !Cities[Seat].Occupier.IsEmpty();
}

TArray<FString> ACampaign1851Map::SaveWar() const
{
	TArray<FString> Out;
	Out.Add(FString::Printf(TEXT("state|%.2f|%d"), Tension, bAtWar ? 1 : 0));
	Out.Add(FString::Printf(TEXT("intel-clock|%d"), LastWarDay));
	for (const FString& E : EventsFired)
	{
		Out.Add(FString::Printf(TEXT("fired|%s"), *E));
	}
	for (const FCampaign1851City& C : Cities)
	{
		if (!C.Occupier.IsEmpty())
		{
			Out.Add(FString::Printf(TEXT("occupied|%s|%s"), *C.Name, *C.Occupier));
		}
	}
	for (const FCampaign1851EnemyCorps& C : EnemyCorps)
	{
		FString Objectives;
		for (int32 O : C.Objectives)
		{
			Objectives += (Objectives.IsEmpty() ? TEXT("") : TEXT(",")) + Cities[O].Name;
		}
		// Saved where it stands (on the march: the town at the end of its current stretch, or the point).
		const int32 At = C.Route.IsValidIndex(C.Leg) ? C.Route[C.Leg].To : C.Town;
		Out.Add(FString::Printf(TEXT("corps|%s|%s|%d|%d|%.3f|%.3f|%s|%s|%d"), *C.Name, *C.Nation, C.Men, C.Guns, C.Km.X, C.Km.Y,
			Cities.IsValidIndex(At) ? *Cities[At].Name : TEXT(""), *Objectives, C.bEngaged ? 1 : 0));
	}
	for (int32 k = 0; k < EnemyCorps.Num(); ++k)
	{
		const FCampaign1851EnemyCorps& C = EnemyCorps[k];
		Out.Add(FString::Printf(TEXT("intel|%d|%.3f|%.3f|%.2f|%d|%d"), k, C.SeenKm.X, C.SeenKm.Y, C.SeenDay, C.SeenMen, C.StartMen));
		FString Barred;
		for (int32 Town : C.Barred)
		{
			Barred += (Barred.IsEmpty() ? TEXT("") : TEXT(",")) + FString::FromInt(Town);
		}
		Out.Add(FString::Printf(TEXT("intel-runtime|%d|%d|%d|%.6f|%.6f|%.6f|%.6f|%d|%.6f|%.6f|%d|%.6f|%s"),
			k, C.Id, C.bSeen ? 1 : 0, C.ReportKm.X, C.ReportKm.Y, C.ReportDay, C.ReportArrive, C.ReportMen,
			C.RestUntil, C.NextThink, C.bWaitingNoted ? 1 : 0, C.CrossingReadyDay,
			*Barred));
		if (Cities.IsValidIndex(C.SiegeTown))
		{
			Out.Add(FString::Printf(TEXT("siege|%d|%s|%d|%.2f"), k, *Cities[C.SiegeTown].Name, C.bSieging ? 1 : 0, C.SiegeStart));
		}
	}
	// Battles at hand (one sent to 3D waits for its result across the trip to the battle map and back).
	for (const FCampaign1851Battle& B : Battles)
	{
		FString Units, FortIds;
		for (int32 r : B.Regiments) { if (Regiments.IsValidIndex(r)) { Units += (Units.IsEmpty() ? TEXT("") : TEXT(",")) + Regiments[r].Id; } }
		for (int32 f : B.Forts) { FortIds += (FortIds.IsEmpty() ? TEXT("") : TEXT(",")) + FString::FromInt(f); }
		Out.Add(FString::Printf(TEXT("battle|%d|%.4f|%.3f|%.3f|%d|%d|%s|%s|%d"), B.Id, B.Day, B.Km.X, B.Km.Y, B.Town, CorpsIndexOf(B), *Units, *FortIds, B.bWaiting ? 1 : 0));
	}
	return Out;
}

void ACampaign1851Map::RestoreWar(const TArray<FString>& Lines)
{
	// The plan (dates, weights) comes from the seed; the state from the save.
	ResetWar();
	LastWarDay = FMath::FloorToInt(CampaignDays);
	for (const FString& Line : Lines)
	{
		TArray<FString> P;
		Line.ParseIntoArray(P, TEXT("|"), false);
		if (P.Num() == 3 && P[0] == TEXT("state"))
		{
			Tension = FCString::Atof(*P[1]);
			bAtWar = P[2] == TEXT("1");
		}
		else if (P.Num() == 2 && P[0] == TEXT("intel-clock"))
		{
			LastWarDay = FCString::Atoi(*P[1]);
		}
		else if (P.Num() == 14 && P[0] == TEXT("intel-runtime") && EnemyCorps.IsValidIndex(FCString::Atoi(*P[1])))
		{
			FCampaign1851EnemyCorps& C = EnemyCorps[FCString::Atoi(*P[1])];
			C.Id = FCString::Atoi(*P[2]);
			NextCorpsId = FMath::Max(NextCorpsId, C.Id + 1);
			C.bSeen = P[3] == TEXT("1");
			C.ReportKm = FVector2D(FCString::Atod(*P[4]), FCString::Atod(*P[5]));
			C.ReportDay = FCString::Atod(*P[6]);
			C.ReportArrive = FCString::Atod(*P[7]);
			C.ReportMen = FCString::Atoi(*P[8]);
			C.RestUntil = FCString::Atod(*P[9]);
			C.NextThink = FCString::Atod(*P[10]);
			C.bWaitingNoted = P[11] == TEXT("1");
			C.CrossingReadyDay = FCString::Atod(*P[12]);
			TArray<FString> Barred;
			P[13].ParseIntoArray(Barred, TEXT(","));
			for (const FString& Town : Barred)
			{
				const int32 Index = FCString::Atoi(*Town);
				if (Cities.IsValidIndex(Index)) { C.Barred.AddUnique(Index); }
			}
		}
		else if (P.Num() == 7 && P[0] == TEXT("intel") && EnemyCorps.IsValidIndex(FCString::Atoi(*P[1])))
		{
			FCampaign1851EnemyCorps& C = EnemyCorps[FCString::Atoi(*P[1])];
			C.SeenKm = FVector2D(FCString::Atod(*P[2]), FCString::Atod(*P[3]));
			C.SeenDay = FCString::Atod(*P[4]);
			C.SeenMen = FCString::Atoi(*P[5]);
			C.StartMen = FMath::Max(0, FCString::Atoi(*P[6]));
		}
		else if (P.Num() == 5 && P[0] == TEXT("siege") && EnemyCorps.IsValidIndex(FCString::Atoi(*P[1])))
		{
			FCampaign1851EnemyCorps& C = EnemyCorps[FCString::Atoi(*P[1])];
			C.SiegeTown = FindCity(P[2]);
			C.bSieging = P[3] == TEXT("1");
			C.SiegeStart = FCString::Atod(*P[4]);
		}
		else if (P.Num() == 2 && P[0] == TEXT("fired"))
		{
			EventsFired.Add(P[1]);
		}
		else if (P.Num() == 3 && P[0] == TEXT("occupied") && FindCity(P[1]) != INDEX_NONE)
		{
			Cities[FindCity(P[1])].Occupier = P[2];
		}
		else if (P.Num() == 10 && P[0] == TEXT("battle"))
		{
			FCampaign1851Battle B;
			B.Id = FCString::Atoi(*P[1]);
			B.Day = FCString::Atod(*P[2]);
			B.Km = FVector2D(FCString::Atod(*P[3]), FCString::Atod(*P[4]));
			B.Town = FCString::Atoi(*P[5]);
			const int32 k = FCString::Atoi(*P[6]);
			if (!EnemyCorps.IsValidIndex(k))
			{
				continue;
			}
			B.CorpsId = EnemyCorps[k].Id;
			EnemyCorps[k].bEngaged = true;
			TArray<FString> Ids;
			P[7].ParseIntoArray(Ids, TEXT(","));
			for (const FString& Id : Ids) { if (FindRegiment(Id) != INDEX_NONE) { B.Regiments.Add(FindRegiment(Id)); } }
			Ids.Reset();
			P[8].ParseIntoArray(Ids, TEXT(","));
			for (const FString& Id : Ids) { B.Forts.Add(FCString::Atoi(*Id)); }
			B.bWaiting = P[9] == TEXT("1");
			NextBattleId = FMath::Max(NextBattleId, B.Id + 1);
			Battles.Add(B);
		}
		else if (P.Num() == 10 && P[0] == TEXT("corps"))
		{
			FCampaign1851EnemyCorps C;
			C.Id = NextCorpsId++;
			C.Name = P[1];
			C.Nation = P[2];
			C.Men = FCString::Atoi(*P[3]);
			C.Guns = FCString::Atoi(*P[4]);
			C.Km = FVector2D(FCString::Atod(*P[5]), FCString::Atod(*P[6]));
			C.Town = FindCity(P[7]);
			if (C.Town != INDEX_NONE)
			{
				C.Km = TownKm(C.Town);
			}
			TArray<FString> Objectives;
			P[8].ParseIntoArray(Objectives, TEXT(","));
			for (const FString& O : Objectives)
			{
				if (FindCity(O) != INDEX_NONE)
				{
					C.Objectives.Add(FindCity(O));
				}
			}
			C.bEngaged = false;   // a battle still at hand is offered again on contact
			C.StartMen = C.Men;
			C.SeenKm = C.Km;
			C.SeenMen = C.Men;
			C.bSeen = !bAtWar;
			EnemyCorps.Add(C);
		}
	}
}
