"""Offlinekontrol af C++-drillens formler og 10 cm-sampling; starter ikke Unreal.
Koer fra repoets rod: python -B Source/Strategy1864/Tests/verify_column_drill_geometry.py
"""
import importlib.util as DrillImport
import math as DrillMath
from pathlib import Path as DrillPath
import sys as DrillSys
import numpy as DrillNp

DrillSys.dont_write_bytecode = True
DrillRoot = DrillPath(__file__).resolve().parents[3]
DrillLoader = DrillImport.spec_from_file_location("drill_independent_spec", DrillRoot / "Tools/Battle/verify_column_drill.py")
DrillSpec = DrillImport.module_from_spec(DrillLoader)
DrillLoader.loader.exec_module(DrillSpec)


def DrillPositions(DrillS, DrillA, DrillStraight, DrillRadius, DrillAngle):
    DrillS = DrillNp.asarray(DrillS)
    DrillArc = abs(DrillAngle) * DrillRadius
    DrillTurn = DrillNp.clip((DrillS - DrillStraight) / DrillRadius, 0, abs(DrillAngle))
    DrillYaw = DrillA + DrillNp.sign(DrillAngle) * DrillTurn
    DrillForward = DrillNp.array([DrillMath.cos(DrillA), DrillMath.sin(DrillA)])
    DrillNormal = DrillNp.array([-DrillMath.sin(DrillA), DrillMath.cos(DrillA)])
    DrillAlong = DrillNp.where(DrillS <= DrillStraight, DrillS,
        DrillStraight + DrillRadius * DrillNp.sin(DrillTurn))
    DrillSide = DrillNp.sign(DrillAngle) * DrillRadius * (1 - DrillNp.cos(DrillTurn))
    DrillPos = DrillAlong[..., None] * DrillForward + DrillSide[..., None] * DrillNormal
    DrillExit = DrillNp.maximum(0, DrillS - DrillStraight - DrillArc)
    DrillPos += DrillExit[..., None] * DrillNp.array([DrillMath.cos(DrillA + DrillAngle), DrillMath.sin(DrillA + DrillAngle)])
    return DrillPos, DrillYaw


def DrillSample(DrillS, DrillLane, DrillStart, DrillA, DrillStraight, DrillRadius, DrillAngle, DrillLength, DrillJunctions):
    DrillSteps = max(1, DrillMath.ceil(DrillLength / 10))
    DrillDistances = DrillNp.minimum(DrillNp.arange(DrillSteps + 1) * 10., DrillLength)
    if DrillJunctions:
        DrillDistances = DrillNp.sort(DrillNp.append(DrillDistances, [DrillStraight, DrillStraight + abs(DrillAngle) * DrillRadius]))
    DrillPos, DrillYaw = DrillPositions(DrillDistances, DrillA, DrillStraight, DrillRadius, DrillAngle)
    # Same bracketing/interpolation as Sample(), including repeated junction distances.
    DrillLow = DrillNp.clip(DrillNp.searchsorted(DrillDistances, DrillS, side="right") - 1, 0, len(DrillDistances) - 2)
    DrillHigh = DrillLow + 1
    DrillAlpha = DrillNp.clip((DrillS - DrillDistances[DrillLow]) / DrillNp.maximum(.01, DrillDistances[DrillHigh] - DrillDistances[DrillLow]), 0, 1)
    DrillInterpolated = DrillPos[DrillLow] + (DrillPos[DrillHigh] - DrillPos[DrillLow]) * DrillAlpha[..., None]
    DrillHeading = DrillYaw[DrillLow] + (DrillYaw[DrillHigh] - DrillYaw[DrillLow]) * DrillAlpha
    DrillNormal = DrillNp.stack([-DrillNp.sin(DrillHeading), DrillNp.cos(DrillHeading)], axis=-1)
    return DrillStart + DrillInterpolated + DrillLane[..., None] * DrillNormal


def DrillCheck(DrillN, DrillLat, DrillRank, DrillEntering, DrillDegrees):
    DrillRows = (DrillN + 2) // 3
    DrillWidth, DrillDepth = (DrillRows - 1) * DrillLat, (DrillRows - 1) * DrillRank
    DrillRadius = max(300., 3 * DrillRank)
    DrillHead = DrillMath.pi / 2 - DrillMath.radians(DrillDegrees)
    DrillEast = DrillDegrees >= 0
    DrillAxis = 0. if DrillEast else DrillMath.pi
    DrillA = DrillAxis if DrillEntering else DrillHead
    DrillB = DrillHead if DrillEntering else DrillAxis
    DrillAngle = (DrillB - DrillA + DrillMath.pi) % (2 * DrillMath.pi) - DrillMath.pi
    DrillArc = abs(DrillAngle) * DrillRadius
    DrillRamp = max(300., DrillArc)
    DrillStraight = DrillWidth if DrillEntering else DrillDepth
    DrillIds = DrillNp.arange(DrillN)
    DrillRanks = DrillIds % 3
    DrillK = (DrillRows - 1 - DrillIds // 3 if DrillEast else DrillIds // 3) if DrillEntering else DrillIds // 3
    DrillLaneIndices = (DrillRanks if DrillEast else 2 - DrillRanks) if DrillEntering else DrillRanks
    DrillRemainder = DrillN % 3
    DrillTargetRow = DrillK.copy()
    DrillTargetLane = DrillLaneIndices.copy()
    if DrillEntering and DrillEast and DrillRemainder:
        DrillTargetRow -= DrillLaneIndices >= DrillRemainder
    if not DrillEast and DrillRemainder:
        DrillTargetLane += (DrillK == DrillRows - 1) * (-1 if DrillEntering else 1) * (3 - DrillRemainder)
    DrillTargetRank = DrillTargetLane if DrillEast else 2 - DrillTargetLane
    DrillTargetFile = (DrillN + 2 - DrillLaneIndices) // 3 - 1 - DrillK if DrillEast else DrillK
    DrillTargets = DrillTargetRow * 3 + DrillTargetLane if DrillEntering else DrillTargetFile * 3 + DrillTargetRank
    DrillEndRows = DrillTargetRow if DrillEntering else (DrillRows - 1 - DrillTargetFile if DrillEast else DrillTargetFile)
    assert sorted(DrillTargets.tolist()) == list(range(DrillN))
    DrillS0 = DrillStraight - DrillK * (DrillLat if DrillEntering else DrillRank)
    DrillEndHead = DrillStraight + DrillRamp + (DrillDepth if DrillEntering else DrillWidth)
    DrillDelta = DrillEndHead - DrillEndRows * (DrillRank if DrillEntering else DrillLat) - DrillS0
    assert float(DrillDelta.min()) > 0
    DrillLane = (1 - DrillLaneIndices) * DrillRank
    DrillLaneShift = DrillLaneIndices - DrillTargetLane
    DrillStart = DrillNp.array([-DrillWidth / 2 * (1 if DrillEast else -1), -DrillRank])
    if not DrillEntering:
        DrillNear = DrillStart.copy()
        DrillEndPos = DrillPositions(DrillStraight + DrillRamp, DrillA, DrillStraight, DrillRadius, DrillAngle)[0]
        DrillStart = DrillNear - DrillEndPos
        # Head-row actor goal, not the geometric middle of the column.
        DrillMarch = DrillNp.array([DrillMath.cos(DrillA), DrillMath.sin(DrillA)])
        DrillAxisVector = DrillNp.array([DrillMath.cos(DrillB), DrillMath.sin(DrillB)])
        DrillArcDelta = DrillPositions(DrillStraight + DrillArc, DrillA, DrillStraight, DrillRadius, DrillAngle)[0] - DrillMarch * DrillStraight
        DrillHeadGoal = DrillStart + DrillMarch * DrillDepth
        # ColumnApproachGoal returns the geometric middle; executor adds half the depth.
        DrillCentreGoal = DrillNear - DrillArcDelta - DrillAxisVector * (DrillRamp - DrillArc) - DrillMarch * (DrillDepth / 2)
        assert DrillNp.linalg.norm(DrillCentreGoal + DrillMarch * (DrillDepth / 2) - DrillHeadGoal) < 1e-8
        if DrillDegrees == 0:
            DrillFreeHead = DrillNp.array([213., -457.])
            DrillFreeOrigin = DrillFreeHead + DrillArcDelta + DrillAxisVector * (DrillRamp - DrillArc + DrillWidth / 2) + DrillNp.array([0., DrillRank])
            assert DrillNp.linalg.norm(DrillFreeOrigin - (DrillFreeHead - DrillHeadGoal)) < 1e-8
    DrillTravel = DrillNp.minimum(DrillNp.linspace(0, float(DrillDelta.max()), 501)[:, None], DrillDelta)
    DrillS = DrillS0 + DrillTravel
    DrillU = DrillNp.clip((DrillS - DrillStraight) / DrillRamp, 0, 1)
    DrillScale = (1 + (DrillLat / DrillRank - 1) * DrillU) if DrillEntering else (DrillLat / DrillRank + (1 - DrillLat / DrillRank) * DrillU)
    DrillTailAlpha = DrillNp.clip(DrillS / (.75 * (DrillLat if DrillEntering else DrillRank)), 0, 1)
    DrillOffset = (DrillLane + DrillLaneShift * DrillRank * DrillTailAlpha) * DrillScale
    DrillAnalytic, DrillYaw = DrillPositions(DrillS, DrillA, DrillStraight, DrillRadius, DrillAngle)
    DrillAnalytic += DrillStart + DrillOffset[..., None] * DrillNp.stack([-DrillNp.sin(DrillYaw), DrillNp.cos(DrillYaw)], axis=-1)
    if DrillEntering:
        DrillFile = DrillRows - 1 - DrillK if DrillEast else DrillK
        DrillInitialSlots = DrillNp.stack([(DrillFile - (DrillRows - 1) / 2) * DrillLat, -DrillRanks * DrillRank], axis=-1)
    else:
        DrillRight = DrillNp.array([DrillMath.sin(DrillA), -DrillMath.cos(DrillA)])
        DrillInitialSlots = DrillHeadGoal - (DrillK * DrillRank)[:, None] * DrillMarch + ((DrillRanks - 1) * DrillLat)[:, None] * DrillRight
    assert float(DrillNp.linalg.norm(DrillAnalytic[0] - DrillInitialSlots, axis=-1).max()) < 1e-8
    DrillSampled = DrillSample(DrillS, DrillOffset, DrillStart, DrillA, DrillStraight, DrillRadius, DrillAngle, DrillEndHead, True)
    DrillOldFinal = DrillSample(DrillS[-1], DrillOffset[-1], DrillStart, DrillA, DrillStraight, DrillRadius, DrillAngle, DrillEndHead, False)
    DrillFinalError = float(DrillNp.linalg.norm(DrillSampled[-1] - DrillAnalytic[-1], axis=-1).max())
    DrillOldError = float(DrillNp.linalg.norm(DrillOldFinal - DrillAnalytic[-1], axis=-1).max())
    assert DrillFinalError < 1e-8, (DrillN, DrillEntering, DrillDegrees, DrillFinalError)
    DrillMinimum = float("inf")
    for DrillChunk in range(0, len(DrillSampled), 32):
        DrillPositionsChunk = DrillSampled[DrillChunk:DrillChunk + 32]
        DrillD = DrillNp.linalg.norm(DrillPositionsChunk[:, :, None] - DrillPositionsChunk[:, None, :], axis=-1)
        DrillD[:, DrillNp.arange(DrillN), DrillNp.arange(DrillN)] = DrillNp.inf
        DrillMinimum = min(DrillMinimum, float(DrillD.min()))
    # Compare every formula/point with the independent spec, with no UE/spec axis ambiguity.
    DrillFactory = DrillSpec.case1 if DrillEntering else DrillSpec.case2
    DrillOracle, DrillSlots, DrillInfo = DrillFactory(DrillN, DrillDegrees, DrillLat, DrillRank, DrillRadius, 150., .9)
    assert DrillNp.array_equal(DrillTargets, DrillOracle.DrillTargetIds)
    assert DrillNp.allclose(DrillS0, DrillOracle.s0, atol=1e-9)
    assert DrillNp.allclose(DrillDelta, DrillOracle.delta, atol=1e-9)
    DrillOraclePositions = DrillNp.stack([DrillOracle.pos_s(DrillRow) for DrillRow in DrillS])
    DrillOracleError = float(DrillNp.linalg.norm(DrillAnalytic - DrillOraclePositions, axis=-1).max())
    assert DrillOracleError < 1e-8, DrillOracleError
    if not DrillEntering:
        assert DrillNp.linalg.norm(DrillHeadGoal - DrillInfo["goal"]) < 1e-8
    DrillGridD = DrillNp.linalg.norm(DrillSampled[-1, :, None] - DrillSlots[None], axis=-1)
    assert float(DrillGridD.min(axis=1).max()) < 1e-8
    assert len(set(DrillGridD.argmin(axis=1))) == DrillN
    assert DrillNp.array_equal(DrillGridD.argmin(axis=1), DrillTargets)
    return DrillFinalError, DrillOldError, DrillMinimum / min(DrillLat, DrillRank), DrillOracleError


def DrillMain():
    DrillHelper = (DrillRoot / "Source/Strategy1864/Visual/StrategyColumnDrill.h").read_text()
    DrillVisual = (DrillRoot / "Source/Strategy1864/Visual/StrategyInfantryVisualComponent.cpp").read_text()
    for DrillSnippet in ["DrillDistances.Sort();", "DrillDistances.Add(DrillStraightA);", "DrillDistances.Add(DrillStraightA + DrillArc);", "return FMath::Max(300.f, 3.f * DrillRankSpacing);"]:
        assert DrillSnippet in DrillHelper, DrillSnippet
    for DrillSnippet in ["DrillWidth + DrillRampLength + DrillDepth", "DrillStraightA + DrillRampLength + DrillWidth", "DrillS - DrillStraightA", "DrillRadius, DrillB, DrillRampLength - DrillArcLength + DrillWidth", "DrillNear - FStrategyColumnDrill::ArcDelta", "DrillB * (DrillRampLength - DrillArcLength)", "DrillMan.DrillDelta = DrillHeadEnd - DrillEndRow * (DrillEntering ? DrillRankSpacing : DrillFileSpacing) - DrillMan.DrillS0;"]:
        assert DrillSnippet in DrillVisual, DrillSnippet
    assert "SoldierComponents.Num() % 3 != 0" not in DrillVisual
    assert "SoldierComponents.Num() % 3 == 0" not in DrillVisual
    for DrillSnippet in ["(SoldierComponents.Num() + 2) / 3", "DrillMan.DrillLaneShift = float(DrillTargetLane - DrillLane)", "DrillS / (0.75f * DrillFinalSpacing)", "DrillTargetRow * 3 + DrillTargetLane"]:
        assert DrillSnippet in DrillVisual, DrillSnippet
    DrillCases, DrillMaxEnd, DrillMaxOld, DrillMinDefault, DrillMaxOracle = 0, 0., 0., 1., 0.
    DrillExtendedBelow = 0
    DrillMinExtended = 1.
    DrillBaseCounts = (190, 189, 188, 46, 47)
    DrillCounts = sorted({45, 60, 90, 120, 1, 2, 3, 4, 7, 8} | {(DrillN + DrillDiv - 1) // DrillDiv for DrillN in DrillBaseCounts for DrillDiv in (1, 2, 5, 10)})
    for DrillN in DrillCounts:
        DrillSpacings = ((75, 90), (90, 75), (90, 90), (75, 120)) if DrillN in (45, 60, 90, 120) else ((75, 90),)
        for DrillLat, DrillRank in DrillSpacings:
            for DrillEntering in (True, False):
                for DrillDegrees in (-135, -90, -30, 0, 30, 90, 135):
                    DrillEnd, DrillOld, DrillRatio, DrillOracleError = DrillCheck(DrillN, DrillLat, DrillRank, DrillEntering, DrillDegrees)
                    DrillCases += 1
                    DrillMaxEnd, DrillMaxOld = max(DrillMaxEnd, DrillEnd), max(DrillMaxOld, DrillOld)
                    DrillMaxOracle = max(DrillMaxOracle, DrillOracleError)
                    if (DrillLat, DrillRank) == (75, 90):
                        DrillMinDefault = min(DrillMinDefault, DrillRatio)
                        assert DrillRatio >= .7, (DrillN, DrillEntering, DrillDegrees, DrillRatio)
                    elif DrillRatio < .7:
                        DrillExtendedBelow += 1
                        DrillMinExtended = min(DrillMinExtended, DrillRatio)
    print(f"Counts including reduced visuals: {DrillCounts}")
    print(f"PASS: {DrillCases} analytic/sampled cases x 501 time points; all final grids unique and front/head anchored.")
    print(f"Max endpoint error: before junction fix {DrillMaxOld:.9f} cm; after {DrillMaxEnd:.12g} cm.")
    print(f"Max analytic disagreement with independent spec: {DrillMaxOracle:.12g} cm.")
    print(f"Lowest default-spacing clearance: {DrillMinDefault:.9%}; extended nondefault cases below 70%: {DrillExtendedBelow} (worst {DrillMinExtended:.9%}).")


if __name__ == "__main__":
    DrillMain()
