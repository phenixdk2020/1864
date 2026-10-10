// Partial, data-driven neighbours; no foreign recruitment through Denmark's economy.
#include "Campaign1851Map.h"
#include "Campaign1851Scenery.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Materials/MaterialInterface.h"
#include "Math/NumericLimits.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool ACampaign1851Map::LoadNeighbours(FJsonObject& MapJson)
{
	const FString NeighbourDir = FPaths::ProjectDir() / TEXT("Data/Campaign1851");
	NeighbourArmyAtStart.Reset();
	NeighbourDistricts.Reset();
	NeighbourDistrictGrid.Reset();
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
	const TArray<TSharedPtr<FJsonValue>>* SouthRows = nullptr;
	if (NeighbourJson->TryGetArrayField(TEXT("districts"), SouthRows))
	{
		for (const TSharedPtr<FJsonValue>& SouthValue : *SouthRows)
		{
			const TSharedPtr<FJsonObject> SouthRow = SouthValue->AsObject();
			FCampaign1851NeighbourDistrict& SouthDistrict = NeighbourDistricts.AddDefaulted_GetRef();
			SouthDistrict.Id = SouthRow->GetStringField(TEXT("id"));
			SouthDistrict.Name = SouthRow->GetStringField(TEXT("name"));
			SouthDistrict.Nation = SouthRow->GetStringField(TEXT("nation"));
			SouthDistrict.Region = SouthRow->GetStringField(TEXT("region"));
			// Seats are resolved after neighbour towns have been appended below.
			SouthRow->TryGetNumberField(TEXT("population"), SouthDistrict.Population);
			SouthRow->TryGetNumberField(TEXT("urban"), SouthDistrict.Urban);
			SouthRow->TryGetNumberField(TEXT("reserveShare"), SouthDistrict.StartShare);
			SouthRow->TryGetNumberField(TEXT("yearlyClassShare"), SouthDistrict.ClassShare);
			SouthRow->TryGetNumberField(TEXT("capShare"), SouthDistrict.CapShare);
			SouthRow->TryGetNumberField(TEXT("urbanTax"), SouthDistrict.UrbanTax);
			SouthRow->TryGetNumberField(TEXT("ruralTax"), SouthDistrict.RuralTax);
			SouthDistrict.Reserve = SouthDistrict.Population * SouthDistrict.StartShare;
		}
		TArray<uint8> SouthGridBytes;
		if (FFileHelper::LoadFileToArray(SouthGridBytes, *(NeighbourDir / TEXT("South_Districts.bin")))
			&& SouthGridBytes.Num() == NeighbourOwnerW * NeighbourOwnerH * 2)
		{
			for (int32 SouthByte = 0; SouthByte < SouthGridBytes.Num(); SouthByte += 2)
			{
				NeighbourDistrictGrid.Add(uint16(SouthGridBytes[SouthByte]) | (uint16(SouthGridBytes[SouthByte + 1]) << 8));
			}
		}
	}
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
		NeighbourRow->TryGetStringField(TEXT("district"), NeighbourCity.NeighbourDistrict);
		if (NeighbourCity.NationId == TEXT("DK"))
		{
			const int32 SouthDanishSeat = FindCity(TEXT("Ratzeburg"));
			if (Cities.IsValidIndex(SouthDanishSeat))
			{
				NeighbourCity.bForeign = false;
				NeighbourCity.AmtId = Cities[SouthDanishSeat].AmtId;
				NeighbourCity.Region = Cities[SouthDanishSeat].Region;
			}
		}
	}
	if (SouthRows)
	{
		for (int32 SouthIndex = 0; SouthIndex < SouthRows->Num(); ++SouthIndex)
		{
			FCampaign1851NeighbourDistrict& SouthDistrict = NeighbourDistricts[SouthIndex];
			SouthDistrict.Seat = FindCity((*SouthRows)[SouthIndex]->AsObject()->GetStringField(TEXT("seat")));
			SouthDistrict.Treasury = 3.0 * (SouthDistrict.Urban * SouthDistrict.UrbanTax + FMath::Max(0, SouthDistrict.Population - SouthDistrict.Urban) * SouthDistrict.RuralTax);
		}
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
			NeighbourRow->TryGetStringField(TEXT("district"), NeighbourArmy.District);
			NeighbourRow->TryGetStringField(TEXT("arm"), NeighbourArmy.Arm);
			NeighbourRow->TryGetStringField(TEXT("corps"), NeighbourArmy.ParentCorps);
			NeighbourRow->TryGetStringField(TEXT("chief"), NeighbourArmy.Chief);
			for (const TCHAR* SouthArmField : { TEXT("companies"), TEXT("squadrons"), TEXT("batteries") })
			{
				const TArray<TSharedPtr<FJsonValue>>* SouthUnits = nullptr;
				if (!NeighbourRow->TryGetArrayField(SouthArmField, SouthUnits)) { continue; }
				for (const TSharedPtr<FJsonValue>& SouthUnit : *SouthUnits)
				{
					const TSharedPtr<FJsonObject> SouthUnitRow = SouthUnit->AsObject();
					NeighbourArmy.Subunits.Add(FString::Printf(TEXT("%s: %d mand"), *SouthUnitRow->GetStringField(TEXT("name")), int32(SouthUnitRow->GetNumberField(TEXT("men")))));
				}
			}
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
	if (LandNation == TEXT("DK"))
	{
		const FCampaign1851Nation* SouthMovingNation = Nations.FindByPredicate([&](const FCampaign1851Nation& N) { return N.Id == MovingNation; });
		return IsNeighbourHostile(MovingNation) || MovingNation == TEXT("DE") || (SouthMovingNation && SouthMovingNation->bAlliance);
	}
	// Federal belligerents use the northern German military transit corridor; not neutral Sweden.
	return IsNeighbourHostile(MovingNation) && (LandNation == TEXT("MEC") || LandNation == TEXT("MST") || LandNation == TEXT("HAN") || LandNation == TEXT("HH") || LandNation == TEXT("LUB") || LandNation == TEXT("OLD") || LandNation == TEXT("BRE") || LandNation == TEXT("PR"));
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
	NeighbourNextMobilisationDay = -1.0;
}

FString ACampaign1851Map::NeighbourDistrictInfo(int32 Town) const
{
	if (!Cities.IsValidIndex(Town)) { return FString(); }
	const FCampaign1851City& SouthCity = Cities[Town];
	const FCampaign1851NeighbourDistrict* SouthDistrict = NeighbourDistricts.FindByPredicate([&](const FCampaign1851NeighbourDistrict& D) { return D.Id == SouthCity.NeighbourDistrict; });
	if (!SouthDistrict) { return TEXT("Distriktsdata: eksisterende monarki / ikke detaljeret"); }
	return FString::Printf(TEXT("%s · %s\nDistrikt ca. %d indb. · reserve %d (+%d/år)\nByskat ca. %d rd./år · distrikt ca. %d rd./år (skøn)"),
		*SouthDistrict->Name, *SouthDistrict->Region, SouthDistrict->Population, FMath::FloorToInt(SouthDistrict->Reserve),
		FMath::RoundToInt(SouthDistrict->Population * SouthDistrict->ClassShare), FMath::RoundToInt(SouthCity.Population * SouthDistrict->UrbanTax),
		FMath::RoundToInt(SouthDistrict->Urban * SouthDistrict->UrbanTax + FMath::Max(0, SouthDistrict->Population - SouthDistrict->Urban) * SouthDistrict->RuralTax));
}

FString ACampaign1851Map::NeighbourRegimentInfo(int32 Town) const
{
	if (!NeighbourDetailsKnown(Town)) { return TEXT("Regimenter og underenheder: ukendt uden opklaring"); }
	FString SouthInfo;
	for (const FCampaign1851NeighbourArmy& SouthArmy : NeighbourArmies)
	{
		if (SouthArmy.Town != Town || SouthArmy.ParentCorps.IsEmpty() || SouthArmy.CorpsId < 0) { continue; }
		const FCampaign1851EnemyCorps* SouthCorps = EnemyCorps.FindByPredicate([&](const FCampaign1851EnemyCorps& C) { return C.Id == SouthArmy.CorpsId; });
		if (SouthArmy.CorpsId && (!SouthCorps || FVector2D::Distance(SouthCorps->Km, TownKm(Town)) >= 8.0)) { continue; }
		SouthInfo += FString::Printf(TEXT("%s: %d mand · %d %s\n"), *SouthArmy.Name, SouthCorps ? SouthCorps->Men : SouthArmy.Men,
			SouthArmy.Subunits.Num(), SouthArmy.Arm == TEXT("cavalry") ? TEXT("eskadroner") : SouthArmy.Arm == TEXT("artillery") ? TEXT("batterier") : TEXT("kompagnier"));
	}
	return SouthInfo.IsEmpty() ? TEXT("Ingen registrerede feltregimenter i byen") : SouthInfo;
}

int32 ACampaign1851Map::TakeNeighbourRecruits(const FCampaign1851NeighbourArmy& Army, int32 Wanted)
{
	if (Wanted <= 0) { return 0; }
	// Northern armies retain the first pass's abstract manpower model.
	if (Army.District.IsEmpty()) { return Wanted; }
	int32 SouthTaken = 0;
	for (int32 SouthPass = 0; SouthPass < 2 && SouthTaken < Wanted; ++SouthPass)
	{
		for (FCampaign1851NeighbourDistrict& SouthDistrict : NeighbourDistricts)
		{
			if (SouthDistrict.Nation != Army.Nation || (SouthDistrict.Id == Army.District) != (SouthPass == 0)
				|| !Cities.IsValidIndex(SouthDistrict.Seat)) { continue; }
			TArray<FCampaign1851Leg> SouthDraftRoute;
			if (SouthDistrict.Seat != Army.Town && !PlanMarch(SouthDistrict.Seat, TownKm(SouthDistrict.Seat), Army.Town, TownKm(Army.Town),
				Campaign1851Supply::ColumnKmPerDay, ECampaign1851RouteMode::RoadsOnly, SouthDraftRoute, nullptr, Army.Nation)) { continue; }
			const int32 SouthDraft = FMath::Min(FMath::Min(int32(Wanted - SouthTaken), int32(FMath::FloorToInt(SouthDistrict.Reserve))),
				int32(FMath::FloorToInt(SouthDistrict.Treasury / Campaign1851Army::ReplacementCostPerMan)));
			if (SouthDraft <= 0) { continue; }
			SouthDistrict.Reserve -= SouthDraft;
			SouthDistrict.Treasury -= SouthDraft * Campaign1851Army::ReplacementCostPerMan;
			SouthTaken += SouthDraft;
			if (SouthTaken >= Wanted) { break; }
		}
	}
	return SouthTaken;
}

void ACampaign1851Map::MonthlyNeighbourDistricts()
{
	for (FCampaign1851NeighbourDistrict& SouthDistrict : NeighbourDistricts)
	{
		SouthDistrict.Reserve = FMath::Min(SouthDistrict.Reserve + SouthDistrict.Population * SouthDistrict.ClassShare / 12.f, SouthDistrict.Population * SouthDistrict.CapShare);
		SouthDistrict.Treasury += (SouthDistrict.Urban * SouthDistrict.UrbanTax + FMath::Max(0, SouthDistrict.Population - SouthDistrict.Urban) * SouthDistrict.RuralTax) / 12.0;
	}
	// Refill the same stock objects Denmark can use under an alliance, paid locally.
	for (int32 SouthTown = 0; SouthTown < Cities.Num(); ++SouthTown)
	{
		if (!Cities[SouthTown].bForeign || !Cities[SouthTown].Occupier.IsEmpty()) { continue; }
		FCampaign1851NeighbourDistrict* SouthDistrict = NeighbourDistricts.FindByPredicate([&](const FCampaign1851NeighbourDistrict& D) { return D.Id == Cities[SouthTown].NeighbourDistrict; });
		if (!SouthDistrict) { continue; }
		const FCampaign1851DepotCapacity SouthCap = DepotCapacity(SouthTown);
		FCampaign1851DepotStock& SouthStock = Depots.FindOrAdd(SouthTown);
		const float SouthFood = FMath::Max(0.f, SouthCap.Food - SouthStock.Food) * Campaign1851Supply::RefillShare;
		const float SouthFodder = FMath::Max(0.f, SouthCap.Fodder - SouthStock.Fodder) * Campaign1851Supply::RefillShare;
		const float SouthAmmo = FMath::Max(0.f, SouthCap.Ammo - SouthStock.Ammo) * Campaign1851Supply::RefillShare;
		const double SouthPrice = SouthFood * Campaign1851Supply::DepotPricePerRation + SouthFodder * Campaign1851Supply::FodderPrice + SouthAmmo * Campaign1851Supply::AmmoLoadPrice;
		if (SouthPrice <= 0.0) { continue; }
		const float SouthPurchaseShare = float(FMath::Clamp(SouthDistrict->Treasury / SouthPrice, 0.0, 1.0));
		SouthStock.Food += SouthFood * SouthPurchaseShare;
		SouthStock.Fodder += SouthFodder * SouthPurchaseShare;
		SouthStock.Ammo += SouthAmmo * SouthPurchaseShare;
		SouthDistrict->Treasury = FMath::Max(0.0, SouthDistrict->Treasury - SouthPrice * SouthPurchaseShare);
	}
}

float ACampaign1851Map::NeighbourMarchPace(int32 CorpsId) const
{
	const FCampaign1851NeighbourArmy* SouthArmy = NeighbourArmies.FindByPredicate([&](const FCampaign1851NeighbourArmy& A) { return A.CorpsId == CorpsId; });
	if (!SouthArmy || SouthArmy->Arm.IsEmpty()) { return 16.f; }
	const ECampaign1851Arm SouthArm = SouthArmy->Arm == TEXT("cavalry") ? ECampaign1851Arm::Cavalry : SouthArmy->Arm == TEXT("artillery") ? ECampaign1851Arm::Artillery : ECampaign1851Arm::Infantry;
	return Campaign1851Army::MarchKmPerDay(SouthArm);
}

void ACampaign1851Map::DailyNeighbourSupply()
{
	for (FCampaign1851NeighbourArmy& SouthArmy : NeighbourArmies)
	{
		if (SouthArmy.CorpsId <= 0 || SouthArmy.District.IsEmpty()) { continue; }
		FCampaign1851EnemyCorps* SouthCorps = EnemyCorps.FindByPredicate([&](const FCampaign1851EnemyCorps& C) { return C.Id == SouthArmy.CorpsId; });
		if (!SouthCorps || SouthCorps->Men <= 0) { continue; }
		SouthArmy.FoodDays = FMath::Max(0.f, SouthArmy.FoodDays - 1.f);
		const bool bSouthHorses = SouthArmy.Arm == TEXT("cavalry") || SouthArmy.Arm == TEXT("artillery");
		if (bSouthHorses) { SouthArmy.FodderDays = FMath::Max(0.f, SouthArmy.FodderDays - 1.f); }
		int32 SouthDepotTown = INDEX_NONE;
		double SouthNearest = TNumericLimits<double>::Max();
		for (const auto& SouthDepot : Depots)
		{
			if (!Cities.IsValidIndex(SouthDepot.Key) || Cities[SouthDepot.Key].NationId != SouthArmy.Nation
				|| !Cities[SouthDepot.Key].Occupier.IsEmpty() || SouthDepot.Value.Food < SouthCorps->Men) { continue; }
			const double SouthDistance = FVector2D::Distance(SouthCorps->Km, TownKm(SouthDepot.Key));
			if (SouthDistance >= SouthNearest) { continue; }
			const TArray<FVector2D> SouthDirectLine = { TownKm(SouthDepot.Key), SouthCorps->Km };
			TArray<FCampaign1851Leg> SouthSupplyRoute;
			const bool bSouthDirect = SouthDistance < 8.0 && IsDryLine(SouthDirectLine[0], SouthDirectLine[1]) && CanEnterLine(SouthArmy.Nation, SouthDirectLine);
			if (!bSouthDirect && !PlanMarch(SouthDepot.Key, TownKm(SouthDepot.Key), SouthCorps->Town, SouthCorps->Km,
				Campaign1851Supply::ColumnKmPerDay, ECampaign1851RouteMode::RoadsOnly, SouthSupplyRoute, nullptr, SouthArmy.Nation)) { continue; }
			SouthNearest = SouthDistance;
			SouthDepotTown = SouthDepot.Key;
		}
		if (SouthDepotTown != INDEX_NONE)
		{
			FCampaign1851DepotStock& SouthStock = Depots[SouthDepotTown];
			const float SouthFood = FMath::Min(SouthStock.Food, FMath::Max(0.f, Campaign1851Supply::FoodCarried - SouthArmy.FoodDays) * SouthCorps->Men);
			SouthStock.Food -= SouthFood;
			SouthArmy.FoodDays += SouthFood / SouthCorps->Men;
			if (bSouthHorses)
			{
				const float SouthHorses = SouthArmy.Arm == TEXT("cavalry") ? float(SouthCorps->Men) : float(SouthCorps->Guns * 6);
				const float SouthFodder = FMath::Min(SouthStock.Fodder, FMath::Max(0.f, Campaign1851Supply::FodderCarried - SouthArmy.FodderDays) * SouthHorses);
				SouthStock.Fodder -= SouthFodder;
				SouthArmy.FodderDays += SouthFodder / FMath::Max(1.f, SouthHorses);
			}
		}
		if (SouthArmy.FoodDays <= 0.f)
		{
			SouthCorps->Men = FMath::Max(0, SouthCorps->Men - FMath::CeilToInt(SouthCorps->Men * 0.001f));
			SouthCorps->RestUntil = FMath::Max(SouthCorps->RestUntil, CampaignDays + 1.0);
		}
		else if (bSouthHorses && SouthArmy.FodderDays <= 0.f) { SouthCorps->RestUntil = FMath::Max(SouthCorps->RestUntil, CampaignDays + 1.0); }
	}
}

void ACampaign1851Map::AdvanceNeighbourArmies()
{
	if (CampaignDays < NeighbourNextMobilisationDay) { return; }
	NeighbourNextMobilisationDay = FMath::FloorToDouble(CampaignDays) + 1.0;
	for (FCampaign1851NeighbourArmy& NeighbourArmy : NeighbourArmies)
	{
		if (!IsNeighbourHostile(NeighbourArmy.Nation) || NeighbourArmy.CorpsId || NeighbourArmy.MobilisedMen <= 0 || !Cities.IsValidIndex(NeighbourArmy.Town)) { continue; }
		// Local garrisons hold their town. The field corps mobilises once, not once per month.
		FCampaign1851EnemyCorps NeighbourCorps;
		NeighbourCorps.Id = NextCorpsId++;
		NeighbourArmy.CorpsId = NeighbourCorps.Id;
		NeighbourCorps.Name = NeighbourArmy.Name;
		NeighbourCorps.Nation = NeighbourArmy.Nation;
		NeighbourCorps.Town = NeighbourArmy.Town;
		NeighbourCorps.Km = TownKm(NeighbourArmy.Town);
		NeighbourCorps.Men = NeighbourArmy.bGarrison ? NeighbourArmy.Men : NeighbourArmy.Men + TakeNeighbourRecruits(NeighbourArmy, FMath::Max(0, NeighbourArmy.MobilisedMen - NeighbourArmy.Men));
		if (NeighbourCorps.Men <= 0) { NeighbourArmy.CorpsId = 0; continue; }
		NeighbourCorps.StartMen = NeighbourCorps.Men;
		if (!NeighbourArmy.bGarrison)
		{
			const int32 NeighbourBorderGoal = FindCity(TEXT("Ratzeburg"));
			if (NeighbourBorderGoal != INDEX_NONE) { NeighbourCorps.Objectives.Add(NeighbourBorderGoal); }
		}
		NeighbourCorps.Guns = NeighbourArmy.District.IsEmpty() ? NeighbourArmy.Guns : FMath::RoundToInt(NeighbourArmy.Guns * NeighbourCorps.Men / float(FMath::Max(1, NeighbourArmy.MobilisedMen)));
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
		if (!NeighbourArmy.District.IsEmpty()) { NeighbourLines.Add(FString::Printf(TEXT("army-supply|%s|%.9f|%.9f"), *NeighbourArmy.Id, NeighbourArmy.FoodDays, NeighbourArmy.FodderDays)); }
	}
	for (const FCampaign1851NeighbourDistrict& SouthDistrict : NeighbourDistricts)
	{
		NeighbourLines.Add(FString::Printf(TEXT("district|%s|%.9f|%.9f"), *SouthDistrict.Id, SouthDistrict.Reserve, SouthDistrict.Treasury));
	}
	return NeighbourLines;
}

void ACampaign1851Map::RestoreNeighbours(const TArray<FString>& Lines)
{
	ResetNeighbourArmies();
	for (FCampaign1851NeighbourDistrict& SouthDistrict : NeighbourDistricts)
	{
		SouthDistrict.Reserve = SouthDistrict.Population * SouthDistrict.StartShare;
		SouthDistrict.Treasury = 3.0 * (SouthDistrict.Urban * SouthDistrict.UrbanTax + FMath::Max(0, SouthDistrict.Population - SouthDistrict.Urban) * SouthDistrict.RuralTax);
	}
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
		if (NeighbourParts.Num() == 4 && NeighbourParts[0] == TEXT("army-supply"))
		{
			FCampaign1851NeighbourArmy* SouthArmy = NeighbourArmies.FindByPredicate([&](const FCampaign1851NeighbourArmy& A) { return A.Id == NeighbourParts[1]; });
			const float SouthFood = FCString::Atof(*NeighbourParts[2]), SouthFodder = FCString::Atof(*NeighbourParts[3]);
			if (SouthArmy && FMath::IsFinite(SouthFood) && FMath::IsFinite(SouthFodder))
			{
				SouthArmy->FoodDays = FMath::Clamp(SouthFood, 0.f, Campaign1851Supply::FoodCarried);
				SouthArmy->FodderDays = FMath::Clamp(SouthFodder, 0.f, Campaign1851Supply::FodderCarried);
			}
			continue;
		}
		if (NeighbourParts.Num() == 4 && NeighbourParts[0] == TEXT("district"))
		{
			FCampaign1851NeighbourDistrict* SouthDistrict = NeighbourDistricts.FindByPredicate([&](const FCampaign1851NeighbourDistrict& D) { return D.Id == NeighbourParts[1]; });
			const double SouthReserve = FCString::Atod(*NeighbourParts[2]), SouthTreasury = FCString::Atod(*NeighbourParts[3]);
			if (SouthDistrict && FMath::IsFinite(SouthReserve) && FMath::IsFinite(SouthTreasury))
			{
				SouthDistrict->Reserve = float(FMath::Clamp(SouthReserve, 0.0, double(SouthDistrict->Population * SouthDistrict->CapShare)));
				SouthDistrict->Treasury = FMath::Max(0.0, SouthTreasury);
			}
			continue;
		}
		if (NeighbourParts.Num() != 2) { continue; }
		FCampaign1851NeighbourArmy* NeighbourArmy = NeighbourArmies.FindByPredicate([&](const FCampaign1851NeighbourArmy& A) { return A.Id == NeighbourParts[0]; });
		if (NeighbourArmy) { NeighbourArmy->CorpsId = FCString::Atoi(*NeighbourParts[1]); }
	}
	// v33 used one aggregate corps. Suppress its new child regiments until the next peace/new game.
	// The aggregate's saved casualties/march remain in War, even if it has been destroyed.
	for (const FCampaign1851NeighbourArmy& SouthAggregate : NeighbourArmies)
	{
		if ((SouthAggregate.Id != TEXT("PR-pomerania") && SouthAggregate.Id != TEXT("MEC-field")) || !SouthAggregate.CorpsId) { continue; }
		for (FCampaign1851NeighbourArmy& SouthChild : NeighbourArmies)
		{
			if (SouthChild.ParentCorps == SouthAggregate.Id) { SouthChild.CorpsId = -1; }
		}
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
	if (NeighbourDistrictGrid.Num() == NeighbourOwners.Num())
	{
		TArray<TArray<FVector>> SouthBorders;
		for (int32 SouthY = 0; SouthY + 1 < NeighbourOwnerH; ++SouthY)
		{
			for (int32 SouthX = 0; SouthX + 1 < NeighbourOwnerW; ++SouthX)
			{
				const int32 SouthCell = SouthY * NeighbourOwnerW + SouthX;
				const uint16 SouthId = NeighbourDistrictGrid[SouthCell];
				if (!SouthId) { continue; }
				const FVector2D SouthCorner(Extent.XMin + (SouthX + 1) * NeighbourDx, Extent.YMax - (SouthY + 1) * NeighbourDy);
				if (NeighbourDistrictGrid[SouthCell + 1] && NeighbourDistrictGrid[SouthCell + 1] != SouthId
					&& NeighbourOwners[SouthCell] == NeighbourOwners[SouthCell + 1])
				{
					SouthBorders.Add({ LocalAtKm(SouthCorner) + FVector(0,0,2.8), LocalAtKm(SouthCorner + FVector2D(0,NeighbourDy)) + FVector(0,0,2.8) });
				}
				if (NeighbourDistrictGrid[SouthCell + NeighbourOwnerW] && NeighbourDistrictGrid[SouthCell + NeighbourOwnerW] != SouthId
					&& NeighbourOwners[SouthCell] == NeighbourOwners[SouthCell + NeighbourOwnerW])
				{
					SouthBorders.Add({ LocalAtKm(SouthCorner) + FVector(0,0,2.8), LocalAtKm(SouthCorner - FVector2D(NeighbourDx,0)) + FVector(0,0,2.8) });
				}
			}
		}
		UStaticMeshComponent* SouthBoundaryMesh = NewObject<UStaticMeshComponent>(this, FName(TEXT("SouthDistrictBorders")));
		SouthBoundaryMesh->SetupAttachment(Root);
		SouthBoundaryMesh->SetStaticMesh(Campaign1851Scenery::BuildRibbons(SouthBorders, float(0.045 * KmToUnits), FLinearColor(0.35f,0.27f,0.19f), NeighbourMaterial, TEXT("SouthDistrictBorders")));
		SouthBoundaryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SouthBoundaryMesh->SetCastShadow(false);
		SouthBoundaryMesh->SetVisibility(false);
		SouthBoundaryMesh->RegisterComponent();
		NeighbourTerritoryMeshes.Add(SouthBoundaryMesh);
	}
}
