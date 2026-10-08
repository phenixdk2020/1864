// The scenarios of the campaign (see ACampaign1851Map::FScenario): the year it starts in and how the 1851 data is carried back or kept.

#include "Campaign1851Map.h"

namespace
{
	int32 GScenario = -1;   // not read from the settings yet
}

const TArray<ACampaign1851Map::FScenario>& ACampaign1851Map::Scenarios()
{
	static const TArray<FScenario> List = []()
	{
		TArray<FScenario> L;
		FScenario Peace;
		Peace.Id = TEXT("1825");
		Peace.Name = TEXT("Danmark 1825");
		Peace.Text = TEXT("En fredelig begyndelse: hæren er lille og uprøvet, byerne mindre og der er ingen jernbaner. Spændingen med Forbundet vokser fra Julirevolutionen til krigen i 1848.");
		Peace.Year = 1825;
		Peace.PopulationFactor = 0.80f;
		Peace.ArmyFactor = 1.f; // army and equipment have their own 1825 data
		Peace.ExperienceFactor = 1.f;
		Peace.StartTension = 10.f;
		L.Add(Peace);
		FScenario War;
		War.Id = TEXT("1851");
		War.Name = TEXT("Danmark 1851");
		War.Text = TEXT("Efter treårskrigen: en hær der har kæmpet, jernbanerne er begyndt og vejen til 1864 ligger foran. Urolig fred.");
		War.Year = 1851;
		L.Add(War);
		return L;
	}();
	return List;
}

int32 ACampaign1851Map::ScenarioIndex()
{
	if (GScenario < 0)
	{
		int32 Chosen = 0;   // 1825 is the default
		if (GConfig)
		{
			GConfig->GetInt(TEXT("Campaign1851"), TEXT("Scenario"), Chosen, GGameUserSettingsIni);
		}
		FString Flag;
		if (FParse::Value(FCommandLine::Get(), TEXT("CampaignScenario="), Flag, false))
		{
			const int32 Found = Scenarios().IndexOfByPredicate([&Flag](const FScenario& S) { return S.Id == Flag; });
			if (Found != INDEX_NONE) { Chosen = Found; }
		}
		GScenario = FMath::Clamp(Chosen, 0, Scenarios().Num() - 1);
	}
	return GScenario;
}

void ACampaign1851Map::SetScenarioIndex(int32 Index)
{
	GScenario = FMath::Clamp(Index, 0, Scenarios().Num() - 1);
	if (GConfig)
	{
		GConfig->SetInt(TEXT("Campaign1851"), TEXT("Scenario"), GScenario, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}
