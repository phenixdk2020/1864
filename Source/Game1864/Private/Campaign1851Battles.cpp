// Battles of the 1851 campaign (Docs/Battle1851.md, backlog item 9): when an enemy corps meets Danish
// troops or forts, the game pauses and offers the battle: to be fought in the 3D battle game (a request
// file out, a result file back), resolved at once, or avoided by retreat. The outcome costs men,
// ammunition, morale, forts and towns.

#include "Campaign1851Map.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	FString BattleDir() { return FPaths::ProjectSavedDir() / TEXT("Battle"); }

	/** The enemy's fighting quality: the Prussian needle gun loads lying down, three times as fast. */
	float EnemyQuality(const FString& Nation, int32 WeaponYear) { return Nation == TEXT("PR") ? (WeaponYear >= 1841 ? 1.3f : 1.f) : Nation == TEXT("AT") ? 1.05f : 0.9f; }
	const TCHAR* EnemyRifle(const FString& Nation, int32 WeaponYear)
	{
		if (WeaponYear < 1841) { return TEXT("Glatløbet flintlåsgevær"); }
		return Nation == TEXT("PR") ? TEXT("Dreyse tændnålsgevær (bagladeriffel)") : Nation == TEXT("AT") && WeaponYear >= 1854 ? TEXT("Lorenz-riffel (forladeriffel)") : TEXT("glatløbet forladergevær");
	}
	constexpr float GunWorth = 60.f;   // a gun in the balance, in men
}

void ACampaign1851Map::CreateBattle(int32 CorpsIndex)
{
	FCampaign1851EnemyCorps& C = EnemyCorps[CorpsIndex];
	FCampaign1851Battle B;
	B.Id = NextBattleId++;
	B.Day = CampaignDays;
	B.Km = C.Km;
	B.Town = NearestTown(C.Km);
	B.CorpsId = C.Id;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		if (Regiments[i].Men > 0 && FVector2D::Distance(Regiments[i].Km, C.Km) < 8.0)
		{
			B.Regiments.Add(i);
		}
	}
	for (const FCampaign1851Fort& F : Forts)
	{
		if (F.bBuilt && FVector2D::Distance(F.Km, C.Km) < 8.0)
		{
			B.Forts.Add(F.Id);
		}
	}
	Battles.Add(B);
	SetSpeed(0);   // the player decides
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|battle|%d at %s: %d units, %d forts vs %s"), B.Id, Cities.IsValidIndex(B.Town) ? *Cities[B.Town].Name : TEXT("?"), B.Regiments.Num(), B.Forts.Num(), *C.Name);
}

int32 ACampaign1851Map::EngageableCorps(const TArray<int32>& Units, double* OutKm) const
{
	int32 Best = INDEX_NONE;
	double BestKm = EngageKm;
	for (int32 c = 0; c < EnemyCorps.Num(); ++c)
	{
		const FCampaign1851EnemyCorps& C = EnemyCorps[c];
		if (C.Men <= 0 || !C.bSeen || C.bEngaged)
		{
			continue;
		}
		for (int32 i : Units)
		{
			if (Regiments.IsValidIndex(i) && Regiments[i].Men > 0
				&& (!C.bSieging || !Cities.IsValidIndex(C.SiegeTown) || FVector2D::Distance(Regiments[i].Km, TownKm(C.SiegeTown)) > 10.0))
			{
				const double Km = FVector2D::Distance(Regiments[i].Km, C.Km);
				if (Km <= BestKm)
				{
					BestKm = Km;
					Best = c;
				}
			}
		}
	}
	if (OutKm) { *OutKm = BestKm; }
	return Best;
}

bool ACampaign1851Map::EngageCorps(int32 CorpsIndex, const TArray<int32>& Units, FString* OutWhy)
{
	double Km = 0.0;
	if (EngageableCorps(Units, &Km) != CorpsIndex || !EnemyCorps.IsValidIndex(CorpsIndex))
	{
		if (OutWhy) { *OutWhy = FString::Printf(TEXT("Intet opklaret fjendtligt korps inden for %.0f km"), EngageKm); }
		return false;
	}
	CreateBattle(CorpsIndex);
	EnemyCorps[CorpsIndex].bEngaged = true;
	// The attacking units fight even if they stood further off than the usual contact.
	FCampaign1851Battle& B = Battles.Last();
	for (int32 i : Units)
	{
		if (Regiments.IsValidIndex(i) && Regiments[i].Men > 0)
		{
			B.Regiments.AddUnique(i);
		}
	}
	News.Add(FString::Printf(TEXT("Angreb på %s (%.0f km)"), *EnemyCorps[CorpsIndex].Name, Km));
	return true;
}

int32 ACampaign1851Map::CorpsIndexOf(const FCampaign1851Battle& B) const
{
	return EnemyCorps.IndexOfByPredicate([&B](const FCampaign1851EnemyCorps& C) { return C.Id == B.CorpsId; });
}

void ACampaign1851Map::BattleStrengths(const FCampaign1851Battle& B, float& OutDanish, float& OutEnemy) const
{
	OutDanish = 0.f;
	const float DoctrineMul = DanishQualityFactor(B);
	for (int32 i : B.Regiments)
	{
		if (!Regiments.IsValidIndex(i))
		{
			continue;
		}
		const FCampaign1851Regiment& R = Regiments[i];
		const Campaign1851Army::FBattleFactors F = ArmyBattleFactors(R);
		const float Quality = (F.Accuracy + 1.f / FMath::Max(F.ReloadTime, 0.5f) + F.Assault) / 3.f * (0.6f + 0.4f * F.Morale) * (0.7f + 0.3f * F.Cohesion);
		const float Supply = FMath::Clamp(0.4f + 0.6f * R.Ammo, 0.4f, 1.f) * (R.Food > 0.f ? 1.f : 0.8f);
		const bool bInfantryWeapon = R.Arm == ECampaign1851Arm::Infantry || (ActiveScenario().Id == TEXT("1825") && (R.Arm == ECampaign1851Arm::Guard || R.Arm == ECampaign1851Arm::Jager));
		OutDanish += R.PresentMen() * Quality * Supply * DoctrineMul * (bInfantryWeapon ? InfantryFactor() : 1.f) + R.Guns * GunWorth * (UnitWeaponLevel(R) == 1 ? 1.4f : 1.f) * (R.WeaponConversionDays > 0.f ? 0.25f : 1.f) + R.Mortars * GunWorth * 0.8f;
	}
	for (int32 Id : B.Forts)
	{
		const int32 Fi = FortIndex(Id);
		if (Fi == INDEX_NONE)
		{
			continue;
		}
		const FCampaign1851Fort& F = Forts[Fi];
		int32 Inside = 0, Reserve = 0;
		FortMen(F, Inside, Reserve);
		// Behind a parapet a man counts for more; the guns need their powder.
		OutDanish += (Inside * (1.f + 2.f * (Campaign1851Forts::CoverPercent(F.Defence) + FortCoverBonus()) / 100.f) + Reserve * (1.f + Campaign1851Forts::ReserveCover(F.bTrenches) / 100.f)) * DoctrineMul
			+ F.Guns * GunWorth * DanishGunFactor() * FMath::Clamp(F.RoundsPerGun / 120.f, 0.2f, 1.f);
	}
	const int32 Ci = CorpsIndexOf(B);
	OutEnemy = Ci != INDEX_NONE ? EnemyCorps[Ci].Men * EnemyQuality(EnemyCorps[Ci].Nation, ActiveScenario().Id == TEXT("1825") ? GetDate().GetYear() : 1864) + EnemyCorps[Ci].Guns * GunWorth : 0.f;
	if (Ci != INDEX_NONE)
	{
		const FCampaign1851NeighbourArmy* NeighbourDefender = NeighbourArmies.FindByPredicate([&](const FCampaign1851NeighbourArmy& A) { return A.CorpsId == EnemyCorps[Ci].Id && A.bGarrison; });
		if (NeighbourDefender && Cities.IsValidIndex(NeighbourDefender->Town) && Cities[NeighbourDefender->Town].bNeighbourFortress
			&& FVector2D::Distance(EnemyCorps[Ci].Km, TownKm(NeighbourDefender->Town)) < 5.0)
		{
			OutEnemy *= 1.3f; // estimated protection of a neighbouring permanent fortress
		}
	}
}

float ACampaign1851Map::BattleOdds(const FCampaign1851Battle& B) const
{
	float D = 0.f, E = 0.f;
	BattleStrengths(B, D, E);
	const float R = E > 0.f ? D / E : 10.f;
	return R * R / (1.f + R * R);
}

bool ACampaign1851Map::FightBattleIn3D(int32 BattleId)
{
	// The request: where, when, who. The units and forts in full are in Units.json and Fortifications.json.
	FCampaign1851Battle* B = Battles.FindByPredicate([BattleId](const FCampaign1851Battle& X) { return X.Id == BattleId; });
	const int32 Ci = B ? CorpsIndexOf(*B) : INDEX_NONE;
	if (!B || Ci == INDEX_NONE)
	{
		return false;
	}
	ExportUnits();
	ExportForts();
	const FCampaign1851EnemyCorps& C = EnemyCorps[Ci];
	TSharedRef<FJsonObject> Doc = MakeShared<FJsonObject>();
	const FVector2D LatLon = Extent.Projection.Inverse(B->Km);
	const FDateTime Now = GetDate();
	Doc->SetStringField(TEXT("format"), TEXT("PROJECT1864-BattleRequest-1"));
	Doc->SetNumberField(TEXT("battleId"), B->Id);
	Doc->SetNumberField(TEXT("battleSeed"), int32(HashCombine(uint32(Seed), uint32(B->Id * 7919))));
	Doc->SetStringField(TEXT("date"), Now.ToIso8601());
	Doc->SetStringField(TEXT("season"), GetSeasonName());
	WriteDoctrineJson(Doc);
	Doc->SetStringField(TEXT("weather"), Campaign1851Weather::Name(GetWeather()));
	Doc->SetNumberField(TEXT("temperatureC"), FMath::RoundToInt(GetTemperature()));
	Doc->SetBoolField(TEXT("ice"), IsIceWinter());
	Doc->SetBoolField(TEXT("snow"), Now.GetMonth() == 12 || Now.GetMonth() <= 2);
	Doc->SetNumberField(TEXT("lat"), LatLon.X);
	Doc->SetNumberField(TEXT("lon"), LatLon.Y);
	Doc->SetStringField(TEXT("nearTown"), Cities.IsValidIndex(B->Town) ? Cities[B->Town].Name : FString());
	TArray<TSharedPtr<FJsonValue>> Units, FortIds;
	for (int32 i : B->Regiments)
	{
		Units.Add(MakeShared<FJsonValueString>(Regiments[i].Id));
	}
	for (int32 Id : B->Forts)
	{
		FortIds.Add(MakeShared<FJsonValueNumber>(Id));
	}
	Doc->SetArrayField(TEXT("danishUnitIds"), Units);
	{
		// The reserves: the nearest Danish regiments not in any battle, standing still within 60 km (at most three); they come on in the morning of the second and third day.
		TArray<TPair<float, int32>> Near;
		for (int32 i = 0; i < Regiments.Num(); ++i)
		{
			const FCampaign1851Regiment& R = Regiments[i];
			bool bBusy = false;
			for (const FCampaign1851Battle& Other : Battles) { bBusy |= Other.Regiments.Contains(i); }
			const float Distance = float(FVector2D::Distance(R.Km, B->Km));
			if (!bBusy && !R.IsMarching() && R.PresentMen() >= 300 && Distance <= 60.f && !R.bDetached)
			{
				Near.Add(TPair<float, int32>(Distance, i));
			}
		}
		Near.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& C) { return A.Key < C.Key; });
		TArray<TSharedPtr<FJsonValue>> Reserve;
		for (int32 k = 0; k < Near.Num() && k < 3; ++k) { Reserve.Add(MakeShared<FJsonValueString>(Regiments[Near[k].Value].Id)); }
		Doc->SetArrayField(TEXT("reserveUnitIds"), Reserve);
	}
	Doc->SetArrayField(TEXT("fortIds"), FortIds);
	Doc->SetStringField(TEXT("unitsFile"), TEXT("Units.json"));
	Doc->SetStringField(TEXT("fortsFile"), TEXT("Fortifications.json"));
	TSharedRef<FJsonObject> Enemy = MakeShared<FJsonObject>();
	Enemy->SetStringField(TEXT("name"), C.Name);
	Enemy->SetStringField(TEXT("nation"), C.Nation);
	Enemy->SetNumberField(TEXT("men"), C.Men);
	Enemy->SetNumberField(TEXT("guns"), C.Guns);
	Enemy->SetStringField(TEXT("rifle"), ActiveScenario().Id == TEXT("1825") ? EnemyRifle(C.Nation, GetDate().GetYear()) : C.Nation == TEXT("PR") ? TEXT("Dreyse tændnålsgevær (bagladeriffel)") : C.Nation == TEXT("AT") ? TEXT("Lorenz-riffel (forladeriffel)") : TEXT("forladeriffel"));
	Enemy->SetNumberField(TEXT("quality"), EnemyQuality(C.Nation, ActiveScenario().Id == TEXT("1825") ? GetDate().GetYear() : 1864));
	// Where the enemy comes from (degrees north of east, the battlefield's axes): from his corps; from the south
	// (the border) when he stands on the battle's own ground.
	const FVector2D Away = C.Km - B->Km;
	Enemy->SetNumberField(TEXT("bearingDeg"), Away.Size() > 0.3 ? FMath::RadiansToDegrees(FMath::Atan2(Away.Y, Away.X)) : -90.0);
	Doc->SetObjectField(TEXT("enemy"), Enemy);
	Doc->SetStringField(TEXT("resultFile"), FString::Printf(TEXT("BattleResult_%d.json"), B->Id));
	// The ground of the battle (the generator, 8 km around it).
	GenerateBattlefield(B->Km, 8.f, FString::Printf(TEXT("Battle_%d"), B->Id));
	Doc->SetStringField(TEXT("battlefieldFile"), BattlefieldFile());
	FString Text;
	FJsonSerializer::Serialize(Doc, TJsonWriterFactory<>::Create(&Text));
	IFileManager::Get().MakeDirectory(*BattleDir(), true);
	FFileHelper::SaveStringToFile(Text, *(BattleDir() / FString::Printf(TEXT("BattleRequest_%d.json"), B->Id)), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	B->bWaiting = true;
	News.Add(FString::Printf(TEXT("Slaget sendt til 3D: BattleRequest_%d.json (venter på BattleResult_%d.json)"), B->Id, B->Id));
	return true;
}

void ACampaign1851Map::PollBattleResults()
{
	for (int32 b = Battles.Num() - 1; b >= 0; --b)
	{
		if (!Battles[b].bWaiting)
		{
			continue;
		}
		const FString Path = BattleDir() / FString::Printf(TEXT("BattleResult_%d.json"), Battles[b].Id);
		FString Text;
		TSharedPtr<FJsonObject> Json;
		if (!FFileHelper::LoadFileToString(Text, *Path)) { continue; }
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|battle|Ugyldig JSON i %s; slaget venter fortsat"), *Path);
			continue;
		}
		// The 3D battle's own numbers: each unit's and fort's losses, the ammunition used, the outcome.
		FCampaign1851BattleOutcome O;
		FString BattleFormat, Outcome;
		if (!Json->TryGetStringField(TEXT("format"), BattleFormat) || BattleFormat != TEXT("PROJECT1864-BattleResult-1") ||
			!Json->TryGetStringField(TEXT("outcome"), Outcome) ||
			(Outcome != TEXT("danish_victory") && Outcome != TEXT("enemy_victory") && Outcome != TEXT("draw")))
		{
			UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|battle|Ugyldigt format eller udfald i %s; slaget venter fortsat"), *Path);
			continue;
		}
		O.bDanishWin = Outcome == TEXT("danish_victory");
		O.bDraw = Outcome == TEXT("draw");
		double EnemyLosses = 0.0;
		Json->TryGetNumberField(TEXT("enemyLosses"), EnemyLosses);
		if (!FMath::IsFinite(EnemyLosses) || EnemyLosses < 0.0 || EnemyLosses > MAX_int32)
		{
			UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|battle|Ugyldigt tabstal i %s; slaget venter fortsat"), *Path);
			continue;
		}
		O.EnemyLosses = int32(EnemyLosses);
		const TArray<TSharedPtr<FJsonValue>>* Units = nullptr;
		if (Json->TryGetArrayField(TEXT("units"), Units))
		{
			for (const TSharedPtr<FJsonValue>& V : *Units)
			{
				const TSharedPtr<FJsonObject> U = V.IsValid() && V->Type == EJson::Object ? V->AsObject() : nullptr;
				FString ResultUnitId;
				if (!U.IsValid() || !U->TryGetStringField(TEXT("id"), ResultUnitId))
				{
					UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|battle|Ugyldig enhed i %s ignoreret"), *Path);
					continue;
				}
				const int32 R = FindRegiment(ResultUnitId);
				if (R != INDEX_NONE)
				{
					double Losses = 0.0, Ammo = 0.0, Kills = 0.0;
					const bool bValidUnitNumbers =
						(!U->HasField(TEXT("losses")) || U->TryGetNumberField(TEXT("losses"), Losses)) &&
						(!U->HasField(TEXT("ammoUsed")) || U->TryGetNumberField(TEXT("ammoUsed"), Ammo)) &&
						(!U->HasField(TEXT("kills")) || U->TryGetNumberField(TEXT("kills"), Kills));
					if (!bValidUnitNumbers || !FMath::IsFinite(Losses) || !FMath::IsFinite(Ammo) || !FMath::IsFinite(Kills) ||
						Losses < 0.0 || Losses > MAX_int32 || Kills < 0.0 || Kills > MAX_int32)
					{
						UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|battle|Ugyldige enhedstal i %s ignoreret"), *Path);
						continue;
					}
					O.UnitLosses.Add(R, int32(Losses));
					O.UnitKills.Add(R, int32(Kills));
					O.UnitAmmo.Add(R, float(Ammo));
				}
			}
		}
		const TArray<TSharedPtr<FJsonValue>>* FortResults = nullptr;
		if (Json->TryGetArrayField(TEXT("forts"), FortResults))
		{
			for (const TSharedPtr<FJsonValue>& V : *FortResults)
			{
				const TSharedPtr<FJsonObject> F = V.IsValid() && V->Type == EJson::Object ? V->AsObject() : nullptr;
				double ResultFortId = 0.0;
				if (!F.IsValid() || !F->TryGetNumberField(TEXT("id"), ResultFortId) ||
					!FMath::IsFinite(ResultFortId) || ResultFortId < 0.0 || ResultFortId > MAX_int32 || ResultFortId != double(int32(ResultFortId)))
				{
					UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|battle|Ugyldigt fort i %s ignoreret"), *Path);
					continue;
				}
				bool bCaptured = false;
				F->TryGetBoolField(TEXT("captured"), bCaptured);
				if (bCaptured)
				{
					O.CapturedForts.Add(int32(ResultFortId));
				}
			}
		}
		O.bFromBattle3D = true;
		ApplyBattle(b, O);
		{
			const TArray<TSharedPtr<FJsonValue>>* OurOfficers = nullptr;
			const TArray<TSharedPtr<FJsonValue>>* EnemyOfficers = nullptr;
			const TArray<TSharedPtr<FJsonValue>> None;
			Json->TryGetArrayField(TEXT("officers"), OurOfficers);
			Json->TryGetArrayField(TEXT("enemyOfficers"), EnemyOfficers);
			ApplyOfficerCasualties(OurOfficers ? *OurOfficers : None, EnemyOfficers ? *EnemyOfficers : None);
		}
		IFileManager::Get().Move(*(Path + TEXT(".read")), *Path);
	}
}

void ACampaign1851Map::AutoResolveBattle(int32 BattleId)
{
	const int32 b = Battles.IndexOfByPredicate([BattleId](const FCampaign1851Battle& X) { return X.Id == BattleId; });
	if (b == INDEX_NONE)
	{
		return;
	}
	const FCampaign1851Battle& B = Battles[b];
	FRandomStream Rng(int32(HashCombine(uint32(Seed), uint32(B.Id * 7919))));
	float D = 0.f, E = 0.f;
	BattleStrengths(B, D, E);
	const float Odds = BattleOdds(B);
	FCampaign1851BattleOutcome O;
	const float Roll = Rng.FRand();
	O.bDanishWin = Roll < Odds - 0.05f;
	O.bDraw = !O.bDanishWin && Roll < Odds + 0.05f;
	// Losses: the loser a fifth or so of those engaged, the winner a tenth; a draw both in between.
	// Overwhelmed (odds under 1 in 10): many are taken prisoner.
	// Sanitation and the doctrine lessen them.
	const float DanishShare = DanishLossFactor() * (O.bDanishWin ? Rng.FRandRange(0.05f, 0.1f) : O.bDraw ? Rng.FRandRange(0.08f, 0.14f)
		: Odds < 0.1f ? Rng.FRandRange(0.35f, 0.55f) : Rng.FRandRange(0.15f, 0.25f));
	const float EnemyShare = O.bDanishWin ? Rng.FRandRange(0.15f, 0.25f) : O.bDraw ? Rng.FRandRange(0.08f, 0.14f) : Rng.FRandRange(0.04f, 0.09f);
	for (int32 i : B.Regiments)
	{
		if (Regiments.IsValidIndex(i))
		{
			O.UnitLosses.Add(i, FMath::RoundToInt(Regiments[i].PresentMen() * DanishShare));
			O.UnitAmmo.Add(i, Rng.FRandRange(0.3f, 0.7f));
		}
	}
	for (int32 Id : B.Forts)
	{
		if (!O.bDanishWin && !O.bDraw)
		{
			O.CapturedForts.Add(Id);
		}
		O.FortLossShare.Add(Id, DanishShare * (O.bDanishWin ? 0.6f : 1.f));
	}
	const int32 Ci = CorpsIndexOf(B);
	// The enemy loses what the Danes can inflict: never more than a share of their own fighting strength.
	O.EnemyLosses = Ci != INDEX_NONE ? FMath::RoundToInt(FMath::Min(EnemyCorps[Ci].Men * EnemyShare, D * Rng.FRandRange(0.25f, 0.45f))) : 0;
	// The enemy's losses shared by the units after their strength (for their service records).
	int32 Engaged = 0;
	for (int32 i : B.Regiments) { Engaged += Regiments.IsValidIndex(i) ? Regiments[i].PresentMen() : 0; }
	for (int32 i : B.Regiments)
	{
		if (Regiments.IsValidIndex(i) && Engaged > 0)
		{
			O.UnitKills.Add(i, FMath::RoundToInt(float(O.EnemyLosses) * Regiments[i].PresentMen() / Engaged));
		}
	}
	ApplyBattle(b, O);
}

void ACampaign1851Map::RetreatFromBattle(int32 BattleId)
{
	const int32 b = Battles.IndexOfByPredicate([BattleId](const FCampaign1851Battle& X) { return X.Id == BattleId; });
	if (b == INDEX_NONE)
	{
		return;
	}
	// No battle: the Danes fall back north, the forts are given up with their guns.
	FCampaign1851BattleOutcome O;
	O.bRetreat = true;
	for (int32 Id : Battles[b].Forts)
	{
		O.CapturedForts.Add(Id);
	}
	ApplyBattle(b, O);
}

void ACampaign1851Map::ApplyBattle(int32 BattleIndex, const FCampaign1851BattleOutcome& O)
{
	FCampaign1851Battle B = Battles[BattleIndex];
	Battles.RemoveAt(BattleIndex);
	const int32 Ci = CorpsIndexOf(B);
	const FString Place = Cities.IsValidIndex(B.Town) ? Cities[B.Town].Name : FString(TEXT("?"));
	int32 DanishLosses = 0, Prisoners = 0;
	for (const TPair<int32, int32>& L : O.UnitLosses)
	{
		if (!Regiments.IsValidIndex(L.Key))
		{
			continue;
		}
		FCampaign1851Regiment& R = Regiments[L.Key];
		const int32 Lost = FMath::Clamp(L.Value, 0, R.Men);
		R.Men -= Lost;
		const int32 PrisonersBefore = Prisoners, SickBefore = R.Sick;
		SplitLosses(L.Key, Lost, !O.bDanishWin && !O.bDraw, Prisoners);
		// The service record: fallen, wounded, taken, and what it did to the enemy.
		FCampaign1851ServiceEntry Entry;
		Entry.Day = CampaignDays;
		Entry.Place = Place;
		Entry.Result = O.bRetreat ? 3 : O.bDanishWin ? 2 : O.bDraw ? 1 : 0;
		Entry.Wounded = Regiments[L.Key].Sick - SickBefore;
		Entry.Captured = Prisoners - PrisonersBefore;
		Entry.Killed = FMath::Max(0, Lost - Entry.Wounded - Entry.Captured);
		Entry.EnemyKilled = O.UnitKills.FindRef(L.Key);
		Entry.bFrom3D = O.bFromBattle3D;
		FCampaign1851Regiment& Rec = Regiments[L.Key];
		Rec.Service.Add(Entry);
		Rec.TotalKilled += Entry.Killed;
		Rec.TotalWounded += Entry.Wounded;
		Rec.TotalCaptured += Entry.Captured;
		Rec.TotalEnemyKilled += Entry.EnemyKilled;
		DanishLosses += Lost;
		R.Morale = FMath::Clamp(R.Morale + (O.bDanishWin ? 0.05f : O.bDraw ? -0.05f : -0.15f), 0.05f, 1.f);
		R.Cohesion = FMath::Max(10.f, R.Cohesion - (O.bDanishWin ? 5.f : 20.f));
		R.Experience = FMath::Min(100.f, R.Experience + 5.f);
		for (const int32 o : { R.Chief, R.General })
		{
			if (Officers.IsValidIndex(o))
			{
				Officers[o].Experience = FMath::Min(100.f, Officers[o].Experience + (O.bDanishWin ? 6.f : 3.f));
			}
		}
	}
	// The country follows the war: victories cheer, defeats are blamed on the Eider policy.
	PoliticalShock(O.bDanishWin ? 3.f : O.bDraw ? -1.f : -3.f, O.bDanishWin ? 2.f : O.bDraw ? 0.f : -2.f);
	for (const TPair<int32, float>& A : O.UnitAmmo)
	{
		if (Regiments.IsValidIndex(A.Key) && FMath::IsFinite(A.Value))
		{
			Regiments[A.Key].Ammo = FMath::Clamp(Regiments[A.Key].Ammo - FMath::Clamp(A.Value, 0.f, 1.f), 0.f, 1.f);
		}
	}
	// Forts: losses among their companies; the captured ones are lost with their guns.
	for (int32 Id : B.Forts)
	{
		const int32 Fi = FortIndex(Id);
		if (Fi == INDEX_NONE)
		{
			continue;
		}
		const float Share = O.FortLossShare.Contains(Id) ? O.FortLossShare[Id] : 0.f;
		for (FCampaign1851FortCompany& C : Forts[Fi].Companies)
		{
			const int32 Lost = FMath::RoundToInt(C.Men * Share);
			C.Men -= Lost;
			DanishLosses += Lost;
		}
		Forts[Fi].RoundsPerGun = FMath::Max(0.f, Forts[Fi].RoundsPerGun - 60.f);
		Forts[Fi].CartridgesPerMan = FMath::Max(0.f, Forts[Fi].CartridgesPerMan - 40.f);
	}
	DanishWarLosses += DanishLosses;
	for (int32 Id : O.CapturedForts)
	{
		const int32 Fi = FortIndex(Id);
		if (Fi != INDEX_NONE)
		{
			// The companies that got away go back to their battalions; the guns are lost.
			News.Add(FString::Printf(TEXT("%s er taget af fjenden"), *Forts[Fi].Name));
			Forts[Fi].Guns = 0;
			FinishFortDemolition(Fi);
		}
	}
	if (Ci != INDEX_NONE)
	{
		FCampaign1851EnemyCorps& C = EnemyCorps[Ci];
		C.Men = FMath::Max(0, C.Men - O.EnemyLosses);
		C.bSieging = false;
		C.SiegeTown = INDEX_NONE;
		EnemyWarLosses += O.EnemyLosses;
		const int32 Taken = FMath::RoundToInt(O.EnemyLosses * (O.bDanishWin ? 0.2f : 0.05f));
		EnemyCaptured += Taken;
		EnemyCapturedTotal += Taken;
		// Of the rest about one in four fell, the others were wounded (the losses of 1848-50 and 1864).
		const int32 Hit = FMath::Max(0, O.EnemyLosses - Taken);
		const int32 Fell = FMath::RoundToInt(Hit * 0.27f);
		EnemyKilled += Fell;
		EnemyWounded += Hit - Fell;
		// The side holding the field gathers what the other left on it: rifles, guns, horses, wagons, colours.
		if (O.bDanishWin || O.bDraw)
		{
			const float Field = O.bDanishWin ? 1.f : 0.15f;
			const int32 Rifles_ = FMath::RoundToInt(O.EnemyLosses * 0.6f * Field);
			const float Broken = FMath::Clamp(float(O.EnemyLosses) / float(FMath::Max(1, C.Men + O.EnemyLosses)), 0.f, 1.f);
			const int32 Guns_ = O.bDanishWin ? FMath::Min(C.Guns, FMath::RoundToInt(C.Guns * FMath::Min(0.3f, Broken * 0.8f))) : 0;
			const int32 Horses_ = FMath::RoundToInt(O.EnemyLosses * 0.04f * Field);
			const int32 Wagons_ = O.bDanishWin ? O.EnemyLosses / 400 : 0;
			const int32 Colours_ = O.bDanishWin && O.EnemyLosses >= 1500 ? 1 : 0;
			C.Guns -= Guns_;
			Rifles += Rifles_;
			GunStock += Guns_;
			Horses += Horses_;
			WagonStock += Wagons_;
			CapturedRifles += Rifles_;
			CapturedGuns += Guns_;
			CapturedHorses += Horses_;
			CapturedWagons += Wagons_;
			CapturedColours += Colours_;
			if (Rifles_ + Guns_ + Horses_ > 0)
			{
				News.Add(FString::Printf(TEXT("Bytte ved %s: %d geværer, %d kanoner, %d heste%s%s"), *Place, Rifles_, Guns_, Horses_,
					Wagons_ > 0 ? *FString::Printf(TEXT(", %d vogne"), Wagons_) : TEXT(""), Colours_ > 0 ? TEXT(" og en fjendtlig fane") : TEXT("")));
			}
		}
		C.bEngaged = false;
		if (O.bDanishWin)
		{
			// Beaten: it falls back and waits for reinforcements (or breaks up if too weak).
			C.RestUntil = CampaignDays + 20.0;
			C.Route.Reset();
			if (C.Men < 4000)
			{
				News.Add(FString::Printf(TEXT("%s er slået i opløsning"), *C.Name));
				EnemyCorps.RemoveAt(Ci);
			}
		}
		else
		{
			C.RestUntil = CampaignDays + 3.0;
		}
	}
	// A defeat or a retreat: the Danes fall back north to the nearest town of their own.
	if (!O.bDanishWin && !O.bDraw)
	{
		int32 Refuge = INDEX_NONE;
		double BestKm = 1e9;
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			const double Dist = FVector2D::Distance(TownKm(c), B.Km);
			if (!Cities[c].bForeign && Cities[c].Occupier.IsEmpty() && TownKm(c).Y > B.Km.Y + 10.0 && Dist < BestKm)
			{
				BestKm = Dist;
				Refuge = c;
			}
		}
		if (Refuge != INDEX_NONE)
		{
			TArray<int32> Column;
			for (int32 i : B.Regiments)
			{
				if (Regiments.IsValidIndex(i) && Regiments[i].Men > 0)
				{
					Column.Add(i);
				}
			}
			if (Column.Num() > 0)
			{
				OrderMarchTo(Column, Refuge, TownKm(Refuge), ECampaign1851RouteMode::RoadsOnly);
			}
		}
	}
	const TCHAR* Result = O.bRetreat ? TEXT("tilbagetog uden kamp") : O.bDanishWin ? TEXT("dansk sejr") : O.bDraw ? TEXT("uafgjort") : TEXT("dansk nederlag");
	News.Add(FString::Printf(TEXT("Slaget ved %s: %s  ·  danske tab %d, fjendens tab %d%s"), *Place, Result, DanishLosses, O.EnemyLosses, O.bFromBattle3D ? TEXT("  (fra 3D-slaget)") : TEXT("")));
	FCampaign1851Decision D;
	D.Day = CampaignDays;
	D.Nation = PlayerNation;
	D.Portfolio = ECampaign1851Portfolio::War;
	D.Action = FString::Printf(TEXT("Slaget ved %s: %s"), *Place, Result);
	D.Reasons = FString::Printf(TEXT("danske tab %d  ·  fjendens tab %d"), DanishLosses, O.EnemyLosses);
	D.bDone = true;
	AddDecision(D);
	LogSiegeTest(Result);
	ExportUnits();
	ExportForts();
}
