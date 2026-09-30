// Weather and the state of the roads (Docs/Weather1851.md, backlog item 4). Each day's weather follows from
// the campaign seed (so a saved game needs to keep nothing): a temperature around the Danish monthly mean,
// with spells of several days and hard or mild winters by the year, and rain or snow by the month. Mud in
// spring and autumn, snow and thaw slow the columns on the roads (less on the chausséer); a storm stops the
// ferries; a hard frost freezes the narrow waters, so Slien and Alssund can be crossed on the ice.

#include "Campaign1851Map.h"

namespace
{
	const float MeanTemp[12] = { 0.f, 0.f, 2.f, 6.f, 11.f, 15.f, 17.f, 16.f, 13.f, 9.f, 5.f, 2.f };
	const float WetChance[12] = { 0.45f, 0.4f, 0.4f, 0.38f, 0.38f, 0.4f, 0.45f, 0.48f, 0.5f, 0.55f, 0.58f, 0.52f };

	float Hash01(uint32 A, uint32 B)
	{
		return FRandomStream(int32(HashCombine(A * 2654435761u, B))).FRand();
	}
}

const TCHAR* Campaign1851Weather::Name(ECampaign1851Weather W)
{
	switch (W)
	{
	case ECampaign1851Weather::Rain: return TEXT("regn");
	case ECampaign1851Weather::Snow: return TEXT("sne");
	case ECampaign1851Weather::Frost: return TEXT("frost");
	case ECampaign1851Weather::Thaw: return TEXT("tø");
	case ECampaign1851Weather::Storm: return TEXT("storm");
	default: return TEXT("klart");
	}
}

float ACampaign1851Map::TemperatureOn(int32 Day) const
{
	const FDateTime Date = StartDate() + FTimespan::FromDays(Day);
	const int32 M = Date.GetMonth() - 1;
	// Between the monthly means, so the year warms and cools smoothly.
	const float Along = (Date.GetDay() - 15) / 30.f;
	const float Mean = FMath::Lerp(MeanTemp[M], MeanTemp[(M + (Along >= 0.f ? 1 : 11)) % 12], FMath::Abs(Along));
	// A hard or mild winter by the year (the winter of 1864 was hard); spells of some days; the day itself.
	const int32 WinterYear = Date.GetMonth() >= 7 ? Date.GetYear() : Date.GetYear() - 1;
	const bool bWinter = Date.GetMonth() >= 11 || Date.GetMonth() <= 3;
	const float Winter = bWinter ? (Hash01(uint32(Seed), uint32(WinterYear)) * 5.f - 3.f) : 0.f;
	const float P1 = Hash01(uint32(Seed) + 11u, 1u) * UE_TWO_PI, P2 = Hash01(uint32(Seed) + 13u, 2u) * UE_TWO_PI;
	const float Spell = 3.f * FMath::Sin(Day * UE_TWO_PI / 9.f + P1) + 2.f * FMath::Sin(Day * UE_TWO_PI / 23.f + P2);
	const float Daily = (Hash01(uint32(Seed) + 17u, uint32(Day)) - 0.5f) * 3.f;
	return Mean + Winter + Spell + Daily;
}

ECampaign1851Weather ACampaign1851Map::WeatherOn(int32 Day) const
{
	const float T = TemperatureOn(Day);
	const int32 M = (StartDate() + FTimespan::FromDays(Day)).GetMonth() - 1;
	const bool bStormSeason = M >= 9 || M <= 2;
	if (Hash01(uint32(Seed) + 23u, uint32(Day)) < (bStormSeason ? 0.05f : 0.02f))
	{
		return ECampaign1851Weather::Storm;
	}
	// Thaw: mild after a cold spell (the roads turn to mud).
	if (T > 1.f)
	{
		float Before = 0.f;
		for (int32 d = 1; d <= 5; ++d)
		{
			Before += TemperatureOn(Day - d);
		}
		if (Before / 5.f < -0.5f)
		{
			return ECampaign1851Weather::Thaw;
		}
	}
	if (Hash01(uint32(Seed) + 29u, uint32(Day)) < WetChance[M])
	{
		return T < 0.5f ? ECampaign1851Weather::Snow : ECampaign1851Weather::Rain;
	}
	return T < -1.f ? ECampaign1851Weather::Frost : ECampaign1851Weather::Clear;
}

ECampaign1851Weather ACampaign1851Map::GetWeather() const
{
	const int32 Day = FMath::FloorToInt(CampaignDays);
	if (Day != WeatherCacheDay)
	{
		WeatherCacheDay = Day;
		WeatherCache = WeatherOn(Day);
		TemperatureCache = TemperatureOn(Day);
		int32 Cold = 0;
		for (int32 d = 0; d < 14; ++d)
		{
			Cold += TemperatureOn(Day - d) < -2.f ? 1 : 0;
		}
		bIceCache = Cold >= 8;
	}
	return WeatherCache;
}

float ACampaign1851Map::GetTemperature() const
{
	GetWeather();
	return TemperatureCache;
}

bool ACampaign1851Map::IsIceWinter() const
{
	GetWeather();
	return bIceCache;
}

FString ACampaign1851Map::GetSeasonAndWeather() const
{
	return FString::Printf(TEXT("%s · %s %.0f°"), *GetSeasonName(), Campaign1851Weather::Name(GetWeather()), GetTemperature());
}

float ACampaign1851Map::LegPace(const FCampaign1851Leg& Leg) const
{
	const ECampaign1851Weather W = GetWeather();
	if (Leg.bWait)
	{
		return 1.f;
	}
	if (Leg.bRail)
	{
		return W == ECampaign1851Weather::Snow || W == ECampaign1851Weather::Storm ? 0.75f : 1.f;   // snow on the line
	}
	const bool bFerry = Links.IsValidIndex(Leg.Link) && Links[Leg.Link].HasFerry();
	if (bFerry && W == ECampaign1851Weather::Storm)
	{
		return 0.25f;   // the ferries wait for the wind to drop
	}
	// Mud in spring and autumn rain is worst; frost makes hard, good roads.
	const int32 M = GetDate().GetMonth();
	const bool bMudSeason = M == 3 || M == 4 || M == 10 || M == 11;
	float F = 1.f;
	switch (W)
	{
	case ECampaign1851Weather::Rain: F = bMudSeason ? 0.7f : 0.85f; break;
	case ECampaign1851Weather::Snow: F = 0.6f; break;
	case ECampaign1851Weather::Thaw: F = 0.5f; break;
	case ECampaign1851Weather::Storm: F = 0.8f; break;
	case ECampaign1851Weather::Frost: F = 0.95f; break;
	default: break;
	}
	// A chaussée (metalled) keeps half the loss away; across the fields it is worse.
	if (Links.IsValidIndex(Leg.Link) && Links[Leg.Link].bChaussee)
	{
		F = 1.f - (1.f - F) * 0.5f;
	}
	else if (Leg.bOffRoad)
	{
		F = 1.f - (1.f - F) * 1.3f;
	}
	return FMath::Clamp(F, 0.2f, 1.f);
}

void ACampaign1851Map::DailyWeather()
{
	const bool bIce = IsIceWinter();
	if (bIce && !bIceNoted)
	{
		News.Add(TEXT("Isvinter: Slien og de smalle sunde er frosset til og kan passeres på isen"));
	}
	if (!bIce && bIceNoted)
	{
		News.Add(TEXT("Isen går op i sundene"));
	}
	bIceNoted = bIce;
	if (GetWeather() == ECampaign1851Weather::Storm)
	{
		News.Add(TEXT("Storm: færgerne ligger stille"));
	}
}
