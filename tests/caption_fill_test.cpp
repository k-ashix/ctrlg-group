// caption_fill_test.cpp
// Pins the idle caption-button background decision (captionfill::, modelling the
// WM_DRAWITEM caption paint in ctrlg-group.wh.cpp). fix thse isseue.md #1/#3/#5.
//
// RED before the fix: the three caption buttons always filled their rect with
// the opaque themed panelBg, so each sat as a PERMANENT grey (dark) / off-white
// (light) block on the title bar. The fix removes that permanent backplate: the
// idle surface now MATCHES the band it sits on (transparent black under a
// confirmed DWM backdrop, else the solid themed panel), so the button blends in
// while hover/press/red-Close remain transient feedback over that idle surface.
//
// Self-contained: own main(), exit 0 = pass.

#include "caption_fill_logic.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

int main() {
    using captionfill::Brush;

    // ---- Idle fill matches the band: transparent BLACK under a confirmed ----
    // ---- backdrop, solid themed panel otherwise. NEVER a grey backplate. ----
    check(captionfill::IdleBrush(true) == Brush::TransparentBlack,
          "backdrop confirmed -> idle caption fill is transparent BLACK");
    check(captionfill::IdleBrush(false) == Brush::Panel,
          "no confirmed backdrop -> idle caption fill is the solid themed panel");

    // ---- No permanent grey block in EITHER state. ----
    check(!captionfill::PaintsPermanentGreyBackplate(),
          "idle caption button never paints a permanent grey backplate");

    // ---- Idle paints no highlight -- feedback is transient (hover/press). ----
    check(!captionfill::PaintsHighlightWhenIdle(),
          "idle caption button paints NO highlight (feedback is transient only)");

    // ---- The idle fill tracks the band for BOTH backdrop states. ----
    check(captionfill::MatchesBand(true),
          "idle fill matches the band when the backdrop is confirmed");
    check(captionfill::MatchesBand(false),
          "idle fill matches the band in the solid-header fallback");

    if (g_fail == 0) {
        printf("[PASS] caption_fill_test: idle caption fill matches the band "
               "(transparent-black / panel), no permanent grey backplate\n");
        return 0;
    }
    printf("[FAIL] caption_fill_test: %d check(s) failed\n", g_fail);
    return 1;
}
