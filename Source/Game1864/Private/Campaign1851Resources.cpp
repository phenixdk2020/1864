// Raw materials, equipment and the raising of new units (Docs/Materiel1851.md). The state keeps stores of
// iron, coal, timber, powder, cloth (uniforms) and leather (belts, knapsacks, saddlery). The country gives
// timber, cloth and leather of itself; the sawmills, coal mine, textile mills, tanneries and powder works add
// to them; iron and coal come mostly from abroad (by sea, which a war can close). The rifle workshop, the
// arsenal and the cannon foundry make their rifles and guns only from iron, coal and timber in store.
// A new unit is raised by type (battalion, jægerkorps, dragoon regiment, battery, horse battery) at a
// garrison of the player's choice, under the general command and with the training he chooses; it takes
// men from the amt, rifles or guns, uniforms, leather, horses and money.

#include "Campaign1851Map.h"

#include "Campaign1851ConstructionSite.h"
#include "Dom/JsonObject.h"

namespace Campaign1851Resources
{
	const FRawInfo& Info(ECampaign1851Raw R)
	{
		static const FRawInfo List[int32(ECampaign1851Raw::Count)] = {
			{ TEXT("Jern"),        TEXT("t"),        40.0, 60.f,   0.f,   true },
			{ TEXT("Kul"),         TEXT("t"),        12.0, 80.f,   0.f,   true },
			{ TEXT("Tømmer"),      TEXT("læs"),       6.0, 200.f,  30.f,  false },
			{ TEXT("Krudt"),       TEXT("tønder"),   25.0, 300.f,  5.f,   true },
			{ TEXT("Klæde"),       TEXT("uniformer"), 6.0, 3000.f, 150.f, false },
			{ TEXT("Læder"),       TEXT("sæt"),       4.0, 2500.f, 100.f, false },
		};
		if (ACampaign1851Map::ActiveScenario().Id != TEXT("1825")) { return List[FMath::Clamp(int32(R), 0, int32(ECampaign1851Raw::Count) - 1)]; }
		static const TArray<FRawInfo> Prices1825 = []()
		{
			TArray<FRawInfo> Result;
			const TCHAR* Keys[] = { TEXT("ironPrice"), TEXT("coalPrice"), TEXT("timberPrice"), TEXT("powderPrice"), TEXT("clothPrice"), TEXT("leatherPrice") };
			for (int32 Index = 0; Index < int32(ECampaign1851Raw::Count); ++Index)
			{
				FRawInfo Entry = List[Index];
				Entry.Price = ACampaign1851Map::EconomyValue(Keys[Index], Entry.Price);
				Result.Add(Entry);
			}
			return Result;
		}();
		const int32 RawIndex = FMath::Clamp(int32(R), 0, int32(ECampaign1851Raw::Count) - 1);
		return ACampaign1851Map::ActiveScenario().Id == TEXT("1825") ? Prices1825[RawIndex] : List[RawIndex];
	}

	/** A month's output of a finished works: raw materials made, and what the arms works turn them into. */
	void Works(const FString& Key, float OutRaw[int32(ECampaign1851Raw::Count)])
	{
		if (Key == TEXT("Sawmill")) { OutRaw[int32(ECampaign1851Raw::Timber)] += 40.f; }
		if (Key == TEXT("Coal_Mine")) { OutRaw[int32(ECampaign1851Raw::Coal)] += 30.f; }
		if (Key == TEXT("Textile_Mill")) { OutRaw[int32(ECampaign1851Raw::Cloth)] += 300.f; }
		if (Key == TEXT("Tannery")) { OutRaw[int32(ECampaign1851Raw::Leather)] += 200.f; }
		if (Key == TEXT("Saddlery")) { OutRaw[int32(ECampaign1851Raw::Leather)] += 120.f; }
		if (Key == TEXT("Gunpowder_Works")) { OutRaw[int32(ECampaign1851Raw::Powder)] += 40.f; }
		if (Key == TEXT("Machine_Workshop")) { OutRaw[int32(ECampaign1851Raw::Iron)] += 4.f; }   // scrap worked up
	}

	/** What an arms works needs a month for its full output (iron, coal, timber). */
	FVector3f ArmsInput(const FString& Key)
	{
		if (Key == TEXT("Wagon_Works")) return FVector3f(1.f, 0.f, 10.f);
		if (Key == TEXT("Rifle_Workshop")) return FVector3f(3.f, 1.f, 5.f);
		if (Key == TEXT("Arsenal")) return FVector3f(4.f, 2.f, 3.f);
		if (Key == TEXT("Cannon_Foundry")) return FVector3f(8.f, 6.f, 0.f);
		if (Key == TEXT("Gunpowder_Works")) return FVector3f(0.f, 2.f, 0.f);
		return FVector3f::ZeroVector;
	}

	const FUnitType& Type(int32 T)
	{
		static const FUnitType List[] = {
			{ TEXT("Linjebataillon"),    TEXT("Bataillon"),       TEXT("B"),  ECampaign1851Arm::Infantry,       760, 760, 0, 14,  760,  760,  1.0 },
			{ TEXT("Jægerkorps"),        TEXT("Jægerkorps"),      TEXT("J"),  ECampaign1851Arm::Jager,          760, 760, 0, 12,  760,  760,  1.15 },
			{ TEXT("Dragonregiment"),    TEXT("Dragonregiment"),  TEXT("D"),  ECampaign1851Arm::Cavalry,        560, 560, 0, 600, 560,  1120, 1.6 },
			{ TEXT("Batteri"),           TEXT("Batteri"),         TEXT("A"),  ECampaign1851Arm::Artillery,      150, 0,   8, 110, 150,  300,  1.3 },
			{ TEXT("Ridende batteri"),   TEXT("Ridende Batteri"), TEXT("RA"), ECampaign1851Arm::HorseArtillery, 180, 0,   6, 230, 180,  460,  1.5 },
			{ TEXT("Morterbatteri"),     TEXT("Morterbatteri"),   TEXT("M"),  ECampaign1851Arm::Artillery,      120, 0,   0, 72,  120,  240,  1.2, 6, 12 },
		};
		static const FUnitType Army1825UnitTypes[] = {
			{ TEXT("Linjebataljon (5 kompagnier)"), TEXT("Bataljon"), TEXT("B"), ECampaign1851Arm::Infantry, 900, 900, 0, 10, 900, 900, 1.0 },
			{ TEXT("Jægerkorps"), TEXT("Jægerkorps"), TEXT("J"), ECampaign1851Arm::Jager, 600, 600, 0, 8, 600, 600, 1.15 },
			{ TEXT("Dragonregiment"), TEXT("Dragonregiment"), TEXT("D"), ECampaign1851Arm::Cavalry, 600, 600, 0, 640, 600, 1200, 1.6 },
			{ TEXT("Batteri"), TEXT("Batteri"), TEXT("A"), ECampaign1851Arm::Artillery, 150, 0, 8, 110, 150, 300, 1.3 },
			{ TEXT("Ridende batteri"), TEXT("Ridende Batteri"), TEXT("RA"), ECampaign1851Arm::HorseArtillery, 180, 0, 6, 230, 180, 460, 1.5 },
			{ TEXT("Morterbatteri"), TEXT("Morterbatteri"), TEXT("M"), ECampaign1851Arm::Artillery, 120, 0, 0, 72, 120, 240, 1.2, 6, 12 },
		};
		return (ACampaign1851Map::ActiveScenario().Id == TEXT("1825") ? Army1825UnitTypes : List)[FMath::Clamp(T, 0, UnitTypes - 1)];
	}
}

namespace Campaign1851Resources
{
	int32 RaiseParts(int32 T, int32 Size)
	{
		return Size <= 0 ? 1 : Size == 1 ? 2 : (T >= 4 ? 3 : 4);
	}

	FUnitType SizedType(int32 T, int32 Size)
	{
		FUnitType RaiseSpec = Type(T);
		const int32 RaiseTotal = RaiseParts(T, 2), RaiseCount = RaiseParts(T, Size);
		// Integer equipment and manpower: the same rounded values are checked, paid and assigned.
		auto ScaleRaise = [RaiseTotal, RaiseCount](int32 Value) { return (Value * RaiseCount + RaiseTotal / 2) / RaiseTotal; };
		RaiseSpec.Men = ScaleRaise(RaiseSpec.Men);
		RaiseSpec.Rifles = ScaleRaise(RaiseSpec.Rifles);
		RaiseSpec.Guns = ScaleRaise(RaiseSpec.Guns);
		RaiseSpec.Horses = ScaleRaise(RaiseSpec.Horses);
		RaiseSpec.Uniforms = ScaleRaise(RaiseSpec.Uniforms);
		RaiseSpec.Leather = ScaleRaise(RaiseSpec.Leather);
		RaiseSpec.Mortars = ScaleRaise(RaiseSpec.Mortars);
		RaiseSpec.Wagons = ScaleRaise(RaiseSpec.Wagons);
		return RaiseSpec;
	}

	FString RaiseSizeName(int32 T, int32 Size)
	{
		if (Size >= 2) { return T <= 1 ? TEXT("Hel bataljon (4 kompagnier)") : T == 2 ? TEXT("Helt regiment (4 eskadroner)") : TEXT("Helt batteri"); }
		return T <= 1 ? (Size == 0 ? TEXT("1 kompagni") : TEXT("2 kompagnier"))
			: T == 2 ? (Size == 0 ? TEXT("1 eskadron") : TEXT("2 eskadroner"))
			: Size == 0 ? TEXT("1 deling (2 skyts)") : T == 3 ? TEXT("Halvbatteri (4 kanoner)") : TEXT("2 delinger (4 skyts)");
	}
}

void ACampaign1851Map::ResetResources()
{
	for (int32 r = 0; r < int32(ECampaign1851Raw::Count); ++r)
	{
		RawStock[r] = Campaign1851Resources::Info(ECampaign1851Raw(r)).Start;
	}
	const TArray<TSharedPtr<FJsonValue>>* ArmyRawArray = nullptr;
	if (ArmyScenarioEquipment.IsValid() && ArmyScenarioEquipment->TryGetArrayField(TEXT("raw"), ArmyRawArray))
	{
		for (int32 r = 0; r < int32(ECampaign1851Raw::Count) && r < ArmyRawArray->Num(); ++r)
		{
			RawStock[r] = float((*ArmyRawArray)[r]->AsNumber());
		}
	}
	MortarStock = int32(ArmyEquipmentNumber(TEXT("mortars"), 12));   // the arsenal's mortars after 1848-50 (estimate)
	WagonStock = int32(ArmyEquipmentNumber(TEXT("wagons"), 150));
}

bool ACampaign1851Map::BuyKit(bool bMortars, int32 Count, FString* OutReason)
{
	if (bMortars && !CanImport(OutReason))
	{
		return false;
	}
	const double Cost = Count * (bMortars ? Campaign1851Resources::MortarPrice : Campaign1851Resources::WagonPrice) * (bAtWar ? 1.5 : 1.0);
	if (Treasury < Cost)
	{
		if (OutReason) { *OutReason = TEXT("ikke råd"); }
		return false;
	}
	AddTransaction(-Cost, FString::Printf(TEXT("Indkøb: %d %s"), Count, bMortars ? TEXT("morterer") : TEXT("vogne")));
	(bMortars ? MortarStock : WagonStock) += Count;
	return true;
}

bool ACampaign1851Map::RaiseTownOk(int32 Town) const
{
	// Finished barracks or a scenario garrison; raised units must never create a barracks by setting Home.
	const ACampaign1851ConstructionSite* Site = FindProject(Town);
	return (Site && Site->IsBarracksDone()) || ArmyAtStart.ContainsByPredicate([Town](const FCampaign1851Regiment& R) { return R.Home == Town; });
}

TArray<int32> ACampaign1851Map::RaiseTowns() const
{
	TArray<int32> Out;
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		if (!Cities[c].bForeign && Cities[c].Occupier.IsEmpty() && RaiseTownOk(c))
		{
			Out.Add(c);
		}
	}
	return Out;
}

bool ACampaign1851Map::CanImport(FString* OutReason) const
{
	if (bAtWar && !HasSeaControl())
	{
		if (OutReason) { *OutReason = TEXT("ingen import: fjenden behersker farvandene"); }
		return false;
	}
	return true;
}

double ACampaign1851Map::RawPrice(ECampaign1851Raw R) const
{
	// Dearer in war (freight and insurance).
	return Campaign1851Resources::Info(R).Price * (bAtWar ? 1.5 : 1.0);
}

bool ACampaign1851Map::BuyRaw(ECampaign1851Raw R, float Amount, FString* OutReason)
{
	if (!CanImport(OutReason))
	{
		return false;
	}
	const double Cost = RawPrice(R) * Amount;
	if (Treasury < Cost)
	{
		if (OutReason) { *OutReason = TEXT("ikke råd"); }
		return false;
	}
	const Campaign1851Resources::FRawInfo& I = Campaign1851Resources::Info(R);
	AddTransaction(-Cost, FString::Printf(TEXT("Indkøb: %.0f %s %s"), Amount, I.Unit, I.Name));
	RawStock[int32(R)] += Amount;
	return true;
}

void ACampaign1851Map::RawFlow(float OutMade[int32(ECampaign1851Raw::Count)], float OutUsed[int32(ECampaign1851Raw::Count)]) const
{
	for (int32 r = 0; r < int32(ECampaign1851Raw::Count); ++r)
	{
		OutMade[r] = Campaign1851Resources::Info(ECampaign1851Raw(r)).Country;
		OutUsed[r] = 0.f;
	}
	for (const ACampaign1851ConstructionSite* Site : Projects)
	{
		if (Site && !Site->IsGarrison() && !Site->IsDemolishing() && Site->IsModuleDone(0))
		{
			Campaign1851Resources::Works(Site->GetKind(), OutMade);
			const FVector3f In = Campaign1851Resources::ArmsInput(Site->GetKind());
			OutUsed[int32(ECampaign1851Raw::Iron)] += In.X;
			OutUsed[int32(ECampaign1851Raw::Coal)] += In.Y;
			OutUsed[int32(ECampaign1851Raw::Timber)] += In.Z;
		}
	}
}

float ACampaign1851Map::MonthlyRawMaterials()
{
	// The country and the works add; the arms works run as far as iron, coal and timber allow (0..1).
	float Made[int32(ECampaign1851Raw::Count)], Used[int32(ECampaign1851Raw::Count)];
	RawFlow(Made, Used);
	for (int32 r = 0; r < int32(ECampaign1851Raw::Count); ++r)
	{
		RawStock[r] += Made[r];
	}
	float Share = 1.f;
	for (int32 r = 0; r < int32(ECampaign1851Raw::Count); ++r)
	{
		if (Used[r] > 0.f)
		{
			Share = FMath::Min(Share, RawStock[r] / Used[r]);
		}
	}
	Share = FMath::Clamp(Share, 0.f, 1.f);
	for (int32 r = 0; r < int32(ECampaign1851Raw::Count); ++r)
	{
		RawStock[r] = FMath::Max(0.f, RawStock[r] - Used[r] * Share);
	}
	if (Share < 0.99f && !bRawShortNoted)
	{
		News.Add(FString::Printf(TEXT("Våbenværkerne mangler jern, kul eller tømmer: de arbejder kun %.0f %%"), Share * 100.f));
	}
	bRawShortNoted = Share < 0.99f;
	return Share;
}

FString ACampaign1851Map::UnitBlockReason(int32 Type, int32 Town, int32 Size) const
{
	const Campaign1851Resources::FUnitType T = Campaign1851Resources::SizedType(Type, Size);
	if (!Cities.IsValidIndex(Town) || Cities[Town].bForeign)
	{
		return TEXT("ingen by med kaserne: rekruttering kræver kaserne, ikke billet");
	}
	if (!RaiseTownOk(Town))
	{
		return TEXT("byen har ingen færdig kaserne; billet kan ikke rekruttere");
	}
	if (!Cities[Town].Occupier.IsEmpty()) { return TEXT("byen er besat"); }
	if (GarrisonMen(Town) + T.Men > GarrisonCapacity(Town)) { return TEXT("kasernen har ikke plads til den nye enhed"); }
	const int32 AmtIndex = AmtIndexOfTown(Town);
	if (!AmtManpower.IsValidIndex(AmtIndex) || AmtManpower[AmtIndex] < T.Men)
	{
		return FString::Printf(TEXT("amtet har kun %d mand i reserve"), AmtManpower.IsValidIndex(AmtIndex) ? FMath::FloorToInt(AmtManpower[AmtIndex]) : 0);
	}
	if (T.Guns > GunStock)
	{
		return FString::Printf(TEXT("mangler %d kanoner på lager"), T.Guns - GunStock);
	}
	if (T.Mortars > MortarStock)
	{
		return FString::Printf(TEXT("mangler %d morterer på lager"), T.Mortars - MortarStock);
	}
	if (RawStock[int32(ECampaign1851Raw::Cloth)] < T.Uniforms)
	{
		return TEXT("mangler klæde til uniformerne");
	}
	if (RawStock[int32(ECampaign1851Raw::Leather)] < T.Leather)
	{
		return TEXT("mangler læder til remtøj og sadler");
	}
	if (T.Rifles > Rifles && !CanImport()) { return TEXT("ingen import af manglende geværer"); }
	if (Treasury < UnitCost(Type, Size))
	{
		return TEXT("ikke råd");
	}
	return FString();
}

double ACampaign1851Map::UnitCost(int32 Type, int32 Size) const
{
	// Pay, kit and quarters in the first months; rifles missing from the store are bought abroad on top.
	const Campaign1851Resources::FUnitType T = Campaign1851Resources::SizedType(Type, Size);
	const int32 MissingRifles = FMath::Max(0, T.Rifles - Rifles);
	const int32 MissingWagons = FMath::Max(0, T.Wagons - WagonStock);
	return Campaign1851Army::RaiseCost() * T.CostFactor * T.Men / 760.0 + MissingRifles * double(Campaign1851Materiel::RifleImportPrice) + MissingWagons * Campaign1851Resources::WagonPrice + FMath::Max(0, T.Horses - Horses) * double(Campaign1851Materiel::HorsePrice);
}

int32 ACampaign1851Map::RaiseUnit(int32 Type, int32 Town, int32 Command, ECampaign1851Program Program, FString* OutReason, int32 Size)
{
	const FString Why = UnitBlockReason(Type, Town, Size);
	if (!Why.IsEmpty())
	{
		if (OutReason) { *OutReason = Why; }
		return INDEX_NONE;
	}
	const Campaign1851Resources::FUnitType T = Campaign1851Resources::SizedType(Type, Size);
	// The next free number of its kind.
	int32 Number = 1;
	while (FindRegiment(FString::Printf(TEXT("%s%d"), T.IdPrefix, Number)) != INDEX_NONE)
	{
		++Number;
	}
	const int32 Index = AddRaisedRegiment(FString::Printf(TEXT("%s%d"), T.IdPrefix, Number), FString::Printf(TEXT("%d. %s"), Number, T.NameSuffix), T.Arm, Town, T.Men, true);
	FCampaign1851Regiment& R = Regiments[Index];
	R.Experience = Campaign1851Army::RecruitExperience;
	for (float& S : R.Skills)
	{
		S = Campaign1851Army::RecruitSkill;
	}
	if (Size < 2)
	{
		R.Name = FString::Printf(TEXT("%d. %s%s"), Number, Type <= 1 ? TEXT("Kompagni") : Type == 2 ? TEXT("Eskadron") : TEXT("Deling"), Type == 1 ? TEXT(" (Jæger)") : Type == 4 ? TEXT(" (Ridende)") : Type == 5 ? TEXT(" (Morterer)") : TEXT(""));
		if (Size == 1) { R.Name += Type <= 1 ? TEXT(" (2 kompagnier)") : Type == 2 ? TEXT(" (2 eskadroner)") : TEXT(" (2 delinger)"); }
	}
	const int32 RaiseCount = Campaign1851Resources::RaiseParts(Type, Size);
	if (Type <= 1)
	{
		R.Captains.Init(INDEX_NONE, RaiseCount);
		R.CompanyFort.Init(0, RaiseCount);
	}
	R.CompanyWeight.Init(1.f, RaiseCount);
	R.RaisingType = FMath::Clamp(Type, 0, Campaign1851Resources::UnitTypes - 1);
	R.Horses = R.MaxHorses = T.Horses;
	R.Guns = TakeGunsFromStock(T.Guns);
	R.Mortars = FMath::Min(T.Mortars, MortarStock);
	MortarStock -= R.Mortars;
	R.Wagons = T.Wagons;
	const int32 WagonsBought = FMath::Max(0, T.Wagons - WagonStock);
	WagonStock = FMath::Max(0, WagonStock - T.Wagons);
	if (WagonsBought > 0)
	{
		AddTransaction(-WagonsBought * Campaign1851Resources::WagonPrice, FString::Printf(TEXT("%d vogne købt: %s"), WagonsBought, *R.Name));
	}
	if (R.Mortars > 0)
	{
		R.PaceKmPerDay *= 0.8f;
	}
	if (Type >= 3)
	{
		R.SectionGuns.Init(float(R.Guns) / RaiseCount, RaiseCount);
		R.SectionHorses.Init(float(R.Horses) / RaiseCount, RaiseCount);
		R.SectionMaxHorses = R.SectionHorses;
	}
	R.Program = Program;
	if (Commands.IsValidIndex(Command))
	{
		R.Command = Command;
	}
	AmtManpower[AmtIndexOfTown(Town)] -= T.Men;
	RawStock[int32(ECampaign1851Raw::Cloth)] -= T.Uniforms;
	RawStock[int32(ECampaign1851Raw::Leather)] -= T.Leather;
	if (T.Rifles > 0)
	{
		TakeRifles(T.Rifles, R.Name);
	}
	TakeHorses(T.Horses, R.Name);
	AddTransaction(-Campaign1851Army::RaiseCost() * T.CostFactor * T.Men / 760.0, FString::Printf(TEXT("%s oprettet i %s"), *R.Name, *Cities[Town].Name));
	// Its officers: a chief and a captain for each company or squadron, hired.
	FRandomStream Rng(int32(HashCombine(uint32(Seed), uint32(Index * 977 + Number))));
	FCampaign1851Officer Chief = MakeOfficer(Rng, false, T.Arm == ECampaign1851Arm::Cavalry ? TEXT("Oberstløjtnant") : T.Guns > 0 ? TEXT("Kaptajn") : TEXT("Major"));
	Chief.Id = FString::Printf(TEXT("R%d"), NextOfficerNumber++);
	Chief.bRecruited = true;
	AssignOfficer(Officers.Add(Chief), Index);
	for (int32 k = 0; k < R.Captains.Num(); ++k)
	{
		FCampaign1851Officer Captain = MakeOfficer(Rng, false, TEXT("Kaptajn"));
		Captain.Id = FString::Printf(TEXT("R%d"), NextOfficerNumber++);
		Captain.bRecruited = true;
		Captain.CaptainOf = Index;
		Captain.Company = k;
		R.Captains[k] = Officers.Add(Captain);
	}
	UpdateRegimentPiece(Index);
	News.Add(FString::Printf(TEXT("%s er oprettet i %s: %d rekrutter%s"), *R.Name, *Cities[Town].Name, T.Men, Commands.IsValidIndex(R.Command) ? *FString::Printf(TEXT(" under %s"), *Commands[R.Command].Name) : TEXT("")));
	return Index;
}

TArray<FString> ACampaign1851Map::SaveResources() const
{
	FString Line = TEXT("raw");
	for (float V : RawStock)
	{
		Line += FString::Printf(TEXT("|%.1f"), V);
	}
	TArray<FString> Out = { Line, FString::Printf(TEXT("kit|%d|%d"), MortarStock, WagonStock) };
	for (const FCampaign1851Regiment& R : Regiments)
	{
		if (R.Mortars > 0 || R.Wagons > 0)
		{
			Out.Add(FString::Printf(TEXT("mortars|%s|%d|%d"), *R.Id, R.Mortars, R.Wagons));
		}
	}
	return Out;
}

void ACampaign1851Map::RestoreResources(const TArray<FString>& Lines)
{
	ResetResources();
	for (const FString& L : Lines)
	{
		TArray<FString> P;
		L.ParseIntoArray(P, TEXT("|"), false);
		if (P.Num() == 3 && P[0] == TEXT("kit"))
		{
			MortarStock = FCString::Atoi(*P[1]);
			WagonStock = FCString::Atoi(*P[2]);
		}
		else if (P.Num() == 4 && P[0] == TEXT("mortars") && FindRegiment(P[1]) != INDEX_NONE)
		{
			Regiments[FindRegiment(P[1])].Mortars = FCString::Atoi(*P[2]);
			Regiments[FindRegiment(P[1])].Wagons = FCString::Atoi(*P[3]);
		}
		else if (P.Num() == int32(ECampaign1851Raw::Count) + 1 && P[0] == TEXT("raw"))
		{
			for (int32 r = 0; r < int32(ECampaign1851Raw::Count); ++r)
			{
				RawStock[r] = FCString::Atof(*P[r + 1]);
			}
		}
	}
}

FString ACampaign1851Map::UnitUpgradeDescription(int32 RegimentIndex, bool* OutCan) const
{
	if (OutCan) { *OutCan = false; }
	if (!Regiments.IsValidIndex(RegimentIndex)) { return TEXT("Ingen enhed valgt"); }
	const FCampaign1851Regiment& CustomRegiment = Regiments[RegimentIndex];
	if (CustomRegiment.WeaponConversionDays > 0.f) { return FString::Printf(TEXT("Ombygning: %.0f dage tilbage; halv træfsikkerhed og dobbelt ladetid"), float(FMath::CeilToInt(CustomRegiment.WeaponConversionDays))); }
	const bool CustomGun = (CustomRegiment.Arm == ECampaign1851Arm::Artillery || CustomRegiment.Arm == ECampaign1851Arm::HorseArtillery);
	const int32 CustomNext = UnitWeaponLevel(CustomRegiment) + 1;
	if (CustomRegiment.Mortars > 0 || CustomNext > (CustomGun ? 1 : 3)) { return TEXT("Ingen yderligere våbenopgradering"); }
	const TCHAR* CustomTopic = CustomGun ? TEXT("riflegun") : CustomNext == 1 ? TEXT("percussion") : CustomNext == 2 ? TEXT("minie") : TEXT("breech");
	const TCHAR* CustomRequirement = CustomGun ? TEXT("Riflede kanoner") : CustomNext == 1 ? TEXT("Perkussionslås") : CustomNext == 2 ? TEXT("Minié-riffel") : TEXT("Bagladegeværet");
	const int32 CustomCount = CustomGun ? CustomRegiment.Guns : CustomRegiment.MaxMen;
	const int32 CustomStock = FMath::Max(0, CustomGun ? GunStock : Rifles);
	// Estimated gameplay procurement prices; generic stock requires conversion as well.
	const double CustomPrice = CustomGun ? 400.0 : CustomNext == 3 ? 18.0 : CustomNext == 2 ? 12.0 : 6.0;
	const double CustomCost = CustomCount * (CustomGun ? 40.0 : 1.0) + FMath::Max(0, CustomCount - CustomStock) * CustomPrice;
	const bool CustomFree = !CustomRegiment.IsMarching() && !IsInBattle(RegimentIndex) && !CustomRegiment.bTraining && CustomCount > 0;
	const bool CustomImportAllowed = CustomCount <= CustomStock || CanImport();
	if (OutCan) { *OutCan = CustomFree && CustomImportAllowed && HasResearch(CustomTopic) && Treasury >= CustomCost; }
	return FString::Printf(TEXT("%d %s fra lager (resten købes), %.0f rd.; %d dage. Kræver %s%s%s"), FMath::Min(CustomCount, CustomStock), CustomGun ? TEXT("kanoner") : TEXT("geværer"), CustomCost, CustomGun ? 14 : 10, CustomRequirement,
		HasResearch(CustomTopic) ? TEXT(" (udforsket)") : TEXT(" (mangler)"), !CustomFree ? TEXT("; enheden skal stå stille uden kamp/uddannelse") : !CustomImportAllowed ? TEXT("; ingen import af manglende våben") : Treasury < CustomCost ? TEXT("; ikke råd") : TEXT(""));
}

bool ACampaign1851Map::UpgradeUnitWeapon(int32 RegimentIndex)
{
	bool CustomCan = false;
	UnitUpgradeDescription(RegimentIndex, &CustomCan);
	if (!CustomCan) { return false; }
	FCampaign1851Regiment& CustomRegiment = Regiments[RegimentIndex];
	const bool CustomGun = (CustomRegiment.Arm == ECampaign1851Arm::Artillery || CustomRegiment.Arm == ECampaign1851Arm::HorseArtillery);
	const int32 CustomNext = UnitWeaponLevel(CustomRegiment) + 1;
	const int32 CustomCount = CustomGun ? CustomRegiment.Guns : CustomRegiment.MaxMen;
	int32& CustomStock = CustomGun ? GunStock : Rifles;
	const int32 CustomTaken = FMath::Min(CustomCount, FMath::Max(0, CustomStock));
	const double CustomPrice = CustomGun ? 400.0 : CustomNext == 3 ? 18.0 : CustomNext == 2 ? 12.0 : 6.0;
	AddTransaction(-(CustomCount * (CustomGun ? 40.0 : 1.0) + (CustomCount - CustomTaken) * CustomPrice), TEXT("Våbenombygning: ") + CustomRegiment.Name);
	CustomStock -= CustomTaken;
	CustomRegiment.PendingWeaponLevel = CustomNext;
	CustomRegiment.WeaponConversionDays = CustomGun ? 14.f : 10.f;
	News.Add(CustomRegiment.Name + TEXT(" påbegynder våbenombygning"));
	return true;
}
