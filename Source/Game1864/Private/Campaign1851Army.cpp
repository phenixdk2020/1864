// Regiments on the 1851 campaign map: ACampaign1851Map's army (see Campaign1851Army.h).

#include "Campaign1851Army.h"

#include "Campaign1851Map.h"
#include "Algo/Reverse.h"
#include "Campaign1851Scenery.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const TCHAR* ArmyMaterialPath = TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery");
	/** Regiments are drawn this much larger than the town pieces, so a formation reads beside a town. */
	constexpr float FormationScale = 4.f;
	/** Camera distance (km) below which the formations are shown (the counters show at every distance). */
	constexpr float FormationMaxDistanceKm = 60.f;
}

namespace Campaign1851Army
{
	ECampaign1851Arm ParseArm(const FString& Text)
	{
		if (Text == TEXT("guard")) return ECampaign1851Arm::Guard;
		if (Text == TEXT("jager")) return ECampaign1851Arm::Jager;
		if (Text == TEXT("cavalry")) return ECampaign1851Arm::Cavalry;
		if (Text == TEXT("artillery")) return ECampaign1851Arm::Artillery;
		return ECampaign1851Arm::Infantry;
	}

	const TCHAR* ArmName(ECampaign1851Arm Arm)
	{
		switch (Arm)
		{
		case ECampaign1851Arm::Guard: return TEXT("Garde");
		case ECampaign1851Arm::Jager: return TEXT("Jægere");
		case ECampaign1851Arm::Cavalry: return TEXT("Kavaleri");
		case ECampaign1851Arm::Artillery: return TEXT("Artilleri");
		default: return TEXT("Linjeinfanteri");
		}
	}

	float MarchKmPerDay(ECampaign1851Arm Arm)
	{
		// Foot 20 km a day on a dirt road; horse 30; guns and limbers 18.
		return Arm == ECampaign1851Arm::Cavalry ? 30.f : Arm == ECampaign1851Arm::Artillery ? 18.f : Campaign1851Network::MarchKmPerDayRoad;
	}

	float ColumnPace(const TArray<const FCampaign1851Regiment*>& Column, FString* OutWhy)
	{
		float Pace = 1000.f;
		int32 Men = 0;
		const FCampaign1851Regiment* Slowest = nullptr;
		for (const FCampaign1851Regiment* R : Column)
		{
			Men += R->Men;
			if (MarchKmPerDay(R->Arm) < Pace)
			{
				Pace = MarchKmPerDay(R->Arm);
				Slowest = R;
			}
		}
		if (!Slowest)
		{
			return MarchKmPerDay(ECampaign1851Arm::Infantry);
		}
		const float Length = FMath::Clamp(1.f - 0.04f * (Men / 1000.f - 2.f), 0.75f, 1.f);
		if (OutWhy)
		{
			*OutWhy = Column.Num() > 1 ? FString::Printf(TEXT("%s bestemmer tempoet"), *Slowest->Name) : FString();
			if (Length < 0.995f)
			{
				*OutWhy += FString::Printf(TEXT("%slang kolonne (%d mand): -%d %%"), OutWhy->IsEmpty() ? TEXT("") : TEXT("  ·  "),
					Men, FMath::RoundToInt((1.f - Length) * 100.f));
			}
		}
		return Pace * Length;
	}
}

// ------------------------------------------------------------------ data

bool ACampaign1851Map::LoadArmy()
{
	FString Text;
	TSharedPtr<FJsonObject> Json;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Campaign1851/Army1851.json")))
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|army|no Data/Campaign1851/Army1851.json"));
		return false;
	}
	ArmyAtStart.Reset();
	FString Nation = TEXT("DK");
	Json->TryGetStringField(TEXT("nation"), Nation);
	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("regiments")))
	{
		const TSharedPtr<FJsonObject> O = Value->AsObject();
		FCampaign1851Regiment R;
		R.Id = O->GetStringField(TEXT("id"));
		R.Name = O->GetStringField(TEXT("name"));
		R.Nation = Nation;
		R.Arm = Campaign1851Army::ParseArm(O->GetStringField(TEXT("arm")));
		R.Home = FindCity(O->GetStringField(TEXT("home")));
		if (R.Home == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|army|%s: unknown town %s"), *R.Name, *O->GetStringField(TEXT("home")));
			continue;
		}
		R.Men = R.MaxMen = int32(O->GetNumberField(TEXT("men")));
		O->TryGetNumberField(TEXT("horses"), R.Horses);
		O->TryGetNumberField(TEXT("guns"), R.Guns);
		R.Town = R.Home;
		ArmyAtStart.Add(MoveTemp(R));
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|army|%d regiments"), ArmyAtStart.Num());
	return ArmyAtStart.Num() > 0;
}

void ACampaign1851Map::ResetArmy()
{
	Regiments = ArmyAtStart;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		PlaceInTown(i);
	}
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		UpdateRegimentPiece(i);
	}
}

// ------------------------------------------------------------------ queries

TArray<int32> ACampaign1851Map::RegimentsIn(int32 CityIndex) const
{
	TArray<int32> Out;
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		if (Regiments[i].Town == CityIndex && !Regiments[i].IsMarching())
		{
			Out.Add(i);
		}
	}
	return Out;
}

TArray<FVector2D> ACampaign1851Map::LegLine(const FCampaign1851Leg& Leg) const
{
	if (!Links.IsValidIndex(Leg.Link))
	{
		return {};
	}
	const FCampaign1851Link& L = Links[Leg.Link];
	TArray<FVector2D> Line = Leg.bRail ? L.RailPath : L.Km;
	if (L.A != Leg.From)
	{
		Algo::Reverse(Line);
	}
	return Line;
}

bool ACampaign1851Map::FindRoute(int32 From, int32 To, float Pace, TArray<FCampaign1851Leg>& OutLegs) const
{
	OutLegs.Reset();
	if (!Cities.IsValidIndex(From) || !Cities.IsValidIndex(To) || From == To)
	{
		return false;
	}
	// Dijkstra over the towns; a leg's cost is its time. Boarding a train costs a day of loading.
	auto LegFor = [&](int32 Link, int32 At, bool bArrivedByRail)
	{
		const FCampaign1851Link& L = Links[Link];
		FCampaign1851Leg Leg;
		Leg.Link = Link;
		Leg.From = At;
		Leg.To = L.A == At ? L.B : L.A;
		Leg.bRail = L.bRailway;
		Leg.Days = L.bRailway
			? L.RailKm / Campaign1851Network::TrainKmPerDay + (bArrivedByRail ? 0.f : Campaign1851Network::TrainLoadingDays)
			: L.RoadKm / (Pace * (L.bChaussee ? Campaign1851Network::MarchKmPerDayChaussee / Campaign1851Network::MarchKmPerDayRoad : 1.f))
				+ (L.HasFerry() ? Campaign1851Network::FerryDays : 0.f);
		return Leg;
	};
	TArray<float> Best;
	Best.Init(TNumericLimits<float>::Max(), Cities.Num());
	TArray<FCampaign1851Leg> Via;
	Via.SetNum(Cities.Num());
	TArray<bool> Done;
	Done.Init(false, Cities.Num());
	Best[From] = 0.f;
	for (;;)
	{
		int32 At = INDEX_NONE;
		for (int32 c = 0; c < Cities.Num(); ++c)
		{
			if (!Done[c] && Best[c] < TNumericLimits<float>::Max() && (At == INDEX_NONE || Best[c] < Best[At]))
			{
				At = c;
			}
		}
		if (At == INDEX_NONE || At == To)
		{
			break;
		}
		Done[At] = true;
		const bool bByRail = At != From && Via[At].bRail;
		for (int32 Link = 0; Link < Links.Num(); ++Link)
		{
			if (Links[Link].A != At && Links[Link].B != At)
			{
				continue;
			}
			const FCampaign1851Leg Leg = LegFor(Link, At, bByRail);
			if (Best[At] + Leg.Days < Best[Leg.To])
			{
				Best[Leg.To] = Best[At] + Leg.Days;
				Via[Leg.To] = Leg;
			}
		}
	}
	if (Best[To] == TNumericLimits<float>::Max())
	{
		return false;
	}
	for (int32 At = To; At != From; At = Via[At].From)
	{
		OutLegs.Insert(Via[At], 0);
	}
	return true;
}

FVector ACampaign1851Map::RegimentWorld(int32 Regiment) const
{
	return Regiments.IsValidIndex(Regiment) ? WorldAtKm(Regiments[Regiment].Km) : FVector::ZeroVector;
}

// ------------------------------------------------------------------ orders

bool ACampaign1851Map::OrderMarch(const TArray<int32>& Column, int32 CityIndex, FString* OutReason)
{
	TArray<const FCampaign1851Regiment*> Members;
	for (int32 i : Column)
	{
		if (Regiments.IsValidIndex(i))
		{
			Members.Add(&Regiments[i]);
		}
	}
	if (Members.Num() == 0 || !Cities.IsValidIndex(CityIndex))
	{
		return false;
	}
	if (Cities[CityIndex].bForeign)
	{
		if (OutReason) { *OutReason = TEXT("Hæren går ikke over grænsen i fredstid"); }
		return false;
	}
	// The column marches at its slowest pace. Regiments already on the march finish the stretch
	// under way and go on from its end; the others set out together.
	const float Pace = Campaign1851Army::ColumnPace(Members);
	static int32 NextGroup = 1;
	const int32 Group = Column.Num() > 1 ? NextGroup++ : 0;
	int32 Ordered = 0;
	for (int32 i : Column)
	{
		if (!Regiments.IsValidIndex(i))
		{
			continue;
		}
		FCampaign1851Regiment& R = Regiments[i];
		const bool bMarching = R.IsMarching();
		const int32 Start = bMarching ? R.Route[R.Leg].To : R.Town;
		TArray<FCampaign1851Leg> Legs;
		if (Start != CityIndex && !FindRoute(Start, CityIndex, Pace, Legs))
		{
			if (OutReason) { *OutReason = FString::Printf(TEXT("Ingen vej til %s"), *Cities[CityIndex].Name); }
			continue;
		}
		R.PaceKmPerDay = Pace;
		R.Group = Group;
		if (bMarching)
		{
			TArray<FCampaign1851Leg> Route = { R.Route[R.Leg] };
			Route.Append(Legs);
			R.Route = MoveTemp(Route);
			R.Leg = 0;
		}
		else if (Legs.Num() > 0)
		{
			R.Route = MoveTemp(Legs);
			R.Leg = 0;
			R.LegElapsed = 0.f;
			R.Town = INDEX_NONE;
		}
		++Ordered;
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|army|%s marches to %s|%d legs|%.1f days|pace %.1f km/day"), *R.Name, *Cities[CityIndex].Name, R.Route.Num(), R.DaysLeft(), Pace);
		UpdateRegimentPiece(i);
	}
	return Ordered > 0;
}

void ACampaign1851Map::HaltRegiment(int32 Regiment)
{
	if (Regiments.IsValidIndex(Regiment) && Regiments[Regiment].IsMarching())
	{
		FCampaign1851Regiment& R = Regiments[Regiment];
		R.Route.SetNum(R.Leg + 1);
	}
}

// ------------------------------------------------------------------ time

void ACampaign1851Map::AdvanceArmy(float DeltaDays, float DeltaSeconds)
{
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		FCampaign1851Regiment& R = Regiments[i];
		if (!R.IsMarching())
		{
			continue;
		}
		R.LegElapsed += DeltaDays;
		while (R.IsMarching() && R.LegElapsed >= R.Route[R.Leg].Days)
		{
			R.LegElapsed -= R.Route[R.Leg].Days;
			++R.Leg;
		}
		if (!R.IsMarching())
		{
			// Arrived.
			R.Town = R.Route.Last().To;
			R.Route.Reset();
			R.Group = 0;
			R.Leg = 0;
			R.LegElapsed = 0.f;
			PlaceInTown(i);
			News.Add(FString::Printf(TEXT("%s er ankommet til %s"), *R.Name, *Cities[R.Town].Name));
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|army|%s arrived in %s"), *R.Name, *Cities[R.Town].Name);
		}
		else
		{
			const FCampaign1851Leg& Leg = R.Route[R.Leg];
			const TArray<FVector2D> Line = LegLine(Leg);
			const double Length = LineLength(Line);
			FVector2D Dir;
			R.Km = AlongLine(Line, Length * FMath::Clamp(R.LegElapsed / FMath::Max(Leg.Days, 0.001f), 0.f, 1.f), &Dir);
			R.Heading = Dir;
		}
		UpdateRegimentPiece(i);
	}
}

void ACampaign1851Map::PlaceInTown(int32 Regiment)
{
	FCampaign1851Regiment& R = Regiments[Regiment];
	if (!Cities.IsValidIndex(R.Town))
	{
		return;
	}
	// Around the town on its fields, one place per regiment there, facing out.
	const TArray<int32> Here = RegimentsIn(R.Town);
	const int32 Slot = FMath::Max(0, Here.IndexOfByKey(Regiment));
	const FCampaign1851City& C = Cities[R.Town];
	const FVector2D Centre = Extent.Projection.Forward(C.Lat, C.Lon);
	// Out on the fields past the last houses; the camps go round the town on land, clear of the sea.
	const float Radius = TownRadiusKm(C.Population) * 1.1f + 0.7f;
	int32 Found = 0;
	for (int32 Step = 0; Step < 36; ++Step)
	{
		const float Angle = FMath::DegreesToRadians(200.f + Step * 30.f + (Step / 12) * 15.f);
		const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));
		const FVector2D Spot = Centre + Dir * (Radius + (Step / 12) * 0.8f);
		if (!IsMonarchyLand(Spot) || !IsMonarchyLand(Spot + Dir * 0.4) || !IsMonarchyLand(Spot - Dir * 0.4))
		{
			continue;
		}
		if (Found++ == Slot)
		{
			R.Heading = Dir;
			R.Km = Spot;
			ClearScenery(Spot, 0.3f);
			return;
		}
	}
	R.Heading = FVector2D(-1.0, 0.0);
	R.Km = Centre + R.Heading * Radius;
}

void ACampaign1851Map::UpdateRegimentPiece(int32 Regiment)
{
	if (ArmyMeshes.Num() == 0)
	{
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, ArmyMaterialPath);
		if (!Material || GridZ.Num() == 0)
		{
			return;
		}
		using Campaign1851Scenery::ESitePiece;
		for (ESitePiece Piece : { ESitePiece::FormationInfantry, ESitePiece::FormationGuard, ESitePiece::FormationJager, ESitePiece::FormationCavalry, ESitePiece::FormationArtillery })
		{
			ArmyMeshes.Add(Campaign1851Scenery::BuildSitePiece(Piece, Material));
		}
	}
	RegimentPieces.SetNum(Regiments.Num());
	UStaticMeshComponent* Piece = RegimentPieces[Regiment];
	const FCampaign1851Regiment& R = Regiments[Regiment];
	if (!Piece)
	{
		Piece = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), *FString::Printf(TEXT("Regiment_%s"), *R.Id)));
		Piece->SetupAttachment(Root);
		Piece->SetStaticMesh(ArmyMeshes[FMath::Min(int32(R.Arm), ArmyMeshes.Num() - 1)]);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetCastShadow(false);
		Piece->SetWorldScale3D(FVector(PieceScale * FormationScale));
		Piece->RegisterComponent();
		RegimentPieces[Regiment] = Piece;
	}
	// The formation faces +X: turn it to the heading (world Y is south).
	Piece->SetWorldLocationAndRotation(WorldAtKm(R.Km) + FVector(0.0, 0.0, 0.5),
		FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(-R.Heading.Y, R.Heading.X)) + float(GetActorRotation().Yaw), 0.f));
	Piece->SetVisibility(LastCameraDistanceKm < FormationMaxDistanceKm);
}

// ------------------------------------------------------------------ save

TArray<FCampaign1851RegimentSave> ACampaign1851Map::SaveArmy() const
{
	TArray<FCampaign1851RegimentSave> Out;
	for (const FCampaign1851Regiment& R : Regiments)
	{
		FCampaign1851RegimentSave& S = Out.AddDefaulted_GetRef();
		S.Id = R.Id;
		S.Men = R.Men;
		S.Morale = R.Morale;
		S.Pace = R.PaceKmPerDay;
		S.Group = R.Group;
		if (R.IsMarching())
		{
			S.Town = Cities[R.Route[R.Leg].From].Name;
			S.LegTo = Cities[R.Route[R.Leg].To].Name;
			S.LegElapsed = R.LegElapsed;
			S.Destination = Cities[R.Destination()].Name;
		}
		else if (Cities.IsValidIndex(R.Town))
		{
			S.Town = Cities[R.Town].Name;
		}
	}
	return Out;
}

int32 ACampaign1851Map::RestoreArmy(const TArray<FCampaign1851RegimentSave>& Saves)
{
	int32 Restored = 0;
	for (const FCampaign1851RegimentSave& S : Saves)
	{
		const int32 i = FindRegiment(S.Id);
		const int32 Town = FindCity(S.Town);
		if (i == INDEX_NONE || Town == INDEX_NONE)
		{
			continue;
		}
		FCampaign1851Regiment& R = Regiments[i];
		R.Men = FMath::Clamp(S.Men, 0, R.MaxMen);
		R.Morale = S.Morale;
		R.Town = Town;
		R.Route.Reset();
		R.Leg = 0;
		R.LegElapsed = 0.f;
		const int32 Destination = FindCity(S.Destination);
		R.PaceKmPerDay = S.Pace > 0.f ? S.Pace : Campaign1851Army::MarchKmPerDay(R.Arm);
		R.Group = S.Group;
		if (Destination != INDEX_NONE && FindRoute(Town, Destination, R.PaceKmPerDay, R.Route))
		{
			// The saved stretch first (the route from its start leads through its end when it is the fastest).
			R.Town = INDEX_NONE;
			R.LegElapsed = FMath::Min(S.LegElapsed, R.Route[0].Days);
		}
		++Restored;
	}
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		if (!Regiments[i].IsMarching())
		{
			PlaceInTown(i);
		}
	}
	AdvanceArmy(0.f, 0.f);
	for (int32 i = 0; i < Regiments.Num(); ++i)
	{
		UpdateRegimentPiece(i);
	}
	return Restored;
}
