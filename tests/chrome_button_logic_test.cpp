// chrome_button_logic_test.cpp
// Pins the PURE hover/pressed visual-state model for the custom title-bar
// chrome buttons (Expand / Minimize / Close) -- fix thse isseue.md #7. The
// owner-draw paint must read these decisions from ONE tested source so the
// controls gain hover + pressed feedback (and the Close button its Win11-style
// danger tint) without a per-button ad-hoc rule.
//
// Self-contained: own main(), exit 0 = pass. RED before chrome_button_logic.h
// exists; GREEN once it mirrors the mod.

#include "chrome_button_logic.h"

#include <cstdio>

using namespace chromebtn;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- state precedence: pressed > hover > flat ----
    check(FillFor(false, false) == Fill::None,    "idle -> None (flat)");
    check(FillFor(true,  false) == Fill::Hover,   "hover -> Hover");
    check(FillFor(false, true)  == Fill::Pressed, "pressed -> Pressed");
    check(FillFor(true,  true)  == Fill::Pressed, "pressed wins over hover");

    // ---- a background is painted for any non-flat state ----
    check(!PaintsBackground(Fill::None),      "flat paints no background");
    check(PaintsBackground(Fill::Hover),      "hover paints a background");
    check(PaintsBackground(Fill::Pressed),    "pressed paints a background");

    // ---- Close gets the danger (red) tint on hover/press; others never do ----
    check(IsDanger(true, Fill::Hover),   "Close hovered -> danger tint");
    check(IsDanger(true, Fill::Pressed), "Close pressed -> danger tint");
    check(!IsDanger(true, Fill::None),   "Close idle -> no danger tint");
    check(!IsDanger(false, Fill::Hover),   "Expand/Minimize never show danger (hover)");
    check(!IsDanger(false, Fill::Pressed), "Expand/Minimize never show danger (pressed)");

    // ---- fix thse isseue.md A/D: the hover/press colour feedback fills the FULL
    // caption-button rectangle -- full header height (top y==0, bottom == band)
    // AND edge-to-edge width (no left/right inset). The old 4px guard left blank
    // strips between the hit area and the visible fill; it is removed (guard 0).
    check(kFillGuard == 0, "caption feedback has NO inset (full-rect fill, fix thse isseue.md A/D)");
    {
        const FillBox fb = FillArea(44, 36, kFillGuard);
        check(fb.y == 0,  "fill starts at the band top (top == header height)");
        check(fb.h == 36, "fill spans the full band height (bottom == header height)");
        check(fb.x == 0,  "fill starts at the cell left edge (no left inset)");
        check(fb.x + fb.w == 44, "fill reaches the cell right edge (no right inset)");
        check(fb.w == 44, "fill spans the FULL cell width (edge-to-edge, matches the hit rect)");
    }
    {
        const FillBox fz = FillArea(44, 36, 0);
        check(fz.x == 0 && fz.w == 44 && fz.y == 0 && fz.h == 36,
              "zero guard => edge-to-edge full-height fill");
    }

    if (g_fail) { printf("chrome_button_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("chrome_button_logic_test: OK\n");
    return 0;
}
