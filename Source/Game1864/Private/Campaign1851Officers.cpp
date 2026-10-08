// Officers of the 1851 campaign: regimental chiefs and generals (design manual 8), ACampaign1851Map's officer corps.

#include "Campaign1851Map.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const TCHAR* StatKeys[] = { TEXT("leadership"), TEXT("inspiration"), TEXT("initiative"), TEXT("tactical"), TEXT("staff"), TEXT("discipline"), TEXT("aggression"), TEXT("composure"), TEXT("political"), TEXT("caution") };
	constexpr int32 NumStats = int32(ECampaign1851OfficerStat::Count);
	/** Officers in reserve at the start, beside one chief per regiment. */
	constexpr int32 ReserveOfficers = 4;

	/** A unit's chief by its arm: colonels for the cavalry regiments, majors for battalions, captains for batteries. */
	const TCHAR* ChiefRank(ECampaign1851Arm Arm)
	{
		switch (Arm)
		{
		case ECampaign1851Arm::Guard: return TEXT("Major");
		case ECampaign1851Arm::Cavalry: return TEXT("Oberst");
		case ECampaign1851Arm::Artillery:
		case ECampaign1851Arm::HorseArtillery: return TEXT("Kaptajn");
		default: return TEXT("Major");
		}
	}

	/** An officer leaves the company he leads (if any). */
	void LeaveCompany(FCampaign1851Officer& O, TArray<FCampaign1851Regiment>& Regiments)
	{
		if (Regiments.IsValidIndex(O.CaptainOf) && Regiments[O.CaptainOf].Captains.IsValidIndex(O.Company))
		{
			Regiments[O.CaptainOf].Captains[O.Company] = INDEX_NONE;
		}
		O.CaptainOf = O.Company = INDEX_NONE;
	}
}

bool ACampaign1851Map::LoadOfficers()
{
	FString Text;
	TSharedPtr<FJsonObject> Json;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Campaign1851/Officers1851.json")))
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|officers|no Data/Campaign1851/Officers1851.json"));
		return false;
	}
	GeneralsAtStart.Reset();
	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("generals")))
	{
		const TSharedPtr<FJsonObject> O = Value->AsObject();
		FCampaign1851Officer G;
		G.Id = O->GetStringField(TEXT("id"));
		G.Name = O->GetStringField(TEXT("name"));
		G.Rank = O->GetStringField(TEXT("rank"));
		O->TryGetNumberField(TEXT("born"), G.Born);
		G.bGeneral = true;
		for (int32 s = 0; s < NumStats; ++s)
		{
			int32 V = 5;
			O->TryGetNumberField(StatKeys[s], V);
			G.Stats[s] = uint8(FMath::Clamp(V, 1, 10));
		}
		double Xp = 50.0;
		O->TryGetNumberField(TEXT("experience"), Xp);
		G.Experience = float(Xp);
		GeneralsAtStart.Add(G);
	}
	Json->TryGetStringArrayField(TEXT("firstNames"), FirstNames);
	Json->TryGetStringArrayField(TEXT("surnames"), Surnames);
	const TSharedPtr<FJsonObject>* Pay = nullptr;
	if (Json->TryGetObjectField(TEXT("pay"), Pay))
	{
		(*Pay)->TryGetNumberField(TEXT("officer"), OfficerPay);
		(*Pay)->TryGetNumberField(TEXT("general"), GeneralPay);
	}
	const TSharedPtr<FJsonObject>* Cost = nullptr;
	if (Json->TryGetObjectField(TEXT("recruitCost"), Cost))
	{
		(*Cost)->TryGetNumberField(TEXT("officer"), OfficerRecruitCost);
		(*Cost)->TryGetNumberField(TEXT("general"), GeneralRecruitCost);
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|officers|%d generals, %d names"), GeneralsAtStart.Num(), FirstNames.Num() * Surnames.Num());
	return true;
}

FCampaign1851Officer ACampaign1851Map::MakeOfficer(FRandomStream& Rng, bool bGeneral, const FString& Rank) const
{
	FCampaign1851Officer O;
	O.Id = FString::Printf(TEXT("O%d"), NextOfficerNumber);
	O.Name = FString::Printf(TEXT("%s %s"), FirstNames.Num() ? *FirstNames[Rng.RandHelper(FirstNames.Num())] : TEXT("Hans"),
		Surnames.Num() ? *Surnames[Rng.RandHelper(Surnames.Num())] : TEXT("Hansen"));
	O.Rank = Rank;
	O.bGeneral = bGeneral;
	// Most officers are middling, a few good or poor: the mean of two rolls; generals a notch better.
	for (int32 s = 0; s < NumStats; ++s)
	{
		const int32 Roll = (Rng.RandRange(2, 9) + Rng.RandRange(2, 9) + 1) / 2 + (bGeneral && s != int32(ECampaign1851OfficerStat::Aggression) ? 1 : 0);
		O.Stats[s] = uint8(FMath::Clamp(Roll, 1, 10));
	}
	O.Experience = bGeneral ? Rng.FRandRange(50.f, 80.f) : Rng.FRandRange(25.f, 65.f);
	O.Born = GetDate().GetYear() - (bGeneral ? Rng.RandRange(50, 64) : Rng.RandRange(34, 52));
	return O;
}

void ACampaign1851Map::ResetOfficers()
{
	Officers = GeneralsAtStart;
	NextOfficerNumber = 1;
	// Every campaign its own officer corps (the seed); the historical generals vary a point here and there.
	FRandomStream Rng(Seed);
	for (FCampaign1851Officer& G : Officers)
	{
		for (uint8& S : G.Stats)
		{
			if (Rng.FRand() < Deviation)
			{
				S = uint8(FMath::Clamp(int32(S) + (Rng.FRand() < 0.5f ? -1 : 1), 1, 10));
			}
		}
	}
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		FCampaign1851Officer Chief = MakeOfficer(Rng, false, ChiefRank(Regiments[i].Arm));
		++NextOfficerNumber;
		Chief.Regiment = i;
		Regiments[i].Chief = Officers.Add(Chief);
		Regiments[i].General = INDEX_NONE;
	}
	// A captain for every company of the battalions (drawn after the chiefs, so they keep their names).
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		Regiments[i].Captains.Init(INDEX_NONE, Campaign1851Army::CompaniesFor(Regiments[i].Arm));
		Regiments[i].CompanyFort.Init(0, Regiments[i].Captains.Num());
		for (int32 k = 0; k < Regiments[i].Captains.Num(); ++k)
		{
			FCampaign1851Officer Captain = MakeOfficer(Rng, false, TEXT("Kaptajn"));
			++NextOfficerNumber;
			Captain.CaptainOf = i;
			Captain.Company = k;
			Regiments[i].Captains[k] = Officers.Add(Captain);
		}
	}
	const TCHAR* ReserveRanks[] = { TEXT("Major"), TEXT("Major"), TEXT("Kaptajn"), TEXT("Oberstløjtnant") };
	for (int32 r = 0; r < ReserveOfficers; ++r)
	{
		Officers.Add(MakeOfficer(Rng, false, ReserveRanks[r % UE_ARRAY_COUNT(ReserveRanks)]));
		++NextOfficerNumber;
	}
	// The general commands and their commanding generals (from the data file).
	Commands = CommandsAtStart;
	for (int32 c = 0; c < Commands.Num(); ++c)
	{
		Commands[c].General = INDEX_NONE;
		const int32 G = Officers.IndexOfByPredicate([&](const FCampaign1851Officer& O) { return O.bGeneral && CommandGeneralIds.IsValidIndex(c) && O.Id == CommandGeneralIds[c]; });
		if (G != INDEX_NONE)
		{
			AssignCommandGeneral(G, c);
		}
	}
}

bool ACampaign1851Map::AssignCommandGeneral(int32 Officer, int32 Command)
{
	if (!Officers.IsValidIndex(Officer) || !Commands.IsValidIndex(Command) || !Officers[Officer].bGeneral)
	{
		return false;
	}
	FCampaign1851Officer& O = Officers[Officer];
	if (Officers.IsValidIndex(Commands[Command].General) && Commands[Command].General != Officer)
	{
		Officers[Commands[Command].General].Command = INDEX_NONE;   // the old commander goes to the pool
	}
	if (Commands.IsValidIndex(O.Command) && O.Command != Command)
	{
		Commands[O.Command].General = INDEX_NONE;
	}
	if (Regiments.IsValidIndex(O.Regiment))
	{
		Regiments[O.Regiment].General = INDEX_NONE;
		O.Regiment = INDEX_NONE;
	}
	O.Command = Command;
	Commands[Command].General = Officer;
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|officers|%s %s commands %s"), *O.Rank, *O.Name, *Commands[Command].Name);
	return true;
}

FString ACampaign1851Map::PromotionBlock(int32 Officer) const
{
	if (!Officers.IsValidIndex(Officer))
	{
		return TEXT("-");
	}
	const FCampaign1851Officer& O = Officers[Officer];
	const int32 Next = Campaign1851Army::RankIndex(O.Rank) + 1;
	if (Next >= Campaign1851Army::Ranks().Num())
	{
		return TEXT("højeste rang");
	}
	const float Need = Campaign1851Army::RankExperience(Next);
	return O.Experience < Need ? FString::Printf(TEXT("%s kræver erfaring %.0f"), *Campaign1851Army::Ranks()[Next], Need) : FString();
}

bool ACampaign1851Map::PromoteOfficer(int32 Officer)
{
	if (!PromotionBlock(Officer).IsEmpty())
	{
		return false;
	}
	FCampaign1851Officer& O = Officers[Officer];
	const int32 Next = Campaign1851Army::RankIndex(O.Rank) + 1;
	O.Rank = Campaign1851Army::Ranks()[Next];
	if (!O.bGeneral && Next >= Campaign1851Army::FirstGeneralRank)
	{
		// A new general leaves his regiment: it needs a new chief, he a general's post.
		if (Regiments.IsValidIndex(O.Regiment))
		{
			Regiments[O.Regiment].Chief = INDEX_NONE;
		}
		O.Regiment = INDEX_NONE;
		O.bGeneral = true;
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|officers|%s promoted to %s"), *O.Name, *O.Rank);
	return true;
}

bool ACampaign1851Map::OrderTroopTrain(int32 Station)
{
	if (Treasury < TroopTrainCost)
	{
		return false;
	}
	if (!Cities.IsValidIndex(Station))
	{
		Station = FindCity(TEXT("København"));
	}
	AddTransaction(-TroopTrainCost, FString::Printf(TEXT("Troppetog bestilt til %s (lokomotiv og vogne)"), Cities.IsValidIndex(Station) ? *Cities[Station].Name : TEXT("?")));
	// X: the station it is landed at, plus one (older saves have 1: the first town, Copenhagen).
	TrainOrders.Add(FVector2D(double(FMath::Max(Station, 0) + 1), CampaignDays + TroopTrainDeliveryDays));
	return true;
}

TArray<ACampaign1851Map::FRailNet> ACampaign1851Map::RailNets() const
{
	// The open railways as networks of towns that hang together (Altona-Kiel with Rendsburg and Glückstadt,
	// Copenhagen-Roskilde, ...). A train can only run on the network it stands on.
	TArray<FRailNet> Nets;
	TArray<int32> NetOf;
	NetOf.Init(INDEX_NONE, Cities.Num());
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		const bool bStation = Links.ContainsByPredicate([c](const FCampaign1851Link& L) { return L.bRailway && (L.A == c || L.B == c); });
		if (!bStation || NetOf[c] != INDEX_NONE)
		{
			continue;
		}
		FRailNet& Net = Nets.AddDefaulted_GetRef();
		const int32 Me = Nets.Num() - 1;
		TArray<int32> Open = { c };
		NetOf[c] = Me;
		while (Open.Num() > 0)
		{
			const int32 At = Open.Pop();
			Net.Stations.Add(At);
			for (const FCampaign1851Link& L : Links)
			{
				if (L.bRailway && (L.A == At || L.B == At))
				{
					const int32 Other = L.A == At ? L.B : L.A;
					if (Cities.IsValidIndex(Other) && NetOf[Other] == INDEX_NONE)
					{
						NetOf[Other] = Me;
						Open.Add(Other);
					}
				}
			}
		}
		// Named after its two largest towns; new trains are landed at its largest port.
		TArray<int32> ByPop = Net.Stations;
		ByPop.Sort([this](int32 A, int32 B) { return Cities[A].Population > Cities[B].Population; });
		Net.Name = ByPop.Num() > 1 ? FString::Printf(TEXT("%s–%s"), *Cities[ByPop[0]].Name, *Cities[ByPop[1]].Name) : Cities[ByPop[0]].Name;
		for (int32 s : ByPop)
		{
			if (IsCoastalTown(s)) { Net.Depot = s; break; }
		}
		Net.Depot = Net.Depot == INDEX_NONE ? ByPop[0] : Net.Depot;
	}
	return Nets;
}

float ACampaign1851Map::TrainTransferDays(int32 Train, int32 Station) const
{
	// Loaded on a ship or on carts: ten days for the work, a day for every 40 km between the stations.
	if (!TroopTrainList.IsValidIndex(Train) || !Cities.IsValidIndex(Station) || !Cities.IsValidIndex(TroopTrainList[Train].Station))
	{
		return 0.f;
	}
	return 10.f + float(FVector2D::Distance(TownKm(TroopTrainList[Train].Station), TownKm(Station))) / 40.f;
}

bool ACampaign1851Map::TransferTrain(int32 Train, int32 Station, FString* OutReason)
{
	auto Fail = [OutReason](const TCHAR* Why) { if (OutReason) { *OutReason = Why; } return false; };
	if (!TroopTrainList.IsValidIndex(Train) || !Cities.IsValidIndex(Station)) return Fail(TEXT("-"));
	FCampaign1851TroopTrain& T = TroopTrainList[Train];
	if (!T.IsFree() || !Cities.IsValidIndex(T.Station)) return Fail(TEXT("toget er i brug"));
	if (T.Station == Station) return Fail(TEXT("toget står der allerede"));
	if (Treasury < TrainTransferCost) return Fail(TEXT("ikke råd"));
	const float Days = TrainTransferDays(Train, Station);
	AddTransaction(-TrainTransferCost, FString::Printf(TEXT("Tog %d flyttes til %s"), T.Id, *Cities[Station].Name));
	T.TransferTo = Station;
	T.TransferDays = Days;
	T.Station = INDEX_NONE;
	News.Add(FString::Printf(TEXT("Tog %d skibes til %s (%s)"), T.Id, *Cities[Station].Name, *FormatDuration(Days)));
	UpdateTroopTrainPieces();
	return true;
}

bool ACampaign1851Map::RemoveFormationCommander(int32 Formation)
{
	const int32 Index = FormationIndex(Formation);
	if (Index == INDEX_NONE || !Officers.IsValidIndex(Formations[Index].Commander))
	{
		return false;
	}
	VacateOfficer(Formations[Index].Commander);
	return true;
}

TArray<int32> ACampaign1851Map::OfficerPool(bool bGenerals) const
{
	TArray<int32> Out;
	for (int32 i = 0; i < Officers.Num(); ++i)
	{
		if (Officers[i].bGeneral == bGenerals && Officers[i].IsFree())
		{
			Out.Add(i);
		}
	}
	// Best first: generals by staff and leadership, chiefs by leadership and experience.
	Out.Sort([this, bGenerals](int32 A, int32 B)
	{
		auto Score = [this, bGenerals](const FCampaign1851Officer& O)
		{
			return O.Stat(ECampaign1851OfficerStat::Leadership) * 2 + O.Stat(bGenerals ? ECampaign1851OfficerStat::Staff : ECampaign1851OfficerStat::Tactical) + O.Experience / 20.f;
		};
		return Score(Officers[A]) > Score(Officers[B]);
	});
	return Out;
}

bool ACampaign1851Map::AssignOfficer(int32 Officer, int32 Regiment)
{
	if (!Officers.IsValidIndex(Officer) || !Regiments.IsValidIndex(Regiment))
	{
		return false;
	}
	FCampaign1851Officer& O = Officers[Officer];
	FCampaign1851Regiment& R = Regiments[Regiment];
	int32& Post = O.bGeneral ? R.General : R.Chief;
	if (Post == Officer)
	{
		return true;
	}
	// The one leaving the post goes to the pool; the new one leaves his old post.
	if (Officers.IsValidIndex(Post))
	{
		Officers[Post].Regiment = INDEX_NONE;
	}
	if (Regiments.IsValidIndex(O.Regiment))
	{
		(O.bGeneral ? Regiments[O.Regiment].General : Regiments[O.Regiment].Chief) = INDEX_NONE;
	}
	LeaveCompany(O, Regiments);
	LeaveStaffPost(Officer);
	Post = Officer;
	O.Regiment = Regiment;
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|officers|%s %s -> %s"), *O.Rank, *O.Name, *R.Name);
	return true;
}

int32 ACampaign1851Map::RecruitOfficer(bool bGeneral)
{
	const int32 Cost = OfficerCost(bGeneral);
	if (Treasury < Cost)
	{
		return INDEX_NONE;
	}
	FRandomStream Rng{ int32(FPlatformTime::Cycles()) };
	FCampaign1851Officer O = MakeOfficer(Rng, bGeneral, bGeneral ? TEXT("Generalmajor") : TEXT("Major"));
	O.Id = FString::Printf(TEXT("R%d"), NextOfficerNumber++);
	O.bRecruited = true;
	AddTransaction(-Cost, FString::Printf(TEXT("%s: %s %s"), bGeneral ? TEXT("Udnævnelse") : TEXT("Officer ansat"), *O.Rank, *O.Name));
	return Officers.Add(O);
}

bool ACampaign1851Map::DismissOfficer(int32 Officer)
{
	if (!Officers.IsValidIndex(Officer) || !Officers[Officer].IsFree())
	{
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|officers|dismissed %s %s"), *Officers[Officer].Rank, *Officers[Officer].Name);
	Officers.RemoveAt(Officer);
	// Posts point at officer indices: those after the removed one move down.
	for (FCampaign1851Regiment& R : Regiments)
	{
		for (int32* Post : { &R.Chief, &R.General })
		{
			if (*Post > Officer)
			{
				--*Post;
			}
		}
		for (int32& Post : R.Captains)
		{
			Post -= Post > Officer ? 1 : 0;
		}
	}
	for (FCampaign1851Command& C : Commands)
	{
		if (C.General > Officer)
		{
			--C.General;
		}
	}
	for (FCampaign1851Formation& F : Formations)
	{
		if (F.Commander > Officer)
		{
			--F.Commander;
		}
		F.Deputy -= F.Deputy > Officer ? 1 : 0;
		F.StaffChief -= F.StaffChief > Officer ? 1 : 0;
	}
	return true;
}

const FCampaign1851Officer* ACampaign1851Map::ColumnGeneral(const TArray<int32>& Column) const
{
	for (int32 i : Column)
	{
		if (Regiments.IsValidIndex(i) && Officers.IsValidIndex(Regiments[i].General))
		{
			return &Officers[Regiments[i].General];
		}
	}
	return nullptr;
}

double ACampaign1851Map::OfficerPayPerMonth() const
{
	double Year = 0.0;
	for (const FCampaign1851Officer& O : Officers)
	{
		Year += Campaign1851Army::RankPay(Campaign1851Army::RankIndex(O.Rank));
	}
	return Year / 12.0;
}

TArray<FCampaign1851OfficerSave> ACampaign1851Map::SaveOfficers() const
{
	TArray<FCampaign1851OfficerSave> Out;
	for (const FCampaign1851Officer& O : Officers)
	{
		FCampaign1851OfficerSave& S = Out.AddDefaulted_GetRef();
		S.Id = O.Id;
		S.Name = O.Name;
		S.Rank = O.Rank;
		S.Born = O.Born;
		S.bGeneral = O.bGeneral;
		S.bRecruited = O.bRecruited;
		S.Stats = TArray<uint8>(O.Stats, NumStats);
		S.Experience = O.Experience;
		S.Regiment = Regiments.IsValidIndex(O.Regiment) ? Regiments[O.Regiment].Id : FString();
		S.Command = Commands.IsValidIndex(O.Command) ? Commands[O.Command].Id : FString();
		S.CaptainOf = Regiments.IsValidIndex(O.CaptainOf) ? Regiments[O.CaptainOf].Id : FString();
		S.Company = O.Company;
		S.Away = O.Away;
		S.AwayUntil = O.Away != 0 ? O.AwayUntil.ToIso8601() : FString();
	}
	return Out;
}

void ACampaign1851Map::RestoreOfficers(const TArray<FCampaign1851OfficerSave>& Saves)
{
	if (Saves.Num() == 0)
	{
		return;   // an older save: keep the 1851 officer corps
	}
	Officers.Reset();
	for (FCampaign1851Regiment& R : Regiments)
	{
		R.Chief = R.General = INDEX_NONE;
		R.Captains.Init(INDEX_NONE, R.SavedCompanies >= 0 ? R.SavedCompanies : Campaign1851Army::CompaniesFor(R.Arm));
		R.CompanyFort.Init(0, R.Captains.Num());
		if (R.Captains.Num() > 0 && R.CompanyWeight.Num() != R.Captains.Num()) { R.CompanyWeight.Reset(); }
	}
	for (FCampaign1851Command& C : Commands)
	{
		C.General = INDEX_NONE;
	}
	for (const FCampaign1851OfficerSave& S : Saves)
	{
		FCampaign1851Officer O;
		O.Id = S.Id;
		O.Name = S.Name;
		O.Rank = S.Rank;
		O.Born = S.Born;
		O.bGeneral = S.bGeneral;
		O.bRecruited = S.bRecruited;
		for (int32 s = 0; s < NumStats && s < S.Stats.Num(); ++s)
		{
			O.Stats[s] = S.Stats[s];
		}
		O.Experience = S.Experience;
		O.Away = S.Away;
		if (S.Away != 0) { FDateTime::ParseIso8601(*S.AwayUntil, O.AwayUntil); }
		const int32 Index = Officers.Add(O);
		const int32 Regiment = FindRegiment(S.Regiment);
		if (Regiment != INDEX_NONE)
		{
			AssignOfficer(Index, Regiment);
		}
		const int32 Command = Commands.IndexOfByPredicate([&S](const FCampaign1851Command& C) { return C.Id == S.Command; });
		if (Command != INDEX_NONE)
		{
			AssignCommandGeneral(Index, Command);
		}
		const int32 CaptainOf = FindRegiment(S.CaptainOf);
		if (CaptainOf != INDEX_NONE && Regiments[CaptainOf].Captains.IsValidIndex(S.Company))
		{
			Regiments[CaptainOf].Captains[S.Company] = Index;
			Officers[Index].CaptainOf = CaptainOf;
			Officers[Index].Company = S.Company;
		}
		if (S.Id.StartsWith(TEXT("R")) || S.Id.StartsWith(TEXT("O")))
		{
			NextOfficerNumber = FMath::Max(NextOfficerNumber, FCString::Atoi(*S.Id.RightChop(1)) + 1);
		}
	}
}

int32 ACampaign1851Map::CompanyNumber(int32 Regiment, int32 Company) const
{
	// Numbered through the regiment: the second battalion of a regiment has companies 5-8.
	if (!Regiments.IsValidIndex(Regiment))
	{
		return Company + 1;
	}
	const FCampaign1851Regiment& R = Regiments[Regiment];
	const int32 Index = FormationIndex(R.Formation);
	int32 Before = 0;
	if (Index != INDEX_NONE && Formations[Index].Echelon == ECampaign1851Echelon::Regiment)
	{
		for (int32 i = 0; i < Regiment; ++i)
		{
			Before += Regiments[i].Formation == R.Formation ? Regiments[i].Captains.Num() : 0;
		}
	}
	return Before + Company + 1;
}

int32 ACampaign1851Map::CompanyMen(int32 Regiment, int32 Company) const
{
	if (!Regiments.IsValidIndex(Regiment))
	{
		return 0;
	}
	// A company of a battalion or a squadron of a cavalry regiment.
	const FCampaign1851Regiment& R = Regiments[Regiment];
	const int32 Parts = SubUnitCount(Regiment);
	if (Parts <= 0 || Company < 0 || Company >= Parts)
	{
		return 0;
	}
	auto InFort = [&](int32 k) { return R.Captains.Num() > 0 && R.CompanyFort.IsValidIndex(k) && R.CompanyFort[k] != 0; };
	// A company in a fort has its own men there; the battalion's men spread over the companies with it.
	if (InFort(Company))
	{
		const int32 FortIdx = FortIndex(R.CompanyFort[Company]);
		const FCampaign1851FortCompany* C = FortIdx == INDEX_NONE ? nullptr
			: Forts[FortIdx].Companies.FindByPredicate([&](const FCampaign1851FortCompany& X) { return X.Regiment == Regiment && X.Company == Company; });
		return C ? C->Men : 0;
	}
	int32 With = 0, Index = 0;
	for (int32 k = 0; k < Parts; ++k)
	{
		if (!InFort(k))
		{
			Index += k < Company ? 1 : 0;
			++With;
		}
	}
	With = FMath::Max(With, 1);
	// Spread by the weights (equal when none are set): the shares add up to exactly the men there are.
	const bool bWeights = R.CompanyWeight.Num() == Parts;
	auto Weight = [&](int32 k) { return bWeights ? FMath::Max(R.CompanyWeight[k], 0.f) : 1.f; };
	double All = 0.0, Before = 0.0;
	for (int32 k = 0; k < Parts; ++k)
	{
		if (!InFort(k))
		{
			All += Weight(k);
			Before += k < Company ? Weight(k) : 0.f;
		}
	}
	if (All <= 0.0)
	{
		return R.Men / With + (Index < R.Men % With ? 1 : 0);
	}
	const double Mine = Weight(Company);
	return int32(FMath::RoundToDouble(R.Men * (Before + Mine) / All) - FMath::RoundToDouble(R.Men * Before / All));
}

void ACampaign1851Map::FreezeCompanyStrength(int32 Regiment)
{
	if (!Regiments.IsValidIndex(Regiment) || SubUnitCount(Regiment) <= 0)
	{
		return;
	}
	TArray<float> W;
	for (int32 k = 0; k < SubUnitCount(Regiment); ++k)
	{
		W.Add(float(CompanyMen(Regiment, k)));
	}
	Regiments[Regiment].CompanyWeight = W;
}

bool ACampaign1851Map::BalanceCompanies(int32 Regiment, int32 A, int32 B, FString* OutWhy)
{
	auto Fail = [OutWhy](const FString& Why) { if (OutWhy) { *OutWhy = Why; } return false; };
	if (!Regiments.IsValidIndex(Regiment) || A == B || A < 0 || B < 0 || A >= Regiments[Regiment].Captains.Num() || B >= Regiments[Regiment].Captains.Num())
	{
		return Fail(TEXT("Ingen kompagnier"));
	}
	FCampaign1851Regiment& R = Regiments[Regiment];
	if (IsInBattle(Regiment))
	{
		return Fail(TEXT("Ikke midt i et slag"));
	}
	if ((R.CompanyFort.IsValidIndex(A) && R.CompanyFort[A] != 0) || (R.CompanyFort.IsValidIndex(B) && R.CompanyFort[B] != 0))
	{
		return Fail(TEXT("Et kompagni i en skanse kan ikke få mænd herfra"));
	}
	FreezeCompanyStrength(Regiment);
	const int32 Ma = CompanyMen(Regiment, A), Mb = CompanyMen(Regiment, B), Total = Ma + Mb;
	const int32 Cap = FMath::Max(1, R.MaxMen / R.Captains.Num());
	const int32 High = FMath::Min((Total + 1) / 2, Cap), Low = Total - High;
	const int32 NewA = Ma >= Mb ? High : Low, NewB = Total - NewA;
	if (NewA == Ma || Low > Cap)
	{
		return Fail(TEXT("Kompagnierne er allerede jævne"));
	}
	R.CompanyWeight[A] = float(NewA);
	R.CompanyWeight[B] = float(NewB);
	if (OutWhy) { *OutWhy = FString::Printf(TEXT("Kompagnierne er udjævnet: %d og %d mand"), NewA, NewB); }
	return true;
}

int32 ACampaign1851Map::CompanyCapacity(int32 Regiment) const
{
	return Regiments.IsValidIndex(Regiment) && SubUnitCount(Regiment) > 0 ? FMath::Max(1, Regiments[Regiment].MaxMen / SubUnitCount(Regiment)) : 0;
}

bool ACampaign1851Map::TransferCompanyMen(int32 FromReg, int32 From, int32 ToReg, int32 To, int32 Count, FString* OutWhy)
{
	auto Fail = [OutWhy](const FString& Why) { if (OutWhy) { *OutWhy = Why; } return false; };
	if (!Regiments.IsValidIndex(FromReg) || !Regiments.IsValidIndex(ToReg) || (FromReg == ToReg && From == To)
		|| From < 0 || To < 0 || From >= SubUnitCount(FromReg) || To >= SubUnitCount(ToReg))
	{
		return Fail(TEXT("Ingen kompagnier"));
	}
	FCampaign1851Regiment& F = Regiments[FromReg];
	FCampaign1851Regiment& T = Regiments[ToReg];
	if (F.Arm != T.Arm || (F.Captains.Num() > 0) != (T.Captains.Num() > 0))
	{
		return Fail(TEXT("Mændene kan kun flyttes mellem enheder af samme slags"));
	}
	if (IsInBattle(FromReg) || IsInBattle(ToReg))
	{
		return Fail(TEXT("Ikke midt i et slag"));
	}
	if (FromReg != ToReg)
	{
		const bool bTogether = !F.IsMarching() && !T.IsMarching() && ((F.Town != INDEX_NONE && F.Town == T.Town) || FVector2D::Distance(F.Km, T.Km) < 2.0);
		if (!bTogether)
		{
			return Fail(TEXT("De to enheder skal stå samme sted (ikke på march)"));
		}
	}
	auto InFort = [](const FCampaign1851Regiment& R, int32 k) { return R.Captains.Num() > 0 && R.CompanyFort.IsValidIndex(k) && R.CompanyFort[k] != 0; };
	if (InFort(F, From) || InFort(T, To))
	{
		return Fail(TEXT("Et kompagni i en skanse kan ikke få mænd herfra"));
	}
	FreezeCompanyStrength(FromReg);
	FreezeCompanyStrength(ToReg);
	const int32 N = FMath::Min3(Count, CompanyMen(FromReg, From), CompanyCapacity(ToReg) - CompanyMen(ToReg, To));
	if (N <= 0)
	{
		return Fail(TEXT("Ingen mænd at flytte"));
	}
	if (FromReg != ToReg)
	{
		// The men bring their training, morale and supplies with them: the receiving unit's figures are mixed by men.
		const float WT = float(FMath::Max(T.Men, 1)), WN = float(N);
		auto Mix = [WT, WN](float X, float Y) { return (X * WT + Y * WN) / (WT + WN); };
		T.Experience = Mix(T.Experience, F.Experience);
		for (int32 s = 0; s < int32(ECampaign1851Skill::Count); ++s) { T.Skills[s] = Mix(T.Skills[s], F.Skills[s]); }
		for (int32 d = 0; d < 4; ++d) { T.FireDrills[d] = Mix(T.FireDrills[d], F.FireDrills[d]); }
		T.Morale = Mix(T.Morale, F.Morale);
		T.Present = Mix(T.Present, F.Present);
		T.Food = Mix(T.Food, F.Food);
		T.Fodder = Mix(T.Fodder, F.Fodder);
		T.Ammo = Mix(T.Ammo, F.Ammo);
		F.Men -= N;
		T.Men += N;
	}
	F.CompanyWeight[From] -= float(N);
	T.CompanyWeight[To] += float(N);
	const bool bCompanies = F.Captains.Num() > 0;
	if (FromReg != ToReg)
	{
		UpdateRegimentPiece(FromReg);
		UpdateRegimentPiece(ToReg);
	}
	if (OutWhy)
	{
		*OutWhy = FString::Printf(TEXT("%d mand flyttet: %s har nu %d, %s har %d"), N,
			*FString::Printf(TEXT("%d. %s"), bCompanies ? CompanyNumber(FromReg, From) : From + 1, bCompanies ? TEXT("kompagni") : TEXT("eskadron")), CompanyMen(FromReg, From),
			*FString::Printf(TEXT("%d. %s"), bCompanies ? CompanyNumber(ToReg, To) : To + 1, bCompanies ? TEXT("kompagni") : TEXT("eskadron")), CompanyMen(ToReg, To));
	}
	return true;
}

bool ACampaign1851Map::EqualizeCompanies(int32 Regiment, FString* OutWhy)
{
	auto Fail = [OutWhy](const FString& Why) { if (OutWhy) { *OutWhy = Why; } return false; };
	if (!Regiments.IsValidIndex(Regiment) || SubUnitCount(Regiment) < 2)
	{
		return Fail(TEXT("Enheden har ikke flere kompagnier"));
	}
	if (IsInBattle(Regiment))
	{
		return Fail(TEXT("Ikke midt i et slag"));
	}
	Regiments[Regiment].CompanyWeight.Reset();
	if (OutWhy) { *OutWhy = FString::Printf(TEXT("%s: mændene er fordelt ligeligt over kompagnierne"), *Regiments[Regiment].Name); }
	return true;
}

FString ACampaign1851Map::OfficerRole(int32 Officer) const
{
	if (!Officers.IsValidIndex(Officer))
	{
		return FString();
	}
	const FCampaign1851Officer& O = Officers[Officer];
	if (O.Away != 0)
	{
		return O.Away == 2 ? FString::Printf(TEXT("Krigsfange (udveksles ca. %d.%d.)"), O.AwayUntil.GetDay(), O.AwayUntil.GetMonth())
			: FString::Printf(TEXT("Såret (tilbage ca. %d.%d.)"), O.AwayUntil.GetDay(), O.AwayUntil.GetMonth());
	}
	const int32 Index = FormationIndex(O.Formation);
	if (Index != INDEX_NONE)
	{
		return FString::Printf(TEXT("%s, %s"), Campaign1851Army::FormationRole(Formations[Index].Echelon), *Formations[Index].Name);
	}
	if (Commands.IsValidIndex(O.Command))
	{
		return FString::Printf(TEXT("Kommanderende general, %s"), *Commands[O.Command].Name);
	}
	if (Regiments.IsValidIndex(O.Regiment))
	{
		return O.bGeneral ? FString::Printf(TEXT("General ved %s"), *Regiments[O.Regiment].Name)
			: FString::Printf(TEXT("%s, %s"), Campaign1851Army::UnitRole(Regiments[O.Regiment].Arm), *Regiments[O.Regiment].Name);
	}
	const int32 StaffIndex = FormationIndex(O.StaffOf);
	if (StaffIndex != INDEX_NONE)
	{
		const bool bActing = O.StaffPost == 1 && !Officers.IsValidIndex(Formations[StaffIndex].Commander);
		return FString::Printf(TEXT("%s%s, %s"), Campaign1851Army::StaffPostName(Formations[StaffIndex].Echelon, O.StaffPost), bActing ? TEXT(" (fungerende chef)") : TEXT(""), *Formations[StaffIndex].Name);
	}
	if (Regiments.IsValidIndex(O.CaptainOf))
	{
		return FString::Printf(TEXT("Kompagnichef, %d. Kompagni (%s)%s"), CompanyNumber(O.CaptainOf, O.Company), *Regiments[O.CaptainOf].Name,
			SeniorCaptain(O.CaptainOf) == Officer ? TEXT(", næstkommanderende") : TEXT(""));
	}
	return TEXT("ledig");
}

void ACampaign1851Map::ApplyOfficerCasualties(const TArray<TSharedPtr<FJsonValue>>& Ours, const TArray<TSharedPtr<FJsonValue>>& Theirs)
{
	FRandomStream Rng(int32(HashCombine(uint32(Seed), uint32(FMath::FloorToInt(CampaignDays) * 131 + Ours.Num()))));
	// The enemy officers we took (each shortens the exchange of ours).
	int32 Taken = 0;
	FString First;
	for (const TSharedPtr<FJsonValue>& V : Theirs)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (O.IsValid() && O->GetStringField(TEXT("fate")) == TEXT("captured"))
		{
			if (Taken == 0) { First = FString::Printf(TEXT("%s %s"), *O->GetStringField(TEXT("rank")), *O->GetStringField(TEXT("name"))); }
			++Taken;
		}
	}
	if (Taken > 0)
	{
		// Prestige: a colonel is worth more than a captain (the mood at home rises a little).
		float Points = 0.f;
		for (const TSharedPtr<FJsonValue>& V : Theirs)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			if (O.IsValid() && O->GetStringField(TEXT("fate")) == TEXT("captured"))
			{
				const FString Rank = O->GetStringField(TEXT("rank"));
				Points += Rank == TEXT("Oberst") ? 3.f : Rank == TEXT("Major") ? 2.f : 1.f;
			}
		}
		PoliticalShock(FMath::Min(4.f, 0.3f * Points), 0.f);
		EnemyOfficersHeld += Taken;
		News.Add(FString::Printf(TEXT("%d fjendtlige officerer er taget til fange (bl.a. %s)"), Taken, *First));
	}
	for (const TSharedPtr<FJsonValue>& V : Ours)
	{
		const TSharedPtr<FJsonObject> J = V->AsObject();
		if (!J.IsValid())
		{
			continue;
		}
		const FString Id = J->GetStringField(TEXT("id"));
		const bool bCaptured = J->GetStringField(TEXT("fate")) == TEXT("captured");
		const int32 Officer = Officers.IndexOfByPredicate([&Id](const FCampaign1851Officer& O) { return O.Id == Id; });
		if (Officer == INDEX_NONE)
		{
			continue;
		}
		VacateOfficer(Officer);
		FCampaign1851Officer& O = Officers[Officer];
		O.Away = bCaptured ? 2 : 1;
		if (bCaptured)
		{
			// An officer lost to the enemy is a blow at home (more for a senior one).
			PoliticalShock(-0.3f * float(1 + Campaign1851Army::RankIndex(O.Rank) / 2), 0.f);
		}
		// Wounded: three to twelve weeks. A prisoner: exchanged after ten weeks, sooner when we hold enemy officers too.
		const int32 Days = bCaptured ? FMath::Max(21, 75 - 20 * EnemyOfficersHeld) : Rng.RandRange(21, 84);
		O.AwayUntil = GetDate() + FTimespan::FromDays(Days);
		News.Add(bCaptured ? FString::Printf(TEXT("%s %s er taget til fange af fjenden"), *O.Rank, *O.Name)
			: FString::Printf(TEXT("%s %s er såret og afgår fra sin post (tilbage om ca. %d dage)"), *O.Rank, *O.Name, Days));
	}
}

void ACampaign1851Map::DailyOfficers()
{
	const FDateTime Now = GetDate();
	for (FCampaign1851Officer& O : Officers)
	{
		if (O.Away != 0 && Now >= O.AwayUntil)
		{
			News.Add(O.Away == 2 ? FString::Printf(TEXT("%s %s er udvekslet og kommer hjem"), *O.Rank, *O.Name) : FString::Printf(TEXT("%s %s er rask og kan få en post igen"), *O.Rank, *O.Name));
			if (O.Away == 2) { EnemyOfficersHeld = FMath::Max(0, EnemyOfficersHeld - 1); }
			O.Away = 0;
		}
	}
}

int32 ACampaign1851Map::RansomCost(int32 Officer) const
{
	static const int32 Cost[] = { 600, 1500, 2200, 3500, 8000, 12000, 20000 };
	return Officers.IsValidIndex(Officer) ? Cost[FMath::Clamp(Campaign1851Army::RankIndex(Officers[Officer].Rank), 0, int32(UE_ARRAY_COUNT(Cost)) - 1)] : 0;
}

bool ACampaign1851Map::RansomOfficer(int32 Officer, FString* OutWhy)
{
	auto Fail = [OutWhy](const FString& Why) { if (OutWhy) { *OutWhy = Why; } return false; };
	if (!Officers.IsValidIndex(Officer) || Officers[Officer].Away != 2)
	{
		return Fail(TEXT("Han er ikke krigsfange"));
	}
	const int32 Cost = RansomCost(Officer);
	if (Treasury < Cost)
	{
		return Fail(FString::Printf(TEXT("Statskassen kan ikke betale løsesummen (%d rd.)"), Cost));
	}
	FCampaign1851Officer& O = Officers[Officer];
	AddTransaction(-double(Cost), FString::Printf(TEXT("Løsesum for %s %s"), *O.Rank, *O.Name));
	O.Away = 0;
	EnemyOfficersHeld = FMath::Max(0, EnemyOfficersHeld - 1);
	News.Add(FString::Printf(TEXT("%s %s er løskøbt for %d rd. og er hjemme igen"), *O.Rank, *O.Name, Cost));
	if (OutWhy) { *OutWhy = FString::Printf(TEXT("%s er løskøbt for %d rd."), *O.Name, Cost); }
	return true;
}
