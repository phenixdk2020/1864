// Research and doctrine of the 1851 campaign (Docs/Research1851.md): one project at a time, paid by the
// month, each opening in its historical year; and the army's doctrine on three levels (strategic,
// operational, tactical), which takes two months to change. Both reach the battles, the forts, the supply
// and the mobilisation, and go out to the 3D battles in Units.json and the battle request.

#include "Campaign1851Map.h"

#include "Dom/JsonObject.h"

namespace Campaign1851Research
{
	const TArray<FCampaign1851ResearchTopic>& Topics()
	{
		static const TArray<FCampaign1851ResearchTopic> List = {
			{ TEXT("sanitation"), TEXT("Sanitetsvæsenet"),          TEXT("Ambulancer og feltlazaretter: tab i slag −20 %"),                          1852, 800.0,  12, nullptr, 0 },
			{ TEXT("fortress"),   TEXT("Fæstningsbyggeri"),         TEXT("Ingeniørkorpsets skole: skansernes dækning +10 %-point"),                  1852, 1200.0, 12, nullptr, 1 },
			{ TEXT("conserves"),  TEXT("Konserves og feltbagerier"), TEXT("Enhederne bærer 2 dages proviant mere (6 i stedet for 4)"),              1853, 1000.0, 10, TEXT("sanitation"), 0 },
			{ TEXT("telegraph"),  TEXT("Felttelegrafen"),           TEXT("Indkaldelsen går 25 % hurtigere; meldinger på timer"),                     1854, 1500.0, 12, nullptr, 2 },
			{ TEXT("staff"),      TEXT("Stabsskolen"),              TEXT("Uddannede stabsofficerer: kampværdi +5 %. I slaget: kommandozonerne 15 % større"), 1855, 1000.0, 18, nullptr, 6 },
			{ TEXT("railmob"),    TEXT("Jernbanemobilisering"),     TEXT("Køreplaner for indkaldelsen: yderligere 25 % hurtigere"),                  1856, 1500.0, 12, TEXT("telegraph"), 2 },
			{ TEXT("hospitals"),  TEXT("Militærhospitaler"),        TEXT("Syge og sårede kommer 40 % hurtigere tilbage"),                           1857, 1200.0, 12, TEXT("conserves"), 0 },
			{ TEXT("riflegun"),   TEXT("Riflede kanoner"),          TEXT("Længere rækkevidde og træfsikkerhed: kanonerne tæller 40 % mere"),         1858, 3000.0, 18, nullptr, 4 },
			{ TEXT("casemates"),  TEXT("Kasematter og blendinger"), TEXT("Skansernes dækning yderligere +10 %-point"),                              1859, 2000.0, 15, TEXT("fortress"), 1 },
			{ TEXT("breech"),     TEXT("Bagladegeværet"),           TEXT("Infanteriet lader liggende og tre gange så hurtigt: kampværdi +25 %. I slaget: ladetid × 0,35"), 1860, 4000.0, 24, TEXT("skirmish"), 3 },
			// Tied to the 3D battle's own systems (Docs/BattleLink1851.md): they go out in battleRules.
			{ TEXT("square"),     TEXT("Karré-eksercitsen"),        TEXT("Kompagniet danner karré på 30 % kortere tid, og hver side skyder 30 % i stedet for 25 %. Kampværdi +2 %"), 1852, 600.0, 8, nullptr, 3 },
			{ TEXT("skirmish"),   TEXT("Kædelinjer og jægertaktik"), TEXT("Spredt orden i kornet og bag hegnene: dækningen der × 1,25, skyttekamp +15 %, tab −10 %"), 1854, 1000.0, 12, TEXT("square"), 3 },
			{ TEXT("recon"),      TEXT("Rytterspejdning"),          TEXT("Rytteriet ser 50 % længere på kortet. I slaget: ordren SPEJD HER, når fjenden ikke længere ses overalt"), 1852, 700.0, 8, nullptr, 5 },
			{ TEXT("carbine"),    TEXT("Dragonernes ildkamp"),      TEXT("Afsiddede dragoner skyder til 45/90/130 m i stedet for 35/70/100 m og lader på 5 s i stedet for 7. Kampværdi +2 % med rytteri"), 1855, 1200.0, 12, TEXT("recon"), 5 },
			{ TEXT("shock"),      TEXT("Rytterchokket"),            TEXT("Rytteriet reformerer 25 % hurtigere, og et angreb i flanke eller ryg ryster 20 % mere. Kampværdi +3 % med rytteri"), 1857, 1500.0, 14, TEXT("carbine"), 5 },
			{ TEXT("genstaff"),   TEXT("Generalstaben"),            TEXT("Brigade- og divisionsstabe: ordrer udføres 25 % hurtigere, kommandozonerne yderligere 15 % større, +5 % i slag med 3 enheder eller flere"), 1858, 2000.0, 18, TEXT("staff"), 6 },
			{ TEXT("pontoon"),    TEXT("Pontonnerkorpset"),         TEXT("Pontonbroer koster 40 % mindre og lægges på halv tid. I slaget: pionererne kan slå en bro over en å"), 1856, 1500.0, 12, TEXT("railmob"), 2 },
			// The fire methods (the battle's fire drill): researched here, then trained by each regiment in garrison
			// (eksercits, skydeøvelser or blandet) to 60 before the companies may use them in battle.
			// The trades: farms, smithies, works and credit (the land's wealth pays for the army).
			{ TEXT("marl"),       TEXT("Mergling og dræning"),      TEXT("Landbruget mergler og dræner jorden: skatten fra landet +8 %"), 1852, 1200.0, 12, nullptr, 7 },
			{ TEXT("agrischool"), TEXT("Landbohøjskolen"),          TEXT("Uddannede forpagtere og bedre sædskifte: skatten fra landet yderligere +7 %"), 1856, 2000.0, 18, TEXT("marl"), 7 },
			{ TEXT("smithy"),     TEXT("Smede og redskaber"),       TEXT("Bedre smedjer og værktøj i byerne: geværværksteder og støberier yder 15 % mere"), 1853, 800.0, 8, nullptr, 7 },
			{ TEXT("steam"),      TEXT("Dampmaskiner"),             TEXT("Dampkraft i værkstederne: værkerne yder yderligere 25 %, og skatten fra byerne +5 %"), 1855, 2500.0, 18, TEXT("smithy"), 7 },
			{ TEXT("credit"),     TEXT("Kreditforeninger"),         TEXT("Kredit til håndværk og handel: skatten fra byerne +5 %"), 1852, 1000.0, 10, nullptr, 7 },
			{ TEXT("tworank"),    TEXT("To-geleds ild"),            TEXT("De to forreste geledder skyder sammen. Skal derefter indøves i regimenterne (eksercits/skydeøvelser)"), 1852, 500.0, 6, nullptr, 3 },
			{ TEXT("firebyrank"), TEXT("Geledild"),                 TEXT("Geledderne skyder på skift, så ilden aldrig hører op. Skal indøves i regimenterne"), 1853, 800.0, 8, TEXT("tworank"), 3 },
			{ TEXT("volley"),     TEXT("Kommanderet salve"),        TEXT("Hele kompagniet på kommando: den tunge salve, der ryster fjenden. Skal indøves i regimenterne"), 1855, 900.0, 8, TEXT("firebyrank"), 3 },
			{ TEXT("independent"), TEXT("Fri ild"),                 TEXT("Hver mand skyder, når han har ladt og sigtet: hurtigere ild, svagere salver. Skal indøves i regimenterne"), 1857, 1000.0, 10, TEXT("volley"), 3 },
		};
		return List;
	}

	int32 FindTopic(const FString& Id)
	{
		return Topics().IndexOfByPredicate([&Id](const FCampaign1851ResearchTopic& T) { return Id == T.Id; });
	}

	int32 DoctrineChoices(int32 Level) { return Level == 2 ? 3 : 2; }

	int32 Tier(int32 Topic)
	{
		// The depth in its branch: I for a root, II for what it opens, and so on.
		int32 Depth = 0;
		for (int32 t = Topic; Topics().IsValidIndex(t) && Topics()[t].Needs && Depth < 10; t = FindTopic(Topics()[t].Needs))
		{
			++Depth;
		}
		return Depth;
	}

	const TCHAR* Roman(int32 Tier)
	{
		static const TCHAR* Numerals[] = { TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"), TEXT("VI"), TEXT("VII"), TEXT("VIII") };
		return Numerals[FMath::Clamp(Tier, 0, 7)];
	}

	const TCHAR* BranchName(int32 Branch)
	{
		static const TCHAR* Names[Branches] = { TEXT("Sanitet og forsyning"), TEXT("Befæstning"), TEXT("Samfærdsel"), TEXT("Infanteriet"), TEXT("Artilleriet"), TEXT("Kavaleriet"), TEXT("Kommando"), TEXT("Næringsliv") };
		return Branch >= 0 && Branch < Branches ? Names[Branch] : TEXT("");
	}

	const TCHAR* LevelName(int32 Level)
	{
		return Level == 0 ? TEXT("Strategisk") : Level == 1 ? TEXT("Operativ") : TEXT("Taktisk");
	}

	const TCHAR* DoctrineName(int32 Level, int32 Choice)
	{
		static const TCHAR* Names[3][3] = {
			{ TEXT("Fæstningen"), TEXT("Felthæren"), TEXT("") },
			{ TEXT("Koncentration"), TEXT("Forsvar i dybden"), TEXT("") },
			{ TEXT("Ildkamp"), TEXT("Bajonetangreb"), TEXT("Spredt orden") } };
		return Level >= 0 && Level < 3 && Choice >= 0 && Choice < 3 ? Names[Level][Choice] : TEXT("");
	}

	const TCHAR* DoctrineEffect(int32 Level, int32 Choice)
	{
		static const TCHAR* Effects[3][3] = {
			{ TEXT("Dannevirke og skanserne bærer forsvaret: dækning +10 %-point, men felthæren uden skanser −5 %"),
			  TEXT("En bevægelig felthær: +5 % i slag uden skanser"), TEXT("") },
			{ TEXT("Samle hæren til det afgørende slag: +8 % med 3 enheder eller flere, ellers −5 %"),
			  TEXT("Kæmpe og vige: tab −20 %, men −3 % i kampkraft"), TEXT("") },
			{ TEXT("Skyd bag dækning: +10 % i slag ved skanser"),
			  TEXT("Den danske tradition fra 1848-50: +10 % mod Østrig og Forbundet, −15 % mod Preussens tændnålsgevær"),
			  TEXT("Kædelinjer og terrænudnyttelse: tab −15 % og +3 %") } };
		return Level >= 0 && Level < 3 && Choice >= 0 && Choice < 3 ? Effects[Level][Choice] : TEXT("");
	}
}

void ACampaign1851Map::ResetResearch()
{
	Researched.Reset();
	Researching = INDEX_NONE;
	ResearchMonths = 0;
	bResearchStalled = false;
	// As the army stood after 1848-50: Dannevirke, concentration, the bayonet.
	Doctrine[0] = 0;
	Doctrine[1] = 0;
	Doctrine[2] = 1;
	DoctrineSettledDay = 0.0;
}

bool ACampaign1851Map::HasResearch(const TCHAR* Id) const
{
	return Researched.Contains(FString(Id));
}

FString ACampaign1851Map::ResearchBlockReason(int32 Topic) const
{
	const TArray<FCampaign1851ResearchTopic>& List = Campaign1851Research::Topics();
	if (!List.IsValidIndex(Topic))
	{
		return TEXT("-");
	}
	const FCampaign1851ResearchTopic& T = List[Topic];
	if (Researched.Contains(T.Id)) return TEXT("færdig");
	if (Researching == Topic) return TEXT("i gang");
	if (T.Needs && !Researched.Contains(T.Needs))
	{
		const int32 Need = Campaign1851Research::FindTopic(T.Needs);
		return FString::Printf(TEXT("kræver %s"), List.IsValidIndex(Need) ? List[Need].Name : T.Needs);
	}
	if (Treasury < T.CostPerMonth) return TEXT("ikke råd");
	return FString();
}

bool ACampaign1851Map::StartResearch(int32 Topic, FString* OutReason)
{
	const FString Why = ResearchBlockReason(Topic);
	if (!Why.IsEmpty())
	{
		if (OutReason) { *OutReason = Why; }
		return false;
	}
	// Switching drops what was done on the old project.
	Researching = Topic;
	ResearchMonths = 0;
	bResearchStalled = false;
	const FCampaign1851ResearchTopic& T = Campaign1851Research::Topics()[Topic];
	News.Add(FString::Printf(TEXT("Forskning: %s påbegyndt (%d måneder, %s rd./md.)"), T.Name, T.Months, *FString::FromInt(int32(T.CostPerMonth))));
	return true;
}

void ACampaign1851Map::MonthlyResearch()
{
	const TArray<FCampaign1851ResearchTopic>& List = Campaign1851Research::Topics();
	// The War Ministry on AUTO (or ADVISORY: it proposes) takes up the first project open to it.
	const bool bAuto = Nations.IsValidIndex(PlayerNation) && Nations[PlayerNation].Mode(ECampaign1851Portfolio::War) == ECampaign1851Delegation::Auto;
	if (Researching == INDEX_NONE && bAuto)
	{
		for (int32 t = 0; t < List.Num(); ++t)
		{
			if (ResearchBlockReason(t).IsEmpty() && Treasury - List[t].CostPerMonth * List[t].Months > Nations[PlayerNation].Reserve
				&& MinistryCanSpend(ECampaign1851Portfolio::War, List[t].CostPerMonth * List[t].Months))
			{
				MinistrySpend(ECampaign1851Portfolio::War, List[t].CostPerMonth * List[t].Months);
				StartResearch(t);
				FCampaign1851Decision D;
				D.Day = CampaignDays;
				D.Nation = PlayerNation;
				D.Portfolio = ECampaign1851Portfolio::War;
				D.Action = FString::Printf(TEXT("Forskning: %s"), List[t].Name);
				D.Reasons = List[t].Effect;
				D.Cost = List[t].CostPerMonth * List[t].Months;
				D.bDone = true;
				AddDecision(D);
				break;
			}
		}
	}
	if (!List.IsValidIndex(Researching))
	{
		return;
	}
	const FCampaign1851ResearchTopic& T = List[Researching];
	if (Treasury < T.CostPerMonth)
	{
		if (!bResearchStalled)
		{
			News.Add(FString::Printf(TEXT("Forskning: %s står stille (ingen penge)"), T.Name));
		}
		bResearchStalled = true;
		return;
	}
	bResearchStalled = false;
	AddTransaction(-T.CostPerMonth, FString::Printf(TEXT("Forskning: %s"), T.Name));
	if (++ResearchMonths >= T.Months)
	{
		Researched.Add(T.Id);
		News.Add(FString::Printf(TEXT("Forskning færdig: %s. %s"), T.Name, T.Effect));
		Researching = INDEX_NONE;
		ResearchMonths = 0;
	}
}

bool ACampaign1851Map::SetDoctrine(int32 Level, int32 Choice, FString* OutReason)
{
	if (Level < 0 || Level > 2 || Choice < 0 || Choice >= Campaign1851Research::DoctrineChoices(Level) || Doctrine[Level] == Choice)
	{
		return false;
	}
	if (IsDoctrineChanging())
	{
		if (OutReason) { *OutReason = TEXT("hæren er stadig ved at omstille sig"); }
		return false;
	}
	if (Treasury < DoctrineChangeCost)
	{
		if (OutReason) { *OutReason = TEXT("ikke råd"); }
		return false;
	}
	AddTransaction(-DoctrineChangeCost, FString::Printf(TEXT("Ny doktrin: %s"), Campaign1851Research::DoctrineName(Level, Choice)));
	Doctrine[Level] = Choice;
	DoctrineSettledDay = CampaignDays + DoctrineChangeDays;
	// Drill to new regulations shakes the ranks for a while.
	for (FCampaign1851Regiment& R : Regiments)
	{
		R.Morale = FMath::Max(0.3f, R.Morale - 0.05f);
	}
	News.Add(FString::Printf(TEXT("%s doktrin: %s. Omstillingen tager %.0f dage (kampkraft −10 %% imens)"), Campaign1851Research::LevelName(Level),
		Campaign1851Research::DoctrineName(Level, Choice), DoctrineChangeDays));
	return true;
}

float ACampaign1851Map::DanishQualityFactor(const FCampaign1851Battle& B) const
{
	const bool bForts = B.Forts.Num() > 0;
	const int32 Ci = CorpsIndexOf(B);
	const FString Enemy = Ci != INDEX_NONE ? EnemyCorps[Ci].Nation : FString();
	float F = 1.f;
	// Strategic.
	if (!bForts)
	{
		F *= Doctrine[0] == 0 ? 0.95f : 1.05f;
	}
	// Operational.
	F *= Doctrine[1] == 0 ? (B.Regiments.Num() >= 3 ? 1.08f : 0.95f) : 0.97f;
	// Tactical.
	if (Doctrine[2] == 0)
	{
		F *= bForts ? 1.1f : 1.f;
	}
	else if (Doctrine[2] == 1)
	{
		F *= Enemy == TEXT("PR") ? 0.85f : 1.1f;
	}
	else
	{
		F *= 1.03f;
	}
	F *= HasResearch(TEXT("staff")) ? 1.05f : 1.f;
	F *= HasResearch(TEXT("square")) ? 1.02f : 1.f;
	F *= HasResearch(TEXT("genstaff")) && B.Regiments.Num() >= 3 ? 1.05f : 1.f;
	const bool bCavalry = B.Regiments.ContainsByPredicate([this](int32 i) { return Regiments.IsValidIndex(i) && Regiments[i].Arm == ECampaign1851Arm::Cavalry; });
	F *= bCavalry && HasResearch(TEXT("carbine")) ? 1.02f : 1.f;
	F *= bCavalry && HasResearch(TEXT("shock")) ? 1.03f : 1.f;
	F *= IsDoctrineChanging() ? 0.9f : 1.f;
	return F;
}

float ACampaign1851Map::DanishLossFactor() const
{
	return (HasResearch(TEXT("sanitation")) ? 0.8f : 1.f) * (Doctrine[1] == 1 ? 0.8f : 1.f) * (Doctrine[2] == 2 ? 0.85f : 1.f) * (HasResearch(TEXT("skirmish")) ? 0.9f : 1.f);
}

float ACampaign1851Map::FortCoverBonus() const
{
	return (HasResearch(TEXT("fortress")) ? 10.f : 0.f) + (HasResearch(TEXT("casemates")) ? 10.f : 0.f) + (Doctrine[0] == 0 ? 10.f : 0.f);
}

float ACampaign1851Map::DanishGunFactor() const
{
	return HasResearch(TEXT("riflegun")) ? 1.4f : 1.f;
}

float ACampaign1851Map::InfantryFactor() const
{
	return HasResearch(TEXT("breech")) ? 1.25f : 1.f;
}

float ACampaign1851Map::FoodCap() const
{
	return Campaign1851Supply::FoodCarried + (HasResearch(TEXT("conserves")) ? 2.f : 0.f);
}

float ACampaign1851Map::CallInFactor() const
{
	return (HasResearch(TEXT("telegraph")) ? 1.25f : 1.f) * (HasResearch(TEXT("railmob")) ? 1.25f : 1.f) * CallInMoodFactor();
}

double ACampaign1851Map::PontoonCost() const
{
	return 25000.0 * (HasResearch(TEXT("pontoon")) ? 0.6 : 1.0);
}

float ACampaign1851Map::PontoonDays() const
{
	return 45.f * (HasResearch(TEXT("pontoon")) ? 0.5f : 1.f);
}

double ACampaign1851Map::RuralTaxFactor() const
{
	return 1.0 + (HasResearch(TEXT("marl")) ? 0.08 : 0.0) + (HasResearch(TEXT("agrischool")) ? 0.07 : 0.0);
}

double ACampaign1851Map::UrbanTaxFactor() const
{
	return 1.0 + (HasResearch(TEXT("steam")) ? 0.05 : 0.0) + (HasResearch(TEXT("credit")) ? 0.05 : 0.0);
}

float ACampaign1851Map::WorksOutputFactor() const
{
	return (HasResearch(TEXT("smithy")) ? 1.15f : 1.f) * (HasResearch(TEXT("steam")) ? 1.25f : 1.f);
}

float ACampaign1851Map::CommandReachFactor() const
{
	return (HasResearch(TEXT("staff")) ? 1.15f : 1.f) * (HasResearch(TEXT("genstaff")) ? 1.15f : 1.f);
}

void ACampaign1851Map::WriteBattleRulesJson(const TSharedRef<FJsonObject>& Doc) const
{
	// The research and doctrine as the 3D battle's own parameters (the Unity prototype F30; Docs/BattleLink1851.md).
	auto Num = [](const TSharedRef<FJsonObject>& O, const TCHAR* Key, double V) { O->SetNumberField(Key, FMath::RoundToDouble(V * 1000.0) / 1000.0); };
	auto Array = [](std::initializer_list<double> Values)
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		for (double V : Values) { Out.Add(MakeShared<FJsonValueNumber>(FMath::RoundToDouble(V))); }
		return Out;
	};
	TSharedRef<FJsonObject> Rules = MakeShared<FJsonObject>();
	TSharedRef<FJsonObject> Inf = MakeShared<FJsonObject>();
	Num(Inf, TEXT("reloadFactor"), HasResearch(TEXT("breech")) ? 0.35 : 1.0);
	Inf->SetBoolField(TEXT("proneLoading"), HasResearch(TEXT("breech")));
	Num(Inf, TEXT("squareFormTimeFactor"), HasResearch(TEXT("square")) ? 0.7 : 1.0);
	Num(Inf, TEXT("squareFaceFireShare"), HasResearch(TEXT("square")) ? 0.3 : 0.25);
	Num(Inf, TEXT("concealmentFactor"), HasResearch(TEXT("skirmish")) ? 1.25 : 1.0);
	Num(Inf, TEXT("skirmishFactor"), (HasResearch(TEXT("skirmish")) ? 1.15 : 1.0) * (Doctrine[2] == 2 ? 1.1 : 1.0));
	Num(Inf, TEXT("deployBufferM"), 35.0);
	Rules->SetObjectField(TEXT("infantry"), Inf);
	TSharedRef<FJsonObject> Cav = MakeShared<FJsonObject>();
	const bool bCarbine = HasResearch(TEXT("carbine"));
	Cav->SetArrayField(TEXT("carbineRangesM"), bCarbine ? Array({ 45.0, 90.0, 130.0 }) : Array({ 35.0, 70.0, 100.0 }));
	Num(Cav, TEXT("carbineReloadS"), bCarbine ? 5.0 : 7.0);
	Num(Cav, TEXT("reformSpeedFactor"), HasResearch(TEXT("shock")) ? 1.25 : 1.0);
	Num(Cav, TEXT("flankShockFactor"), HasResearch(TEXT("shock")) ? 1.2 : 1.0);
	Cav->SetBoolField(TEXT("reconOrder"), HasResearch(TEXT("recon")));
	Rules->SetObjectField(TEXT("cavalry"), Cav);
	TSharedRef<FJsonObject> Art = MakeShared<FJsonObject>();
	Num(Art, TEXT("rangeFactor"), HasResearch(TEXT("riflegun")) ? 1.4 : 1.0);
	Num(Art, TEXT("accuracyFactor"), HasResearch(TEXT("riflegun")) ? 1.3 : 1.0);
	Rules->SetObjectField(TEXT("artillery"), Art);
	TSharedRef<FJsonObject> Cmd = MakeShared<FJsonObject>();
	const double Reach = CommandReachFactor();
	Num(Cmd, TEXT("reachFactor"), Reach);
	TSharedRef<FJsonObject> Bands = MakeShared<FJsonObject>();
	Bands->SetArrayField(TEXT("major"), Array({ 320.0 * Reach, 450.0 * Reach }));
	Bands->SetArrayField(TEXT("regiment"), Array({ 800.0 * Reach, 1100.0 * Reach }));
	Bands->SetArrayField(TEXT("brigade"), Array({ 1350.0 * Reach, 1850.0 * Reach }));
	Bands->SetArrayField(TEXT("division"), Array({ 2100.0 * Reach, 2850.0 * Reach }));
	Cmd->SetObjectField(TEXT("reachBandsM"), Bands);
	Num(Cmd, TEXT("orderDelayFactor"), HasResearch(TEXT("genstaff")) ? 0.75 : 1.0);
	Cmd->SetBoolField(TEXT("higherHqAi"), HasResearch(TEXT("genstaff")));
	Rules->SetObjectField(TEXT("command"), Cmd);
	TSharedRef<FJsonObject> Eng = MakeShared<FJsonObject>();
	Eng->SetBoolField(TEXT("pioneerBridge"), HasResearch(TEXT("pontoon")));
	Num(Eng, TEXT("fortCoverBonusPercent"), FortCoverBonus());
	Rules->SetObjectField(TEXT("engineering"), Eng);
	Num(Rules, TEXT("lossFactor"), DanishLossFactor());
	Doc->SetObjectField(TEXT("battleRules"), Rules);
	// The doctrine as the battle AI's defaults: the higher HQs' DEF/BAL/OFF, the companies' fire policy.
	TSharedRef<FJsonObject> Ai = MakeShared<FJsonObject>();
	Ai->SetStringField(TEXT("higherDoctrine"), Doctrine[1] == 1 ? TEXT("DEF") : Doctrine[2] == 1 ? TEXT("OFF") : TEXT("BAL"));
	Ai->SetStringField(TEXT("firePolicy"), Doctrine[2] == 0 ? TEXT("LONG") : Doctrine[2] == 1 ? TEXT("CLOSE") : TEXT("MED"));
	Ai->SetBoolField(TEXT("chargeAtWill"), Doctrine[2] == 1);
	Ai->SetBoolField(TEXT("openOrder"), Doctrine[2] == 2);
	Ai->SetBoolField(TEXT("holdForts"), Doctrine[0] == 0);
	Doc->SetObjectField(TEXT("aiDefaults"), Ai);
}

void ACampaign1851Map::WriteDoctrineJson(const TSharedRef<FJsonObject>& Doc) const
{
	TSharedRef<FJsonObject> D = MakeShared<FJsonObject>();
	D->SetStringField(TEXT("strategic"), Campaign1851Research::DoctrineName(0, Doctrine[0]));
	D->SetStringField(TEXT("operational"), Campaign1851Research::DoctrineName(1, Doctrine[1]));
	D->SetStringField(TEXT("tactical"), Campaign1851Research::DoctrineName(2, Doctrine[2]));
	D->SetBoolField(TEXT("changing"), IsDoctrineChanging());
	D->SetNumberField(TEXT("lossFactor"), DanishLossFactor());
	D->SetNumberField(TEXT("fortCoverBonusPercent"), FortCoverBonus());
	D->SetNumberField(TEXT("gunFactor"), DanishGunFactor());
	D->SetNumberField(TEXT("infantryFactor"), InfantryFactor());
	Doc->SetObjectField(TEXT("doctrine"), D);
	WriteBattleRulesJson(Doc);
	TArray<TSharedPtr<FJsonValue>> Done;
	for (const FString& Id : Researched)
	{
		Done.Add(MakeShared<FJsonValueString>(Id));
	}
	Doc->SetArrayField(TEXT("research"), Done);
}

TArray<FString> ACampaign1851Map::SaveResearch() const
{
	TArray<FString> Out;
	Out.Add(FString::Printf(TEXT("state|%s|%d|%d|%d|%d|%.2f"), Campaign1851Research::Topics().IsValidIndex(Researching) ? Campaign1851Research::Topics()[Researching].Id : TEXT(""),
		ResearchMonths, Doctrine[0], Doctrine[1], Doctrine[2], DoctrineSettledDay));
	for (const FString& Id : Researched)
	{
		Out.Add(TEXT("done|") + Id);
	}
	return Out;
}

void ACampaign1851Map::RestoreResearch(const TArray<FString>& Lines)
{
	ResetResearch();
	for (const FString& Line : Lines)
	{
		TArray<FString> P;
		Line.ParseIntoArray(P, TEXT("|"), false);
		if (P.Num() == 7 && P[0] == TEXT("state"))
		{
			Researching = Campaign1851Research::FindTopic(P[1]);
			ResearchMonths = FCString::Atoi(*P[2]);
			for (int32 l = 0; l < 3; ++l)
			{
				Doctrine[l] = FMath::Clamp(FCString::Atoi(*P[3 + l]), 0, Campaign1851Research::DoctrineChoices(l) - 1);
			}
			DoctrineSettledDay = FCString::Atod(*P[6]);
		}
		else if (P.Num() == 2 && P[0] == TEXT("done") && Campaign1851Research::FindTopic(P[1]) != INDEX_NONE)
		{
			Researched.Add(P[1]);
		}
	}
}
