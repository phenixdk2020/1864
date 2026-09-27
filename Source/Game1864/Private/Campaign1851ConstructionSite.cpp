#include "Campaign1851ConstructionSite.h"

#include "Campaign1851Scenery.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	// Stage boundaries (fraction of the project) and heights (piece units above the site origin).
	constexpr float StakedEnd = 0.08f, FoundationEnd = 0.2f, WallsEnd = 0.8f, RoofEnd = 0.95f;
	constexpr float PlinthTop = 0.6f;
	constexpr float WagonSpeed = 22.f;   // world units per second
	const FName BuildTopParam(TEXT("BuildTop"));

	float Phase(float P, float A, float B) { return FMath::Clamp((P - A) / (B - A), 0.f, 1.f); }
}

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
	Building = Part(TEXT("Building"));
	Scaffold = Part(TEXT("Scaffold"));
	CraneMast = Part(TEXT("CraneMast"));
	CraneJib = Part(TEXT("CraneJib"));
	Wagon = Part(TEXT("Wagon"));
	Flagpole = Part(TEXT("Flagpole"));
	Flag = Part(TEXT("Flag"));
}

void ACampaign1851ConstructionSite::Setup(int32 InCityIndex, const FString& InName, const TArray<FVector>& InWagonPath, UMaterialInterface* Material)
{
	using namespace Campaign1851Scenery;
	CityIndex = InCityIndex;
	BuildingName = InName;
	WagonPath = InWagonPath;
	WagonDistance.Reset();
	float Total = 0.f;
	for (int32 i = 0; i < WagonPath.Num(); ++i)
	{
		Total += i > 0 ? float(FVector::Dist(WagonPath[i - 1], WagonPath[i])) : 0.f;
		WagonDistance.Add(Total);
	}

	Ground->SetStaticMesh(BuildSitePiece(ESitePiece::Ground, Material));
	Building->SetStaticMesh(BuildSitePiece(ESitePiece::Barracks, Material));
	Scaffold->SetStaticMesh(BuildSitePiece(ESitePiece::Scaffold, Material));
	CraneMast->SetStaticMesh(BuildSitePiece(ESitePiece::CraneMast, Material));
	CraneJib->SetStaticMesh(BuildSitePiece(ESitePiece::CraneJib, Material));
	Wagon->SetStaticMesh(BuildSitePiece(ESitePiece::Wagon, Material));
	Flagpole->SetStaticMesh(BuildSitePiece(ESitePiece::Flagpole, Material));
	Flag->SetStaticMesh(BuildSitePiece(ESitePiece::Flag, Material));

	CraneMast->SetRelativeLocation(FVector(-10.4, -4.8, 0.0));
	CraneJib->SetRelativeLocation(FVector(-10.4, -4.8, 0.0));
	Flagpole->SetRelativeLocation(FVector(0.0, 12.4, 0.0));

	BuildingMid = UMaterialInstanceDynamic::Create(Material, this);
	ScaffoldMid = UMaterialInstanceDynamic::Create(Material, this);
	Building->SetMaterial(0, BuildingMid);
	Scaffold->SetMaterial(0, ScaffoldMid);
	Apply(0.f);
}

FString ACampaign1851ConstructionSite::GetStageName() const
{
	const float P = GetProgress();
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
	ElapsedDays = FMath::Min(ElapsedDays + DeltaSeconds * DaysPerSecond, DurationDays);
	Clock += DeltaSeconds;
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
	using namespace Campaign1851Scenery;
	const float P = GetProgress();

	// Building: plinth, then the walls storey by storey, then roof and chimneys.
	float Top = -1.f;
	if (P >= StakedEnd)
	{
		Top = FMath::Lerp(0.f, PlinthTop, Phase(P, StakedEnd, FoundationEnd));
		Top = FMath::Lerp(Top, BarracksEave, Phase(P, FoundationEnd, WallsEnd));
		Top = FMath::Lerp(Top, BarracksTop, Phase(P, WallsEnd, RoofEnd));
	}
	SetClip(BuildingMid, Top);

	// Scaffold: goes up ahead of the walls, comes down top first while the building is fitted out.
	const bool bScaffold = P >= StakedEnd && P < 1.f;
	Scaffold->SetVisibility(bScaffold);
	const float ScaffoldTop = P < RoofEnd ? FMath::Min(Top + 2.4f, BarracksTop + 1.f) : FMath::Lerp(BarracksTop + 1.f, 0.f, Phase(P, RoofEnd, 1.f));
	SetClip(ScaffoldMid, FMath::Max(ScaffoldTop, 2.2f * Phase(P, StakedEnd, StakedEnd + 0.04f)));

	// Crane swings while the walls and roof go up.
	const bool bCrane = P >= FoundationEnd && P < RoofEnd;
	CraneMast->SetVisibility(bCrane);
	CraneJib->SetVisibility(bCrane);
	CraneJib->SetRelativeRotation(FRotator(0.f, 35.f + 45.f * FMath::Sin(Clock * 0.45f), 0.f));

	// Materials go out from the town while there is work to do.
	const bool bWagon = P >= 0.03f && P < RoofEnd && WagonPath.Num() >= 2;
	Wagon->SetVisibility(bWagon);
	if (bWagon)
	{
		MoveWagon(DeltaSeconds);
	}

	// Dannebrog goes up the pole when the barracks is finished, then flies.
	const float Hoist = Phase(P, RoofEnd, 1.f);
	Flag->SetVisibility(P >= RoofEnd);
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
