// Officers of the 1851 campaign: regimental chiefs and generals (design manual 8), ACampaign1851Map's officer corps.

#include "Campaign1851Map.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const TCHAR* StatKeys[] = { TEXT("leadership"), TEXT("inspiration"), TEXT("initiative"), TEXT("tactical"), TEXT("staff"), TEXT("discipline"), TEXT("aggression"), TEXT("composure"), TEXT("political") };
	constexpr int32 NumStats = int32(ECampaign1851OfficerStat::Count);
	/** Officers in reserve at the start, beside one chief per regiment. */
	constexpr int32 ReserveOfficers = 4;

	/** A regiment's chief by its arm: colonels for the Guard and cavalry regiments, lieutenant-colonels for battalions, captains for batteries. */
	const TCHAR* ChiefRank(ECampaign1851Arm Arm)
	{
		switch (Arm)
		{
		case ECampaign1851Arm::Guard:
		case ECampaign1851Arm::Cavalry: return TEXT("Oberst");
		case ECampaign1851Arm::Artillery:
		case ECampaign1851Arm::HorseArtillery: return TEXT("Kaptajn");
		default: return TEXT("Oberstløjtnant");
		}
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
	FRandomStream Rng(1851);
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		FCampaign1851Officer Chief = MakeOfficer(Rng, false, ChiefRank(Regiments[i].Arm));
		++NextOfficerNumber;
		Chief.Regiment = i;
		Regiments[i].Chief = Officers.Add(Chief);
		Regiments[i].General = INDEX_NONE;
	}
	const TCHAR* ReserveRanks[] = { TEXT("Major"), TEXT("Major"), TEXT("Kaptajn"), TEXT("Oberstløjtnant") };
	for (int32 r = 0; r < ReserveOfficers; ++r)
	{
		Officers.Add(MakeOfficer(Rng, false, ReserveRanks[r % UE_ARRAY_COUNT(ReserveRanks)]));
		++NextOfficerNumber;
	}
}

TArray<int32> ACampaign1851Map::OfficerPool(bool bGenerals) const
{
	TArray<int32> Out;
	for (int32 i = 0; i < Officers.Num(); ++i)
	{
		if (Officers[i].bGeneral == bGenerals && Officers[i].Regiment == INDEX_NONE)
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
		Year += O.bGeneral ? GeneralPay : OfficerPay;
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
		const int32 Index = Officers.Add(O);
		const int32 Regiment = FindRegiment(S.Regiment);
		if (Regiment != INDEX_NONE)
		{
			AssignOfficer(Index, Regiment);
		}
		if (S.Id.StartsWith(TEXT("R")))
		{
			NextOfficerNumber = FMath::Max(NextOfficerNumber, FCString::Atoi(*S.Id.RightChop(1)) + 1);
		}
	}
}
