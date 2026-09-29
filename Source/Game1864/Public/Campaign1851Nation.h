#pragma once

#include "CoreMinimal.h"

/**
 * Nations of the 1851 campaign (design manual: "Multi-nation AI parity", "Government, ministre og
 * granular AI-delegation"). Every nation runs on the same rules; who controls it (the player or the AI)
 * is only an assignment. The player can hand single portfolios to his ministers (the AI): MANUAL,
 * ADVISORY (they recommend, he decides) or AUTO (they act within his guardrails).
 *
 * Nations without a map of their own yet (Sverige-Norge, Preussen, Østrig) grow on an abstract model
 * (population, development budget, railway km, army, industry) until their map is made.
 */
enum class ECampaign1851Controller : uint8
{
	Player,
	AI
};

/** Portfolios a nation's government decides in (semantic, not historical titles). */
enum class ECampaign1851Portfolio : uint8
{
	Interior,      // Indenrigs: civil town buildings (schools, town halls, trade, industry)
	PublicWorks,   // Offentlige arbejder: chausséer and railways
	War,           // Krigsministeriet: training, officers and their posts
	Transport,     // Transport: troop trains
	Count
};

enum class ECampaign1851Delegation : uint8
{
	Manual,     // the player decides alone
	Advisory,   // the ministry recommends; the player carries it out (or not)
	Auto        // the ministry acts on its own, within the guardrails
};

/** What a decision does, so an advice can be carried out later. */
enum class ECampaign1851DecisionKind : uint8
{
	Note,          // a report only (nations without a map)
	CivilBuilding, // A = town, Key = building
	LinkWork,      // A = link, B = ECampaign1851LinkWork
	TroopTrain,
	Training,      // A = regiment, B = ECampaign1851Program
	FillPost,      // A = officer, B = post code (see ACampaign1851Map::ExecuteDecision), Key = target
	Recruit
};

struct FCampaign1851Nation
{
	FString Id;
	FString Name;
	/** True when the nation has its own map (towns, amter, links, army); else it grows on the abstract model. */
	bool bOnMap = false;
	ECampaign1851Controller Controller = ECampaign1851Controller::AI;
	ECampaign1851Delegation Modes[int32(ECampaign1851Portfolio::Count)] = { ECampaign1851Delegation::Auto, ECampaign1851Delegation::Auto, ECampaign1851Delegation::Auto, ECampaign1851Delegation::Auto };
	/** Priorities of the government per portfolio (historical tendency, then varied by the campaign seed). */
	float BaseWeights[int32(ECampaign1851Portfolio::Count)] = { 1.f, 1.f, 1.f, 1.f };
	float Weights[int32(ECampaign1851Portfolio::Count)] = { 1.f, 1.f, 1.f, 1.f };
	/** 0 bold .. 1 careful: how much of the money above the reserve it dares spend in a month. */
	float Caution = 0.5f;
	/** Guardrail: cash the ministries must leave in the treasury. */
	double Reserve = 100000.0;
	FString Note;

	// The abstract model (nations without a map). Money in rigsdaler-equivalents.
	double Population = 0.0;
	double Treasury = 0.0;
	double TaxPerHead = 0.25;   // the development budget per head and year
	double RailKm = 0.0;
	double ArmyMen = 0.0;
	double Industry = 1.0;      // an index: 1 in 1851
	float BaseGrowth = 1.f;     // % a year, historical estimate
	float GrowthMul = 1.f;      // the campaign's variation

	bool IsPlayer() const { return Controller == ECampaign1851Controller::Player; }
	/** How a portfolio is run: an AI nation runs everything itself. */
	ECampaign1851Delegation Mode(ECampaign1851Portfolio P) const { return IsPlayer() ? Modes[int32(P)] : ECampaign1851Delegation::Auto; }
};

/** A decision or recommendation of a ministry, with its reasons (explainable AI). */
struct FCampaign1851Decision
{
	double Day = 0.0;   // campaign days
	int32 Nation = 0;
	ECampaign1851Portfolio Portfolio = ECampaign1851Portfolio::Interior;
	ECampaign1851DecisionKind Kind = ECampaign1851DecisionKind::Note;
	FString Action;
	FString Reasons;
	double Cost = 0.0;
	bool bDone = false;       // carried out (AUTO, or the player took the advice)
	bool bAdvice = false;     // a recommendation (ADVISORY)
	int32 A = INDEX_NONE;
	int32 B = 0;
	FString Key;
	float Score = 0.f;
};

/** Growth effect of a finished civil building on its town and amt. */
struct FCampaign1851CivilEffect
{
	float UrbanGrowth = 0.f;   // percentage points a year for the town
	float RuralGrowth = 0.f;   // for the amt's countryside
	int32 IncomeRd = 0;        // trade tax, customs, postage a year to the state
	bool bPrivate = false;     // private investors may raise it on their own
};

namespace Campaign1851Nations
{
	const TCHAR* PortfolioName(ECampaign1851Portfolio P);
	/** What the portfolio covers, for the player. */
	const TCHAR* PortfolioScope(ECampaign1851Portfolio P);
	const TCHAR* DelegationName(ECampaign1851Delegation D);
	/** Civil buildings' effects by building key (null for military and other buildings). */
	const FCampaign1851CivilEffect* CivilEffect(const FString& Key);

	/** Base growth a year (%) before buildings and railways: the kingdom of the 1850s. */
	constexpr float BaseUrbanGrowth = 1.4f;
	constexpr float BaseRuralGrowth = 0.8f;
	/** A railway station in town, and a chaussée out of it (percentage points a year). */
	constexpr float StationGrowth = 0.6f;
	constexpr float ChausseeGrowth = 0.15f;
}
