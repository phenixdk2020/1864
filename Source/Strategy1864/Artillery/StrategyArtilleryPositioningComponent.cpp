#include "StrategyArtilleryPositioningComponent.h"

#include "StrategyArtilleryBatteryUnit.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "Engine/World.h"

UStrategyArtilleryPositioningComponent::UStrategyArtilleryPositioningComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyArtilleryPositioningComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerBattery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
}

FStrategyTerrainPositionAssessment
UStrategyArtilleryPositioningComponent::EvaluatePosition(
    const FVector& CandidatePosition,
    const FVector& ThreatPosition) const
{
    FStrategyTerrainPositionAssessment Assessment;

    if (!OwnerBattery)
    {
        Assessment.Score = -100000.0f;
        return Assessment;
    }

    const FVector CandidateGround =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerBattery,
            CandidatePosition);

    const FVector ThreatGround =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerBattery,
            ThreatPosition);

    Assessment.GroundZ = CandidateGround.Z;

    Assessment.LocalSlopeDegrees =
        UStrategyTerrainQueryLibrary::GetLocalSlopeDegrees(
            OwnerBattery,
            CandidateGround,
            500.0f);

    Assessment.ElevationAdvantageCm =
        CandidateGround.Z - ThreatGround.Z;

    FVector Start = CandidateGround;
    FVector End = ThreatGround;
    Start.Z += 160.0f;
    End.Z += 120.0f;

    const bool bTerrainBlocked =
        UStrategyTerrainQueryLibrary::IsTerrainProfileOccluded(
            OwnerBattery,
            Start,
            End,
            15.0f,
            40);

    Assessment.bHasDirectLOS =
        !bTerrainBlocked &&
        HasPhysicalStaticLOS(Start, End);

    Assessment.bInDeadGroundFromThreat =
        UStrategyTerrainQueryLibrary::IsPointInDeadGroundFrom(
            OwnerBattery,
            ThreatGround,
            CandidateGround,
            160.0f,
            120.0f);

    FVector CrestPoint;
    float CrestExcess = 0.0f;

    const bool bHasCrest =
        UStrategyTerrainQueryLibrary::FindCrestPoint(
            OwnerBattery,
            End,
            Start,
            CrestPoint,
            CrestExcess,
            40);

    Assessment.bNearCrest =
        bHasCrest &&
        FVector::Dist2D(CrestPoint, CandidateGround) <=
            CrestExposureDistanceCm;

    float Score = 0.0f;

    Score += Assessment.bHasDirectLOS
        ? DirectLOSScore
        : -DirectLOSScore;

    Score +=
        FMath::Clamp(
            Assessment.ElevationAdvantageCm / 100.0f,
            -40.0f,
            40.0f) *
        ElevationScorePer100Cm;

    Score -=
        Assessment.LocalSlopeDegrees *
        SlopePenaltyPerDegree;

    if (Assessment.bNearCrest)
    {
        Score -= CrestExposurePenalty;
    }

    if (Assessment.bInDeadGroundFromThreat)
    {
        Score -= DeadGroundPenalty;
    }

    const float MoveDistance =
        FVector::Dist2D(
            OwnerBattery->GetActorLocation(),
            CandidateGround);

    Score -=
        (MoveDistance / 1000.0f) *
        MovementPenaltyPer1000Cm;

    if (Assessment.LocalSlopeDegrees >
        MaximumDirectFireSlopeDegrees)
    {
        Score -= 500.0f;
    }

    Assessment.Score = Score;
    return Assessment;
}

TArray<FVector>
UStrategyArtilleryPositioningComponent::GenerateCandidatePositions(
    const FVector& Center,
    float SearchRadiusCm,
    int32 CandidateCount) const
{
    TArray<FVector> Result;

    if (!OwnerBattery)
    {
        return Result;
    }

    const int32 Count = FMath::Clamp(CandidateCount, 4, 64);
    const float Radius = FMath::Max(500.0f, SearchRadiusCm);

    Result.Reserve(Count + 1);

    Result.Add(
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerBattery,
            Center));

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float Alpha =
            static_cast<float>(Index) /
            static_cast<float>(Count);

        const float Angle = Alpha * 2.0f * PI;

        const float RingFraction =
            (Index % 2 == 0) ? 1.0f : 0.55f;

        const FVector Candidate =
            Center +
            FVector(
                FMath::Cos(Angle) * Radius * RingFraction,
                FMath::Sin(Angle) * Radius * RingFraction,
                0.0f);

        Result.Add(
            UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
                OwnerBattery,
                Candidate));
    }

    return Result;
}

bool UStrategyArtilleryPositioningComponent::FindBestDirectFirePosition(
    const FVector& ThreatPosition,
    float SearchRadiusCm,
    int32 CandidateCount,
    FVector& OutPosition,
    FStrategyTerrainPositionAssessment& OutAssessment) const
{
    if (!OwnerBattery)
    {
        return false;
    }

    const TArray<FVector> Candidates =
        GenerateCandidatePositions(
            OwnerBattery->GetActorLocation(),
            SearchRadiusCm,
            CandidateCount);

    bool bFound = false;
    float BestScore = -TNumericLimits<float>::Max();

    for (const FVector& Candidate : Candidates)
    {
        const FStrategyTerrainPositionAssessment Assessment =
            EvaluatePosition(
                Candidate,
                ThreatPosition);

        if (!Assessment.bHasDirectLOS ||
            Assessment.bInDeadGroundFromThreat ||
            Assessment.LocalSlopeDegrees >
                MaximumDirectFireSlopeDegrees)
        {
            continue;
        }

        if (!bFound || Assessment.Score > BestScore)
        {
            bFound = true;
            BestScore = Assessment.Score;
            OutPosition = Candidate;
            OutAssessment = Assessment;
        }
    }

    return bFound;
}

bool UStrategyArtilleryPositioningComponent::HasPhysicalStaticLOS(
    const FVector& Start,
    const FVector& End) const
{
    if (!GetWorld())
    {
        return false;
    }

    FHitResult Hit;

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(StrategyArtilleryPositionLOS),
        false);

    if (OwnerBattery)
    {
        QueryParams.AddIgnoredActor(OwnerBattery);
    }

    return !GetWorld()->LineTraceSingleByObjectType(
        Hit,
        Start,
        End,
        ObjectParams,
        QueryParams);
}
