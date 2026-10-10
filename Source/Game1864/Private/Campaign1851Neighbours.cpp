// Partial, data-driven neighbours; no foreign recruitment through Denmark's economy.
#include "Campaign1851Map.h"
#include "Campaign1851Scenery.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool ACampaign1851Map::LoadNeighbours(FJsonObject& MapJson)
{
	const FString NeighbourDir = FPaths::ProjectDir() / TEXT("Data/Campaign1851");
	NeighbourArmyAtStart.Reset();
	NeighbourOwnerIds.Reset();
	NeighbourColours.Reset();
	FString NeighbourText;
	TSharedPtr<FJsonObject> NeighbourJson;
	if (!FFileHelper::LoadFileToString(NeighbourText, *(NeighbourDir / (TEXT("cities_neighbours_") + ActiveScenario().Id + TEXT(".json"))))
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(NeighbourText), NeighbourJson) || !NeighbourJson.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-1851|neighbours|ingen scenariedata; bevarer oprindeligt kort"));
		return false;
	}
	const TSharedPtr<FJsonObject> NeighbourGrid = NeighbourJson->GetObjectField(TEXT("ownerGrid"));
	NeighbourGrid->TryGetNumberField(TEXT("width"), NeighbourOwnerW);
	NeighbourGrid->TryGetNumberField(TEXT("height"), NeighbourOwnerH);
	if (NeighbourOwnerW <= 0 || NeighbourOwnerH <= 0 || NeighbourOwnerW > 2048 || NeighbourOwnerH > 2048
		|| !FFileHelper::LoadFileToArray(NeighbourOwners, *(NeighbourDir / TEXT("Neighbours_Owners.bin")))
		|| NeighbourOwners.Num() != NeighbourOwnerW * NeighbourOwnerH)
	{
		NeighbourOwners.Reset();
		UE_LOG(LogTemp, Error, TEXT("CAMPAIGN-1851|neighbours|ugyldigt ejergrid"));
		return false;
	}
	const TSharedPtr<FJsonObject> NeighbourIds = NeighbourGrid->GetObjectField(TEXT("ids"));
	for (const auto& NeighbourPair : NeighbourIds->Values)
	{
		NeighbourOwnerIds.Add(uint8(NeighbourPair.Value->AsNumber()), FString(NeighbourPair.Key.ToView()));
	}
	for (const TSharedPtr<FJsonValue>& NeighbourValue : NeighbourJson->GetArrayField(TEXT("nations")))
	{
		const TSharedPtr<FJsonObject> NeighbourRow = NeighbourValue->AsObject();
		const FString NeighbourId = NeighbourRow->GetStringField(TEXT("id"));
		const TArray<TSharedPtr<FJsonValue>>& NeighbourRGB = NeighbourRow->GetArrayField(TEXT("colour"));
		NeighbourColours.Add(NeighbourId, FLinearColor::FromSRGBColor(FColor(uint8(NeighbourRGB[0]->AsNumber()), uint8(NeighbourRGB[1]->AsNumber()), uint8(NeighbourRGB[2]->AsNumber()))));
		if (NeighbourId != TEXT("DK"))
		{
			FCampaign1851Label& NeighbourLabel = Labels.AddDefaulted_GetRef();
			NeighbourLabel.Text = NeighbourRow->GetStringField(TEXT("name"));
			NeighbourLabel.Lat = NeighbourRow->GetNumberField(TEXT("lat"));
			NeighbourLabel.Lon = NeighbourRow->GetNumberField(TEXT("lon"));
			NeighbourLabel.Kind = TEXT("foreign");
		}
	}
	for (const TSharedPtr<FJsonValue>& NeighbourValue : NeighbourJson->GetArrayField(TEXT("cities")))
	{
		const TSharedPtr<FJsonObject> NeighbourRow = NeighbourValue->AsObject();
		const FString NeighbourName = NeighbourRow->GetStringField(TEXT("name"));
		int32 NeighbourIndex = FindCity(NeighbourName);
		if (NeighbourIndex != INDEX_NONE && !Cities[NeighbourIndex].bForeign) { continue; }
		if (NeighbourIndex == INDEX_NONE)
		{
			NeighbourIndex = Cities.AddDefaulted(); // append only: old save indices remain stable
			Cities[NeighbourIndex].Name = NeighbourName;
		}
		FCampaign1851City& NeighbourCity = Cities[NeighbourIndex];
		NeighbourCity.bForeign = true;
		NeighbourCity.Lat = NeighbourRow->GetNumberField(TEXT("lat"));
		NeighbourCity.Lon = NeighbourRow->GetNumberField(TEXT("lon"));
		NeighbourCity.Population = int32(NeighbourRow->GetNumberField(TEXT("pop")));
		NeighbourCity.NationId = NeighbourRow->GetStringField(TEXT("owner"));
		NeighbourRow->TryGetStringField(TEXT("region"), NeighbourCity.Region);
		NeighbourRow->TryGetStringField(TEXT("fortName"), NeighbourCity.NeighbourFortName);
		NeighbourRow->TryGetBoolField(TEXT("harbour"), NeighbourCity.bHarbour);
		NeighbourRow->TryGetBoolField(TEXT("fortress"), NeighbourCity.bNeighbourFortress);
		NeighbourRow->TryGetNumberField(TEXT("garrison"), NeighbourCity.NeighbourGarrison);
		NeighbourRow->TryGetNumberField(TEXT("guns"), NeighbourCity.NeighbourGuns);
	}
	// Merge JSON network before its normal loader; do not reset or renumber existing links.
	TArray<TSharedPtr<FJsonValue>> NeighbourLinks = MapJson.GetArrayField(TEXT("links"));
	TArray<TSharedPtr<FJsonValue>> NeighbourRoads = MapJson.GetArrayField(TEXT("roads"));
	for (const TSharedPtr<FJsonValue>& NeighbourValue : NeighbourJson->GetArrayField(TEXT("links")))
	{
		NeighbourLinks.Add(NeighbourValue);
		TSharedPtr<FJsonObject> NeighbourRoad = MakeShared<FJsonObject>();
		NeighbourRoad->SetStringField(TEXT("kind"), NeighbourValue->AsObject()->GetNumberField(TEXT("ferryKm")) > 0 ? TEXT("ferry") : TEXT("main"));
		NeighbourRoad->SetArrayField(TEXT("km"), NeighbourValue->AsObject()->GetArrayField(TEXT("km")));
		NeighbourRoads.Add(MakeShared<FJsonValueObject>(NeighbourRoad));
	}
	MapJson.SetArrayField(TEXT("links"), MoveTemp(NeighbourLinks));
	MapJson.SetArrayField(TEXT("roads"), MoveTemp(NeighbourRoads));
	TArray<TSharedPtr<FJsonValue>> NeighbourRails = MapJson.GetArrayField(TEXT("railways"));
	NeighbourRails.Append(NeighbourJson->GetArrayField(TEXT("railways")));
	MapJson.SetArrayField(TEXT("railways"), MoveTemp(NeighbourRails));
	NeighbourText.Reset();
	TSharedPtr<FJsonObject> NeighbourArmyJson;
	if (FFileHelper::LoadFileToString(NeighbourText, *(NeighbourDir / (TEXT("Army_Neighbours_") + ActiveScenario().Id + TEXT(".json"))))
		&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(NeighbourText), NeighbourArmyJson) && NeighbourArmyJson.IsValid())
	{
		for (const TSharedPtr<FJsonValue>& NeighbourValue : NeighbourArmyJson->GetArrayField(TEXT("armies")))
		{
			const TSharedPtr<FJsonObject> NeighbourRow = NeighbourValue->AsObject();
			FCampaign1851NeighbourArmy NeighbourArmy;
			NeighbourArmy.Id = NeighbourRow->GetStringField(TEXT("id"));
			NeighbourArmy.Name = NeighbourRow->GetStringField(TEXT("name"));
			NeighbourArmy.Nation = NeighbourRow->GetStringField(TEXT("nation"));
			NeighbourArmy.Town = FindCity(NeighbourRow->GetStringField(TEXT("town")));
			NeighbourRow->TryGetNumberField(TEXT("men"), NeighbourArmy.Men);
			NeighbourRow->TryGetNumberField(TEXT("mobilisedMen"), NeighbourArmy.MobilisedMen);
			NeighbourRow->TryGetNumberField(TEXT("guns"), NeighbourArmy.Guns);
			NeighbourRow->TryGetBoolField(TEXT("garrison"), NeighbourArmy.bGarrison);
			if (Cities.IsValidIndex(NeighbourArmy.Town)) { NeighbourArmyAtStart.Add(NeighbourArmy); }
		}
	}
	UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|neighbours|scenarie=%s|byer=%d|hære=%d|grid=%dx%d"), *ActiveScenario().Id, NeighbourJson->GetArrayField(TEXT("cities")).Num(), NeighbourArmyAtStart.Num(), NeighbourOwnerW, NeighbourOwnerH);
	return true;
}

FString ACampaign1851Map::TerritoryNation(const FVector2D& Km) const
{
	if (IsMonarchyLand(Km)) { return TEXT("DK"); }
	const FVector2D NeighbourUv = UvFromKm(Km);
	if (NeighbourUv.X < 0 || NeighbourUv.X >= 1 || NeighbourUv.Y < 0 || NeighbourUv.Y >= 1) { return FString(); }
	if (NeighbourOwners.Num() == 0) { return IsMonarchyLand(Km) ? FString(TEXT("DK")) : FString(); }
	const int32 NeighbourX = FMath::Clamp(int32(NeighbourUv.X * NeighbourOwnerW), 0, NeighbourOwnerW - 1);
	const int32 NeighbourY = FMath::Clamp(int32((1 - NeighbourUv.Y) * NeighbourOwnerH), 0, NeighbourOwnerH - 1);
	const FString* NeighbourId = NeighbourOwnerIds.Find(NeighbourOwners[NeighbourY * NeighbourOwnerW + NeighbourX]);
	return NeighbourId ? *NeighbourId : FString();
}

FLinearColor ACampaign1851Map::NeighbourColour(const FString& NationId) const
{
	const FLinearColor* NeighbourColourValue = NeighbourColours.Find(NationId);
	return NeighbourColourValue ? *NeighbourColourValue : FLinearColor(0.4f, 0.4f, 0.4f);
}

bool ACampaign1851Map::IsNeighbourHostile(const FString& NationId) const
{
	return bAtWar && (NationId == TEXT("PR") || NationId == TEXT("AT") || NationId == TEXT("DE"));
}

bool ACampaign1851Map::CanEnterNation(const FString& MovingNation, const FString& LandNation) const
{
	if (LandNation.IsEmpty()) { return false; }
	if (MovingNation == LandNation) { return true; }
	if (MovingNation == TEXT("DK"))
	{
		if (IsNeighbourHostile(LandNation)) { return true; }
		const FCampaign1851Nation* NeighbourNation = Nations.FindByPredicate([&](const FCampaign1851Nation& N) { return N.Id == LandNation; });
		return NeighbourNation && NeighbourNation->bAlliance; // alliance grants mutual passage
	}
	// The existing event-driven federal expedition already has passage in the duchies.
	if (LandNation == TEXT("DK")) { return IsNeighbourHostile(MovingNation) || MovingNation == TEXT("DE"); }
	// Federal belligerents use the northern German military transit corridor; not neutral Sweden.
	return IsNeighbourHostile(MovingNation) && (LandNation == TEXT("MEC") || LandNation == TEXT("HAN") || LandNation == TEXT("HH") || LandNation == TEXT("LUB") || LandNation == TEXT("OLD") || LandNation == TEXT("BRE") || LandNation == TEXT("PR"));
}

bool ACampaign1851Map::CanEnterLine(const FString& MovingNation, const TArray<FVector2D>& Line) const
{
	if (Line.Num() == 1) { const FString NeighbourLand = TerritoryNation(Line[0]); return NeighbourLand.IsEmpty() || CanEnterNation(MovingNation, NeighbourLand); }
	for (int32 NeighbourPoint = 0; NeighbourPoint + 1 < Line.Num(); ++NeighbourPoint)
	{
		const int32 NeighbourSteps = FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(Line[NeighbourPoint], Line[NeighbourPoint + 1]) / 0.5));
		for (int32 NeighbourStep = 0; NeighbourStep <= NeighbourSteps; ++NeighbourStep)
		{
			const FString NeighbourId = TerritoryNation(FMath::Lerp(Line[NeighbourPoint], Line[NeighbourPoint + 1], double(NeighbourStep) / NeighbourSteps));
			// Water is a physical routing concern (explicit ferries or IsDryLine), not a nation.
			if (!NeighbourId.IsEmpty() && !CanEnterNation(MovingNation, NeighbourId)) { return false; }
		}
	}
	return true;
}

bool ACampaign1851Map::NeighbourDetailsKnown(int32 Town) const
{
	if (!Cities.IsValidIndex(Town)) { return false; }
	for (const FCampaign1851Regiment& NeighbourScout : Regiments)
	{
		const float NeighbourSight = NeighbourScout.Arm == ECampaign1851Arm::Cavalry ? 20.f : 8.f;
		if (NeighbourScout.Men > 0 && FVector2D::Distance(NeighbourScout.Km, TownKm(Town)) < NeighbourSight) { return true; }
	}
	for (const FCampaign1851Fort& NeighbourScoutFort : Forts)
	{
		if (NeighbourScoutFort.bBuilt && NeighbourScoutFort.Garrison > 0 && FVector2D::Distance(NeighbourScoutFort.Km, TownKm(Town)) < 8.0) { return true; }
	}
	return false;
}

FString ACampaign1851Map::NeighbourTownInfo(int32 Town) const
{
	if (!Cities.IsValidIndex(Town)) { return FString(); }
	const FCampaign1851City& NeighbourCity = Cities[Town];
	int32 NeighbourMen = 0, NeighbourFieldMen = 0;
	for (const FCampaign1851NeighbourArmy& NeighbourArmy : NeighbourArmies)
	{
		if (NeighbourArmy.Town != Town) { continue; }
		const FCampaign1851EnemyCorps* NeighbourCorps = EnemyCorps.FindByPredicate([&](const FCampaign1851EnemyCorps& C) { return C.Id == NeighbourArmy.CorpsId; });
		const int32 NeighbourPresent = NeighbourArmy.CorpsId ? (NeighbourCorps && FVector2D::Distance(NeighbourCorps->Km, TownKm(Town)) < 8.0 ? NeighbourCorps->Men : 0) : NeighbourArmy.Men;
		if (NeighbourArmy.bGarrison) { NeighbourMen += NeighbourPresent; }
		else { NeighbourFieldMen += NeighbourPresent; }
	}
	return FString::Printf(TEXT("%s · %s"), NeighbourCity.bNeighbourFortress ? *NeighbourCity.NeighbourFortName : TEXT("Ingen aktiv fæstning"),
		NeighbourDetailsKnown(Town) ? *FString::Printf(TEXT("garnison ca. %d; felt ca. %d (skøn)"), FMath::RoundToInt(NeighbourMen / 100.f) * 100, FMath::RoundToInt(NeighbourFieldMen / 100.f) * 100) : TEXT("garnison/felthær: ukendt"));
}

void ACampaign1851Map::ResetNeighbourArmies()
{
	NeighbourArmies = NeighbourArmyAtStart;
}

void ACampaign1851Map::AdvanceNeighbourArmies()
{
	for (FCampaign1851NeighbourArmy& NeighbourArmy : NeighbourArmies)
	{
		if (!IsNeighbourHostile(NeighbourArmy.Nation) || NeighbourArmy.CorpsId || !Cities.IsValidIndex(NeighbourArmy.Town)) { continue; }
		// Local garrisons hold their town. The field corps mobilises once, not once per month.
		FCampaign1851EnemyCorps NeighbourCorps;
		NeighbourCorps.Id = NextCorpsId++;
		NeighbourArmy.CorpsId = NeighbourCorps.Id;
		NeighbourCorps.Name = NeighbourArmy.Name;
		NeighbourCorps.Nation = NeighbourArmy.Nation;
		NeighbourCorps.Town = NeighbourArmy.Town;
		NeighbourCorps.Km = TownKm(NeighbourArmy.Town);
		NeighbourCorps.Men = NeighbourArmy.bGarrison ? NeighbourArmy.Men : NeighbourArmy.MobilisedMen;
		NeighbourCorps.StartMen = NeighbourCorps.Men;
		if (!NeighbourArmy.bGarrison)
		{
			const int32 NeighbourBorderGoal = FindCity(TEXT("Ratzeburg"));
			if (NeighbourBorderGoal != INDEX_NONE) { NeighbourCorps.Objectives.Add(NeighbourBorderGoal); }
		}
		NeighbourCorps.Guns = NeighbourArmy.Guns;
		NeighbourCorps.bSeen = false;
		NeighbourCorps.SeenDay = -1;
		EnemyCorps.Add(NeighbourCorps);
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|neighbours|mobilisering|%s|%s"), *NeighbourArmy.Nation, *NeighbourArmy.Name);
	}
}

TArray<FString> ACampaign1851Map::SaveNeighbours() const
{
	TArray<FString> NeighbourLines;
	for (const FCampaign1851NeighbourArmy& NeighbourArmy : NeighbourArmies)
	{
		if (NeighbourArmy.CorpsId) { NeighbourLines.Add(FString::Printf(TEXT("%s|%d"), *NeighbourArmy.Id, NeighbourArmy.CorpsId)); }
	}
	return NeighbourLines;
}

void ACampaign1851Map::RestoreNeighbours(const TArray<FString>& Lines)
{
	ResetNeighbourArmies();
	// Pre-v32 saves have no foreign stock records; preserve any saved (including empty) magazines.
	for (int32 NeighbourDepotTown = 0; NeighbourDepotTown < Cities.Num(); ++NeighbourDepotTown)
	{
		if (Cities[NeighbourDepotTown].NationId.IsEmpty() || !Cities[NeighbourDepotTown].bForeign || Depots.Contains(NeighbourDepotTown)) { continue; }
		const FCampaign1851DepotCapacity NeighbourDepotCap = DepotCapacity(NeighbourDepotTown);
		Depots.Add(NeighbourDepotTown, { NeighbourDepotCap.Food, NeighbourDepotCap.Fodder, NeighbourDepotCap.Ammo });
	}
	for (const FString& NeighbourLine : Lines)
	{
		TArray<FString> NeighbourParts;
		NeighbourLine.ParseIntoArray(NeighbourParts, TEXT("|"), false);
		if (NeighbourParts.Num() != 2) { continue; }
		FCampaign1851NeighbourArmy* NeighbourArmy = NeighbourArmies.FindByPredicate([&](const FCampaign1851NeighbourArmy& A) { return A.Id == NeighbourParts[0]; });
		if (NeighbourArmy) { NeighbourArmy->CorpsId = FCString::Atoi(*NeighbourParts[1]); }
	}
}

void ACampaign1851Map::SetNeighbourPoliticalView(bool bPolitical)
{
	for (UStaticMeshComponent* NeighbourMesh : NeighbourTerritoryMeshes) { if (NeighbourMesh) { NeighbourMesh->SetVisibility(bPolitical); } }
}

void ACampaign1851Map::BuildNeighbourMeshes()
{
	UMaterialInterface* NeighbourMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Campaign1851/M_Campaign1851Scenery.M_Campaign1851Scenery"));
	if (!NeighbourMaterial || NeighbourOwners.Num() == 0) { return; }
	const double NeighbourDx = SizeKm.X / NeighbourOwnerW, NeighbourDy = SizeKm.Y / NeighbourOwnerH;
	for (const auto& NeighbourNation : NeighbourOwnerIds)
	{
		TArray<TArray<FVector>> NeighbourStrips, NeighbourBorders;
		for (int32 NeighbourY = 0; NeighbourY < NeighbourOwnerH; ++NeighbourY)
		{
			for (int32 NeighbourX = 0; NeighbourX < NeighbourOwnerW; )
			{
				if (NeighbourOwners[NeighbourY * NeighbourOwnerW + NeighbourX] != NeighbourNation.Key) { ++NeighbourX; continue; }
				const int32 NeighbourStartX = NeighbourX;
				while (NeighbourX < NeighbourOwnerW && NeighbourOwners[NeighbourY * NeighbourOwnerW + NeighbourX] == NeighbourNation.Key) { ++NeighbourX; }
				TArray<FVector>& NeighbourStrip = NeighbourStrips.AddDefaulted_GetRef();
				for (int32 NeighbourSample = NeighbourStartX; NeighbourSample <= NeighbourX; ++NeighbourSample)
				{
					NeighbourStrip.Add(LocalAtKm(FVector2D(Extent.XMin + NeighbourSample * NeighbourDx, Extent.YMax - (NeighbourY + 0.5) * NeighbourDy)) + FVector(0, 0, 1.1));
				}
			}
		}
		for (int32 NeighbourY = 0; NeighbourY + 1 < NeighbourOwnerH; ++NeighbourY)
		{
			for (int32 NeighbourX = 0; NeighbourX + 1 < NeighbourOwnerW; ++NeighbourX)
			{
				const uint8 NeighbourId = NeighbourOwners[NeighbourY * NeighbourOwnerW + NeighbourX];
				if (NeighbourId != NeighbourNation.Key) { continue; }
				const FVector2D NeighbourCorner(Extent.XMin + (NeighbourX + 1) * NeighbourDx, Extent.YMax - (NeighbourY + 1) * NeighbourDy);
				const uint8 NeighbourRight = NeighbourOwners[NeighbourY * NeighbourOwnerW + NeighbourX + 1];
				const uint8 NeighbourBelow = NeighbourOwners[(NeighbourY + 1) * NeighbourOwnerW + NeighbourX];
				if (NeighbourRight && NeighbourRight != NeighbourId) { NeighbourBorders.Add({ LocalAtKm(NeighbourCorner) + FVector(0,0,2.5), LocalAtKm(NeighbourCorner + FVector2D(0,NeighbourDy)) + FVector(0,0,2.5) }); }
				if (NeighbourBelow && NeighbourBelow != NeighbourId) { NeighbourBorders.Add({ LocalAtKm(NeighbourCorner) + FVector(0,0,2.5), LocalAtKm(NeighbourCorner - FVector2D(NeighbourDx,0)) + FVector(0,0,2.5) }); }
			}
		}
		for (int32 NeighbourBorderPass = 0; NeighbourBorderPass < 2; ++NeighbourBorderPass)
		{
			const FString NeighbourMeshName = TEXT("Neighbour_") + NeighbourNation.Value + (NeighbourBorderPass ? TEXT("_Border") : TEXT("_Political"));
			UStaticMeshComponent* NeighbourMesh = NewObject<UStaticMeshComponent>(this, FName(*NeighbourMeshName));
			NeighbourMesh->SetupAttachment(Root);
			NeighbourMesh->SetStaticMesh(Campaign1851Scenery::BuildRibbons(NeighbourBorderPass ? NeighbourBorders : NeighbourStrips,
				float(NeighbourBorderPass ? 0.08 * KmToUnits : NeighbourDy * 0.5 * KmToUnits),
				NeighbourBorderPass ? FLinearColor(0.12f,0.10f,0.08f) : NeighbourColour(NeighbourNation.Value), NeighbourMaterial, *NeighbourMeshName));
			NeighbourMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			NeighbourMesh->SetCastShadow(false);
			NeighbourMesh->SetVisibility(NeighbourBorderPass != 0);
			NeighbourMesh->RegisterComponent();
			if (!NeighbourBorderPass) { NeighbourTerritoryMeshes.Add(NeighbourMesh); }
			else { RiverMeshes.Add(NeighbourMesh); }
		}
	}
}
