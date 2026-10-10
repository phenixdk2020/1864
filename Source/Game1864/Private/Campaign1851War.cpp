// Data-driven monthly events (Docs/Events.md), war outbreak, enemy corps and occupation.

#include "Campaign1851Map.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/DefaultValueHelper.h"

namespace CampaignEventMVP
{
	constexpr float WarThreshold = 80.f;
	// Grammar: conjunction of atoms; an atom is [ikke] fact [comparison number].
	bool Expression(const FString& Input, TFunctionRef<bool(const FString&, float&)> Fact, FString& Why, bool& Valid)
	{
		Valid = true;
		if (Input.IsEmpty()) { Why = TEXT("ingen blokering"); return true; }
		TArray<FString> Atoms, Details;
		Input.ParseIntoArray(Atoms, TEXT(" og "), false);
		bool Result = true;
		for (FString Atom : Atoms)
		{
			Atom.TrimStartAndEndInline();
			const bool Negate = Atom.RemoveFromStart(TEXT("ikke "));
			TArray<FString> Tokens; Atom.ParseIntoArrayWS(Tokens);
			float Value = 0.f;
			bool Ok = Tokens.Num() == 1 || Tokens.Num() == 3;
			Ok = Ok && Fact(Tokens[0], Value);
			bool Truth = Value != 0.f;
			if (Ok && Tokens.Num() == 3)
			{
				float Limit = 0.f;
				Ok = FDefaultValueHelper::ParseFloat(Tokens[2], Limit) && FMath::IsFinite(Limit);
				const FString& Op = Tokens[1];
				if (Op == TEXT("<")) Truth = Value < Limit;
				else if (Op == TEXT("<=")) Truth = Value <= Limit;
				else if (Op == TEXT(">")) Truth = Value > Limit;
				else if (Op == TEXT(">=")) Truth = Value >= Limit;
				else if (Op == TEXT("==")) Truth = Value == Limit;
				else if (Op == TEXT("!=")) Truth = Value != Limit;
				else Ok = false;
			}
			Truth = Ok && (Negate ? !Truth : Truth);
			Valid &= Ok; Result &= Truth;
			Details.Add(FString::Printf(TEXT("%s%s: %s (værdi %.2f)"), Negate ? TEXT("ikke ") : TEXT(""), *Atom, !Ok ? TEXT("ugyldigt") : Truth ? TEXT("sand") : TEXT("falsk"), Value));
		}
		Why = FString::Join(Details, TEXT("; "));
		return Result && Valid;
	}
}

bool ACampaign1851Map::EventFact(const FString& Key, float& Out) const
{
	Out = 0.f;
	if (Key == TEXT("spaending")) Out = Tension;
	else if (Key == TEXT("stemning")) Out = Mood;
	else if (Key == TEXT("kasse")) Out = float(Treasury);
	else if (Key == TEXT("mobiliseret")) Out = Footing != ECampaign1851Footing::Peace ? 1.f : 0.f;
	else if (Key == TEXT("maegling")) Out = HasPeaceConference() ? 1.f : 0.f;
	else if (Key.StartsWith(TEXT("opinion.")))
	{
		const FString EventCurrent = Key.Mid(8);
		const int32 EventIndex = EventCurrent == TEXT("helstat") ? 0 : EventCurrent == TEXT("ejder") ? 1 : EventCurrent == TEXT("skandinavisk") ? 2 : INDEX_NONE;
		if (EventIndex == INDEX_NONE) return false;
		Out = Support[EventIndex];
	}
	else if (Key.StartsWith(TEXT("forhold.")) || Key.StartsWith(TEXT("garant.")))
	{
		const bool EventRelation = Key.StartsWith(TEXT("forhold."));
		const FString EventNationId = Key.Mid(EventRelation ? 8 : 7);
		const FCampaign1851Nation* EventNation = Nations.FindByPredicate([&](const FCampaign1851Nation& N) { return N.Id == EventNationId; });
		if (!EventNation) return false;
		Out = EventRelation ? EventNation->Relation : EventNation->bGuarantee ? 1.f : 0.f;
	}
	else if (Key.StartsWith(TEXT("forskning.")))
	{
		const FString EventTopic = Key.Mid(10);
		if (Campaign1851Research::FindTopic(EventTopic) == INDEX_NONE) return false;
		Out = HasResearch(*EventTopic) ? 1.f : 0.f;
	}
	else if (Key.StartsWith(TEXT("besat.")))
	{
		const int32 EventTown = FindCity(Key.Mid(6));
		if (!Cities.IsValidIndex(EventTown)) return false;
		Out = Cities[EventTown].Occupier.IsEmpty() ? 0.f : 1.f;
	}
	else if (Key.StartsWith(TEXT("vaerk.")))
	{
		const FString EventWork = Key.Mid(6);
		const int32 EventIndex = EventWork == TEXT("dannevirke") ? 0 : EventWork == TEXT("dybbol") ? 1 : EventWork == TEXT("fredericia") ? 2 : INDEX_NONE;
		if (EventIndex == INDEX_NONE) return false;
		Out = ProgrammeState.IsValidIndex(EventIndex) && ProgrammeState[EventIndex] == 2 ? 1.f : 0.f;
	}
	else return false;
	return true;
}

void ACampaign1851Map::ResetWar()
{
	SiegeTestTown = INDEX_NONE;
	SiegeTestCorpsId = 0;
	SiegeTestEndDay = -1.0;
	Tension = ActiveScenario().StartTension;   // 1851: after the war of 1848-50 an uneasy peace; 1825: a quiet one
	bAtWar = false;
	EventsFired.Reset();
	EventsBlocked.Reset();
	EnemyCorps.Reset();
	ResetNeighbourArmies();
	Battles.Reset();
	for (FCampaign1851City& C : Cities)
	{
		C.Occupier.Reset();
	}
	EventPlan.Reset();
	FString EventJson;
	const FString EventPath = FPaths::ProjectDir() / TEXT("Data/Campaign1851") / (ActiveScenario().Id == TEXT("1825") ? TEXT("Events_1825.json") : TEXT("Events.json"));
	TArray<TSharedPtr<FJsonValue>> EventRows;
	if (!FFileHelper::LoadFileToString(EventJson, *EventPath) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(EventJson), EventRows))
	{
		UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-EVENT|kunne ikke læse %s"), *EventPath); return;
	}
	TSet<FString> EventIds;
	for (const TSharedPtr<FJsonValue>& EventRow : EventRows)
	{
		const TSharedPtr<FJsonObject> EventObj = EventRow->Type == EJson::Object ? EventRow->AsObject() : nullptr;
		FPlannedEvent Event;
		const TSharedPtr<FJsonObject>* EventWindow = nullptr;
		double EventYear = 0, EventMonth = 0, EventChance = 0;
		bool EventValid = EventObj.IsValid() && EventObj->TryGetStringField(TEXT("id"), Event.Id) && !Event.Id.IsEmpty() && !EventIds.Contains(Event.Id)
			&& EventObj->TryGetStringField(TEXT("navn"), Event.Text) && EventObj->TryGetObjectField(TEXT("vindue"), EventWindow)
			&& (*EventWindow)->TryGetNumberField(TEXT("aar"), EventYear) && (*EventWindow)->TryGetNumberField(TEXT("maaned"), EventMonth)
			&& FMath::IsFinite(EventYear) && FMath::IsFinite(EventMonth) && EventYear >= 1 && EventYear <= 9998 && EventMonth >= 1 && EventMonth <= 12 && EventYear == FMath::FloorToDouble(EventYear) && EventMonth == FMath::FloorToDouble(EventMonth)
			&& EventObj->TryGetNumberField(TEXT("sandsynlighed"), EventChance) && FMath::IsFinite(EventChance) && EventChance >= 0 && EventChance <= 1
			&& EventObj->TryGetStringField(TEXT("forudsaetninger"), Event.Preconditions) && !Event.Preconditions.IsEmpty()
			&& EventObj->TryGetStringField(TEXT("blokeringer"), Event.Blockers)
			&& EventObj->TryGetStringField(TEXT("avis"), Event.Gazette) && EventObj->TryGetStringField(TEXT("log"), Event.CouncilLog);
		if (EventValid)
		{
			FString EventWhy; bool EventExpressionValid;
			const auto EventFacts = [this](const FString& EventK, float& V) { return EventFact(EventK, V); };
			CampaignEventMVP::Expression(Event.Preconditions, EventFacts, EventWhy, EventExpressionValid); EventValid &= EventExpressionValid;
			CampaignEventMVP::Expression(Event.Blockers, EventFacts, EventWhy, EventExpressionValid); EventValid &= EventExpressionValid;
			const auto EventReadEffects = [&](const TCHAR* EventField, TMap<FString, float>& EventDest)
			{
				const TSharedPtr<FJsonObject>* EventEffectObject = nullptr;
				if (!EventObj->TryGetObjectField(EventField, EventEffectObject) || (*EventEffectObject)->Values.IsEmpty()) return false;
				for (const auto& EventPair : (*EventEffectObject)->Values)
				{
					double EventAmount = 0; float EventExisting = 0;
					const FString EventK(EventPair.Key.ToView());   // the JSON map's key is a shared string in 5.8
					double EventCap = EventK == TEXT("spaending") ? 25 : EventK == TEXT("stemning") ? 10 : EventK == TEXT("kasse") ? 50000 : EventK == TEXT("gaeldrente") ? 100 : EventK == TEXT("forbundskorps") ? 1 : EventK.StartsWith(TEXT("forhold.")) ? 15 : EventK.StartsWith(TEXT("forskning.")) ? 1 : -1;
					if (EventCap < 0 || !EventPair.Value->TryGetNumber(EventAmount) || !FMath::IsFinite(EventAmount) || FMath::Abs(EventAmount) > EventCap
						|| ((EventK.StartsWith(TEXT("forhold.")) || EventK.StartsWith(TEXT("forskning."))) && !EventFact(EventK, EventExisting))
						|| ((EventK.StartsWith(TEXT("forskning.")) || EventK == TEXT("forbundskorps")) && EventAmount != 1))
					{ UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-EVENT|%s|afvist effekt %s"), *Event.Id, *EventK); return false; }
					EventDest.Add(EventK, float(EventAmount));
				}
				return true;
			};
			EventValid &= EventReadEffects(TEXT("effekter"), Event.Effects);
			EventValid &= EventReadEffects(TEXT("udeblivelse"), Event.BlockedEffects);
			EventValid &= Event.BlockedEffects.FindRef(TEXT("spaending")) != 0.f || Event.BlockedEffects.FindRef(TEXT("kasse")) != 0.f || Event.BlockedEffects.FindRef(TEXT("stemning")) != 0.f;
		}
		if (!EventValid) { UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN-EVENT|afvist event %s"), *Event.Id); continue; }
		EventIds.Add(Event.Id);
		Event.Day = (FDateTime(int32(EventYear), int32(EventMonth), 1) - StartDate()).GetTotalDays();
		Event.Probability = float(EventChance);
		EventPlan.Add(MoveTemp(Event));
	}
}

void ACampaign1851Map::EvaluateEvents(bool bMonthly)
{
	FString EventForcedText, EventWhyId;
	FParse::Value(FCommandLine::Get(), TEXT("CampaignEvents="), EventForcedText);
	FParse::Value(FCommandLine::Get(), TEXT("CampaignEventWhy="), EventWhyId);
	TArray<FString> EventForced; EventForcedText.ParseIntoArray(EventForced, TEXT(","), true);
	const bool EventVerbose = FParse::Param(FCommandLine::Get(), TEXT("CampaignEventLog"));
	const int32 EventToday = FMath::FloorToInt(CampaignDays);
	for (const FPlannedEvent& Event : EventPlan)
	{
		const bool EventForce = EventForced.Contains(Event.Id);
		const bool EventResolved = EventsFired.Contains(Event.Id) || EventsBlocked.Contains(Event.Id);
		if (!bMonthly && !EventForce && Event.Id != EventWhyId) continue;
		FString EventWhy, EventBlockWhy; bool EventValid = true, EventBlockValid = true;
		const auto EventFacts = [this](const FString& EventK, float& V) { return EventFact(EventK, V); };
		const bool EventPreconditions = CampaignEventMVP::Expression(Event.Preconditions, EventFacts, EventWhy, EventValid);
		const bool EventBlocked = !Event.Blockers.IsEmpty() && CampaignEventMVP::Expression(Event.Blockers, EventFacts, EventBlockWhy, EventBlockValid);
		const float EventChance = FMath::Clamp(Event.Probability * (1.f + (0.5f - Event.Probability) * Deviation), 0.f, 1.f);
		// Stateless draw: ordering, debug explanations and save/load never consume random state.
		const uint32 EventSeed = HashCombine(HashCombine(uint32(Seed), GetTypeHash(ActiveScenario().Id)), GetTypeHash(Event.Id));
		const float EventRoll = FRandomStream(int32(HashCombine(EventSeed, uint32(EventToday)))).FRand();
		const bool EventDue = CampaignDays >= Event.Day;
		if (EventVerbose || Event.Id == EventWhyId)
			UE_LOG(LogTemp, Display, TEXT("CAMPAIGN-EVENT|%s|dag=%d|vindue=%d|afgjort=%d|chance=%.4f|kast=%.4f|%s|blokering: %s"),
				*Event.Id, EventToday, EventDue ? 1 : 0, EventResolved ? 1 : 0, EventChance, EventRoll, *EventWhy, *EventBlockWhy);
		if (EventResolved || (!EventForce && (!bMonthly || !EventDue))) continue;
		const bool EventFired = EventForce || (EventValid && EventBlockValid && EventPreconditions && !EventBlocked && EventRoll < EventChance);
		if (EventFired) EventsFired.AddUnique(Event.Id); else EventsBlocked.AddUnique(Event.Id);
		FString EventExplanation = EventForce ? TEXT("Tvunget af CampaignEvents") : !EventPreconditions ? EventWhy : EventBlocked ? TEXT("Blokering: ") + EventBlockWhy
			: EventFired ? EventWhy : FString::Printf(TEXT("Forudsætninger opfyldt: %s; tilfældighed %.3f >= %.3f"), *EventWhy, EventRoll, EventChance);
		// Cite an existing related decision, never invent a player action for a random failure.
		const FString CausalExpression = !EventPreconditions ? Event.Preconditions : EventBlocked ? Event.Blockers : Event.Preconditions;
		FString EventSource = TEXT("Kilde: kampagnens aktuelle tilstand; ingen registreret relevant handling");
		for (int32 DecisionIndex = Decisions.Num() - 1; DecisionIndex >= 0; --DecisionIndex)
		{
			const FCampaign1851Decision& EventPrevious = Decisions[DecisionIndex];
			if (EventPrevious.Nation != PlayerNation || !EventPrevious.bDone || EventPrevious.Action.StartsWith(TEXT("Begivenhed:"))) continue;
			TArray<FString> SourceFacts; EventPrevious.Key.ParseIntoArray(SourceFacts, TEXT(","), true);
			const bool EventRelated = SourceFacts.ContainsByPredicate([&](const FString& EventK)
			{
				TArray<FString> SourceAtoms; CausalExpression.ParseIntoArray(SourceAtoms, TEXT(" og "), false);
				for (FString SourceAtom : SourceAtoms)
				{
					SourceAtom.TrimStartAndEndInline();
					FString SourceKey = SourceAtom; SourceKey.RemoveFromStart(TEXT("ikke "));
					TArray<FString> SourceTokens; SourceKey.ParseIntoArrayWS(SourceTokens);
					if (SourceTokens.IsEmpty() || SourceTokens[0] != EventK) continue;
					FString SourceWhy; bool SourceValid;
					const bool SourceTrue = CampaignEventMVP::Expression(SourceAtom, EventFacts, SourceWhy, SourceValid);
					if (SourceValid && (EventPreconditions || !SourceTrue)) return true;
				}
				return false;
			});
			if (!EventRelated) continue;
			EventSource = FString::Printf(TEXT("Kilde: egen registreret beslutning '%s' (dag %.0f); aktuel tilstand ovenfor"), *EventPrevious.Action, EventPrevious.Day);
			break;
		}
		EventExplanation += TEXT(". ") + EventSource;
		const TMap<FString, float>& EventApplied = EventFired ? Event.Effects : Event.BlockedEffects;
		TArray<FString> EffectKeys; EventApplied.GetKeys(EffectKeys); EffectKeys.Sort();
		TArray<FString> EffectSummary;
		for (const FString& EventK : EffectKeys)
		{
			const float EventAmount = EventApplied[EventK];
			float EventActualAmount = EventAmount;
			const float EventBeforeTension = Tension, EventBeforeMood = Mood, EventBeforeRate = DebtRate;
			if (EventK == TEXT("spaending")) Tension = FMath::Clamp(Tension + FMath::Clamp(EventAmount * (EventAmount > 0.f ? GuaranteeDamping() * GovernmentTensionFactor() : 1.f), -25.f, 25.f), 0.f, 100.f);
			else if (EventK == TEXT("stemning")) Mood = FMath::Clamp(Mood + EventAmount, 0.f, 100.f);
			else if (EventK == TEXT("kasse")) AddTransaction(EventAmount, Event.Text);
			else if (EventK == TEXT("gaeldrente")) DebtRate = FMath::Clamp(DebtRate + EventAmount / 100.f, 0.f, 1.f);
			else if (EventK.StartsWith(TEXT("forskning."))) GrantResearch(EventK.Mid(10));
			else if (EventK.StartsWith(TEXT("forhold.")))
			{
				for (FCampaign1851Nation& EventNation : Nations)
					if (EventNation.Id == EventK.Mid(8))
					{
						const float EventBeforeRelation = EventNation.Relation;
						EventNation.Relation = FMath::Clamp(EventNation.Relation + EventAmount, -100.f, 100.f);
						EventActualAmount = EventNation.Relation - EventBeforeRelation;
					}
			}
			if (EventK == TEXT("spaending")) EventActualAmount = Tension - EventBeforeTension;
			else if (EventK == TEXT("stemning")) EventActualAmount = Mood - EventBeforeMood;
			else if (EventK == TEXT("gaeldrente")) EventActualAmount = (DebtRate - EventBeforeRate) * 100.f;
			EffectSummary.Add(FString::Printf(TEXT("%s %+.2f"), *EventK, EventActualAmount));
		}
		const FString EventHeadline = EventFired ? Event.Gazette : Event.Text + TEXT(" udebliver");
		News.Add(EventHeadline);
		News.Add(EventExplanation + TEXT(". Følger: ") + FString::Join(EffectSummary, TEXT(", ")));
		FCampaign1851Decision EventDecision;
		EventDecision.Day = CampaignDays;
		EventDecision.Nation = PlayerNation;
		EventDecision.Portfolio = ECampaign1851Portfolio::War;
		EventDecision.Action = TEXT("Begivenhed: ") + (EventFired ? Event.CouncilLog : Event.Text + TEXT(" udebliver"));
		EventDecision.Reasons = EventExplanation + TEXT(". Følger: ") + FString::Join(EffectSummary, TEXT(", "));
		EventDecision.bDone = true;
		AddDecision(EventDecision);
		// Closed, explicit military consequence selected by the data.
		if (EventApplied.Contains(TEXT("forbundskorps")) && !bAtWar)
			SpawnCorps(TEXT("Forbundskorpset (Sachsen, Hannover)"), TEXT("DE"), 0.09f, TEXT("Altona"), { TEXT("Rendsborg") }, 0.f);
	}
	if (!bAtWar && Tension >= CampaignEventMVP::WarThreshold) DeclareWar();
}

void ACampaign1851Map::DailyWar()
{
	EvaluateEvents(false);
	if (!bAtWar && Tension >= CampaignEventMVP::WarThreshold)
	{
		DeclareWar();
	}
	// Liberation: an occupied town (not ceded) is free again when 300 Danish soldiers or more have stood within
	// 3 km of it for two days with no enemy corps within 10 km; its amt then pays again.
	for (int32 c = 0; c < Cities.Num(); ++c)
	{
		FCampaign1851City& City = Cities[c];
		if (City.Occupier.IsEmpty() || City.bCeded)
		{
			City.LiberationDays = 0.f;
			continue;
		}
		const FVector2D At = TownKm(c);
		int32 Men = 0;
		for (const FCampaign1851Regiment& R : Regiments)
		{
			Men += !R.IsMarching() && FVector2D::Distance(R.Km, At) < 3.0 ? R.PresentMen() : 0;
		}
		const bool bEnemyNear = EnemyCorps.ContainsByPredicate([&](const FCampaign1851EnemyCorps& E) { return E.Men > 0 && FVector2D::Distance(E.Km, At) < 10.0; });
		City.LiberationDays = Men >= LiberationMen && !bEnemyNear ? City.LiberationDays + 1.f : 0.f;
		if (City.LiberationDays >= 2.f)
		{
			City.Occupier.Reset();
			City.LiberationDays = 0.f;
			News.Add(FString::Printf(TEXT("%s er befriet: de danske tropper har holdt byen i to døgn"), *City.Name));
		}
	}
}

void ACampaign1851Map::MonthlyWar()
{
	EvaluateEvents(true);
	// The tension drifts back towards an uneasy peace; a mobilised Danish army raises it.
	if (!bAtWar)
	{
		Tension += Footing != ECampaign1851Footing::Peace ? 3.f * GuaranteeDamping() : (25.f - Tension) * 0.03f;
		Tension = FMath::Clamp(Tension, 0.f, 100.f);
	}
}

void ACampaign1851Map::DeclareWar()
{
	bAtWar = true;
	WarStartDay = CampaignDays;
	DanishWarLosses = EnemyWarLosses = 0;
	News.Add(TEXT("KRIG: Preussen og Østrig erklærer Danmark krig"));
	FCampaign1851Decision D;
	D.Day = CampaignDays;
	D.Nation = Nations.IndexOfByPredicate([](const FCampaign1851Nation& N) { return N.Id == TEXT("PR"); });
	D.Portfolio = ECampaign1851Portfolio::War;
	D.Action = TEXT("Preussen og Østrig erklærer krig");
	D.Reasons = FString::Printf(TEXT("spændingen nåede %.0f"), Tension);
	D.bDone = true;
	AddDecision(D);
	// The allied corps (a share of each nation's army on the abstract model) come over the Eider.
	SpawnCorps(TEXT("Preussisk I. Korps"), TEXT("PR"), 0.2f, TEXT("Kiel"), { TEXT("Egernførde"), TEXT("Slesvig"), TEXT("Flensborg"), TEXT("Sønderborg") }, 0.f);
	SpawnCorps(TEXT("Østrigsk VI. Korps"), TEXT("AT"), 0.06f, TEXT("Neumünster"), { TEXT("Rendsborg"), TEXT("Slesvig"), TEXT("Flensborg"), TEXT("Kolding"), TEXT("Fredericia") }, 0.f);
}

void ACampaign1851Map::SpawnCorps(const FString& Name, const FString& NationId, float ShareOfArmy, const FString& From, const TArray<FString>& Objectives, float Delay)
{
	const int32 Town = FindCity(From);
	if (!Cities.IsValidIndex(Town))
	{
		return;
	}
	const FCampaign1851Nation* N = Nations.FindByPredicate([&NationId](const FCampaign1851Nation& X) { return X.Id == NationId; });
	FCampaign1851EnemyCorps C;
	C.Id = NextCorpsId++;
	C.Name = Name;
	C.Nation = NationId;
	C.Men = N ? FMath::RoundToInt(N->ArmyMen * ShareOfArmy) : 12000;
	C.StartMen = C.Men;
	C.SeenKm = TownKm(Town);
	C.SeenDay = CampaignDays;
	C.SeenMen = C.Men;
	C.Guns = FMath::Max(12, C.Men / 250);
	C.Km = TownKm(Town);
	C.Town = Town;
	for (const FString& O : Objectives)
	{
		if (FindCity(O) != INDEX_NONE)
		{
			C.Objectives.Add(FindCity(O));
		}
	}
	EnemyCorps.Add(C);
	News.Add(FString::Printf(TEXT("%s (%d mand, %d kanoner) står ved %s"), *C.Name, C.Men, C.Guns, *Cities[Town].Name));
}

void ACampaign1851Map::AdvanceWar(float DeltaDays)
{
	if (DeltaDays <= 0.f)
	{
		return;
	}
	if (FMath::FloorToInt(CampaignDays) != LastWarDay)
	{
		LastWarDay = FMath::FloorToInt(CampaignDays);
		DailyWar();
		DailyWeather();
		DailyHealth();
		DailyOfficers();
		DailySieges();
		LogSiegeTest(TEXT("døgn"));
		DailyBridges();
		EnemyReinforcements();
	}
	AdvanceNeighbourArmies();
	UpdateIntel();
	if (Battles.ContainsByPredicate([](const FCampaign1851Battle& B) { return B.bWaiting; }) && FPlatformTime::Seconds() - LastBattlePoll > 1.0)
	{
		LastBattlePoll = FPlatformTime::Seconds();
		PollBattleResults();
	}
	for (int32 k = 0; k < EnemyCorps.Num(); ++k)
	{
		FCampaign1851EnemyCorps& C = EnemyCorps[k];
		if (C.bEngaged || CampaignDays < C.RestUntil)
		{
			continue;
		}
		// Come before the position it means to besiege: it digs in there.
		if (bAtWar && Cities.IsValidIndex(C.SiegeTown) && !C.bSieging && FVector2D::Distance(C.Km, TownKm(C.SiegeTown)) < 9.0)
		{
			const FVector2D SiegeCentre = TownKm(C.SiegeTown);
			const FVector2D SiegeOffset = C.Km - SiegeCentre;
			C.Km = SiegeCentre + (SiegeOffset.SizeSquared() > 0.01 ? SiegeOffset / SiegeOffset.Size() : FVector2D(0.0, -1.0)) * 8.9;
			C.bSieging = true;
			C.SiegeStart = CampaignDays;
			C.Route.Reset();
			C.Town = INDEX_NONE;
			News.Add(FString::Printf(TEXT("%s belejrer stillingen ved %s"), *C.Name, *Cities[C.SiegeTown].Name));
		}
		// Contact: Danish troops or a fort within 5 km stop the corps; a battle is at hand. A besieging corps
		// fights only a relief from outside the position.
		FString Contact;
		const FVector2D SiegeAt = C.bSieging && Cities.IsValidIndex(C.SiegeTown) ? TownKm(C.SiegeTown) : FVector2D(1e9, 1e9);
		for (const FCampaign1851Regiment& R : Regiments)
		{
			if (FVector2D::Distance(R.Km, C.Km) < 5.0 && R.Men > 0 && FVector2D::Distance(R.Km, SiegeAt) > 10.0)
			{
				Contact = R.Name;
				break;
			}
		}
		for (const FCampaign1851Fort& F : Forts)
		{
			if (Contact.IsEmpty() && FVector2D::Distance(F.Km, C.Km) < 5.0 && F.bBuilt && !C.bSieging)
			{
				Contact = F.Name;
			}
		}
		if (C.bSieging && Contact.IsEmpty())
		{
			continue;   // the siege goes on (DailySieges)
		}
		if (!Contact.IsEmpty() && bAtWar)
		{
			C.bEngaged = true;
			const int32 Near = NearestTown(C.Km);
			News.Add(FString::Printf(TEXT("%s møder %s ved %s: slag forestår"), *C.Name, *Contact, Cities.IsValidIndex(Near) ? *Cities[Near].Name : TEXT("")));
			CreateBattle(k);
			continue;
		}
		if (!Contact.IsEmpty())
		{
			continue;   // not at war yet: the federal corps faces the Danes and waits
		}
		// Marching: along the roads to the next objective (only while at war; the federal corps waits at the Eider).
		if (!bAtWar && C.Nation == TEXT("DE"))
		{
			if (C.Objectives.Num() > 0 && C.Route.Num() == 0 && C.Town == C.Objectives[0])
			{
				continue;
			}
		}
		if (!bAtWar && C.Nation != TEXT("DE"))
		{
			continue;
		}
		const bool bNeighbourHolding = NeighbourArmies.ContainsByPredicate([&](const FCampaign1851NeighbourArmy& A) { return A.CorpsId == C.Id && A.bGarrison; });
		if (bNeighbourHolding) { continue; } // local garrisons fight contact, but never march away
		if (C.Route.Num() == 0)
		{
			// The enemy chooses its next objective by what it believes of the Danish defence.
			if (bAtWar && C.Nation != TEXT("DE") && !ChooseCorpsObjective(k))
			{
				continue;
			}
			if (C.Objectives.Num() == 0)
			{
				continue;
			}
			const int32 Goal = C.Objectives[0];
			TArray<FCampaign1851Leg> Legs;
			if (C.Town == Goal || !PlanMarch(C.Town, C.Km, Goal, TownKm(Goal), 16.f, ECampaign1851RouteMode::RoadsOnly, Legs, nullptr, C.Nation))
			{
				C.Objectives.RemoveAt(0);
				continue;
			}
			// Over water: the Danish fleet bars the Belts; a narrow sound takes a week of gathering boats.
			FString Ferry;
			const int32 Water = WaterCrossing(C, Legs, Ferry);
			if (Water == 2)
			{
				News.Add(FString::Printf(TEXT("%s kan ikke gå over %s: den danske flåde behersker farvandet"), *C.Name, *Ferry));
				C.Barred.AddUnique(Goal);
				C.Objectives.RemoveAt(0);
				C.RestUntil = CampaignDays + 2.0;
				continue;
			}
			if (Water == 1)
			{
				if (C.CrossingReadyDay < 0.0)
				{
					C.CrossingReadyDay = CampaignDays + 7.0;
					News.Add(FString::Printf(TEXT("Efterretning: %s samler både ved %s"), *C.Name, *Ferry));
				}
				continue;
			}
			C.CrossingReadyDay = -1.0;
			C.Route = MoveTemp(Legs);
			C.Leg = 0;
			C.LegElapsed = 0.f;
			C.Town = INDEX_NONE;
		}
		C.LegElapsed += DeltaDays * (C.Route.IsValidIndex(C.Leg) ? LegPace(C.Route[C.Leg]) : 1.f);
		while (C.Route.IsValidIndex(C.Leg) && C.LegElapsed >= C.Route[C.Leg].Days)
		{
			C.LegElapsed -= C.Route[C.Leg].Days;
			C.Km = C.Route[C.Leg].ToKm;
			++C.Leg;
		}
		if (C.Route.IsValidIndex(C.Leg))
		{
			const TArray<FVector2D> Line = LegLine(C.Route[C.Leg]);
			C.Km = AlongLine(Line, LineLength(Line) * FMath::Clamp(C.LegElapsed / FMath::Max(C.Route[C.Leg].Days, 0.001f), 0.f, 1.f));
		}
		else
		{
			// Arrived at the objective: an undefended town is occupied.
			const int32 Goal = C.Objectives.Num() > 0 ? C.Objectives[0] : INDEX_NONE;
			C.Route.Reset();
			C.Town = Goal;
			const bool bDefended = Cities.IsValidIndex(Goal) && (HasFortsNear(Goal) || C.SiegeTown == Goal || Regiments.ContainsByPredicate([&](const FCampaign1851Regiment& R) { return R.Men > 0 && FVector2D::Distance(R.Km, TownKm(Goal)) < 5.0; }));
			if (Cities.IsValidIndex(Goal) && bDefended)
			{
				C.Km = TownKm(Goal);   // the defenders are met at the town: contact next day
			}
			else if (Cities.IsValidIndex(Goal))
			{
				C.Km = TownKm(Goal);
				if (Cities[Goal].Occupier.IsEmpty() && !Cities[Goal].bForeign)
				{
					Cities[Goal].Occupier = C.Nation == TEXT("DE") ? TEXT("PR") : C.Nation;
					News.Add(FString::Printf(TEXT("%s er besat af %s"), *Cities[Goal].Name, *C.Name));
				}
			}
			if (C.Objectives.Num() > 0 && (bAtWar || C.Nation != TEXT("DE")))
			{
				C.Objectives.RemoveAt(0);
			}
		}
	}
}

bool ACampaign1851Map::IsAmtOccupied(const FCampaign1851Amt& A) const
{
	const int32 Seat = FindCity(A.Seat);
	return Cities.IsValidIndex(Seat) && !Cities[Seat].Occupier.IsEmpty();
}

TArray<FString> ACampaign1851Map::SaveWar() const
{
	TArray<FString> Out;
	Out.Add(FString::Printf(TEXT("state|%.2f|%d"), Tension, bAtWar ? 1 : 0));
	Out.Add(FString::Printf(TEXT("intel-clock|%d"), LastWarDay));
	Out.Add(TEXT("events-format|29"));
	for (const FString& EventId : EventsBlocked) Out.Add(TEXT("blocked|") + EventId);
	for (const FString& E : EventsFired)
	{
		Out.Add(FString::Printf(TEXT("fired|%s"), *E));
	}
	for (const FCampaign1851City& C : Cities)
	{
		if (!C.Occupier.IsEmpty())
		{
			Out.Add(FString::Printf(TEXT("occupied|%s|%s"), *C.Name, *C.Occupier));
		}
	}
	for (const FCampaign1851EnemyCorps& C : EnemyCorps)
	{
		FString Objectives;
		for (int32 O : C.Objectives)
		{
			Objectives += (Objectives.IsEmpty() ? TEXT("") : TEXT(",")) + Cities[O].Name;
		}
		// Saved where it stands (on the march: the town at the end of its current stretch, or the point).
		const int32 At = C.Route.IsValidIndex(C.Leg) ? C.Route[C.Leg].To : C.Town;
		Out.Add(FString::Printf(TEXT("corps|%s|%s|%d|%d|%.3f|%.3f|%s|%s|%d"), *C.Name, *C.Nation, C.Men, C.Guns, C.Km.X, C.Km.Y,
			Cities.IsValidIndex(At) ? *Cities[At].Name : TEXT(""), *Objectives, C.bEngaged ? 1 : 0));
	}
	for (int32 k = 0; k < EnemyCorps.Num(); ++k)
	{
		const FCampaign1851EnemyCorps& C = EnemyCorps[k];
		Out.Add(FString::Printf(TEXT("intel|%d|%.3f|%.3f|%.2f|%d|%d"), k, C.SeenKm.X, C.SeenKm.Y, C.SeenDay, C.SeenMen, C.StartMen));
		FString Barred;
		for (int32 Town : C.Barred)
		{
			Barred += (Barred.IsEmpty() ? TEXT("") : TEXT(",")) + FString::FromInt(Town);
		}
		Out.Add(FString::Printf(TEXT("intel-runtime|%d|%d|%d|%.6f|%.6f|%.6f|%.6f|%d|%.6f|%.6f|%d|%.6f|%s"),
			k, C.Id, C.bSeen ? 1 : 0, C.ReportKm.X, C.ReportKm.Y, C.ReportDay, C.ReportArrive, C.ReportMen,
			C.RestUntil, C.NextThink, C.bWaitingNoted ? 1 : 0, C.CrossingReadyDay,
			*Barred));
		if (Cities.IsValidIndex(C.SiegeTown))
		{
			Out.Add(FString::Printf(TEXT("siege|%d|%s|%d|%.2f"), k, *Cities[C.SiegeTown].Name, C.bSieging ? 1 : 0, C.SiegeStart));
		}
	}
	// Battles at hand (one sent to 3D waits for its result across the trip to the battle map and back).
	for (const FCampaign1851Battle& B : Battles)
	{
		FString Units, FortIds;
		for (int32 r : B.Regiments) { if (Regiments.IsValidIndex(r)) { Units += (Units.IsEmpty() ? TEXT("") : TEXT(",")) + Regiments[r].Id; } }
		for (int32 f : B.Forts) { FortIds += (FortIds.IsEmpty() ? TEXT("") : TEXT(",")) + FString::FromInt(f); }
		Out.Add(FString::Printf(TEXT("battle|%d|%.4f|%.3f|%.3f|%d|%d|%s|%s|%d"), B.Id, B.Day, B.Km.X, B.Km.Y, B.Town, CorpsIndexOf(B), *Units, *FortIds, B.bWaiting ? 1 : 0));
	}
	return Out;
}

void ACampaign1851Map::RestoreWar(const TArray<FString>& Lines)
{
	// Definitions come from scenario data; resolved outcomes come from the save.
	ResetWar();
	LastWarDay = FMath::FloorToInt(CampaignDays);
	// A save from before the event files: the events already past are taken as resolved.
	if (!Lines.Contains(TEXT("events-format|29")))
	{
		for (const FPlannedEvent& Event : EventPlan)
			if (Event.Day < CampaignDays) EventsBlocked.AddUnique(Event.Id);
	}
	for (const FString& Line : Lines)
	{
		TArray<FString> P;
		Line.ParseIntoArray(P, TEXT("|"), false);
		if (P.Num() == 3 && P[0] == TEXT("state"))
		{
			Tension = FCString::Atof(*P[1]);
			bAtWar = P[2] == TEXT("1");
		}
		else if (P.Num() == 2 && P[0] == TEXT("intel-clock"))
		{
			LastWarDay = FCString::Atoi(*P[1]);
		}
		else if (P.Num() == 14 && P[0] == TEXT("intel-runtime") && EnemyCorps.IsValidIndex(FCString::Atoi(*P[1])))
		{
			FCampaign1851EnemyCorps& C = EnemyCorps[FCString::Atoi(*P[1])];
			C.Id = FCString::Atoi(*P[2]);
			NextCorpsId = FMath::Max(NextCorpsId, C.Id + 1);
			C.bSeen = P[3] == TEXT("1");
			C.ReportKm = FVector2D(FCString::Atod(*P[4]), FCString::Atod(*P[5]));
			C.ReportDay = FCString::Atod(*P[6]);
			C.ReportArrive = FCString::Atod(*P[7]);
			C.ReportMen = FCString::Atoi(*P[8]);
			C.RestUntil = FCString::Atod(*P[9]);
			C.NextThink = FCString::Atod(*P[10]);
			C.bWaitingNoted = P[11] == TEXT("1");
			C.CrossingReadyDay = FCString::Atod(*P[12]);
			TArray<FString> Barred;
			P[13].ParseIntoArray(Barred, TEXT(","));
			for (const FString& Town : Barred)
			{
				const int32 Index = FCString::Atoi(*Town);
				if (Cities.IsValidIndex(Index)) { C.Barred.AddUnique(Index); }
			}
		}
		else if (P.Num() == 7 && P[0] == TEXT("intel") && EnemyCorps.IsValidIndex(FCString::Atoi(*P[1])))
		{
			FCampaign1851EnemyCorps& C = EnemyCorps[FCString::Atoi(*P[1])];
			C.SeenKm = FVector2D(FCString::Atod(*P[2]), FCString::Atod(*P[3]));
			C.SeenDay = FCString::Atod(*P[4]);
			C.SeenMen = FCString::Atoi(*P[5]);
			C.StartMen = FMath::Max(0, FCString::Atoi(*P[6]));
		}
		else if (P.Num() == 5 && P[0] == TEXT("siege") && EnemyCorps.IsValidIndex(FCString::Atoi(*P[1])))
		{
			FCampaign1851EnemyCorps& C = EnemyCorps[FCString::Atoi(*P[1])];
			C.SiegeTown = FindCity(P[2]);
			C.bSieging = P[3] == TEXT("1");
			C.SiegeStart = FCString::Atod(*P[4]);
			if (C.bSieging) { C.Route.Reset(); C.Town = INDEX_NONE; }
		}
		else if (P.Num() == 2 && P[0] == TEXT("blocked"))
		{ EventsBlocked.AddUnique(P[1]); }
		else if (P.Num() == 2 && P[0] == TEXT("fired"))
		{
			EventsFired.AddUnique(P[1]);
			EventsBlocked.Remove(P[1]);
		}
		else if (P.Num() == 3 && P[0] == TEXT("occupied") && FindCity(P[1]) != INDEX_NONE)
		{
			Cities[FindCity(P[1])].Occupier = P[2];
		}
		else if (P.Num() == 10 && P[0] == TEXT("battle"))
		{
			FCampaign1851Battle B;
			B.Id = FCString::Atoi(*P[1]);
			B.Day = FCString::Atod(*P[2]);
			B.Km = FVector2D(FCString::Atod(*P[3]), FCString::Atod(*P[4]));
			B.Town = FCString::Atoi(*P[5]);
			const int32 k = FCString::Atoi(*P[6]);
			if (!EnemyCorps.IsValidIndex(k))
			{
				continue;
			}
			B.CorpsId = EnemyCorps[k].Id;
			EnemyCorps[k].bEngaged = true;
			TArray<FString> Ids;
			P[7].ParseIntoArray(Ids, TEXT(","));
			for (const FString& Id : Ids) { if (FindRegiment(Id) != INDEX_NONE) { B.Regiments.Add(FindRegiment(Id)); } }
			Ids.Reset();
			P[8].ParseIntoArray(Ids, TEXT(","));
			for (const FString& Id : Ids) { B.Forts.Add(FCString::Atoi(*Id)); }
			B.bWaiting = P[9] == TEXT("1");
			NextBattleId = FMath::Max(NextBattleId, B.Id + 1);
			Battles.Add(B);
		}
		else if (P.Num() == 10 && P[0] == TEXT("corps"))
		{
			FCampaign1851EnemyCorps C;
			C.Id = NextCorpsId++;
			C.Name = P[1];
			C.Nation = P[2];
			C.Men = FCString::Atoi(*P[3]);
			C.Guns = FCString::Atoi(*P[4]);
			C.Km = FVector2D(FCString::Atod(*P[5]), FCString::Atod(*P[6]));
			C.Town = FindCity(P[7]);
			if (C.Town != INDEX_NONE)
			{
				C.Km = TownKm(C.Town);
			}
			TArray<FString> Objectives;
			P[8].ParseIntoArray(Objectives, TEXT(","));
			for (const FString& O : Objectives)
			{
				if (FindCity(O) != INDEX_NONE)
				{
					C.Objectives.Add(FindCity(O));
				}
			}
			C.bEngaged = false;   // a battle still at hand is offered again on contact
			C.StartMen = C.Men;
			C.SeenKm = C.Km;
			C.SeenMen = C.Men;
			C.bSeen = !bAtWar;
			EnemyCorps.Add(C);
		}
	}
}
