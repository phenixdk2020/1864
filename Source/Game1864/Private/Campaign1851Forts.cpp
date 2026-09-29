// Field fortifications (skanser) of the 1851 campaign: raised anywhere on the monarchy's land, armed and
// strengthened step by step, and handed to the 3D battles as data. ACampaign1851Map's fort layer.

#include "Campaign1851Map.h"

#include "Campaign1851Buildings.h"
#include "Campaign1851Fort.h"
#include "Campaign1851Scenery.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace Campaign1851Forts
{
	const TCHAR* DefenceName(int32 Level)
	{
		switch (Level)
		{
		case 1: return TEXT("Brystværn og grav");
		case 2: return TEXT("Palisader og ulvegrave");
		case 3: return TEXT("Bombesikkert blokhus");
		case 4: return TEXT("Traverser og bombesikre magasiner");
		default: return TEXT("-");
		}
	}

	const TCHAR* DefenceNote(int32 Level)
	{
		switch (Level)
		{
		case 2: return TEXT("pælerækker i graven og huller foran: stormen standses under ild");
		case 3: return TEXT("et tømret, jorddækket blokhus: besætningen overlever bombardementet");
		case 4: return TEXT("jordvolde mellem kanonerne og sikre krudtrum: én granat tager kun én kanon");
		default: return TEXT("");
		}
	}

	const TCHAR* GunType()
	{
		return TEXT("glatløbet fæstningskanon, 24-pund");
	}

	void GunSlots(bool bLarge, TArray<FVector2f>& OutPos, TArray<float>& OutYaw)
	{
		OutPos.Reset();
		OutYaw.Reset();
		// Faces in the order guns are placed: the front first, then the flanks, then the rear.
		struct FFace { FVector2f A, B; TArray<float> T; };
		TArray<FFace> Faces;
		FVector2f Centre;
		if (bLarge)
		{
			Centre = FVector2f(0.f, 0.f);
			Faces = { { FVector2f(14.f, 0.f), FVector2f(4.f, -14.f), { 0.25f, 0.5f, 0.75f } }, { FVector2f(4.f, 14.f), FVector2f(14.f, 0.f), { 0.25f, 0.5f, 0.75f } },
				{ FVector2f(4.f, -14.f), FVector2f(-12.f, -14.f), { 0.33f, 0.66f } }, { FVector2f(-12.f, 14.f), FVector2f(4.f, 14.f), { 0.33f, 0.66f } },
				{ FVector2f(-12.f, -14.f), FVector2f(-12.f, 14.f), { 0.3f, 0.7f } } };
		}
		else
		{
			Centre = FVector2f(-2.f, 0.f);
			Faces = { { FVector2f(-4.f, -9.f), FVector2f(8.f, 0.f), { 0.4f, 0.75f } }, { FVector2f(8.f, 0.f), FVector2f(-4.f, 9.f), { 0.25f, 0.6f } } };
		}
		for (const FFace& F : Faces)
		{
			const FVector2f Along = (F.B - F.A).GetSafeNormal();
			FVector2f Out(Along.Y, -Along.X);
			const FVector2f Mid = (F.A + F.B) * 0.5f;
			if (FVector2f::DotProduct(Out, Mid - Centre) < 0.f)
			{
				Out = -Out;
			}
			for (float T : F.T)
			{
				OutPos.Add(F.A + (F.B - F.A) * T - Out * 2.4f);
				OutYaw.Add(FMath::RadiansToDegrees(FMath::Atan2(Out.Y, Out.X)));
			}
		}
	}

	FString Compass(float Yaw)
	{
		// World yaw: 0 = east, 90 = south (north is -Y). Bearing from north, clockwise.
		const float Bearing = FMath::Fmod(FMath::Fmod(Yaw + 90.f, 360.f) + 360.f, 360.f);
		static const TCHAR* Points[] = { TEXT("N"), TEXT("NØ"), TEXT("Ø"), TEXT("SØ"), TEXT("S"), TEXT("SV"), TEXT("V"), TEXT("NV") };
		return FString::Printf(TEXT("%s (%.0f°)"), Points[FMath::RoundToInt(Bearing / 45.f) % 8], Bearing);
	}
}

namespace
{
	const TCHAR* FortArmyMaterial = TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery");
}

int32 ACampaign1851Map::NearestTown(const FVector2D& Km) const
{
	// The nearest town of the monarchy as the crow flies (across water too: Dybbøl is "ved Sønderborg").
	int32 Best = INDEX_NONE;
	double BestKm = 1e9;
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		const double D = FVector2D::Distance(TownKm(c), Km);
		if (!Cities[c].bForeign && D < BestKm)
		{
			BestKm = D;
			Best = c;
		}
	}
	return Best;
}

int32 ACampaign1851Map::FortIndex(int32 Id) const
{
	return Forts.IndexOfByPredicate([Id](const FCampaign1851Fort& F) { return F.Id == Id; });
}

FString ACampaign1851Map::FortBlockReason(const FVector2D& Km, bool bLarge) const
{
	if (!IsMonarchyLand(Km))
	{
		return TEXT("Skanser bygges på monarkiets land");
	}
	for (const FCampaign1851City& C : Cities)
	{
		if (!C.bForeign && FVector2D::Distance(TownKm(&C - Cities.GetData()), Km) < TownRadiusKm(C.Population) * 0.8f)
		{
			return FString::Printf(TEXT("For tæt på %s: skanser ligger uden for byerne"), *C.Name);
		}
	}
	for (const FCampaign1851Fort& F : Forts)
	{
		if (FVector2D::Distance(F.Km, Km) < 0.35)
		{
			return FString::Printf(TEXT("For tæt på %s"), *F.Name);
		}
	}
	if (!CanAfford(Campaign1851Forts::BuildCost(bLarge)))
	{
		return TEXT("Ikke råd til udbetalingen");
	}
	return FString();
}

int32 ACampaign1851Map::StartFort(const FVector2D& Km, bool bLarge, float Yaw, FString* OutReason)
{
	const FString Why = FortBlockReason(Km, bLarge);
	if (!Why.IsEmpty())
	{
		if (OutReason) { *OutReason = Why; }
		return INDEX_NONE;
	}
	FCampaign1851Fort F;
	F.Id = NextFortId++;
	F.Km = Km;
	F.Yaw = Yaw;
	F.bLarge = bLarge;
	F.Guns = 0;
	F.Defence = 1;
	const int32 Near = NearestTown(Km);
	F.Name = FString::Printf(TEXT("Skanse %d%s"), F.Id, Cities.IsValidIndex(Near) ? *FString::Printf(TEXT(" ved %s"), *Cities[Near].Name) : TEXT(""));
	F.Work = ECampaign1851FortWork::Build;
	// Trees and farms on the ground give way to the works.
	ClearScenery(Km, (bLarge ? 20.f : 14.f) * PieceScale / float(KmToUnits));
	F.WorkCost = Campaign1851Forts::BuildCost(bLarge);
	F.WorkDays = Campaign1851Forts::BuildDays(bLarge);
	AddTransaction(-F.WorkCost * Campaign1851Buildings::DownPayment, FString::Printf(TEXT("%s: %s skanse påbegyndt"), *F.Name, bLarge ? TEXT("stor") : TEXT("lille")));
	Forts.Add(F);
	UpdateFortVisual(Forts.Num() - 1);
	ExportForts();
	return F.Id;
}

bool ACampaign1851Map::UpgradeFort(int32 Id, ECampaign1851FortWork Work, FString* OutReason)
{
	const int32 Index = FortIndex(Id);
	auto Fail = [OutReason](const TCHAR* Why) { if (OutReason) { *OutReason = Why; } return false; };
	if (Index == INDEX_NONE)
	{
		return Fail(TEXT("Ukendt skanse"));
	}
	FCampaign1851Fort& F = Forts[Index];
	if (!F.bBuilt || F.Work != ECampaign1851FortWork::None)
	{
		return Fail(TEXT("Der arbejdes allerede på skansen"));
	}
	int32 Cost = 0;
	float Days = 1.f;
	if (Work == ECampaign1851FortWork::Guns)
	{
		if (F.Guns >= Campaign1851Forts::MaxGuns(F.bLarge))
		{
			return Fail(TEXT("Skansen har alle sine kanoner"));
		}
		Cost = Campaign1851Forts::GunsCost;
		Days = Campaign1851Forts::GunsDays;
	}
	else if (Work == ECampaign1851FortWork::Defence)
	{
		if (F.Defence >= Campaign1851Forts::MaxDefence)
		{
			return Fail(TEXT("Skansen er fuldt udbygget"));
		}
		Cost = Campaign1851Forts::DefenceCost(F.Defence + 1, F.bLarge);
		Days = Campaign1851Forts::DefenceDays(F.Defence + 1);
	}
	else
	{
		return Fail(TEXT("-"));
	}
	if (!CanAfford(Cost))
	{
		return Fail(TEXT("Ikke råd til udbetalingen"));
	}
	F.Work = Work;
	F.WorkCost = Cost;
	F.WorkDays = Days;
	F.DaysBuilt = 0.f;
	AddTransaction(-Cost * Campaign1851Buildings::DownPayment, FString::Printf(TEXT("%s: %s"), *F.Name,
		Work == ECampaign1851FortWork::Guns ? TEXT("to kanoner mere") : Campaign1851Forts::DefenceName(F.Defence + 1)));
	ExportForts();
	return true;
}

void ACampaign1851Map::TurnFort(int32 Id, float DeltaYaw)
{
	const int32 Index = FortIndex(Id);
	if (Index != INDEX_NONE)
	{
		Forts[Index].Yaw = FMath::Fmod(Forts[Index].Yaw + DeltaYaw + 360.f, 360.f);
		UpdateFortVisual(Index);
		ExportForts();
	}
}

void ACampaign1851Map::AdvanceForts(float DeltaDays)
{
	if (DeltaDays <= 0.f)
	{
		return;
	}
	bool bChanged = false;
	for (int32 i = 0; i < Forts.Num(); ++i)
	{
		FCampaign1851Fort& F = Forts[i];
		if (F.Work == ECampaign1851FortWork::None)
		{
			continue;
		}
		// Wages day by day (earthworks: frost nearly stops them), as far as the treasury can pay.
		float Work = DeltaDays * Campaign1851Buildings::WorkRate(TEXT("jordværk"), GetDate());
		const double PerDay = F.WorkCost * (1.0 - Campaign1851Buildings::DownPayment) / FMath::Max(F.WorkDays, 1.f);
		F.bStalled = Work * PerDay > Treasury;
		if (F.bStalled)
		{
			Work = PerDay > 0.0 ? float(Treasury / PerDay) : Work;
		}
		Treasury -= Work * PerDay;
		MonthSpend.FindOrAdd(FString::Printf(TEXT("Skanser: %s"), *F.Name)) += Work * PerDay;
		const float Before = F.Progress();
		F.DaysBuilt += Work;
		if (F.DaysBuilt >= F.WorkDays)
		{
			if (F.Work == ECampaign1851FortWork::Build)
			{
				F.bBuilt = true;
				F.Guns = Campaign1851Forts::StartGuns(F.bLarge);
				F.Garrison = Campaign1851Forts::InfantryCapacity(F.bLarge);
				News.Add(FString::Printf(TEXT("%s står færdig med %d kanoner"), *F.Name, F.Guns));
			}
			else if (F.Work == ECampaign1851FortWork::Guns)
			{
				F.Guns = FMath::Min(F.Guns + Campaign1851Forts::GunStep, Campaign1851Forts::MaxGuns(F.bLarge));
				News.Add(FString::Printf(TEXT("%s har nu %d kanoner"), *F.Name, F.Guns));
			}
			else
			{
				F.Defence = FMath::Min(F.Defence + 1, Campaign1851Forts::MaxDefence);
				News.Add(FString::Printf(TEXT("%s: %s færdig"), *F.Name, Campaign1851Forts::DefenceName(F.Defence)));
			}
			F.Work = ECampaign1851FortWork::None;
			F.DaysBuilt = 0.f;
			bChanged = true;
			UpdateFortVisual(i);
		}
		else if (F.Work == ECampaign1851FortWork::Build && FMath::FloorToInt(F.Progress() * 10.f) != FMath::FloorToInt(Before * 10.f))
		{
			UpdateFortVisual(i);   // the earthwork rises in tenths
		}
	}
	if (bChanged)
	{
		ExportForts();
	}
}

double ACampaign1851Map::FortUpkeepPerYear() const
{
	double Total = 0.0;
	for (const FCampaign1851Fort& F : Forts)
	{
		Total += F.bBuilt ? Campaign1851Forts::UpkeepPerYear(F.bLarge) : 0.0;
	}
	return Total;
}

FTransform ACampaign1851Map::FortTransform(const FCampaign1851Fort& F) const
{
	// The fort stands on the highest ground under it; its earth foot (in the mesh) slopes down to the terrain,
	// so on a hillside it is built up, never sunk into the slope.
	FVector Base = GetActorTransform().TransformPosition(LocalAtKm(F.Km));
	const double Reach = (F.bLarge ? 17.0 : 12.0) * PieceScale / KmToUnits;
	for (int32 a = 0; a < 12; ++a)
	{
		for (const double R : { Reach * 0.5, Reach })
		{
			const double A = a * UE_TWO_PI / 12.0;
			Base.Z = FMath::Max(Base.Z, GetActorTransform().TransformPosition(LocalAtKm(F.Km + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R)).Z);
		}
	}
	return FTransform(FRotator(0.f, F.Yaw, 0.f), Base + FVector(0.0, 0.0, 0.05), FVector(PieceScale));
}

void ACampaign1851Map::UpdateFortVisual(int32 Index)
{
	if (!Forts.IsValidIndex(Index))
	{
		return;
	}
	using Campaign1851Scenery::ESitePiece;
	if (FortMeshes.Num() == 0)
	{
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, FortArmyMaterial);
		if (!Material)
		{
			return;
		}
		for (ESitePiece Piece : { ESitePiece::RedoubtSmall, ESitePiece::RedoubtLarge, ESitePiece::FortGun, ESitePiece::PalisadeSmall, ESitePiece::PalisadeLarge,
			ESitePiece::Blockhouse, ESitePiece::Traverse })
		{
			FortMeshes.Add(Campaign1851Scenery::BuildSitePiece(Piece, Material));
		}
	}
	const FCampaign1851Fort& F = Forts[Index];
	// Rebuilt from scratch: the parts are few.
	for (int32 p = FortParts.Num() - 1; p >= 0; --p)
	{
		if (FortPartOwner[p] == F.Id)
		{
			if (FortParts[p])
			{
				FortParts[p]->DestroyComponent();
			}
			FortParts.RemoveAt(p);
			FortPartOwner.RemoveAt(p);
		}
	}
	const FTransform Xf = FortTransform(F);
	auto Part = [&](int32 Mesh, const FVector2f& Local, float LocalYaw, float ZScale)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetupAttachment(Root);
		C->SetStaticMesh(FortMeshes[Mesh]);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		const FTransform LocalXf(FRotator(0.f, LocalYaw, 0.f), FVector(Local.X, Local.Y, 0.f), FVector(1.f, 1.f, ZScale));
		C->SetWorldTransform(LocalXf * Xf);
		C->SetVisibility(bSceneryVisible);
		C->RegisterComponent();
		FortParts.Add(C);
		FortPartOwner.Add(F.Id);
	};
	// The earthwork rises while it is dug; the guns, palisades, blockhouse and traverses come with the levels.
	const float Rise = F.bBuilt ? 1.f : FMath::Max(0.08f, F.Progress());
	Part(F.bLarge ? 1 : 0, FVector2f(0.f, 0.f), 0.f, Rise);
	TArray<FVector2f> Slots;
	TArray<float> Yaws;
	Campaign1851Forts::GunSlots(F.bLarge, Slots, Yaws);
	for (int32 g = 0; g < F.Guns && g < Slots.Num(); ++g)
	{
		Part(2, Slots[g], Yaws[g], 1.f);
		if (F.Defence >= 4)
		{
			const float Rad = FMath::DegreesToRadians(Yaws[g]);
			const FVector2f Side(-FMath::Sin(Rad), FMath::Cos(Rad));
			Part(6, Slots[g] + Side * 1.5f, Yaws[g], 1.f);
		}
	}
	if (F.Defence >= 2)
	{
		Part(F.bLarge ? 4 : 3, FVector2f(0.f, 0.f), 0.f, 1.f);
	}
	if (F.Defence >= 3)
	{
		Part(5, F.bLarge ? FVector2f(-4.f, 0.f) : FVector2f(-4.5f, 0.f), 0.f, 1.f);
	}
}

void ACampaign1851Map::ResetForts()
{
	for (UStaticMeshComponent* C : FortParts)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	FortParts.Reset();
	FortPartOwner.Reset();
	Forts.Reset();
	NextFortId = 1;
}

FVector ACampaign1851Map::FortWorld(int32 Index) const
{
	return Forts.IsValidIndex(Index) ? FortTransform(Forts[Index]).GetLocation() : FVector::ZeroVector;
}

TArray<FCampaign1851FortSave> ACampaign1851Map::SaveForts() const
{
	TArray<FCampaign1851FortSave> Out;
	for (const FCampaign1851Fort& F : Forts)
	{
		FCampaign1851FortSave& S = Out.AddDefaulted_GetRef();
		S.Id = F.Id;
		S.Name = F.Name;
		S.Km = F.Km;
		S.Yaw = F.Yaw;
		S.bLarge = F.bLarge;
		S.Guns = F.Guns;
		S.Defence = F.Defence;
		S.Garrison = F.Garrison;
		S.bBuilt = F.bBuilt;
		S.Work = uint8(F.Work);
		S.DaysBuilt = F.DaysBuilt;
		S.WorkDays = F.WorkDays;
		S.WorkCost = F.WorkCost;
	}
	return Out;
}

void ACampaign1851Map::RestoreForts(const TArray<FCampaign1851FortSave>& Saves)
{
	ResetForts();
	for (const FCampaign1851FortSave& S : Saves)
	{
		FCampaign1851Fort F;
		F.Id = S.Id;
		F.Name = S.Name;
		F.Km = S.Km;
		F.Yaw = S.Yaw;
		F.bLarge = S.bLarge;
		F.Guns = S.Guns;
		F.Defence = FMath::Clamp(S.Defence, 1, Campaign1851Forts::MaxDefence);
		F.Garrison = S.Garrison;
		F.bBuilt = S.bBuilt;
		F.Work = ECampaign1851FortWork(FMath::Min<uint8>(S.Work, uint8(ECampaign1851FortWork::Defence)));
		F.DaysBuilt = S.DaysBuilt;
		F.WorkDays = S.WorkDays;
		F.WorkCost = S.WorkCost;
		NextFortId = FMath::Max(NextFortId, F.Id + 1);
		ClearScenery(F.Km, (F.bLarge ? 20.f : 14.f) * PieceScale / float(KmToUnits));
		Forts.Add(F);
		UpdateFortVisual(Forts.Num() - 1);
	}
	ExportForts();
}

void ACampaign1851Map::ExportForts() const
{
	// The 3D battles read this (Docs/Fortifications1851.md): where each fort is, which way it faces,
	// its guns, cover and works, and who is in it.
	TSharedRef<FJsonObject> Doc = MakeShared<FJsonObject>();
	Doc->SetStringField(TEXT("format"), TEXT("PROJECT1864-Fortifications-1"));
	Doc->SetStringField(TEXT("date"), GetDate().ToIso8601());
	Doc->SetNumberField(TEXT("seed"), Seed);
	TArray<TSharedPtr<FJsonValue>> List;
	for (const FCampaign1851Fort& F : Forts)
	{
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		const FVector2D LatLon = Extent.Projection.Inverse(F.Km);
		const float Bearing = FMath::Fmod(FMath::Fmod(F.Yaw + 90.f, 360.f) + 360.f, 360.f);
		const int32 Near = NearestTown(F.Km);
		O->SetNumberField(TEXT("id"), F.Id);
		O->SetStringField(TEXT("name"), F.Name);
		O->SetStringField(TEXT("type"), F.bLarge ? TEXT("redoubt") : TEXT("lunette"));
		O->SetNumberField(TEXT("lat"), LatLon.X);
		O->SetNumberField(TEXT("lon"), LatLon.Y);
		O->SetNumberField(TEXT("mapKmX"), F.Km.X);
		O->SetNumberField(TEXT("mapKmY"), F.Km.Y);
		O->SetStringField(TEXT("nearTown"), Cities.IsValidIndex(Near) ? Cities[Near].Name : FString());
		O->SetNumberField(TEXT("facingBearingDeg"), Bearing);
		O->SetBoolField(TEXT("built"), F.bBuilt);
		O->SetNumberField(TEXT("buildProgress"), F.bBuilt ? 1.0 : F.Progress());
		O->SetNumberField(TEXT("guns"), F.Guns);
		O->SetNumberField(TEXT("maxGuns"), Campaign1851Forts::MaxGuns(F.bLarge));
		O->SetStringField(TEXT("gunType"), Campaign1851Forts::GunType());
		O->SetNumberField(TEXT("gunners"), F.Guns * Campaign1851Forts::GunnersPerGun);
		O->SetNumberField(TEXT("defenceLevel"), F.Defence);
		O->SetStringField(TEXT("defenceName"), Campaign1851Forts::DefenceName(F.Defence));
		O->SetNumberField(TEXT("coverPercent"), Campaign1851Forts::CoverPercent(F.Defence));
		O->SetNumberField(TEXT("parapetHeightM"), Campaign1851Forts::ParapetHeightM(F.bLarge, F.Defence));
		O->SetNumberField(TEXT("ditchDepthM"), Campaign1851Forts::DitchDepthM(F.bLarge));
		O->SetBoolField(TEXT("palisade"), F.Defence >= 2);
		O->SetBoolField(TEXT("blockhouse"), F.Defence >= 3);
		O->SetBoolField(TEXT("traverses"), F.Defence >= 4);
		O->SetNumberField(TEXT("infantryCapacity"), Campaign1851Forts::InfantryCapacity(F.bLarge));
		O->SetNumberField(TEXT("garrison"), F.Garrison);
		// Gun platforms in the fort's frame, metres from its centre (x towards the front, y to the right);
		// the map draws forts larger than life, so the battle gets the real scale (about 3 m a map unit).
		constexpr float MetresPerUnit = 3.f;
		O->SetNumberField(TEXT("sizeM"), (F.bLarge ? 28.f : 17.f) * MetresPerUnit);
		TArray<FVector2f> Slots;
		TArray<float> Yaws;
		Campaign1851Forts::GunSlots(F.bLarge, Slots, Yaws);
		TArray<TSharedPtr<FJsonValue>> Guns;
		for (int32 g = 0; g < Slots.Num(); ++g)
		{
			TSharedRef<FJsonObject> G = MakeShared<FJsonObject>();
			G->SetNumberField(TEXT("xM"), Slots[g].X * MetresPerUnit);
			G->SetNumberField(TEXT("yM"), Slots[g].Y * MetresPerUnit);
			G->SetNumberField(TEXT("yawDeg"), Yaws[g]);
			G->SetBoolField(TEXT("armed"), g < F.Guns);
			Guns.Add(MakeShared<FJsonValueObject>(G));
		}
		O->SetArrayField(TEXT("gunPlatforms"), Guns);
		List.Add(MakeShared<FJsonValueObject>(O));
	}
	Doc->SetArrayField(TEXT("fortifications"), List);
	FString Text;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
	FJsonSerializer::Serialize(Doc, Writer);
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Battle/Fortifications.json");
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void ACampaign1851Map::CompleteForts()
{
	for (int32 i = 0; i < Forts.Num(); ++i)
	{
		FCampaign1851Fort& F = Forts[i];
		F.bBuilt = true;
		F.Work = ECampaign1851FortWork::None;
		F.Guns = Campaign1851Forts::MaxGuns(F.bLarge);
		F.Defence = Campaign1851Forts::MaxDefence;
		F.Garrison = Campaign1851Forts::InfantryCapacity(F.bLarge);
		UpdateFortVisual(i);
	}
	ExportForts();
}
