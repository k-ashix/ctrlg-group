// caption_click_test.cpp
// fix thse isseue.md: a DEDICATED guard for the "header controls swallow clicks"
// bug. Clicks were swallowed by TWO distinct causes:
//
//   (1) GEOMETRY  -- the caption buttons were a 28px square at y=6 inside a 36px
//       band, so the 6px above / 2px below each button fell through to the
//       DRAGGABLE caption: a press there moved the window instead of clicking
//       (and could stick in the modal move loop = "never releases"). This file
//       pins that EVERY pixel row of the band over a button column now belongs
//       to that button, so no press is ever misread as a drag.
//
//   (2) COMPOSITING -- the buttons were WS_EX_LAYERED + LWA_COLORKEY(black), so
//       on the acrylic band every non-glyph pixel was CLICK-THROUGH; presses
//       missed the button and fell through to the drag-caption (swallowed /
//       stuck). That is fixed in the mod by making the buttons PLAIN opaque
//       children (pinned by scripts/regression_check.ps1 source invariants).
//       The g++ harness cannot execute Win32 layered-window compositing, so
//       that half is a source invariant + a manual on-device check, not a unit
//       assertion here.
//
// This file owns cause (1): using the REAL layout rects + the tested
// titlebar::Classify, it proves there is no dead strip and that a click on a
// caption button is NEVER classified as draggable caption (the "stuck" path).
//
// Self-contained: own main(), exit 0 = pass.

#include "layout_logic.h"
#include "titlebar_logic.h"

#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

static titlebar::Box toBox(const layout::Rect& r) {
    return titlebar::Box{r.x, r.y, r.w, r.h};
}

int main() {
    using namespace titlebar;

    const layout::PromptLayout g = layout::ComputePromptLayout(true);
    const int bandH = layout::TitleBandH();

    const Box expand = toBox(g.chromeSizeBtn);   // Maximize / Restore (middle)
    const Box mn     = toBox(g.chromeMinBtn);    // Minimize (left)
    const Box cl     = toBox(g.chromeCloseBtn);  // Close (right)

    // The caption controls must be real, full-band-height targets.
    check(!g.chromeMinBtn.empty() && !g.chromeSizeBtn.empty() && !g.chromeCloseBtn.empty(),
          "all three caption controls exist");
    check(g.chromeMinBtn.y == 0 && g.chromeSizeBtn.y == 0 && g.chromeCloseBtn.y == 0,
          "caption controls start at the band top (y==0, no top dead strip)");
    check(g.chromeMinBtn.h == bandH && g.chromeSizeBtn.h == bandH && g.chromeCloseBtn.h == bandH,
          "caption controls span the FULL band height (no bottom dead strip)");

    struct Col { Box box; Region expect; };
    const Col cols[3] = {
        { mn,     Region::Minimize },
        { expand, Region::Expand   },
        { cl,     Region::Close    },
    };

    // (1) NO DEAD STRIP: sweep every pixel row of the band at each button's
    // horizontal centre. Each row must resolve to THAT button and never to the
    // draggable caption -- the top row (y==0) and bottom row (y==bandH-1) are
    // exactly the pixels the old 28px-at-y=6 button missed and swallowed.
    for (const Col& c : cols) {
        const int cx = c.box.x + c.box.w / 2;
        bool allOwned = true;
        bool neverCaption = true;
        for (int y = 0; y < bandH; ++y) {
            Region r = Classify(cx, y, bandH, expand, mn, cl);
            if (r != c.expect)        allOwned = false;
            if (r == Region::Caption) neverCaption = false;
        }
        check(allOwned,     "button column owns EVERY band row (no dead strip -> no swallow)");
        check(neverCaption, "a press on the button is NEVER caption (never a drag / stuck)");
        check(Classify(cx, 0,         bandH, expand, mn, cl) == c.expect,
              "TOP band row is the button, not caption (fix swallow)");
        check(Classify(cx, bandH - 1, bandH, expand, mn, cl) == c.expect,
              "BOTTOM band row is the button, not caption (fix swallow)");
    }

    // Below the band over a button column is client -- never a button, never a drag.
    const int cxClose = cl.x + cl.w / 2;
    check(Classify(cxClose, bandH + 4, bandH, expand, mn, cl) == Region::Client,
          "below the band is client, not a swallowed button hit");

    // Empty band area to the LEFT of the buttons stays draggable caption so the
    // window can still be moved (we fixed swallowing without killing the drag).
    check(Classify(4, bandH / 2, bandH, expand, mn, cl) == Region::Caption,
          "empty band area is still draggable caption");

    if (g_fail) { printf("caption_click_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("caption_click_test: OK\n");
    return 0;
}
