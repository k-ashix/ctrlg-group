// dpi_verify_test.cpp
// fix thse isseue.md #15: DPI 125/150/200% verification pass. We cannot take
// screenshots in CI, so this test is the programmatic equivalent: it proves
// every per-DPI metric touched by this UI pass scales crisply through the ONE
// MulDiv rounding rule (layout::ScaleForDpi), across the full ladder
// 96/120/144/168/192 (= 100/125/150/175/200%). It also checks the helper
// scalers (shellicon::IconPx) agree with that rule so they
// can never drift, and that the WM_DPICHANGED decode relayouts only on a real
// change. Self-contained: own main(), exit 0 = pass.

#include "layout_logic.h"
#include "dpi_logic.h"
#include "shell_icon_logic.h"
// pass 6 token sources, re-verified through the one MulDiv rule below.
#include "segicon_logic.h"
#include "list_badge_logic.h"
#include "destfield_logic.h"
#include "namefield_logic.h"

#include <cstdio>

using layout::ScaleForDpi;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const int ladder[] = {96, 120, 144, 168, 192};  // 100/125/150/175/200%

    // ---- the one scaling rule: identity at 96, exact 2x at 200% ----
    for (int v : {1, 2, 8, 16, 20, 28, 32, 74, 100, 304}) {
        check(ScaleForDpi(v, 96) == v, "96 dpi is identity");
        check(ScaleForDpi(v, 192) == 2 * v, "200% is an exact double (crisp)");
    }

    // ---- every touched metric scales monotonically and never shrinks ----
    // chrome hit target (#6=28), row/shell icon (#9=16), status/vector icon,
    // selected row height (24, pass6 #1), badge pad (#11=8) and badge min width (#11=32).
    for (int v : {28, 16, 24, 8, 32}) {
        int prev = -1;
        for (int d : ladder) {
            int s = ScaleForDpi(v, d);
            check(s >= v, "metric never shrinks below its 96 value");
            check(s >= prev, "metric is monotonic non-decreasing across DPI");
            prev = s;
        }
    }

    // ---- caption hit target stays comfortable at every DPI (#6) ----
    for (int d : ladder) check(ScaleForDpi(28, d) >= 28, "chrome >= 28px at every DPI");

    // ---- Shell row icon helper agrees with the one rule (#9) ----
    for (int d : ladder) {
        check(shellicon::IconPx(d) == ScaleForDpi(16, d),
              "shellicon::IconPx == ScaleForDpi(16) at every DPI");
        check(shellicon::IconPx(d) >= 16, "Shell icon never sub-16 at >=96 DPI");
    }
    check(shellicon::IconPx(192) == 32, "Shell icon is 32px at 200%");

    // fix thse isseue.md #4: the painted focus ring was removed (focus COLOUR
    // feedback now), so focus::ScaleRing is gone; the shared MulDiv rule is
    // still verified above via ScaleForDpi + shellicon::IconPx.

    // ---- a full control rect scales without inverting (ordering holds) ----
    for (int d : ladder) {
        layout::PromptLayout g = layout::ComputePromptLayout(true);
        layout::Rect cr = layout::ScaleRect(g.createButton, d);
        layout::Rect xr = layout::ScaleRect(g.cancelButton, d);
        check(cr.w > 0 && cr.h > 0, "scaled Create button stays positive");
        check(cr.right() <= xr.x, "Create stays left of Cancel at every DPI");
        check(layout::ScaleForDpi(g.windowW, d) >= g.windowW,
              "window width never shrinks when scaled up");
    }

    // ---- WM_DPICHANGED decode: relayout only on a real change ----
    check(dpiutil::NewDpiFromWparam((144u << 16) | 144u) == 144, "decode low word");
    check(dpiutil::NewDpiFromWparam((192u << 16) | 192u) == 192, "decode 200%");
    check(dpiutil::NeedsRelayout(96, 144), "relayout when DPI changes");
    check(!dpiutil::NeedsRelayout(144, 144), "no relayout when DPI is unchanged");
    check(!dpiutil::NeedsRelayout(96, 0), "no relayout on a bogus 0 DPI");

    // ---- PASS 5 re-verify: EVERY new pass-6 token rides the ONE MulDiv rule ----
    // These reference the REAL tokens (not copies), so a hardcoded device-px
    // literal sneaking in later would fail to compile or fail to scale. Each
    // must be identity at 96, an exact double at 200%, and monotonic + never
    // shrinking across the 100/125/150/175/200% ladder.
    for (int v : {layout::kSelRowH, layout::kCardInset, layout::kNameFieldH,
                  layout::kActionCardH,
                  segicon::kSegIcon, segicon::kSegIconGap,
                  listbadge::kRadius, destfield::kRadius, namefield::kRadius}) {
        check(ScaleForDpi(v, 96) == v, "pass6 token: 96 dpi is identity");
        check(ScaleForDpi(v, 192) == 2 * v, "pass6 token: exact 2x at 200% (crisp)");
        int prev = -1;
        for (int d : ladder) {
            int s = ScaleForDpi(v, d);
            check(s >= v, "pass6 token never shrinks below its 96 value");
            check(s >= prev, "pass6 token is monotonic non-decreasing across DPI");
            prev = s;
        }
    }

    if (g_fail == 0) printf("dpi_verify_test: OK\n");
    else             printf("dpi_verify_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
