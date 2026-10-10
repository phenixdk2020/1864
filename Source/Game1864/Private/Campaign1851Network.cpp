// Roads and railways of the 1851 campaign map: ACampaign1851Map's network (see Campaign1851Network.h).

#include "Campaign1851Network.h"

#include "Campaign1851Map.h"

#include "Campaign1851Buildings.h"
#include "Campaign1851Scenery.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* NetworkMaterialPath = TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery");
#ifndef CAMPAIGN1851_SRGB   // one definition per unity blob
#define CAMPAIGN1851_SRGB
	FLinearColor Srgb(uint8 R, uint8 G, uint8 B) { return FLinearColor::FromSRGBColor(FColor(R, G, B)); }
#endif

	/** The line from distance From to To (km along it). */
	TArray<FVector2D> SubLine(const TArray<FVector2D>& Line, double From, double To)
	{
		TArray<FVector2D> Out;
		if (Line.Num() < 2 || To <= From)
		{
			return Out;
		}
		double At = 0.0;
		for (int32 i = 0; i + 1 < Line.Num(); ++i)
		{
			const double Len = FVector2D::Distance(Line[i], Line[i + 1]);
			const double A = At, B = At + Len;
			At = B;
			if (B < From || Len <= 0.0)
			{
				continue;
			}
			if (A > To)
			{
				break;
			}
			if (Out.Num() == 0)
			{
				Out.Add(FMath::Lerp(Line[i], Line[i + 1], FMath::Clamp((From - A) / Len, 0.0, 1.0)));
			}
			Out.Add(FMath::Lerp(Line[i], Line[i + 1], FMath::Clamp((To - A) / Len, 0.0, 1.0)));
		}
		return Out;
	}

	TArray<FVector2D> Resample(const TArray<FVector2D>& Line, double StepKm)
	{
		TArray<FVector2D> Out;
		for (int32 i = 0; i + 1 < Line.Num(); ++i)
		{
			const double Len = FVector2D::Distance(Line[i], Line[i + 1]);
			const int32 Steps = FMath::Max(1, FMath::CeilToInt(Len / StepKm));
			for (int32 s = 0; s < Steps; ++s)
			{
				Out.Add(FMath::Lerp(Line[i], Line[i + 1], double(s) / Steps));
			}
		}
		if (Line.Num() > 0)
		{
			Out.Add(Line.Last());
		}
		return Out;
	}

	/** Dashes along a line: On km drawn, Off km left out (the white of a railway's map symbol). */
	void AddDashes(const TArray<FVector2D>& Line, double On, double Off, TArray<TArray<FVector2D>>& Out)
	{
		const double Length = ACampaign1851Map::LineLength(Line);
		for (double At = 0.0; At < Length; At += On + Off)
		{
			TArray<FVector2D> Dash = Resample(SubLine(Line, At, FMath::Min(At + On, Length)), 0.1);
			if (Dash.Num() >= 2)
			{
				Out.Add(MoveTemp(Dash));
			}
		}
	}

	/** The line moved sideways by OffsetKm (left of the direction of travel for positive values). */
	TArray<FVector2D> OffsetLine(const TArray<FVector2D>& Line, double OffsetKm)
	{
		TArray<FVector2D> Out;
		for (int32 i = 0; i < Line.Num(); ++i)
		{
			const FVector2D Dir = (Line[FMath::Min(i + 1, Line.Num() - 1)] - Line[FMath::Max(i - 1, 0)]).GetSafeNormal();
			Out.Add(Line[i] + FVector2D(-Dir.Y, Dir.X) * OffsetKm);
		}
		return Out;
	}

	TArray<FVector2D> ReadPoints(const TArray<TSharedPtr<FJsonValue>>& Array)
	{
		TArray<FVector2D> Out;
		for (const TSharedPtr<FJsonValue>& Point : Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& XY = Point->AsArray();
			Out.Add(FVector2D(XY[0]->AsNumber(), XY[1]->AsNumber()));
		}
		return Out;
	}

	FDateTime ReadDate(const FJsonObject& O, const TCHAR* Field, const FDateTime& Default)
	{
		FString Text;
		FDateTime Date;
		return O.TryGetStringField(Field, Text) && FDateTime::ParseIso8601(*Text, Date) ? Date : Default;
	}

	/** "½", "1", "1½", ... */
	FString HalfDays(float Days)
	{
		const int32 Halves = FMath::Max(1, FMath::RoundToInt(Days * 2.f));
		return Halves == 1 ? FString(TEXT("½")) : Halves % 2 == 0 ? FString::FromInt(Halves / 2) : FString::Printf(TEXT("%d½"), Halves / 2);
	}

	/** Railway map symbol and works: widths (km), colours and heights over the terrain (units). */
	constexpr float BedWidthKm = 0.16f, TrackWidthKm = 0.075f, WorksWidthKm = 0.2f, ChausseeWidthKm = 0.17f;
	constexpr double DashOnKm = 0.5, DashOffKm = 0.5;
	/** Share of a railway project spent on earthworks before the track follows. */
	constexpr float EarthworksShare = 0.6f;
	/** Visual train speed (km per real second) and the wait at each end. */
	constexpr float TrainKmPerSecond = 0.7f;
	constexpr float TrainPauseSeconds = 4.f;
}

namespace Campaign1851Network
{
	const TCHAR* WorkName(ECampaign1851LinkWork Work)
	{
		return Work == ECampaign1851LinkWork::Chaussee ? TEXT("chaussé") : Work == ECampaign1851LinkWork::Railway ? TEXT("jernbane") : TEXT("");
	}
}

// ------------------------------------------------------------------ geometry helpers

double ACampaign1851Map::LineLength(const TArray<FVector2D>& Line)
{
	double Length = 0.0;
	for (int32 i = 0; i + 1 < Line.Num(); ++i)
	{
		Length += FVector2D::Distance(Line[i], Line[i + 1]);
	}
	return Length;
}

FVector2D ACampaign1851Map::AlongLine(const TArray<FVector2D>& Line, double Distance, FVector2D* OutDirection)
{
	if (Line.Num() == 0)
	{
		return FVector2D::ZeroVector;
	}
	double At = 0.0;
	for (int32 i = 0; i + 1 < Line.Num(); ++i)
	{
		const double Len = FVector2D::Distance(Line[i], Line[i + 1]);
		if (At + Len >= Distance || i + 2 == Line.Num())
		{
			if (OutDirection)
			{
				*OutDirection = (Line[i + 1] - Line[i]).GetSafeNormal();
			}
			return FMath::Lerp(Line[i], Line[i + 1], Len > 0.0 ? FMath::Clamp((Distance - At) / Len, 0.0, 1.0) : 0.0);
		}
		At += Len;
	}
	return Line[0];
}

FVector ACampaign1851Map::WorldAtKm(const FVector2D& Km) const
{
	return GetActorTransform().TransformPosition(LocalAtKm(Km));
}

// ------------------------------------------------------------------ data

bool ACampaign1851Map::LoadNetwork(const FJsonObject& Json)
{
	Links.Reset();
	Railways.Reset();
	const TArray<TSharedPtr<FJsonValue>>* LinkArray = nullptr;
	if (Json.TryGetArrayField(TEXT("links"), LinkArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *LinkArray)
		{
			const TSharedPtr<FJsonObject> O = Value->AsObject();
			FCampaign1851Link L;
			L.A = FindCity(O->GetStringField(TEXT("a")));
			L.B = FindCity(O->GetStringField(TEXT("b")));
			if (L.A == INDEX_NONE || L.B == INDEX_NONE)
			{
				continue;
			}
			double Number = 0.0;
			L.RoadKm = O->TryGetNumberField(TEXT("roadKm"), Number) ? float(Number) : 0.f;
			L.FerryKm = O->TryGetNumberField(TEXT("ferryKm"), Number) ? float(Number) : 0.f;
			O->TryGetStringField(TEXT("ferry"), L.Ferry);
			L.Km = ReadPoints(O->GetArrayField(TEXT("km")));
			const TArray<TSharedPtr<FJsonValue>>* Rail = nullptr;
			if (O->TryGetArrayField(TEXT("rail"), Rail))
			{
				L.RailPath = ReadPoints(*Rail);
				L.RailKm = float(LineLength(L.RailPath));
			}
			O->TryGetBoolField(TEXT("chaussee"), L.bHistoricChaussee);
			L.bChaussee = L.bHistoricChaussee;
			Links.Add(MoveTemp(L));
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* RailArray = nullptr;
	if (ActiveScenario().Id != TEXT("1825") && Json.TryGetArrayField(TEXT("railways"), RailArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *RailArray)
		{
			const TSharedPtr<FJsonObject> O = Value->AsObject();
			FCampaign1851Railway R;
			R.Name = O->GetStringField(TEXT("name"));
			O->TryGetStringArrayField(TEXT("stations"), R.Stations);
			TArray<FString> Towns;
			O->TryGetStringArrayField(TEXT("towns"), Towns);
			for (const FString& Town : Towns)
			{
				const int32 Index = FindCity(Town);
				if (Index != INDEX_NONE)
				{
					R.Towns.Add(Index);
				}
			}
			R.Km = ReadPoints(O->GetArrayField(TEXT("km")));
			R.LengthKm = float(LineLength(R.Km));
			R.Opened = ReadDate(*O, TEXT("opened"), StartDate());
			R.Begun = ReadDate(*O, TEXT("begun"), R.Opened);
			Railways.Add(MoveTemp(R));
		}
	}
	HistoricRailways = Railways.Num();
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|network|links=%d|railways=%d"), Links.Num(), Railways.Num());
	return Links.Num() > 0;
}

// ------------------------------------------------------------------ queries

TArray<int32> ACampaign1851Map::LinksOf(int32 CityIndex) const
{
	TArray<int32> Out;
	for (int32 i = 0; i < Links.Num(); ++i)
	{
		if (Links[i].A == CityIndex || Links[i].B == CityIndex)
		{
			Out.Add(i);
		}
	}
	Out.Sort([this](int32 X, int32 Y) { return Links[X].RoadKm + Links[X].FerryKm < Links[Y].RoadKm + Links[Y].FerryKm; });
	return Out;
}

bool ACampaign1851Map::HasStation(int32 CityIndex) const
{
	const FDateTime Now = GetDate();
	for (const FCampaign1851Railway& R : Railways)
	{
		if (R.IsOpen(Now) && R.Towns.Contains(CityIndex))
		{
			return true;
		}
	}
	return false;
}

FString ACampaign1851Map::LinkBlockReason(int32 Link, ECampaign1851LinkWork Work) const
{
	if (!Links.IsValidIndex(Link))
	{
		return TEXT("-");
	}
	const FCampaign1851Link& L = Links[Link];
	if (Cities[L.A].bForeign || Cities[L.B].bForeign) { return TEXT("Udenlandsk vej: ingen danske anlægsordrer"); }
	if (L.Work != ECampaign1851LinkWork::None)
	{
		return FString::Printf(TEXT("%s under anlæg"), Campaign1851Network::WorkName(L.Work));
	}
	if (Work == ECampaign1851LinkWork::Chaussee)
	{
		return L.bChaussee ? TEXT("allerede chaussé") : TEXT("");
	}
	if (L.bRailway)
	{
		return TEXT("jernbanen findes");
	}
	if (!HasResearch(TEXT("railway")))
	{
		return TEXT("kræver forskning: Jernbaneanlæg");
	}
	if (L.RailPath.Num() < 2)
	{
		return L.HasFerry() ? TEXT("ingen bane over færgestedet") : TEXT("terrænet tillader ingen bane");
	}
	return TEXT("");
}

int32 ACampaign1851Map::LinkWorkCost(int32 Link, ECampaign1851LinkWork Work) const
{
	if (!Links.IsValidIndex(Link))
	{
		return 0;
	}
	const FCampaign1851Link& L = Links[Link];
	double Cost = 0.0;
	if (Work == ECampaign1851LinkWork::Chaussee)
	{
		Cost = L.RoadKm * Campaign1851Network::ChausseeRdPerKm * (HasResearch(TEXT("roads")) ? 0.85 : 1.0);
	}
	else if (Work == ECampaign1851LinkWork::Railway)
	{
		Cost = L.RailKm * Campaign1851Network::RailwayRdPerKm;
		// A station at each end that has none yet.
		const FCampaign1851BuildingDef* Station = Campaign1851Buildings::Find(TEXT("Railway_Station"));
		const int32 StationCost = Station ? Station->CostRd : 19100;
		Cost += (HasStation(L.A) ? 0 : StationCost) + (HasStation(L.B) ? 0 : StationCost);
	}
	return FMath::RoundToInt(Cost / 100.0) * 100;
}

float ACampaign1851Map::LinkWorkDays(int32 Link, ECampaign1851LinkWork Work) const
{
	if (!Links.IsValidIndex(Link))
	{
		return 1.f;
	}
	const FCampaign1851Link& L = Links[Link];
	return Work == ECampaign1851LinkWork::Railway
		? FMath::RoundToFloat(Campaign1851Network::RailwayDaysBase + Campaign1851Network::RailwayDaysPerKm * L.RailKm)
		: FMath::RoundToFloat(Campaign1851Network::ChausseeDaysBase + Campaign1851Network::ChausseeDaysPerKm * L.RoadKm);
}

float ACampaign1851Map::LinkTravelDays(int32 Link) const
{
	if (!Links.IsValidIndex(Link))
	{
		return 0.f;
	}
	const FCampaign1851Link& L = Links[Link];
	if (L.bRailway)
	{
		return Campaign1851Network::TrainLoadingDays + L.RailKm / Campaign1851Network::TrainKmPerDay;
	}
	return L.RoadKm / (L.bChaussee ? Campaign1851Network::MarchKmPerDayChaussee : Campaign1851Network::MarchKmPerDayRoad)
		+ (L.HasFerry() ? Campaign1851Network::FerryDays : 0.f);
}

FString ACampaign1851Map::LinkTravelText(int32 Link) const
{
	if (!Links.IsValidIndex(Link))
	{
		return FString();
	}
	const FCampaign1851Link& L = Links[Link];
	const float Days = LinkTravelDays(Link);
	if (L.bRailway)
	{
		return FString::Printf(TEXT("jernbane %.0f km  ·  %s dag med tog"), L.RailKm, *HalfDays(Days));
	}
	return FString::Printf(TEXT("%.0f km %s  ·  %s dagsmarch%s"), L.RoadKm + L.FerryKm, L.bChaussee ? TEXT("chaussé") : TEXT("landevej"), *HalfDays(Days),
		L.HasFerry() ? TEXT("  ·  færge") : TEXT(""));
}

double ACampaign1851Map::NetworkUpkeepPerYear() const
{
	double Total = 0.0;
	for (const FCampaign1851Link& L : Links)
	{
		Total += L.bChaussee && !L.bHistoricChaussee ? L.RoadKm * Campaign1851Network::ChausseeUpkeepPerKm : 0.0;
	}
	for (int32 r = HistoricRailways; r < Railways.Num(); ++r)
	{
		Total += Railways[r].LengthKm * Campaign1851Network::RailwayUpkeepPerKm;
	}
	return Total;
}

// ------------------------------------------------------------------ projects

bool ACampaign1851Map::StartLinkWork(int32 Link, ECampaign1851LinkWork Work, bool bCharge, FString* OutReason)
{
	const FString Why = Work == ECampaign1851LinkWork::None ? FString(TEXT("-")) : LinkBlockReason(Link, Work);
	const int32 Cost = LinkWorkCost(Link, Work);
	if (!Why.IsEmpty() || (bCharge && !CanAfford(Cost)))
	{
		if (OutReason)
		{
			*OutReason = !Why.IsEmpty() ? Why : TEXT("Ikke råd til udbetalingen endnu");
		}
		return false;
	}
	FCampaign1851Link& L = Links[Link];
	L.Work = Work;
	L.DaysBuilt = 0.f;
	L.WorkDays = LinkWorkDays(Link, Work);
	L.WorkCost = Cost;
	L.bStalled = false;
	const FString Name = FString::Printf(TEXT("%s %s–%s"), Campaign1851Network::WorkName(Work), *Cities[L.A].Name, *Cities[L.B].Name);
	if (bCharge)
	{
		AddTransaction(-Cost * Campaign1851Buildings::DownPayment, FString::Printf(TEXT("Udbetaling: %s"), *Name));
		UseMaterials((TownKm(L.A) + TownKm(L.B)) * 0.5, Cost, Name);
	}
	if (Work == ECampaign1851LinkWork::Railway)
	{
		ClearSceneryAlong(L.RailPath, 0.06f);
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|network|start|%s|%d rd|%.0f days"), *Name, Cost, L.WorkDays);
	RebuildNetworkMeshes();
	return true;
}

void ACampaign1851Map::FinishLinkWork(int32 Link)
{
	FCampaign1851Link& L = Links[Link];
	const FString Ends = FString::Printf(TEXT("%s–%s"), *Cities[L.A].Name, *Cities[L.B].Name);
	if (L.Work == ECampaign1851LinkWork::Chaussee)
	{
		L.bChaussee = true;
		News.Add(FString::Printf(TEXT("Chausséen %s er færdig"), *Ends));
	}
	else if (L.Work == ECampaign1851LinkWork::Railway)
	{
		FCampaign1851Railway& R = Railways.AddDefaulted_GetRef();
		R.Name = FString::Printf(TEXT("%s Jernbane"), *Ends);
		R.Stations = { Cities[L.A].Name, Cities[L.B].Name };
		R.Towns = { L.A, L.B };
		R.Km = L.RailPath;
		R.LengthKm = L.RailKm;
		R.Begun = R.Opened = GetDate();
		R.Link = Link;
		R.bAnnounced = true;
		News.Add(FString::Printf(TEXT("Jernbanen %s er åbnet"), *Ends));
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|network|done|%s %s"), Campaign1851Network::WorkName(L.Work), *Ends);
	L.Work = ECampaign1851LinkWork::None;
	L.DaysBuilt = 0.f;
	L.bStalled = false;
	UpdateOpenRailways(false);
	RebuildNetworkMeshes();
}

void ACampaign1851Map::UpdateOpenRailways(bool bAnnounce)
{
	const FDateTime Now = GetDate();
	for (FCampaign1851Link& L : Links)
	{
		L.bRailway = false;
	}
	auto MarkLink = [this](int32 X, int32 Y)
	{
		for (FCampaign1851Link& L : Links)
		{
			if ((L.A == X && L.B == Y) || (L.A == Y && L.B == X))
			{
				L.bRailway = true;
			}
		}
	};
	bool bChanged = false;
	for (FCampaign1851Railway& R : Railways)
	{
		if (!R.IsOpen(Now))
		{
			bChanged |= R.bAnnounced;
			R.bAnnounced = false;
			continue;
		}
		for (int32 t = 0; t + 1 < R.Towns.Num(); ++t)
		{
			MarkLink(R.Towns[t], R.Towns[t + 1]);
		}
		if (!R.bAnnounced)
		{
			R.bAnnounced = true;
			bChanged = true;
			if (bAnnounce)
			{
				News.Add(FString::Printf(TEXT("%s er åbnet"), *R.Name));
			}
		}
	}
	if (bChanged)
	{
		RebuildNetworkMeshes();
	}
}

void ACampaign1851Map::ResetNetwork()
{
	for (FCampaign1851Link& L : Links)
	{
		L.Work = ECampaign1851LinkWork::None;
		L.DaysBuilt = 0.f;
		L.bStalled = false;
		L.bChaussee = L.bHistoricChaussee;
	}
	Railways.SetNum(HistoricRailways);
	for (FCampaign1851Railway& R : Railways)
	{
		R.bAnnounced = R.IsOpen(GetDate());
	}
	UpdateOpenRailways(false);
	RebuildNetworkMeshes();
	DetectBridges();
}

TArray<FCampaign1851LinkSave> ACampaign1851Map::SaveNetwork() const
{
	TArray<FCampaign1851LinkSave> Out;
	for (int32 i = 0; i < Links.Num(); ++i)
	{
		const FCampaign1851Link& L = Links[i];
		const bool bOwnRailway = Railways.ContainsByPredicate([i](const FCampaign1851Railway& R) { return R.Link == i; });
		if ((L.bChaussee && !L.bHistoricChaussee) || bOwnRailway || L.Work != ECampaign1851LinkWork::None)
		{
			FCampaign1851LinkSave& S = Out.AddDefaulted_GetRef();
			S.A = Cities[L.A].Name;
			S.B = Cities[L.B].Name;
			S.bChaussee = L.bChaussee && !L.bHistoricChaussee;
			S.bRailway = bOwnRailway;
			S.Work = uint8(L.Work);
			S.DaysBuilt = L.DaysBuilt;
		}
	}
	return Out;
}

int32 ACampaign1851Map::RestoreNetwork(const TArray<FCampaign1851LinkSave>& Saves)
{
	int32 Restored = 0;
	for (const FCampaign1851LinkSave& S : Saves)
	{
		const int32 A = FindCity(S.A), B = FindCity(S.B);
		const int32 Link = Links.IndexOfByPredicate([A, B](const FCampaign1851Link& L) { return (L.A == A && L.B == B) || (L.A == B && L.B == A); });
		if (Link == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|save|no link %s–%s"), *S.A, *S.B);
			continue;
		}
		FCampaign1851Link& L = Links[Link];
		L.bChaussee |= S.bChaussee;
		if (S.bRailway && L.RailPath.Num() >= 2)
		{
			L.Work = ECampaign1851LinkWork::Railway;
			FinishLinkWork(Link);
			ClearSceneryAlong(L.RailPath, 0.06f);
		}
		if (S.Work == uint8(ECampaign1851LinkWork::Chaussee) || S.Work == uint8(ECampaign1851LinkWork::Railway))
		{
			const ECampaign1851LinkWork Work = ECampaign1851LinkWork(S.Work);
			L.Work = Work;
			L.WorkDays = LinkWorkDays(Link, Work);
			L.WorkCost = LinkWorkCost(Link, Work);
			L.DaysBuilt = S.DaysBuilt;
			if (Work == ECampaign1851LinkWork::Railway)
			{
				ClearSceneryAlong(L.RailPath, 0.06f);
			}
		}
		++Restored;
	}
	News.Reset();   // nothing "opens" when a save is loaded
	UpdateOpenRailways(false);
	RebuildNetworkMeshes();
	return Restored;
}

// ------------------------------------------------------------------ time

void ACampaign1851Map::AdvanceNetwork(float DeltaDays, float DeltaSeconds)
{
	const FDateTime Now = GetDate();
	for (int32 i = 0; i < Links.Num(); ++i)
	{
		FCampaign1851Link& L = Links[i];
		if (L.Work == ECampaign1851LinkWork::None || DeltaDays <= 0.f)
		{
			continue;
		}
		// Wages day by day, as for the buildings: slower in frost, and only as far as the treasury can pay.
		float Work = DeltaDays * Campaign1851Buildings::WorkRate(Campaign1851Network::WorkType(), Now);
		const double PerDay = L.WorkCost * (1.0 - Campaign1851Buildings::DownPayment) / FMath::Max(L.WorkDays, 1.f);
		L.bStalled = Work * PerDay > Treasury;
		if (L.bStalled)
		{
			Work = PerDay > 0.0 ? float(Treasury / PerDay) : Work;
		}
		Treasury -= Work * PerDay;
		MonthSpend.FindOrAdd(FString::Printf(TEXT("Anlæg: %s %s–%s"), Campaign1851Network::WorkName(L.Work), *Cities[L.A].Name, *Cities[L.B].Name)) += Work * PerDay;
		L.DaysBuilt += Work;
		if (L.DaysBuilt >= L.WorkDays)
		{
			FinishLinkWork(i);
		}
	}
	UpdateOpenRailways(true);

	// Redraw the works when they have grown by a few hundred metres.
	double WorkKm = 0.0;
	for (const FCampaign1851Link& L : Links)
	{
		WorkKm += L.Work == ECampaign1851LinkWork::Railway ? L.Progress() * L.RailKm : L.Work == ECampaign1851LinkWork::Chaussee ? L.Progress() * L.RoadKm : 0.0;
	}
	for (int32 r = 0; r < HistoricRailways; ++r)
	{
		WorkKm += Railways[r].IsOpen(Now) ? 0.0 : Railways[r].BuildProgress(Now) * Railways[r].LengthKm;
	}
	if (FMath::Abs(WorkKm - DrawnWorkKm) > 0.3)
	{
		RebuildNetworkMeshes();
	}

	if (DeltaSeconds <= 0.f || !bSceneryVisible)
	{
		return;
	}
	// Trains shuttle along the open lines; they face the way they run.
	for (int32 r = 0; r < Railways.Num() && (r + 1) * TrainVehicles <= Trains.Num(); ++r)
	{
		if (!Trains[r * TrainVehicles])
		{
			continue;
		}
		const FCampaign1851Railway& R = Railways[r];
		if (TrainPause[r] > 0.f)
		{
			TrainPause[r] -= DeltaSeconds;
		}
		else
		{
			TrainAt[r] += TrainDirection[r] * TrainKmPerSecond * DeltaSeconds;
			if (TrainAt[r] <= 0.f || TrainAt[r] >= R.LengthKm)
			{
				TrainAt[r] = FMath::Clamp(TrainAt[r], 0.f, R.LengthKm);
				TrainDirection[r] = -TrainDirection[r];
				TrainPause[r] = TrainPauseSeconds;
			}
		}
		TArray<UStaticMeshComponent*> Parts;
		for (int32 v = 0; v < TrainVehicles; ++v)
		{
			Parts.Add(Trains[r * TrainVehicles + v]);
		}
		PlaceTrain(Parts, R.Km, TrainAt[r], TrainDirection[r], PieceScale);
	}
	// Work gangs: a cart shuttles just behind the head of the works.
	GangClock += DeltaSeconds;
	for (int32 i = 0; i < Gangs.Num(); ++i)
	{
		UStaticMeshComponent* Gang = Gangs[i];
		if (!Gang)
		{
			continue;
		}
		const TArray<FVector2D>* Line = nullptr;
		double Head = 0.0;
		if (i < Links.Num())
		{
			const FCampaign1851Link& L = Links[i];
			Line = L.Work == ECampaign1851LinkWork::Railway ? &L.RailPath : &L.Km;
			const double Length = LineLength(*Line);
			Head = L.Work == ECampaign1851LinkWork::Railway ? FMath::Min(1.f, L.Progress() / EarthworksShare) * Length : L.Progress() * Length;
		}
		else if (Railways.IsValidIndex(i - Links.Num()))
		{
			const FCampaign1851Railway& R = Railways[i - Links.Num()];
			Line = &R.Km;
			Head = FMath::Min(1.f, R.BuildProgress(Now) / EarthworksShare) * R.LengthKm;
		}
		if (!Line)
		{
			continue;
		}
		const float Swing = 0.5f - 0.5f * FMath::Cos(GangClock * 0.9f + i);
		FVector2D Dir;
		const FVector2D At = AlongLine(*Line, FMath::Max(0.0, Head - 0.1 - 0.4 * Swing), &Dir);
		if (FMath::Sin(GangClock * 0.9f + i) < 0.f)
		{
			Dir = -Dir;   // driving back for the next load
		}
		Gang->SetWorldLocationAndRotation(WorldAtKm(At), FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(-Dir.Y, Dir.X)) + float(GetActorRotation().Yaw), 0.f));
	}
}

// ------------------------------------------------------------------ trains

void ACampaign1851Map::EnsureTrainParts()
{
	if (TrainParts.Num() > 0)
	{
		return;
	}
	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, NetworkMaterialPath))
	{
		using Campaign1851Scenery::ESitePiece;
		for (ESitePiece Piece : { ESitePiece::TrainEngine, ESitePiece::TrainCarBrown, ESitePiece::TrainCarGreen })
		{
			TrainParts.Add(Campaign1851Scenery::BuildSitePiece(Piece, Material));
		}
	}
}

void ACampaign1851Map::PlaceTrain(const TArray<UStaticMeshComponent*>& Parts, const TArray<FVector2D>& Line, double FrontKm, float Direction, float Scale) const
{
	// Vehicle fronts and lengths behind the engine's front, in piece units (as Campaign1851Scenery builds them).
	static const float Front[] = { 0.f, 5.35f, 8.5f, 11.65f };
	static const float Length[] = { 5.22f, 2.9f, 2.9f, 2.9f };
	const double Length0 = LineLength(Line);
	const double UnitKm = Scale / KmToUnits;
	for (int32 v = 0; v < Parts.Num() && v < TrainVehicles; ++v)
	{
		if (!Parts[v])
		{
			continue;
		}
		const double A = FMath::Clamp(FrontKm - Direction * Front[v] * UnitKm, 0.0, Length0);
		const double B = FMath::Clamp(FrontKm - Direction * (Front[v] + Length[v]) * UnitKm, 0.0, Length0);
		FVector2D Tangent;
		const FVector2D PA = AlongLine(Line, A, &Tangent), PB = AlongLine(Line, B);
		FVector2D Dir = (PA - PB).GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			Dir = Tangent * Direction;
		}
		Parts[v]->SetWorldLocationAndRotation(WorldAtKm((PA + PB) * 0.5) + FVector(0.0, 0.0, 1.0),
			FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(-Dir.Y, Dir.X)) + float(GetActorRotation().Yaw), 0.f));
	}
}

// ------------------------------------------------------------------ drawing

UStaticMeshComponent* ACampaign1851Map::MakeRibbon(const TArray<TArray<FVector2D>>& Lines, float WidthKm, const FLinearColor& Colour, const TCHAR* Name, float Lift)
{
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, NetworkMaterialPath);
	TArray<TArray<FVector>> Local;
	for (const TArray<FVector2D>& Line : Lines)
	{
		if (Line.Num() < 2)
		{
			continue;
		}
		TArray<FVector>& Out = Local.AddDefaulted_GetRef();
		for (const FVector2D& P : Resample(Line, 0.2))
		{
			Out.Add(LocalAtKm(P) + FVector(0.0, 0.0, Lift));
		}
	}
	if (!Material || Local.Num() == 0)
	{
		return nullptr;
	}
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), Name));
	Mesh->SetupAttachment(Root);
	Mesh->SetStaticMesh(Campaign1851Scenery::BuildRibbons(Local, WidthKm * 0.5f * float(KmToUnits), Colour, Material, *FString::Printf(TEXT("SM_Campaign1851_%s"), Name)));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->RegisterComponent();
	return Mesh;
}

void ACampaign1851Map::RebuildNetworkMeshes()
{
	if (GridZ.Num() == 0)
	{
		return;
	}
	for (TObjectPtr<UStaticMeshComponent>* Slot : { &Chaussees, &RailBed, &RailTrack, &RailWorks, &RailGravel, &RailSleepers, &RailSteel })
	{
		if (*Slot)
		{
			(*Slot)->DestroyComponent();
			*Slot = nullptr;
		}
	}
	const FDateTime Now = GetDate();
	TArray<TArray<FVector2D>> Paved, Bed, Track, Works;
	double WorkKm = 0.0;
	auto AddRailway = [&](const TArray<FVector2D>& Line, float Progress)
	{
		const double Length = LineLength(Line);
		if (Progress >= 1.f)
		{
			Bed.Add(Line);
			AddDashes(Line, DashOnKm, DashOffKm, Track);
			return;
		}
		// Earthworks lead; the track follows once the bank is ready.
		Works.Add(SubLine(Line, 0.0, FMath::Min(1.f, Progress / EarthworksShare) * Length));
		const double Laid = FMath::Max(0.f, (Progress - EarthworksShare) / (1.f - EarthworksShare)) * Length;
		if (Laid > 0.05)
		{
			const TArray<FVector2D> Done = SubLine(Line, 0.0, Laid);
			Bed.Add(Done);
			AddDashes(Done, DashOnKm, DashOffKm, Track);
		}
		WorkKm += Progress * Length;
	};
	for (const FCampaign1851Railway& R : Railways)
	{
		AddRailway(R.Km, R.IsOpen(Now) ? 1.f : R.BuildProgress(Now));
	}
	for (const FCampaign1851Link& L : Links)
	{
		if (L.bChaussee)
		{
			Paved.Add(L.Km);
		}
		else if (L.Work == ECampaign1851LinkWork::Chaussee)
		{
			Paved.Add(SubLine(L.Km, 0.0, L.Progress() * LineLength(L.Km)));
			WorkKm += L.Progress() * L.RoadKm;
		}
		if (L.Work == ECampaign1851LinkWork::Railway)
		{
			AddRailway(L.RailPath, FMath::Min(L.Progress(), 0.999f));
		}
	}
	DrawnWorkKm = WorkKm;
	Chaussees = MakeRibbon(Paved, ChausseeWidthKm, Srgb(222, 214, 192), TEXT("Chaussees"), 1.5f);
	RailWorks = MakeRibbon(Works, WorksWidthKm, Srgb(150, 112, 70), TEXT("RailWorks"), 1.7f);
	RailBed = MakeRibbon(Bed, BedWidthKm, Srgb(46, 42, 40), TEXT("RailBed"), 1.9f);
	RailTrack = MakeRibbon(Track, TrackWidthKm, Srgb(242, 238, 228), TEXT("RailTrack"), 2.2f);
	// Close in: a gravel bed, sleepers every 30 m and two iron rails (board-game scale, like the houses).
	TArray<TArray<FVector2D>> Sleepers, Steel, Fine;
	for (const TArray<FVector2D>& Line : Bed)
	{
		const TArray<FVector2D> Dense = Resample(Line, 0.05);
		Fine.Add(Dense);
		AddDashes(Line, 0.011, 0.019, Sleepers);
		Steel.Add(OffsetLine(Dense, 0.013));
		Steel.Add(OffsetLine(Dense, -0.013));
	}
	RailGravel = MakeRibbon(Fine, 0.075f, Srgb(124, 114, 98), TEXT("RailGravel"), 1.9f);
	RailSleepers = MakeRibbon(Sleepers, 0.052f, Srgb(92, 66, 44), TEXT("RailSleepers"), 2.1f);
	RailSteel = MakeRibbon(Steel, 0.006f, Srgb(58, 58, 64), TEXT("RailSteel"), 2.3f);

	// Trains on the open lines, stations in their towns, and a work gang at every head of works.
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, NetworkMaterialPath);
	EnsureTrainParts();
	if (Material && !TrainMesh)
	{
		TrainMesh = Campaign1851Scenery::BuildSitePiece(Campaign1851Scenery::ESitePiece::Train, Material);
		GangMesh = Campaign1851Scenery::BuildSitePiece(Campaign1851Scenery::ESitePiece::Wagon, Material);
		StationMesh = Campaign1851Scenery::BuildSitePiece(Campaign1851Scenery::ESitePiece::Station, Material);
	}
	auto MakePiece = [this](UStaticMesh* Mesh, const TCHAR* Name)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), Name));
		C->SetupAttachment(Root);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetWorldScale3D(FVector(PieceScale));
		C->RegisterComponent();
		return C;
	};
	for (int32 t = Railways.Num() * TrainVehicles; t < Trains.Num(); ++t)   // lines gone with a new game or a load
	{
		if (Trains[t])
		{
			Trains[t]->DestroyComponent();
		}
	}
	Trains.SetNum(Railways.Num() * TrainVehicles);
	TrainAt.SetNum(Railways.Num());
	TrainDirection.SetNum(Railways.Num());
	TrainPause.SetNum(Railways.Num());
	for (int32 r = 0; r < Railways.Num(); ++r)
	{
		const bool bOpen = Railways[r].IsOpen(Now);
		const int32 First = r * TrainVehicles;
		if (bOpen && !Trains[First] && TrainParts.Num() == 3)
		{
			TArray<UStaticMeshComponent*> Parts;
			for (int32 v = 0; v < TrainVehicles; ++v)
			{
				Trains[First + v] = MakePiece(TrainParts[v == 0 ? 0 : v == 1 ? 1 : 2], TEXT("Train"));
				Parts.Add(Trains[First + v]);
			}
			TrainAt[r] = Railways[r].LengthKm * (0.2f + 0.6f * FMath::Frac(r * 0.37f));
			TrainDirection[r] = r % 2 ? -1.f : 1.f;
			TrainPause[r] = 0.f;
			PlaceTrain(Parts, Railways[r].Km, TrainAt[r], TrainDirection[r], PieceScale);
		}
		else if (!bOpen && Trains[First])
		{
			for (int32 v = 0; v < TrainVehicles; ++v)
			{
				if (Trains[First + v])
				{
					Trains[First + v]->DestroyComponent();
					Trains[First + v] = nullptr;
				}
			}
		}
	}
	for (UStaticMeshComponent* Station : Stations)
	{
		if (Station)
		{
			Station->DestroyComponent();
		}
	}
	Stations.Reset();
	TSet<int32> WithStation;
	for (const FCampaign1851Railway& R : Railways)
	{
		for (int32 Town : R.IsOpen(Now) && StationMesh ? R.Towns : TArray<int32>())
		{
			if (WithStation.Contains(Town) || !Cities.IsValidIndex(Town) || Cities[Town].bForeign)
			{
				continue;
			}
			WithStation.Add(Town);
			// On the line, a little out from the town centre, beside the track.
			const FVector2D Centre = Extent.Projection.Forward(Cities[Town].Lat, Cities[Town].Lon);
			const double Length = LineLength(R.Km);
			const bool bAtStart = FVector2D::Distance(R.Km[0], Centre) < FVector2D::Distance(R.Km.Last(), Centre);
			double Best = bAtStart ? 0.0 : Length, BestDist = 1e9;
			for (double At = 0.0; At <= Length; At += 0.1)
			{
				const double D = FVector2D::Distance(AlongLine(R.Km, At), Centre);
				if (D < BestDist)
				{
					BestDist = D;
					Best = At;
				}
			}
			Best = FMath::Clamp(Best, 0.5, FMath::Max(0.5, Length - 0.5));
			FVector2D Dir;
			const FVector2D At = AlongLine(R.Km, Best, &Dir);
			const FVector2D Side(-Dir.Y, Dir.X);
			const FVector2D Km = At + Side * 0.14;
			UStaticMeshComponent* Station = MakePiece(StationMesh, TEXT("Station"));
			// The station's -Y (platform) faces the track: its +Y points along Side (world Y = -km Y).
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-Side.Y, Side.X)) - 90.f + float(GetActorRotation().Yaw);
			Station->SetWorldLocationAndRotation(WorldAtKm(Km), FRotator(0.f, Yaw, 0.f));
			Stations.Add(Station);
		}
	}
	// Gangs: index i < Links.Num() for a link project, Links.Num() + r for a historical line being built.
	Gangs.SetNum(Links.Num() + HistoricRailways);
	for (int32 i = 0; i < Gangs.Num(); ++i)
	{
		const bool bBusy = i < Links.Num() ? Links[i].Work != ECampaign1851LinkWork::None : !Railways[i - Links.Num()].IsOpen(Now);
		if (bBusy && !Gangs[i] && GangMesh)
		{
			Gangs[i] = MakePiece(GangMesh, TEXT("Gang"));
		}
		else if (!bBusy && Gangs[i])
		{
			Gangs[i]->DestroyComponent();
			Gangs[i] = nullptr;
		}
	}
	UpdateNetworkVisibility();
}

void ACampaign1851Map::UpdateNetworkVisibility()
{
	const bool bTrack = LastCameraDistanceKm < RailTrackMaxDistanceKm;
	for (UStaticMeshComponent* Mesh : { Chaussees.Get(), RailWorks.Get() })
	{
		if (Mesh)
		{
			Mesh->SetVisibility(bRoadsVisible);
		}
	}
	for (UStaticMeshComponent* Mesh : { RailBed.Get(), RailTrack.Get() })
	{
		if (Mesh)
		{
			Mesh->SetVisibility(bRoadsVisible && !bTrack);
		}
	}
	for (UStaticMeshComponent* Mesh : { RailGravel.Get(), RailSleepers.Get(), RailSteel.Get() })
	{
		if (Mesh)
		{
			Mesh->SetVisibility(bRoadsVisible && bTrack);
		}
	}
	for (const TArray<TObjectPtr<UStaticMeshComponent>>* List : { &Trains, &Stations, &Gangs })
	{
		for (UStaticMeshComponent* Piece : *List)
		{
			if (Piece)
			{
				Piece->SetVisibility(bSceneryVisible);
			}
		}
	}
}

void ACampaign1851Map::ClearSceneryAlong(const TArray<FVector2D>& Line, float RadiusKm)
{
	// Samples of the line in a coarse grid, then one pass over every scenery instance.
	constexpr double Cell = 0.5;
	TMap<FIntPoint, TArray<FVector2D>> Grid;
	for (const FVector2D& P : Resample(Line, 0.05))
	{
		Grid.FindOrAdd(FIntPoint(FMath::FloorToInt(P.X / Cell), FMath::FloorToInt(P.Y / Cell))).Add(P);
	}
	const double Radius2 = FMath::Square(RadiusKm + 0.04);   // plus about half a house
	int32 Removed = 0;
	for (UHierarchicalInstancedStaticMeshComponent* Part : Scenery)
	{
		TArray<int32> Remove;
		for (int32 i = 0; i < Part->GetInstanceCount(); ++i)
		{
			FTransform T;
			if (!Part->GetInstanceTransform(i, T, false))
			{
				continue;
			}
			const FVector L = T.GetLocation();
			const FVector2D Km(L.X / KmToUnits + SizeKm.X * 0.5 + Extent.XMin, -L.Y / KmToUnits + SizeKm.Y * 0.5 + Extent.YMin);
			const FIntPoint C(FMath::FloorToInt(Km.X / Cell), FMath::FloorToInt(Km.Y / Cell));
			bool bHit = false;
			for (int32 dy = -1; dy <= 1 && !bHit; ++dy)
			{
				for (int32 dx = -1; dx <= 1 && !bHit; ++dx)
				{
					if (const TArray<FVector2D>* Points = Grid.Find(C + FIntPoint(dx, dy)))
					{
						for (const FVector2D& P : *Points)
						{
							if (FVector2D::DistSquared(P, Km) < Radius2)
							{
								bHit = true;
								break;
							}
						}
					}
				}
			}
			if (bHit)
			{
				Remove.Add(i);
			}
		}
		if (Remove.Num() > 0)
		{
			Part->RemoveInstances(Remove);
			Removed += Remove.Num();
		}
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|network|cleared %d scenery pieces along a %.1f km line"), Removed, LineLength(Line));
}
