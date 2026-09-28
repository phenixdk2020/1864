#include "Campaign1851ConstructionSite.h"

#include "Campaign1851Scenery.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	using Campaign1851Scenery::ESitePiece;

	/** A building of the garrison complex and its slot beside the parade ground (piece units). */
	struct FModuleDef
	{
		const TCHAR* Name;
		ESitePiece Piece;
		float Days;
		FVector2D Slot;
		float Yaw;
		float Length, Width, Eave, Top;
		const TCHAR* Card;
	};

	// The parade ground runs along the barracks front (+Y); stables and depot flank it, the
	// infirmary stands behind the barracks.
	const FModuleDef Modules[] = {
		{ TEXT("Infanterikaserne"), ESitePiece::Barracks, 90.f, FVector2D(0.0, 0.0), 0.f, Campaign1851Scenery::BarracksLength, Campaign1851Scenery::BarracksWidth,
			Campaign1851Scenery::BarracksEave, Campaign1851Scenery::BarracksTop, TEXT("/Game/Campaign1851/Buildings/T_Barracks_Infantry.T_Barracks_Infantry") },
		{ TEXT("Stalde"), ESitePiece::Stables, 45.f, FVector2D(-15.0, 7.5), 90.f, 12.f, 4.4f, 3.2f, 6.4f,
			TEXT("/Game/Campaign1851/Buildings/T_Module_Stables.T_Module_Stables") },
		{ TEXT("Depot og magasin"), ESitePiece::Depot, 60.f, FVector2D(15.0, 7.5), 90.f, 7.f, 5.6f, 7.f, 11.6f,
			TEXT("/Game/Campaign1851/Buildings/T_Module_Depot.T_Module_Depot") },
		{ TEXT("Sygestue"), ESitePiece::Infirmary, 40.f, FVector2D(0.0, -10.0), 0.f, 9.f, 4.6f, 4.6f, 8.2f,
			TEXT("/Game/Campaign1851/Buildings/T_Module_Infirmary.T_Module_Infirmary") },
	};

	// Stage boundaries (fraction of a module's time).
	constexpr float StakedEnd = 0.08f, FoundationEnd = 0.2f, WallsEnd = 0.8f, RoofEnd = 0.95f;
	constexpr float PlinthTop = 0.6f;
	constexpr float WagonSpeed = 22.f;   // world units per second
	const FName BuildTopParam(TEXT("BuildTop"));

	float Phase(float P, float A, float B) { return FMath::Clamp((P - A) / (B - A), 0.f, 1.f); }
}

int32 ACampaign1851ConstructionSite::NumModules() { return UE_ARRAY_COUNT(Modules); }
FString ACampaign1851ConstructionSite::ModuleName(int32 Module) { return Modules[FMath::Clamp(Module, 0, NumModules() - 1)].Name; }
const TCHAR* ACampaign1851ConstructionSite::ModuleCard(int32 Module) { return Modules[FMath::Clamp(Module, 0, NumModules() - 1)].Card; }
float ACampaign1851ConstructionSite::ModuleDays(int32 Module) { return Modules[FMath::Clamp(Module, 0, NumModules() - 1)].Days; }

ACampaign1851ConstructionSite::ACampaign1851ConstructionSite()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	auto Part = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Root);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		return C;
	};
	Ground = Part(TEXT("Ground"));
	CraneMast = Part(TEXT("CraneMast"));
	CraneJib = Part(TEXT("CraneJib"));
	Wagon = Part(TEXT("Wagon"));
	Flagpole = Part(TEXT("Flagpole"));
	Flag = Part(TEXT("Flag"));
}

void ACampaign1851ConstructionSite::Setup(int32 InCityIndex, const TArray<FVector>& InWagonPath, UMaterialInterface* Material)
{
	using namespace Campaign1851Scenery;
	CityIndex = InCityIndex;
	WagonPath = InWagonPath;
	WagonDistance.Reset();
	float Total = 0.f;
	for (int32 i = 0; i < WagonPath.Num(); ++i)
	{
		Total += i > 0 ? float(FVector::Dist(WagonPath[i - 1], WagonPath[i])) : 0.f;
		WagonDistance.Add(Total);
	}

	Ground->SetStaticMesh(BuildSitePiece(ESitePiece::Ground, Material));
	CraneMast->SetStaticMesh(BuildSitePiece(ESitePiece::CraneMast, Material));
	CraneJib->SetStaticMesh(BuildSitePiece(ESitePiece::CraneJib, Material));
	Wagon->SetStaticMesh(BuildSitePiece(ESitePiece::Wagon, Material));
	Flagpole->SetStaticMesh(BuildSitePiece(ESitePiece::Flagpole, Material));
	Flag->SetStaticMesh(BuildSitePiece(ESitePiece::Flag, Material));
	CraneMast->SetRelativeLocation(FVector(-10.4, -4.8, 0.0));
	CraneJib->SetRelativeLocation(FVector(-10.4, -4.8, 0.0));
	Flagpole->SetRelativeLocation(FVector(0.0, 12.4, 0.0));

	for (int32 m = 0; m < NumModules(); ++m)
	{
		const FModuleDef& Def = Modules[m];
		auto Make = [&](const TCHAR* Kind, UStaticMesh* Mesh, TArray<TObjectPtr<UStaticMeshComponent>>& Out, TArray<TObjectPtr<UMaterialInstanceDynamic>>& Mids)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("%s_%d"), Kind, m));
			C->SetupAttachment(Root);
			C->SetRelativeLocationAndRotation(FVector(Def.Slot.X, Def.Slot.Y, 0.0), FRotator(0.f, Def.Yaw, 0.f));
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			C->SetStaticMesh(Mesh);
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, this);
			C->SetMaterial(0, Mid);
			C->SetVisibility(false);
			C->RegisterComponent();
			Out.Add(C);
			Mids.Add(Mid);
		};
		Make(TEXT("Building"), BuildSitePiece(Def.Piece, Material), Buildings, BuildingMids);
		Make(TEXT("Scaffold"), BuildScaffold(Def.Length, Def.Width, Def.Top + 1.f, Material), Scaffolds, ScaffoldMids);
	}

	Elapsed.Init(-1.f, NumModules());
	StartModule(0);
	Apply(0.f);
}

bool ACampaign1851ConstructionSite::CanStartModule(int32 Module) const
{
	return Module > 0 && Module < NumModules() && IsBarracksDone() && Active == INDEX_NONE && !IsModuleStarted(Module);
}

void ACampaign1851ConstructionSite::StartModule(int32 Module)
{
	if (Module == 0 ? IsModuleStarted(0) : !CanStartModule(Module))
	{
		return;
	}
	Elapsed[Module] = 0.f;
	Active = Module;
}

float ACampaign1851ConstructionSite::GetModuleProgress(int32 Module) const
{
	return IsModuleStarted(Module) ? FMath::Clamp(Elapsed[Module] / ModuleDays(Module), 0.f, 1.f) : 0.f;
}

FString ACampaign1851ConstructionSite::GetStageName(int32 Module) const
{
	if (!IsModuleStarted(Module)) return TEXT("Ikke bygget");
	const float P = GetModuleProgress(Module);
	if (P >= 1.f) return TEXT("Færdig");
	if (P < StakedEnd) return TEXT("Grunden afstikkes");
	if (P < FoundationEnd) return TEXT("Fundament");
	if (P < WallsEnd) return TEXT("Murværk");
	if (P < RoofEnd) return TEXT("Tag");
	return TEXT("Indretning");
}

void ACampaign1851ConstructionSite::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	// -CampaignAutoBuild: raise the whole complex, module after module (for demos and captures).
	static const bool bAutoBuild = FParse::Param(FCommandLine::Get(), TEXT("CampaignAutoBuild"));
	if (bAutoBuild && Active == INDEX_NONE)
	{
		for (int32 m = 1; m < NumModules(); ++m)
		{
			if (CanStartModule(m))
			{
				StartModule(m);
				break;
			}
		}
	}
	if (Active != INDEX_NONE)
	{
		Elapsed[Active] = FMath::Min(Elapsed[Active] + DeltaSeconds * DaysPerSecond, ModuleDays(Active));
		if (IsModuleDone(Active))
		{
			// Let the last frame show the finished state before the next project can start.
			Apply(DeltaSeconds);
			Active = INDEX_NONE;
			return;
		}
	}
	Apply(DeltaSeconds);
}

void ACampaign1851ConstructionSite::SetClip(UMaterialInstanceDynamic* Mid, float PieceHeight) const
{
	if (Mid)
	{
		Mid->SetScalarParameterValue(BuildTopParam, float(GetActorLocation().Z) + PieceHeight * float(GetActorScale3D().Z));
	}
}

void ACampaign1851ConstructionSite::Apply(float DeltaSeconds)
{
	for (int32 m = 0; m < Buildings.Num(); ++m)
	{
		const FModuleDef& Def = Modules[m];
		const bool bStarted = IsModuleStarted(m);
		const float P = GetModuleProgress(m);
		Buildings[m]->SetVisibility(bStarted);

		// Plinth, then the walls storey by storey, then the roof (and chimneys).
		float Top = -1.f;
		if (bStarted && P >= StakedEnd)
		{
			Top = FMath::Lerp(0.f, PlinthTop, Phase(P, StakedEnd, FoundationEnd));
			Top = FMath::Lerp(Top, Def.Eave, Phase(P, FoundationEnd, WallsEnd));
			Top = FMath::Lerp(Top, Def.Top, Phase(P, WallsEnd, RoofEnd));
		}
		SetClip(BuildingMids[m], Top);

		// Scaffold: goes up ahead of the walls, comes down top first while the building is fitted out.
		const bool bScaffold = bStarted && P >= StakedEnd && P < 1.f;
		Scaffolds[m]->SetVisibility(bScaffold);
		const float ScaffoldTop = P < RoofEnd ? FMath::Min(Top + 2.4f, Def.Top + 1.f) : FMath::Lerp(Def.Top + 1.f, 0.f, Phase(P, RoofEnd, 1.f));
		SetClip(ScaffoldMids[m], FMath::Max(ScaffoldTop, 2.2f * Phase(P, StakedEnd, StakedEnd + 0.04f)));
	}

	// Crane and supply wagon work for whichever module is being built.
	const float P = Active != INDEX_NONE ? GetModuleProgress(Active) : 1.f;
	const bool bCrane = Active != INDEX_NONE && P >= FoundationEnd && P < RoofEnd;
	CraneMast->SetVisibility(bCrane);
	CraneJib->SetVisibility(bCrane);
	if (bCrane)
	{
		// Stand the crane at the working module's corner, jib swinging over it.
		const FModuleDef& Def = Modules[Active];
		const FVector Corner = FRotator(0.f, Def.Yaw, 0.f).RotateVector(FVector(-Def.Length * 0.5 - 2.4, -Def.Width * 0.5 - 2.0, 0.0));
		CraneMast->SetRelativeLocation(FVector(Def.Slot.X, Def.Slot.Y, 0.0) + Corner);
		CraneJib->SetRelativeLocation(FVector(Def.Slot.X, Def.Slot.Y, 0.0) + Corner);
		CraneJib->SetRelativeRotation(FRotator(0.f, Def.Yaw + 35.f + 45.f * FMath::Sin(Clock * 0.45f), 0.f));
	}
	const bool bWagon = Active != INDEX_NONE && P >= 0.03f && P < RoofEnd && WagonPath.Num() >= 2;
	Wagon->SetVisibility(bWagon);
	if (bWagon)
	{
		MoveWagon(DeltaSeconds);
	}

	// Dannebrog goes up the pole when the barracks is finished, then flies.
	const float Barracks = GetModuleProgress(0);
	const float Hoist = Phase(Barracks, RoofEnd, 1.f);
	Flag->SetVisibility(Barracks >= RoofEnd);
	Flag->SetRelativeLocation(FVector(0.15, 12.4, FMath::Lerp(1.2f, 11.4f, Hoist)));
	Flag->SetRelativeRotation(FRotator(0.f, 8.f * FMath::Sin(Clock * 2.1f) + 4.f * FMath::Sin(Clock * 3.7f), 0.f));
}

void ACampaign1851ConstructionSite::MoveWagon(float DeltaSeconds)
{
	const float Length = WagonDistance.Last();
	if (WagonPause > 0.f)
	{
		WagonPause -= DeltaSeconds;
	}
	else
	{
		WagonAt += WagonDirection * WagonSpeed * DeltaSeconds;
		if (WagonAt >= Length || WagonAt <= 0.f)
		{
			WagonAt = FMath::Clamp(WagonAt, 0.f, Length);
			WagonDirection = -WagonDirection;
			WagonPause = 1.5f;
		}
	}
	int32 i = 1;
	while (i < WagonDistance.Num() - 1 && WagonDistance[i] < WagonAt)
	{
		++i;
	}
	const float Span = FMath::Max(WagonDistance[i] - WagonDistance[i - 1], 1e-3f);
	const FVector At = FMath::Lerp(WagonPath[i - 1], WagonPath[i], (WagonAt - WagonDistance[i - 1]) / Span);
	const FVector Dir = (WagonPath[i] - WagonPath[i - 1]) * WagonDirection;
	Wagon->SetWorldLocationAndRotation(At, FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)), 0.f));
}

void ACampaign1851ConstructionSite::RestoreState(const TArray<float>& InDays, int32 InActive)
{
	for (int32 m = 0; m < NumModules(); ++m)
	{
		Elapsed[m] = InDays.IsValidIndex(m) ? FMath::Min(InDays[m], ModuleDays(m)) : -1.f;
	}
	Active = (Elapsed.IsValidIndex(InActive) && IsModuleStarted(InActive) && !IsModuleDone(InActive)) ? InActive : INDEX_NONE;
	Apply(0.f);
}
