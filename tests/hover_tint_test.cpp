// hover_tint_test.cpp
// Pins fix thse isseue.md #14: the Create/Cancel buttons and Copy/Move segments
// get a HOVER affordance (lighten the fill) in addition to the existing pressed
// fill + focus ring. Self-contained: own main(), exit 0 = pass.

#include "hover_tint_logic.h"

#include <cstdio>

using namespace hovertint;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- state precedence ----
    check(VisFor(false, false) == Vis::Normal,  "idle -> Normal");
    check(VisFor(true,  false) == Vis::Hover,   "hot -> Hover");
    check(VisFor(false, true)  == Vis::Pressed, "pressed -> Pressed");
    check(VisFor(true,  true)  == Vis::Pressed, "pressed beats hover");

    // ---- only Hover repaints the fill ----
    check(PaintsHover(Vis::Hover),    "hover repaints");
    check(!PaintsHover(Vis::Normal),  "normal does not repaint");
    check(!PaintsHover(Vis::Pressed), "pressed uses its own fill, not the hover tint");

    // ---- blend math (clamped, monotonic) ----
    check(BlendChannel(100, 255, 0)   == 100, "0% blend keeps the channel");
    check(BlendChannel(100, 255, 100) == 255, "100% blend reaches the target");
    check(BlendChannel(100, 255, 50)  == 177, "50% blend is the midpoint-ish (100+77)");
    check(BlendChannel(0, 255, 12)    == 30,  "12% of 0->255 is 30");
    check(BlendChannel(255, 255, 12)  == 255, "already-white stays white");
    check(BlendChannel(300, 255, 50)  <= 255, "over-range input is clamped high");
    check(BlendChannel(-5, 255, 0)    >= 0,   "under-range input is clamped low");
    check(BlendChannel(100, 255, 12)  >  100, "hover lightens (channel increases)");
    check(BlendChannel(100, 255, 12)  <  BlendChannel(100, 255, 40),
          "more percent lightens more");

    // ---- hover amount is a sensible, subtle value ----
    check(kHoverPct > 0 && kHoverPct <= 25, "hover tint is subtle (0 < pct <= 25)");

    // ---- focus-colour feedback (#4) replaces the painted ring with a subtle
    // accent tint; it is its own token so focus and hover stay independent ----
    check(kFocusPct > 0 && kFocusPct <= 25, "focus tint is subtle (0 < pct <= 25)");
    check(BlendChannel(200, 0, kFocusPct) == 168, "focus tint blends a channel toward the accent (200->168 @16%)");
    check(BlendChannel(100, 255, kFocusPct) > 100, "focus tint toward a lighter accent still moves the channel");

    if (g_fail == 0) printf("hover_tint_test: OK\n");
    else             printf("hover_tint_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
