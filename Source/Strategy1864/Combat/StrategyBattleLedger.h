#pragma once

#include "CoreMinimal.h"

/** Counts are events, not the difference between strength snapshots (evacuated crews survive). */
struct FStrategyReportEquipment
{
    int32 Mortars = 0, Guns = 0, SmallArms = 0, Horses = 0, Wagons = 0, Colours = 0;
    void Add(const FStrategyReportEquipment& Other)
    {
        Mortars += Other.Mortars; Guns += Other.Guns; SmallArms += Other.SmallArms; Horses += Other.Horses;
        Wagons += Other.Wagons; Colours += Other.Colours;
    }
    FString Text() const
    {
        return FString::Printf(TEXT("%dK %dM %dG %dH %dV %dF"), Guns, Mortars, SmallArms, Horses, Wagons, Colours);
    }
};

struct FStrategyBattleLedger
{
    bool bInitialized = false, bFrozen = false, bEquipmentAbandoned = false;
    uint8 OriginalSide = 0;
    FString Name;
    int32 StartMen = 0, Killed = 0, Wounded = 0, Prisoners = 0;
    int32 EnemyKilled = 0;
    int32 OfficersWounded = 0, OfficersCaptured = 0;
    int32 AmmoFired = 0, Volleys = 0, LastVolleyRounds = 0, StartAmmo = 0;
    /** Food: every unit starts a battle with three days of provisions (no campaign figure is passed yet); the battle clock uses it up. */
    float StartFoodDays = 3.0f, FoodUsedDays = 0.0f;
    int32 AmmoLeft() const { return FMath::Max(0, StartAmmo - AmmoFired); }
    float FoodDaysLeft() const { return FMath::Max(0.0f, StartFoodDays - FoodUsedDays); }
    float CombatUntil = 0.f, CombatSeconds = 0.f, HighestMoraleLoss = 0.f, StartMorale = 100.f;
    FStrategyReportEquipment Captured, Lost, Abandoned;
    TArray<int32> VolleyRounds;
    TMap<FName, int32> LossByCause;
    void Add(const FStrategyBattleLedger& Other)
    {
        StartMen += Other.StartMen; Killed += Other.Killed; Wounded += Other.Wounded; Prisoners += Other.Prisoners;
        AmmoFired += Other.AmmoFired; Volleys += Other.Volleys; StartAmmo += Other.StartAmmo;
        VolleyRounds.Append(Other.VolleyRounds);
        for (const TPair<FName, int32>& ReportCause : Other.LossByCause) { LossByCause.FindOrAdd(ReportCause.Key) += ReportCause.Value; }
        EnemyKilled += Other.EnemyKilled; OfficersWounded += Other.OfficersWounded; OfficersCaptured += Other.OfficersCaptured;
        Captured.Add(Other.Captured); Lost.Add(Other.Lost);
        CombatSeconds += Other.CombatSeconds;
        HighestMoraleLoss = FMath::Max(HighestMoraleLoss, Other.HighestMoraleLoss);
    }
    FString Row(const FString& Label) const
    {
        // Label, men at start, killed, wounded, prisoners, captured kit, lost kit, ammunition left/start, food days left, rounds fired, combat seconds, morale loss.
        return FString::Printf(TEXT("%s\t%d\t%d\t%d\t%d\t%s\t%s\t%d/%d\t%.1f\t%d\t%.1f\t%.1f"), *Label, StartMen, Killed, Wounded,
            Prisoners, *Captured.Text(), *Lost.Text(), AmmoLeft(), StartAmmo, FoodDaysLeft(), AmmoFired, CombatSeconds, HighestMoraleLoss);
    }
};
