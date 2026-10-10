#include "Campaign1851Buildings.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	TArray<FCampaign1851BuildingDef> Defs;
	bool bLoaded = false;

	/** Splits one CSV line, honouring "quoted, fields" and "" escapes (as Python's csv writes them). */
	TArray<FString> SplitCsv(const FString& Line)
	{
		TArray<FString> Out;
		FString Field;
		bool bQuoted = false;
		for (int32 i = 0; i < Line.Len(); ++i)
		{
			const TCHAR C = Line[i];
			if (bQuoted)
			{
				if (C == TEXT('"') && i + 1 < Line.Len() && Line[i + 1] == TEXT('"')) { Field.AppendChar(C); ++i; }
				else if (C == TEXT('"')) { bQuoted = false; }
				else { Field.AppendChar(C); }
			}
			else if (C == TEXT('"')) { bQuoted = true; }
			else if (C == TEXT(',')) { Out.Add(Field); Field.Reset(); }
			else { Field.AppendChar(C); }
		}
		Out.Add(Field);
		return Out;
	}
}

namespace Campaign1851Buildings
{
	bool Load()
	{
		if (bLoaded)
		{
			return true;
		}
		TArray<FString> Lines;
		if (!FFileHelper::LoadFileToStringArray(Lines, *(FPaths::ProjectDir() / TEXT("Data/Campaign1851/Buildings1851.csv"))) || Lines.Num() < 2)
		{
			UE_LOG(LogTemp, Error, TEXT("CAMPAIGN-1851|buildings|Data/Campaign1851/Buildings1851.csv missing; run Tools/Campaign/building_costs.py"));
			return false;
		}
		const TArray<FString> Header = SplitCsv(Lines[0].TrimEnd());   // Python's csv ends its lines with CR LF
		auto Col = [&Header](const TCHAR* Name) { return Header.IndexOfByKey(FString(Name)); };
		const int32 CKey = Col(TEXT("key")), CName = Col(TEXT("name")), CCat = Col(TEXT("category")), COwner = Col(TEXT("owner")),
			CSize = Col(TEXT("size")), CType = Col(TEXT("type")), CCost = Col(TEXT("cost_rd")), CDays = Col(TEXT("days")),
			CUpkeep = Col(TEXT("upkeep_rd_year")), CReq = Col(TEXT("requires")), CProv = Col(TEXT("provides")), CImg = Col(TEXT("image"));
		for (int32 l = 1; l < Lines.Num(); ++l)
		{
			const TArray<FString> F = SplitCsv(Lines[l].TrimEnd());
			if (F.Num() < Header.Num())
			{
				continue;
			}
			FCampaign1851BuildingDef& D = Defs.AddDefaulted_GetRef();
			D.Key = F[CKey];
			D.Name = F[CName];
			D.Category = F[CCat];
			D.Owner = F[COwner];
			D.Size = F[CSize];
			D.Type = F[CType];
			D.CostRd = FCString::Atoi(*F[CCost]);
			D.Days = FCString::Atoi(*F[CDays]);
			D.UpkeepRdPerYear = FCString::Atoi(*F[CUpkeep]);
			const int32 GarrisonMinPopColumn = Col(TEXT("min_population"));
			D.MinPopulation = F.IsValidIndex(GarrisonMinPopColumn) ? FCString::Atoi(*F[GarrisonMinPopColumn]) : 0;
			D.Requires = F[CReq];
			D.Provides = F[CProv];
			D.Image = F[CImg];
		}
		bLoaded = true;
		UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-1851|buildings|%d types loaded"), Defs.Num());
		return true;
	}

	const FCampaign1851BuildingDef* Find(const FString& Key)
	{
		Load();
		return Defs.FindByPredicate([&Key](const FCampaign1851BuildingDef& D) { return D.Key == Key; });
	}

	const TArray<FCampaign1851BuildingDef>& All()
	{
		Load();
		return Defs;
	}

	float WorkRate(const FString& Type, const FDateTime& Date)
	{
		const int32 Month = Date.GetMonth();
		if (Month != 12 && Month > 2)
		{
			return 1.f;
		}
		if (Type == TEXT("jordværk")) return 0.3f;       // frozen ground
		if (Type == TEXT("bindingsværk")) return 0.8f;   // timber framing goes on, slower
		return 0.5f;                                     // brick and stone: mortar must not freeze
	}
}
