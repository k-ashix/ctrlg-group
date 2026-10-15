// seg_glide_test.cpp
// Pins the PURE Copy/Move fill-glide model (fix thse isseue.md #3): the active
// fill COLOUR glides between the Copy and Move segments over a short eased ramp
// instead of flipping instantly. Self-contained: own main(), exit 0 = pass.
// RED before seg_glide_logic.h exists; GREEN once it mirrors the mod.

#include "seg_glide_logic.h"

#include <cstdio>

using namespace segglide;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- cadence is subtle but not sluggish ----
    check(kStepMs > 0 && kStepMs <= 33, "glide ticks at ~30-60fps");
    check(kStepPct > 0 && kStepPct < 100, "progress advances a fraction per tick (animated, not instant)");

    // ---- Advance moves toward the target, clamps, never overshoots ----
    check(Advance(0, 100) == kStepPct, "advancing up adds one step");
    check(Advance(100, 0) == 100 - kStepPct, "advancing down subtracts one step");
    check(Advance(50, 50) == 50, "at target: no movement");
    check(Advance(kMax - 1, kMax) == kMax, "lands exactly on the top target (no overshoot)");
    check(Advance(kMin + 1, kMin) == kMin, "lands exactly on the bottom target (no overshoot)");
    // a full ramp terminates in a finite number of ticks and ends exactly at target
    int p = 0; int ticks = 0;
    while (p != 100 && ticks < 1000) { p = Advance(p, 100); ++ticks; }
    check(p == 100, "a full glide reaches the target");
    check(ticks >= 11 && ticks <= 14, "a full glide is ~13 frames (~208ms): an eased 160-220ms slide, not an instant flip and not the old sluggish ~400ms");

    // ---- easing: exact endpoints, monotonic, S-curve middle ----
    check(Ease(0) == 0, "ease(0) == 0");
    check(Ease(100) == 100, "ease(100) == 100");
    check(Ease(50) == 50, "ease(50) == 50 (symmetric midpoint)");
    int prev = -1;
    for (int q = 0; q <= 100; q += 5) {
        int e = Ease(q);
        check(e >= 0 && e <= 100, "ease stays in range");
        check(e >= prev, "ease is monotonic non-decreasing");
        prev = e;
    }
    check(Ease(25) < 25, "ease-in: early progress lags linear");
    check(Ease(75) > 75, "ease-out: late progress leads linear");

    // ---- sliding ACTIVE PILL: one pill that slides between Copy and Move ----
    // (fix thse isseue.md #5 C). At rest the pill fills exactly one half; the
    // two halves' covered spans always sum to one half-width, so it reads as ONE
    // pill sliding across the seam -- not two independent blends.
    { PillBand b = PillBandPct(0, true);  check(b.startPct == 0   && b.endPct == 100, "p=0: Copy half fully covered by the pill"); }
    { PillBand b = PillBandPct(0, false); check(b.startPct == 0   && b.endPct == 0,   "p=0: Move half shows no pill"); }
    { PillBand b = PillBandPct(100, true);  check(b.startPct == 100 && b.endPct == 100, "p=100: Copy half shows no pill"); }
    { PillBand b = PillBandPct(100, false); check(b.startPct == 0   && b.endPct == 100, "p=100: Move half fully covered by the pill"); }
    for (int q = 0; q <= 100; q += 10) {
        PillBand c = PillBandPct(q, true);
        PillBand m = PillBandPct(q, false);
        check(c.endPct >= c.startPct && m.endPct >= m.startPct, "pill spans are well-formed (start <= end)");
        int cw = c.endPct - c.startPct;   // Copy half coverage
        int mw = m.endPct - m.startPct;   // Move half coverage
        check(cw + mw == 100, "pill coverage is width-conserving across the seam (one pill, not two)");
    }
    // mid-slide the pill straddles the seam: Copy covers its right part, Move its left part.
    { PillBand c = PillBandPct(50, true); check(c.startPct == Ease(50) && c.endPct == 100, "p=50: Copy pill hugs the seam (right part of its half)"); }
    { PillBand m = PillBandPct(50, false); check(m.startPct == 0 && m.endPct == Ease(50), "p=50: Move pill hugs the seam (left part of its half)"); }

    // ---- resting progress seeds the correct side on open ----
    check(RestProgress(false) == 0,   "Copy rests at 0");
    check(RestProgress(true)  == 100, "Move rests at 100");

    // ---- muted accent amplitude: calmer, less "flashy" segment fill (#6) ----
    // The glide timing is fixed (calm ~400ms above); the remaining lever for
    // "animations yet feels flashy" is AMPLITUDE. The sliding highlight is now a
    // MUTED accent tint instead of the full-strength Create blue, so the active
    // patch reads as a gentle, premium accent rather than a loud flash.
    check(kFillAccentPct > 0 && kFillAccentPct < 100,
          "segment accent is muted below full strength");
    check(MutedAccentFrac(0)   == 0,   "no accent stays no accent");
    check(MutedAccentFrac(100) == kFillAccentPct,
          "a fully-active segment lands on the muted accent, not full strength");
    check(MutedAccentFrac(100) < 100, "fully-active segment is softer than the loud full accent");
    int mprev = -1;
    for (int q = 0; q <= 100; q += 10) {
        int m = MutedAccentFrac(q);
        check(m >= 0 && m <= kFillAccentPct, "muted accent stays within [0, kFillAccentPct]");
        check(m >= mprev, "muted accent is monotonic non-decreasing");
        check(m <= q, "muting never increases the accent");
        mprev = m;
    }

    if (g_fail) { printf("seg_glide_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("seg_glide_test: OK\n");
    return 0;
}
