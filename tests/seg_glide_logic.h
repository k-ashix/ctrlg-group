// seg_glide_logic.h
//
// PURE Copy/Move "fill glide" model (fix thse isseue.md #3): switching the
// Copy|Move segmented control must GLIDE the active-fill COLOUR between the two
// segments over a short eased ramp instead of FLIPPING instantly. The two
// segments are separate owner-drawn HWNDs, so each one independently blends its
// own fill between the neutral and the accent colour by an "accent fraction"
// derived from a shared glide progress; together they read as one accent patch
// sliding across the seam.
//
// Byte-for-byte mirror of namespace segglide in ctrlg-group.wh.cpp. This header
// owns ONLY the pure numbers (timer cadence, easing, per-segment accent
// fraction, progress advance); the GDI colour blend + SetTimer wiring live in
// the mod and are pinned by source invariants.
//
#pragma once

namespace segglide {

// Progress runs 0..100: 0 == Copy fully accented, 100 == Move fully accented.
inline constexpr int kMin  = 0;
inline constexpr int kMax  = 100;

// Animation cadence (fix thse isseue.md #5 C): an eased Copy<->Move slide of
// ~200ms. The model is DPI-INDEPENDENT -- progress is a 0..100 percentage
// advanced kStepPct per ~kStepMs tick, so ceil(100/kStepPct) ticks * kStepMs ms
// == ~208ms, inside the issue's 160-220ms target, on every monitor and DPI.
// Smaller would read as an instant flip; the old 20ms/5% (~400ms, 20 ticks) was
// the sluggish, wiggly glide the issue asks to replace.
inline constexpr int kStepMs  = 16;
inline constexpr int kStepPct = 8;

// Advance `cur` one tick toward `target`, clamped so it never overshoots and
// always lands exactly on the target (so the glide terminates cleanly).
inline int Advance(int cur, int target) {
    if (target < kMin) target = kMin;
    if (target > kMax) target = kMax;
    if (cur < target) { cur += kStepPct; if (cur > target) cur = target; }
    else if (cur > target) { cur -= kStepPct; if (cur < target) cur = target; }
    return cur;
}

// Smoothstep ease-in-out of a 0..100 progress, returned as 0..100. Endpoints are
// exact; the middle accelerates then decelerates so the colour glide feels
// smooth rather than linear.
inline int Ease(int p) {
    if (p <= kMin) return kMin;
    if (p >= kMax) return kMax;
    double t = p / 100.0;
    double e = t * t * (3.0 - 2.0 * t);   // 3t^2 - 2t^3
    int r = static_cast<int>(e * 100.0 + 0.5);
    if (r < kMin) r = kMin;
    if (r > kMax) r = kMax;
    return r;
}

// fix thse isseue.md #5 C: a single ACTIVE PILL that SLIDES between Copy and
// Move, replacing the old independent per-half colour blend (AccentFracForSeg).
// Each segment is its own HWND (one half of the track), so the sliding pill is
// expressed as the sub-span of THIS half that the pill currently covers, in
// 0..100 of the half width. The eased progress positions the pill: at rest it
// fills exactly one half; mid-slide the two halves' spans meet at the seam to
// form one continuous pill centred on the interpolated position. startPct <
// endPct means "draw a pill band here"; startPct == endPct means this half shows
// no pill.
//   * Copy (left half): the pill hugs the SEAM (right edge) and grows left as
//     progress -> 0, so span == [Ease(p), 100]  (full at p=0, empty at p=100).
//   * Move (right half): the pill hugs the SEAM (left edge) and grows right as
//     progress -> 100, so span == [0, Ease(p)]  (empty at p=0, full at p=100).
// Across the two halves the covered width always sums to exactly one half
// (100), so the pill is width-conserving as it crosses the seam -- one pill, not
// two blends.
struct PillBand { int startPct; int endPct; };
inline PillBand PillBandPct(int p, bool isCopySeg) {
    int e = Ease(p);
    if (isCopySeg) return PillBand{e, kMax};
    return               PillBand{kMin, e};
}

// Muted amplitude for the sliding pill (fix thse isseue.md #6: "Copy and move
// animations yet feels flashy"). The glide TIMING is already calm (~200ms
// above); the remaining lever is how LOUD the moving pill is. The mod fills the
// sliding pill at MutedAccentFrac(kMax) == kFillAccentPct of the way from the
// neutral segment fill toward the Create accent, so even the fully-landed pill
// is a gentle, premium accent TINT instead of the full-strength Create blue --
// the slide reads as calm, not a flash.
inline constexpr int kFillAccentPct = 65;   // keep 65% of the accent -> muted
inline int MutedAccentFrac(int accentFrac) {
    if (accentFrac < kMin) accentFrac = kMin;
    if (accentFrac > kMax) accentFrac = kMax;
    return accentFrac * kFillAccentPct / 100;
}

// The resting progress for a given Copy/Move choice (no animation in flight):
// Copy rests at 0, Move rests at 100. Used to seed the progress when a popup
// opens already on Move so it never glides in from the wrong side.
inline int RestProgress(bool isMove) { return isMove ? kMax : kMin; }

}  // namespace segglide
