#!/usr/bin/env python3
"""Map toltrig reducer limits over logarithmic input ranges.

This is a diagnostic script, not a production implementation. Reducer-only
references use Decimal arithmetic. Full-pipeline references use the platform
math library, so treat those rows as characterization data rather than a proof
of correct rounding.
"""

from __future__ import annotations

import argparse
import csv
import math
import random
import statistics
import struct
import time
from dataclasses import dataclass
from decimal import Decimal, ROUND_FLOOR, ROUND_HALF_EVEN, getcontext
from pathlib import Path
from typing import Callable

getcontext().prec = 90

PI = Decimal(
    "3.14159265358979323846264338327950288419716939937510582097494459230781640628620899"
)
HALF_PI = PI / 2

PI_D = 3.141592653589793238462643383279502884
HALF_PI_D = PI_D / 2.0
QUARTER_PI_D = PI_D / 4.0

PI_HI = 3.14159262180328369140625
PI_MID = 3.178650954705639668e-08
PI_LO = 1.224646799147353207e-16
HALF_PI_HI = 1.570796310901641845703125
HALF_PI_MID = 1.589325477352819834e-08
HALF_PI_LO = 6.123233995736766036e-17

TAN_B = -0.06860571525033407
TAN_D = -0.40178219629842254


@dataclass
class Reduced:
    r: float
    sign: float = 1.0
    quadrant: int | None = None
    ok: bool = True


def dfloat(x: float) -> Decimal:
    return Decimal.from_float(float(x))


def nearest_decimal(x: Decimal) -> Decimal:
    return x.to_integral_value(rounding=ROUND_HALF_EVEN)


def mod4_decimal(q: Decimal) -> int:
    base = (q / 4).to_integral_value(rounding=ROUND_FLOOR) * 4
    return int(q - base)


def parity_sign(q: Decimal) -> float:
    return -1.0 if int(q % 2) else 1.0


def nearbyint_pi(x: float) -> Reduced:
    if not math.isfinite(x):
        return Reduced(math.nan, ok=False)
    q = float(round(x / PI_D))
    return Reduced(x - q * PI_D, -1.0 if int(q) % 2 else 1.0)


def nearbyint_half_pi(x: float) -> Reduced:
    if not math.isfinite(x):
        return Reduced(math.nan, ok=False)
    q = float(round(x / HALF_PI_D))
    return Reduced(x - q * HALF_PI_D, quadrant=int(q) % 4)


def cody_waite_pi(x: float) -> Reduced:
    if not math.isfinite(x):
        return Reduced(math.nan, ok=False)
    q = float(round(x / PI_D))
    r = ((x - q * PI_HI) - q * PI_MID) - q * PI_LO
    return Reduced(r, -1.0 if int(q) % 2 else 1.0)


def cody_waite_half_pi(x: float) -> Reduced:
    if not math.isfinite(x):
        return Reduced(math.nan, ok=False)
    q = float(round(x / HALF_PI_D))
    r = ((x - q * HALF_PI_HI) - q * HALF_PI_MID) - q * HALF_PI_LO
    return Reduced(r, quadrant=int(q) % 4)


def remainder_pi(x: float) -> Reduced:
    if not math.isfinite(x):
        return Reduced(math.nan, ok=False)
    r = math.remainder(x, PI_D)
    q = round((x - r) / PI_D)
    return Reduced(r, -1.0 if int(q) % 2 else 1.0)


def remainder_half_pi(x: float) -> Reduced:
    if not math.isfinite(x):
        return Reduced(math.nan, ok=False)
    r = math.remainder(x, HALF_PI_D)
    q = round((x - r) / HALF_PI_D)
    return Reduced(r, quadrant=int(q) % 4)


REDUCERS: list[tuple[str, bool, Callable[[float], Reduced]]] = [
    ("nearbyint_pi", False, nearbyint_pi),
    ("cody_waite_pi", False, cody_waite_pi),
    ("remainder_pi", False, remainder_pi),
    ("nearbyint_half_pi", True, nearbyint_half_pi),
    ("cody_waite_half_pi", True, cody_waite_half_pi),
    ("remainder_half_pi", True, remainder_half_pi),
]


def cos_reduced_n8(x: float) -> float:
    y = x * x
    coeffs = [
        1.0,
        -1.0 / 2.0,
        1.0 / 24.0,
        -1.0 / 720.0,
        1.0 / 40320.0,
        -1.0 / 3628800.0,
        1.0 / 479001600.0,
        -1.0 / 87178291200.0,
        1.0 / 20922789888000.0,
    ]
    acc = 0.0
    for c in reversed(coeffs):
        acc = acc * y + c
    return acc


def sin_reduced_n8(x: float) -> float:
    y = x * x
    coeffs = [
        1.0,
        -1.0 / 6.0,
        1.0 / 120.0,
        -1.0 / 5040.0,
        1.0 / 362880.0,
        -1.0 / 39916800.0,
        1.0 / 6227020800.0,
        -1.0 / 1307674368000.0,
        1.0 / 355687428096000.0,
    ]
    acc = 0.0
    for c in reversed(coeffs):
        acc = acc * y + c
    return x * acc


def tan_reduced_pade(x: float) -> float:
    x2 = x * x
    return x * (1.0 + TAN_B * x2) / (1.0 + TAN_D * x2)


def cos_pipeline(x: float, reducer: Callable[[float], Reduced], quadrant: bool) -> float:
    r = reducer(x)
    if quadrant:
        c = cos_reduced_n8(r.r)
        s = sin_reduced_n8(r.r)
        return [c, -s, -c, s][r.quadrant or 0]
    return r.sign * cos_reduced_n8(r.r)


def sin_pipeline(x: float, reducer: Callable[[float], Reduced], quadrant: bool) -> float:
    if x == 0.0:
        return x
    r = reducer(x)
    if quadrant:
        s = sin_reduced_n8(r.r)
        c = cos_reduced_n8(r.r)
        return [s, c, -s, -c][r.quadrant or 0]
    return r.sign * sin_reduced_n8(r.r)


def tan_pipeline(x: float, reducer: Callable[[float], Reduced], quadrant: bool) -> float:
    if x == 0.0:
        return x
    r = reducer(x)
    if quadrant:
        t = tan_reduced_pade(r.r)
        if (r.quadrant or 0) & 1:
            return math.copysign(math.inf, -r.r) if t == 0.0 else -1.0 / t
        return t
    ar = abs(r.r)
    if ar <= QUARTER_PI_D:
        return tan_reduced_pade(r.r)
    u = HALF_PI_D - ar
    if u == 0.0:
        return math.copysign(math.inf, r.r)
    return math.copysign(1.0, r.r) / tan_reduced_pade(u)


def ordered_bits(x: float) -> int:
    u = struct.unpack(">Q", struct.pack(">d", x))[0]
    sign = 1 << 63
    return ~u & ((1 << 64) - 1) if u & sign else u | sign


def ulp(a: float, b: float) -> int:
    if a == b:
        return 0
    if not math.isfinite(a) or not math.isfinite(b):
        return (1 << 64) - 1
    return abs(ordered_bits(a) - ordered_bits(b))


def make_inputs(bound: float, samples: int, seed: int) -> list[float]:
    rng = random.Random(seed)
    xs = [rng.uniform(-bound, bound) for _ in range(samples)]
    for s in (-1.0, 1.0):
        xs.extend(
            [
                s * bound,
                math.nextafter(s * bound, 0.0),
                s * math.floor(bound / PI_D) * PI_D,
                s * math.floor(bound / HALF_PI_D) * HALF_PI_D,
                s * (math.floor(bound / PI_D) * PI_D + HALF_PI_D),
                math.nextafter(s * HALF_PI_D, 0.0),
                math.nextafter(s * HALF_PI_D, s * 2.0),
            ]
        )
    xs.extend([0.0, PI_D, HALF_PI_D])
    return xs


def reducer_reference(x: float, quadrant: bool) -> tuple[Decimal, Decimal, float, int | None]:
    mx = dfloat(x)
    period = HALF_PI if quadrant else PI
    q = nearest_decimal(mx / period)
    r = mx - q * period
    return q, r, parity_sign(q), mod4_decimal(q) if quadrant else None


def pole_distance_reference(x: float) -> Decimal:
    mx = dfloat(x)
    q = nearest_decimal((mx - HALF_PI) / PI)
    return abs(mx - (q * PI + HALF_PI))


def reducer_metrics(xs: list[float], quadrant: bool, fn: Callable[[float], Reduced]) -> dict:
    t0 = time.perf_counter()
    sink = 0.0
    for x in xs:
        r = fn(x)
        sink += r.r
    elapsed = time.perf_counter() - t0
    if sink == 123456789.0:
        print("")
    errors: list[float] = []
    qfail = sfail = fail = 0
    for x in xs:
        q, r_ref, sign_ref, quad_ref = reducer_reference(x, quadrant)
        got = fn(x)
        if not got.ok or not math.isfinite(got.r):
            fail += 1
            continue
        err = abs(dfloat(got.r) - r_ref)
        if not quadrant:
            err = min(err, abs(abs(dfloat(got.r)) - abs(r_ref)))
        errors.append(float(err))
        if quadrant:
            qfail += int(got.quadrant != quad_ref)
        else:
            sfail += int(math.copysign(1.0, got.sign) != math.copysign(1.0, sign_ref))
    return {
        "sample_count": len(xs),
        "ns_per_sample": elapsed * 1e9 / len(xs),
        "max_reduction_error": max(errors) if errors else math.nan,
        "mean_reduction_error": statistics.fmean(errors) if errors else math.nan,
        "rmse": math.sqrt(statistics.fmean([e * e for e in errors])) if errors else math.nan,
        "quadrant_failures": qfail,
        "sign_failures": sfail,
        "failure_count": fail,
    }


def pipeline_metrics(
    xs: list[float],
    trig: str,
    quadrant: bool,
    reducer: Callable[[float], Reduced],
) -> dict:
    errors: list[float] = []
    rels: list[float] = []
    ulps: list[int] = []
    fail = 0
    pole_risk = 0.0
    for x in xs:
        ref = getattr(math, trig)(x)
        if trig == "cos":
            got = cos_pipeline(x, reducer, quadrant)
        elif trig == "sin":
            got = sin_pipeline(x, reducer, quadrant)
        else:
            got = tan_pipeline(x, reducer, quadrant)
        if not math.isfinite(got) or not math.isfinite(ref):
            if not (math.isinf(got) and math.isinf(ref) and math.copysign(1.0, got) == math.copysign(1.0, ref)):
                fail += 1
            continue
        e = abs(got - ref)
        errors.append(e)
        if ref:
            rels.append(e / abs(ref))
        ulps.append(ulp(got, ref))
        if trig == "tan":
            d = pole_distance_reference(x)
            if d < Decimal("1e-3") and ref:
                pole_risk = max(pole_risk, e / abs(ref))
    return {
        "max_abs_error": max(errors) if errors else math.nan,
        "mean_abs_error": statistics.fmean(errors) if errors else math.nan,
        "pipeline_rmse": math.sqrt(statistics.fmean([e * e for e in errors])) if errors else math.nan,
        "max_relative_error": max(rels) if rels else 0.0,
        "max_ulp_error": max(ulps) if ulps else 0,
        "failure_count": fail,
        "tan_pole_risk_score": pole_risk,
    }


def reducer_status(m: dict, quadrant: bool) -> str:
    structural_limit = max(8, int(0.01 * m["sample_count"]))
    structural = (
        m["failure_count"]
        or m["sign_failures"] > structural_limit
        or (quadrant and m["quadrant_failures"] > structural_limit)
    )
    err = m["max_reduction_error"]
    if structural or err > 1e-3:
        return "Fail"
    if err > 1e-8:
        return "Warning"
    if err > 1e-11:
        return "OK"
    return "Good"


def pipeline_status(m: dict, trig: str) -> str:
    if m["failure_count"] or m["max_abs_error"] > 1e-3 or (trig == "tan" and m["tan_pole_risk_score"] > 1e-3):
        return "Fail"
    if m["max_abs_error"] > 1e-8 or (trig == "tan" and m["tan_pole_risk_score"] > 1e-6):
        return "Warning"
    if m["max_abs_error"] > 1e-11:
        return "OK"
    return "Good"


def write_svg(summary_rows: list[dict[str, str]], path: Path) -> None:
    colors = {"Good": "#16a34a", "OK": "#84cc16", "Warning": "#f59e0b", "Fail": "#dc2626"}
    labels = ["nearbyint_pi", "cody_waite_pi", "remainder_pi", "cos_safe", "sin_safe", "tan_safe"]
    cell_w, cell_h = 94, 24
    left, top = 120, 44
    width = left + cell_w * len(labels) + 24
    height = top + cell_h * len(summary_rows) + 60
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#fff"/>',
        '<text x="18" y="24" font-family="Segoe UI, Arial" font-size="16" font-weight="700">toltrig reduction limit map</text>',
    ]
    lx = width - 316
    for idx, status in enumerate(("Good", "OK", "Warning", "Fail")):
        x = lx + idx * 76
        parts.append(f'<rect x="{x}" y="12" width="12" height="12" fill="{colors[status]}"/>')
        parts.append(f'<text x="{x + 16}" y="22" font-family="Segoe UI, Arial" font-size="10" fill="#334155">{status}</text>')
    for j, label in enumerate(labels):
        x = left + j * cell_w
        parts.append(f'<text x="{x + 4}" y="{top - 10}" font-family="Segoe UI, Arial" font-size="10">{label}</text>')
    for i, row in enumerate(summary_rows):
        y = top + i * cell_h
        parts.append(f'<text x="18" y="{y + 16}" font-family="Segoe UI, Arial" font-size="11">{row["range"]}</text>')
        for j, label in enumerate(labels):
            x = left + j * cell_w
            status = row[label]
            parts.append(f'<rect x="{x}" y="{y}" width="{cell_w - 2}" height="{cell_h - 2}" fill="{colors[status]}"/>')
            parts.append(f'<text x="{x + 6}" y="{y + 15}" font-family="Segoe UI, Arial" font-size="10" fill="#fff">{status}</text>')
    parts.append('<text x="18" y="{}" font-family="Segoe UI, Arial" font-size="11" fill="#475569">Generated from Decimal reducer references and platform math pipeline references.</text>'.format(height - 18))
    parts.append("</svg>")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(parts), encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--samples", type=int, default=2000)
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--csv", default="reduction-limits.csv")
    ap.add_argument("--summary-csv", default="reduction-limits-summary.csv")
    ap.add_argument("--svg", default="docs/assets/reduction-limits-summary.svg")
    args = ap.parse_args()
    if args.quick:
        args.samples = 500

    detail_fields = [
        "range_power",
        "input_bound",
        "category",
        "model",
        "samples",
        "ns_per_sample",
        "max_reduction_error",
        "mean_reduction_error",
        "rmse",
        "quadrant_failures",
        "sign_failures",
        "failure_count",
        "max_abs_error",
        "mean_abs_error",
        "pipeline_rmse",
        "max_relative_error",
        "max_ulp_error",
        "tan_pole_risk_score",
        "status",
    ]
    summary_fields = [
        "range",
        "nearbyint_pi",
        "cody_waite_pi",
        "remainder_pi",
        "nearbyint_half_pi",
        "cody_waite_half_pi",
        "remainder_half_pi",
        "cos_safe",
        "sin_safe",
        "tan_safe",
    ]

    detail_rows: list[dict] = []
    summary_rows: list[dict[str, str]] = []
    for p in range(17):
        bound = float(10**p)
        xs = make_inputs(bound, args.samples, 20260605 + p)
        summary = {"range": f"1e{p}"}
        reducer_statuses: dict[str, str] = {}
        for name, quadrant, fn in REDUCERS:
            m = reducer_metrics(xs, quadrant, fn)
            status = reducer_status(m, quadrant)
            reducer_statuses[name] = status
            row = {field: "" for field in detail_fields}
            row.update(
                {
                    "range_power": p,
                    "input_bound": bound,
                    "category": "reducer",
                    "model": name,
                    "samples": len(xs),
                    **{k: v for k, v in m.items() if k != "sample_count"},
                    "status": status,
                }
            )
            detail_rows.append(row)
        summary.update(reducer_statuses)

        for trig in ("cos", "sin", "tan"):
            safe = "Good"
            for name, quadrant, fn in REDUCERS[:3]:
                m = pipeline_metrics(xs, trig, quadrant, fn)
                status = pipeline_status(m, trig)
                if name == "nearbyint_pi":
                    safe = status
                row = {field: "" for field in detail_fields}
                row.update(
                    {
                        "range_power": p,
                        "input_bound": bound,
                        "category": trig,
                        "model": name,
                        "samples": len(xs),
                        **m,
                        "status": status,
                    }
                )
                detail_rows.append(row)
            summary[f"{trig}_safe"] = safe
        summary_rows.append(summary)

    with open(args.csv, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=detail_fields)
        w.writeheader()
        w.writerows(detail_rows)
    with open(args.summary_csv, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=summary_fields)
        w.writeheader()
        w.writerows(summary_rows)
    write_svg(summary_rows, Path(args.svg))
    print(f"CSV written to {args.csv} and {args.summary_csv}")
    print(f"SVG written to {args.svg}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
