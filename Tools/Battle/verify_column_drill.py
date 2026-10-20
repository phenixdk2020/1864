#!/usr/bin/env python3
"""Offline spec check of the "column drill" geometry (line <-> column), all in cm.

Spec: Saved/Codex/drill_refs/PLAN.md sections 1-3, reference animations in
Saved/Codex/drill_refs/*.py (path()/state()/travel()).  Independent of the C++.

Frames: line frame x = right along the front, y = forward (F).  The line slot
grid follows UStrategyFormationComponent::GenerateSoldierSlots:
  line   : file j -> x=(j-(nf-1)/2)*Lat, rank k -> y=-k*RankSp  (origin = front-rank centre)
  column : col c -> lateral (c-1)*Lat, row r -> -r*RankSp along heading (origin = head row)
Men follow ONE shared centre path P(s) (straight, arc of radius R, straight) with
their own lateral lane; the lane offset ramps between RankSp (line) and Lat
(column) over Lr = max(300, arc) cm after the arc start, because the column
grid's lateral spacing is SoldierLateralSpacingCm, not the rank spacing.

Cases:
  1  line -> column, march dir D = 0,45,90,-45,-90 deg rel. front
  2  column -> line at target T / facing F, column heading M = F, F+-45, F+-90
  3  column -> line without target (straight on, quarter arc right, along axis)

Usage: python Tools/Battle/verify_column_drill.py [--plot DIR] [--n 45 60 90 120]
Exit code 1 on any FAIL.
"""
from __future__ import annotations

import argparse
import math
import os
import sys

import numpy as np

FPS = 20
RANKS = 3  # always 3 ranks / 3 abreast


# ----------------------------------------------------------------------------
# centre path
# ----------------------------------------------------------------------------
def nleft(h):
    return np.array([-math.sin(h), math.cos(h)])


def wrap(a):
    return (a + math.pi) % (2 * math.pi) - math.pi


class Path:
    """straight (len sA) -> arc (radius R, signed turn alpha) -> straight."""

    def __init__(self, origin, h0, sA, R, alpha):
        self.o = np.asarray(origin, float)
        self.h0, self.sA, self.R, self.alpha = h0, sA, R, alpha
        self.La = abs(alpha) * R
        self.sg = 1.0 if alpha >= 0 else -1.0
        d0 = np.array([math.cos(h0), math.sin(h0)])
        self.pA = self.o + d0 * sA
        self.h1 = h0 + alpha

    def pos_head(self, s):
        """s: ndarray -> (positions Nx2, headings N)."""
        s = np.asarray(s, float)
        d0 = np.array([math.cos(self.h0), math.sin(self.h0)])
        d1 = np.array([math.cos(self.h1), math.sin(self.h1)])
        pos = np.empty(s.shape + (2,))
        hd = np.empty(s.shape)
        a = s <= self.sA
        pos[a] = self.o + np.outer(s[a], d0)
        hd[a] = self.h0
        b = (s > self.sA) & (s < self.sA + self.La)
        if b.any() and self.La > 0:
            f = (s[b] - self.sA) / self.La
            h = self.h0 + self.alpha * f
            n0 = nleft(self.h0)
            nh = np.stack([-np.sin(h), np.cos(h)], axis=-1)
            pos[b] = self.pA + self.sg * self.R * (n0 - nh)
            hd[b] = h
        c = s >= self.sA + self.La
        if c.any():
            pe = self.pA + self.sg * self.R * (nleft(self.h0) - nleft(self.h1)) if self.La > 0 else self.pA
            pos[c] = pe + np.outer(s[c] - self.sA - self.La, d1)
            hd[c] = self.h1
        return pos, hd


class Drill:
    """Common mover: men with s0, lane (left-normal offset at line scale), delta."""

    def __init__(self, path, s0, lane, delta, g_fn, t0, speed, turn_sec, shift=(0, 0), DrillLaneShift=None, DrillShiftLength=1., DrillRankSpacing=1.):
        self.path, self.s0, self.lane, self.delta = path, np.asarray(s0, float), np.asarray(lane, float), np.asarray(delta, float)
        self.g_fn, self.t0, self.v, self.turn = g_fn, t0, speed, turn_sec
        self.shift = np.asarray(shift, float)
        self.DrillLaneShift = np.zeros_like(self.lane) if DrillLaneShift is None else np.asarray(DrillLaneShift, float)
        self.DrillShiftLength = DrillShiftLength
        self.DrillRankSpacing = DrillRankSpacing
        self.dur = t0 + float(self.delta.max()) / speed + turn_sec * (1 if t0 == 0 else 0)

    def s_at(self, t):
        return self.s0 + np.clip(self.v * (t - self.t0), 0.0, self.delta)

    def pos(self, t):
        return self.pos_s(self.s_at(t))

    def pos_s(self, s):
        p, h = self.path.pos_head(s)
        nl = np.stack([-np.sin(h), np.cos(h)], axis=-1)
        DrillLane = self.lane + self.DrillLaneShift * self.DrillRankSpacing * np.clip(np.asarray(s) / self.DrillShiftLength, 0., 1.)
        return p + DrillLane[:, None] * self.g_fn(s)[:, None] * nl + self.shift


def make_g(sA, Lr, a, b):
    """lateral scale factor (relative to RankSp unless a,b given in cm/RankSp)."""
    def g(s):
        u = np.clip((np.asarray(s, float) - sA) / Lr, 0.0, 1.0)
        return a + (b - a) * u
    return g


# ----------------------------------------------------------------------------
# cases
# ----------------------------------------------------------------------------
def DrillAssignment(DrillN, DrillEntering, DrillEast):
    """Independent assignment: generate both grids, then zip ordered lanes.
    A short rear packet shifts in the empty area behind full-lane start points.
    """
    DrillIds = np.arange(DrillN)
    DrillRows = (DrillN + 2) // 3
    DrillSourceRow = (DrillRows - 1 - DrillIds // 3 if DrillEast else DrillIds // 3) if DrillEntering else DrillIds // 3
    DrillSourceLane = (DrillIds % 3 if DrillEast else 2 - DrillIds % 3) if DrillEntering else DrillIds % 3
    DrillTargetLane = DrillIds % 3 if DrillEntering or DrillEast else 2 - DrillIds % 3
    DrillTargetRow = DrillIds // 3 if DrillEntering or not DrillEast else DrillRows - 1 - DrillIds // 3
    DrillTargets = np.full(DrillN, -1, dtype=int)
    DrillUsed = set()
    if not DrillEast and DrillN % 3:
        DrillRear = sorted(DrillIds[DrillSourceRow == DrillRows - 1], key=lambda DrillI: DrillSourceLane[DrillI])
        DrillShort = sorted(DrillIds[-(DrillN % 3):], key=lambda DrillI: DrillTargetLane[DrillI])
        for DrillSource, DrillTarget in zip(DrillRear, DrillShort):
            DrillTargets[DrillSource] = DrillTarget
            DrillUsed.add(int(DrillTarget))
    for DrillLane in range(3):
        DrillSources = sorted((DrillI for DrillI in DrillIds if DrillSourceLane[DrillI] == DrillLane and DrillTargets[DrillI] < 0), key=lambda DrillI: DrillSourceRow[DrillI])
        DrillDestinations = sorted((DrillI for DrillI in DrillIds if DrillTargetLane[DrillI] == DrillLane and DrillI not in DrillUsed), key=lambda DrillI: DrillTargetRow[DrillI])
        assert len(DrillSources) == len(DrillDestinations)
        for DrillSource, DrillTarget in zip(DrillSources, DrillDestinations):
            DrillTargets[DrillSource] = DrillTarget
    assert sorted(DrillTargets.tolist()) == list(range(DrillN))
    return DrillSourceRow, DrillSourceLane, DrillTargets, DrillTargetRow[DrillTargets], DrillTargetLane[DrillTargets]


def case1(n, ang_deg, Lat, RS, R, v, turn):
    nf = (n + RANKS - 1) // RANKS
    lead_right = ang_deg >= 0 and True if ang_deg > -1 else False  # right if D right or ahead
    lead_right = ang_deg > -1
    hA = 0.0 if lead_right else math.pi
    hD = math.pi / 2 - math.radians(ang_deg)
    alpha = wrap(hD - hA)
    sA = (nf - 1) * Lat
    xfar = -(nf - 1) / 2 * Lat * (1 if lead_right else -1)
    O = (xfar, -RS)
    P = Path(O, hA, sA, R, alpha)
    Lr = max(300.0, P.La)
    Lc = max(P.La, Lr)
    DrillSourceRows, DrillSourceLanes, DrillTargetIds, DrillTargetRows, DrillTargetLanes = DrillAssignment(n, True, lead_right)
    s0 = sA - DrillSourceRows * Lat
    lane = (1 - DrillSourceLanes) * RS
    sf0 = sA + Lc + (nf - 1) * RS
    delta = sf0 - DrillTargetRows * RS - s0
    g = make_g(sA, Lr, 1.0, Lat / RS)
    dr = Drill(P, s0, lane, delta, g, turn, v, turn,
               DrillLaneShift=DrillSourceLanes - DrillTargetLanes, DrillShiftLength=.75 * Lat, DrillRankSpacing=RS)
    dr.DrillTargetIds = DrillTargetIds
    # target column grid: head row centre = P(final s of row 0), forward D
    sf0 = sA + Lc + (nf - 1) * RS
    cc = P.pos_head(np.array([sf0]))[0][0]
    D = np.array([math.cos(hD), math.sin(hD)])
    Rt = np.array([math.sin(hD), -math.cos(hD)])
    slots = np.array([cc - r * RS * D + (c - 1) * Lat * Rt for r in range(nf) for c in range(RANKS) if r * RANKS + c < n])
    info = dict(kind="line->col", lead="right" if lead_right else "left", alpha=math.degrees(alpha),
                R=R, Lr=Lr, centre=cc, heading=math.degrees(hD))
    return dr, slots, info


def case2(n, mu_deg, Lat, RS, R, v, turn, label="col->line"):
    """Column heading M = F + mu (cw positive); line at T=(0,0) facing F=(0,1)."""
    nf = (n + RANKS - 1) // RANKS
    hM = math.pi / 2 - math.radians(mu_deg)
    # near flank: the axis direction with the smaller turn, tie -> east (flip if >135 deg)
    aE, aW = wrap(0.0 - hM), wrap(math.pi - hM)
    east = abs(aE) <= abs(aW) + 1e-9
    hA = 0.0 if east else math.pi
    alpha = wrap(hA - hM)
    sA = (nf - 1) * RS
    P = Path((0, 0), hM, sA, R, alpha)
    Lr = max(300.0, P.La)
    Lc = max(P.La, Lr)
    sgn = 1.0 if east else -1.0
    xnear = -sgn * (nf - 1) / 2 * Lat
    tail_end_local = P.pos_head(np.array([sA + Lc]))[0][0]
    shift = np.array([xnear, -RS]) - tail_end_local
    DrillSourceRows, DrillSourceLanes, DrillTargetIds, DrillTargetRows, DrillTargetLanes = DrillAssignment(n, False, east)
    s0 = sA - DrillSourceRows * RS
    lane = (1 - DrillSourceLanes) * RS
    delta = sA + Lc + (nf - 1) * Lat - DrillTargetRows * Lat - s0
    g = make_g(sA, Lr, Lat / RS, 1.0)
    dr = Drill(P, s0, lane, delta, g, 0.0, v, turn, shift,
               DrillLaneShift=DrillSourceLanes - DrillTargetLanes, DrillShiftLength=.75 * RS, DrillRankSpacing=RS)
    dr.DrillTargetIds = DrillTargetIds
    # target = line slots centred on T facing F
    slots = np.array([[(j - (nf - 1) / 2) * Lat, -k * RS] for j in range(nf) for k in range(RANKS) if j * RANKS + k < n])
    goal = P.pos_head(np.array([sA]))[0][0] + shift
    info = dict(kind=label, flank="left(near)" if east else "right(near)", alpha=math.degrees(alpha),
                R=R, Lr=Lr, goal=goal, heading=math.degrees(hM))
    return dr, slots, info


# ----------------------------------------------------------------------------
# checks
# ----------------------------------------------------------------------------
def min_dist(dr):
    nfr = int(math.ceil(dr.dur * FPS)) + 1
    best, tb = 1e18, 0.0
    for k in range(nfr):
        t = min(k / FPS, dr.dur)
        p = dr.pos(t)
        d = np.sqrt(((p[:, None] - p[None]) ** 2).sum(-1))
        np.fill_diagonal(d, np.inf)
        m = d.min()
        if m < best:
            best, tb = float(m), t
    return best, tb


def seg_cross(A, B):
    """count proper crossings between polylines A and B (Mx2, Kx2)."""
    a0, a1 = A[:-1][:, None], A[1:][:, None]
    b0, b1 = B[:-1][None], B[1:][None]

    def cr(p, q, r):
        return (q[..., 0] - p[..., 0]) * (r[..., 1] - p[..., 1]) - (q[..., 1] - p[..., 1]) * (r[..., 0] - p[..., 0])
    e = 1e-9
    d1, d2 = cr(a0, a1, b0), cr(a0, a1, b1)
    d3, d4 = cr(b0, b1, a0), cr(b0, b1, a1)
    return int(((d1 * d2 < -e) & (d3 * d4 < -e)).sum())


def path_crossings(dr):
    """Check distinct ground curves; merge shared lanes after a short packet joins.
    Splitting at the shift boundary avoids falsely counting two samplings of the
    same curved lane as crossing polylines.
    """
    DrillPieces = {}
    for DrillLane, DrillShift in set(zip(dr.lane, dr.DrillLaneShift)):
        DrillMask = (dr.lane == DrillLane) & (dr.DrillLaneShift == DrillShift)
        DrillLo = float(dr.s0[DrillMask].min())
        DrillHi = float((dr.s0[DrillMask] + dr.delta[DrillMask]).max())
        for DrillPhase, DrillA, DrillB in ((0, DrillLo, min(DrillHi, dr.DrillShiftLength)), (1, max(DrillLo, dr.DrillShiftLength), DrillHi)):
            if DrillB <= DrillA:
                continue
            DrillKey = (DrillPhase, DrillLane, DrillShift) if DrillPhase == 0 else (1, DrillLane + DrillShift * dr.DrillRankSpacing, 0.)
            if DrillKey in DrillPieces:
                DrillA = min(DrillA, DrillPieces[DrillKey][0])
                DrillB = max(DrillB, DrillPieces[DrillKey][1])
            DrillPieces[DrillKey] = (DrillA, DrillB)
    DrillCurves = []
    for (DrillPhase, DrillLane, DrillShift), (DrillLo, DrillHi) in DrillPieces.items():
        DrillS = np.linspace(DrillLo, DrillHi, max(2, int((DrillHi - DrillLo) / 5) + 2))
        DrillSub = Drill(dr.path, DrillS, np.full(DrillS.shape, DrillLane), np.zeros(DrillS.shape), dr.g_fn, 0, 1, 0, dr.shift,
                         DrillLaneShift=np.full(DrillS.shape, DrillShift), DrillShiftLength=dr.DrillShiftLength, DrillRankSpacing=dr.DrillRankSpacing)
        DrillCurves.append(DrillSub.pos_s(DrillS))
    return sum(seg_cross(DrillCurves[DrillI], DrillCurves[DrillJ])
               for DrillI in range(len(DrillCurves)) for DrillJ in range(DrillI + 1, len(DrillCurves)))


def grid_error(dr, slots):
    fin = dr.pos_s(dr.s0 + dr.delta)
    d = np.sqrt(((fin[:, None] - slots[None]) ** 2).sum(-1))
    nearest = d.argmin(1)
    err = float(d.min(1).max())
    unique = len(set(nearest.tolist())) == len(slots) == len(fin)
    return err if unique else 1e9


def evaluate(n, Lat, RS, R, v, turn):
    thr = 0.7 * min(Lat, RS)
    rows = []
    specs = [("1 line->col", a, case1(n, a, Lat, RS, R, v, turn)) for a in (0, 45, 90, -45, -90)]
    specs += [("2 col->line", a, case2(n, a, Lat, RS, R, v, turn)) for a in (0, 45, 90, -45, -90)]
    specs += [("3 col->line free", 0, case2(n, 0, Lat, RS, R, v, turn, "col->line(no target)"))]
    for name, a, (dr, slots, info) in specs:
        md, tm = min_dist(dr)
        cx = path_crossings(dr)
        ge = grid_error(dr, slots)
        ok = md >= thr and cx == 0 and ge <= 1.0
        rows.append(dict(case=name, angle=a, n=n, mind=md, tmin=tm, dur=dr.dur, plen=float(dr.delta.max()),
                         cross=cx, gerr=ge, ok=ok, thr=thr, dr=dr, slots=slots, info=info))
    return rows


def print_rows(rows, Lat, RS, R):
    print(f"\n=== N={rows[0]['n']}  lateral={Lat:g}  rank={RS:g}  R={R:g}  (min dist >= {rows[0]['thr']:.2f}) ===")
    print(f"{'case':<17}{'ang':>5}{'mind':>8}{'t_min':>7}{'dur s':>7}{'path cm':>9}{'cross':>6}{'gridErr':>9}  res")
    for r in rows:
        ge = "mismatch" if r['gerr'] > 1e8 else f"{r['gerr']:.3f}"
        print(f"{r['case']:<17}{r['angle']:>5}{r['mind']:>8.2f}{r['tmin']:>7.2f}{r['dur']:>7.2f}"
              f"{r['plen']:>9.0f}{r['cross']:>6}{ge:>9}  {'PASS' if r['ok'] else 'FAIL'}")


def print_goals(rows):
    print("  Case 2 approach goal (column head-row/actor centre BEFORE the drill; line frame, T=(0,0), F=+y):")
    for r in rows:
        if r['case'].startswith("2") or r['case'].startswith("3"):
            g = r['info']['goal']
            print(f"    mu={r['angle']:>4}: goal=({g[0]:8.1f},{g[1]:8.1f}) cm  near flank={r['info']['flank']}  "
                  f"turn={r['info']['alpha']:+.0f} deg  actor-to-T={math.hypot(*g):.0f} cm")


# ----------------------------------------------------------------------------
# plotting
# ----------------------------------------------------------------------------
def plot_rows(rows, outdir):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    os.makedirs(outdir, exist_ok=True)
    paths = []
    for r in rows:
        dr = r['dr']
        times = [0.0, dr.dur * 0.5, dr.dur]
        fig, axs = plt.subplots(1, 3, figsize=(15, 5))
        s = np.linspace(dr.s0.min(), (dr.s0 + dr.delta).max(), 400)
        for ax, t in zip(axs, times):
            for L in np.unique(np.round(dr.lane, 6)):
                sub = Drill(dr.path, s, np.full(s.shape, L), np.zeros(s.shape), dr.g_fn, 0, 1, 0, dr.shift)
                c = sub.pos_s(s)
                ax.plot(c[:, 0], c[:, 1], color="#bbb", lw=0.8, ls="--")
            p = dr.pos(t)
            ax.scatter(p[:, 0], p[:, 1], s=14, c=np.arange(len(p)) % 3, cmap="viridis")
            ax.scatter(r['slots'][:, 0], r['slots'][:, 1], s=60, facecolors="none", edgecolors="r", lw=0.5)
            ax.set_aspect("equal")
            ax.set_title(f"t={t:.1f}s")
            ax.grid(alpha=0.3)
        fig.suptitle(f"{r['case']}  angle {r['angle']}  N={r['n']}  min {r['mind']:.1f}cm  "
                     f"{'PASS' if r['ok'] else 'FAIL'}")
        tag = r['case'].split()[0]
        fn = os.path.join(outdir, f"case{tag}_N{r['n']}_{r['angle']:+d}.png")
        fig.savefig(fn, dpi=80)
        plt.close(fig)
        paths.append(fn)
    return paths


def smallest_radius(n, Lat, RS, R0, v, turn):
    R = R0
    while R <= 2000:
        if all(r['ok'] for r in evaluate(n, Lat, RS, R, v, turn)):
            return R
        R += 10
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--n", type=int, nargs="+", default=[45, 60, 90, 120, 190, 189, 188, 46, 47, 95, 94, 38, 19, 24, 23, 10, 5, 1, 2, 3, 4, 7, 8])
    ap.add_argument("--lateral", type=float, default=75.0)   # StrategyFormationComponent.h
    ap.add_argument("--rank", type=float, default=90.0)
    ap.add_argument("--radius", type=float, default=None)
    ap.add_argument("--speed", type=float, default=150.0)
    ap.add_argument("--turn", type=float, default=0.9)
    ap.add_argument("--plot", metavar="DIR")
    ap.add_argument("--plot-n", type=int, nargs="+", default=[45])
    a = ap.parse_args()
    R = a.radius if a.radius else max(300.0, 3 * a.rank)
    allok = True
    # Includes ceil(N/divisor) for 190/189/188/46/47 and divisors 1, 2, 5, 10.
    for n in a.n:
        rows = evaluate(n, a.lateral, a.rank, R, a.speed, a.turn)
        print_rows(rows, a.lateral, a.rank, R)
        if n == a.n[0] or n == 45:
            print_goals(rows)
        if a.plot and n in a.plot_n:
            for p in plot_rows(rows, a.plot):
                print("  PNG:", os.path.abspath(p))
        if not all(r['ok'] for r in rows):
            allok = False
            Rs = smallest_radius(n, a.lateral, a.rank, R, a.speed, a.turn)
            print(f"  -> FAIL; smallest radius (10 cm steps) passing all cases for N={n}: {Rs}")
    print("\nRESULT:", "ALL PASS" if allok else "FAIL")
    sys.exit(0 if allok else 1)


if __name__ == "__main__":
    main()
