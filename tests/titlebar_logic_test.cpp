// titlebar_logic_test.cpp
// Pins the PURE custom-title-bar hit classification (fix thse isseue.md #2).
// The Group popup is now WS_POPUP (no native caption), so a custom title band
// holds the title + Expand/Minimize/Close controls and must be draggable. This
// header owns the ONE decision a unit test can see: given a client point, the
// band height and the three button boxes, which region was hit? The Win32
// wiring (WS_POPUP style, WM_NCHITTEST -> HTCAPTION, the owner-draw buttons) is
// pinned by source invariants in scripts/regression_check.ps1.
//
// Self-contained: own main(), exit 0 = pass. RED before titlebar_logic.h exists.

#include "titlebar_logic.h"

#include <cstdio>

using namespace titlebar;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // 36px band; three 16px buttons right-aligned in order expand, min, close.
    Box expand{260, 6, 16, 16};
    Box mn{282, 6, 16, 16};
    Box cl{304, 6, 16, 16};
    const int bandH = 36;

    check(Classify(312, 12, bandH, expand, mn, cl) == Region::Close,
          "point on the close button -> Close");
    check(Classify(290, 12, bandH, expand, mn, cl) == Region::Minimize,
          "point on the minimize button -> Minimize");
    check(Classify(268, 12, bandH, expand, mn, cl) == Region::Expand,
          "point on the expand button -> Expand");
    check(Classify(40, 12, bandH, expand, mn, cl) == Region::Caption,
          "empty band area is draggable caption");
    check(Classify(40, 50, bandH, expand, mn, cl) == Region::Client,
          "below the band is client (not draggable)");
    check(Classify(304, 10, bandH, expand, mn, cl) == Region::Close,
          "a button inside the band still wins over caption");

    // fix thse isseue.md #1: a click in the GAP BETWEEN two buttons must invoke
    // NO button action -- it falls through to the draggable caption. The gaps
    // are expand(260..276) | gap 276..282 | min(282..298) | gap 298..304 | close.
    check(Classify(279, 12, bandH, expand, mn, cl) == Region::Caption,
          "gap between Expand and Minimize is caption (no button action)");
    check(Classify(301, 12, bandH, expand, mn, cl) == Region::Caption,
          "gap between Minimize and Close is caption (no button action)");
    check(Classify(200, 12, bandH, expand, mn, cl) == Region::Caption,
          "band area left of all buttons is caption");

    // fix thse isseue.md #1: a point OUTSIDE the title band (above it, y<0, or
    // below it) is never a caption or a button -- it is client.
    check(Classify(290, -1, bandH, expand, mn, cl) == Region::Client,
          "above the band (y<0) is client, not caption/button");
    check(Classify(312, 50, bandH, expand, mn, cl) == Region::Client,
          "below the band under a button column is still client");

    // fix thse isseue.md #1: EXACTLY ONE action per click. Sweep the whole band
    // width and assert every point resolves to one region and that only points
    // inside a button rect ever resolve to that button (no overlap/spill).
    for (int x = 0; x < 330; ++x) {
        Region r = Classify(x, 12, bandH, expand, mn, cl);
        bool inExpand = expand.contains(x, 12);
        bool inMin    = mn.contains(x, 12);
        bool inClose  = cl.contains(x, 12);
        int hits = (inExpand?1:0) + (inMin?1:0) + (inClose?1:0);
        check(hits <= 1, "button rects never overlap (<=1 button under any point)");
        check((r == Region::Expand)   == inExpand, "Expand region iff inside the Expand rect");
        check((r == Region::Minimize) == inMin,    "Minimize region iff inside the Minimize rect");
        check((r == Region::Close)    == inClose,  "Close region iff inside the Close rect");
        if (hits == 0) check(r == Region::Caption, "any non-button band point is draggable caption");
    }

    // reference order: expand left of min left of close (close is rightmost).
    check(cl.x > mn.x && mn.x > expand.x,
          "button order is expand < min < close (close rightmost)");

    // Box::contains is half-open on the right/bottom edge.
    check(cl.contains(304, 6) && !cl.contains(320, 6),
          "Box::contains is [x, x+w) half-open");

    if (g_fail) { printf("titlebar_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("titlebar_logic_test: OK\n");
    return 0;
}
