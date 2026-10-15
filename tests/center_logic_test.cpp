// center_logic_test.cpp
// Pins the PURE center-preserving resize geometry (fix thse isseue.md #1):
//   * when the popup changes size (compact<->expanded or DPI), the NEW top-left
//     must keep the window's CURRENT center fixed -- not its top-left;
//   * the centered rectangle is then clamped to the monitor work area so the
//     whole window stays on-screen;
//   * a window larger than the work area on an axis pins to the work-area origin
//     on that axis (never a negative/offscreen origin).
// Per the project model this does NOT unit test SetWindowPos -- only the pure
// decision the mod mirrors into namespace center. The Win32 wiring (GetWindowRect
// + MonitorFromWindow + the resize log, no SWP_NOMOVE) is pinned by a source
// invariant in scripts/regression_check.ps1.
//
// Self-contained: own main(), exit 0 = pass. RED before center_logic.h exists;
// GREEN once it mirrors the mod.

#include "center_logic.h"

#include <cstdio>

using namespace center;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- CenteredTopLeft keeps the center fixed while the size changes ----
    {
        // old 200x200 at (100,100) -> center (200,200). Grow to 400x400.
        Rect old{100, 100, 300, 300};
        Size outer{400, 400};
        Point tl = CenteredTopLeft(old, outer);
        check(tl.x == 0 && tl.y == 0, "grow keeps center (0,0 top-left)");
        check(tl.x + outer.cx / 2 == 200, "new center X unchanged");
        check(tl.y + outer.cy / 2 == 200, "new center Y unchanged");
    }
    {
        // asymmetric: old 200x100 at (100,100) center (200,150) -> 300x100.
        Rect old{100, 100, 300, 200};
        Size outer{300, 100};
        Point tl = CenteredTopLeft(old, outer);
        check(tl.x == 50 && tl.y == 100, "asymmetric grow keeps center");
    }
    {
        // shrink also keeps center: old 400x400 at (0,0) center (200,200)->100x100
        Rect old{0, 0, 400, 400};
        Size outer{100, 100};
        Point tl = CenteredTopLeft(old, outer);
        check(tl.x == 150 && tl.y == 150, "shrink keeps center");
    }

    // ---- ClampToWorkArea keeps the whole window inside the work area ----
    {
        Rect work{0, 0, 1000, 800};
        Size outer{400, 400};
        // off the right/bottom -> pulled back so right/bottom edges fit.
        Point c = ClampToWorkArea(Point{800, 600}, outer, work);
        check(c.x == 600 && c.y == 400, "clamp pulls window back onto screen");
        // already inside -> unchanged.
        Point in = ClampToWorkArea(Point{100, 100}, outer, work);
        check(in.x == 100 && in.y == 100, "inside window is left untouched");
        // off the top/left -> pinned to work-area origin.
        Point neg = ClampToWorkArea(Point{-50, -70}, outer, work);
        check(neg.x == 0 && neg.y == 0, "negative origin pinned to work origin");
    }
    {
        // work area not at (0,0) (e.g. taskbar on the left / top).
        Rect work{48, 32, 1048, 832};
        Size outer{400, 300};
        Point c = ClampToWorkArea(Point{0, 0}, outer, work);
        check(c.x == 48 && c.y == 32, "clamp respects a non-zero work origin");
    }
    {
        // window LARGER than the work area on both axes -> pin to origin.
        Rect work{0, 0, 300, 200};
        Size outer{500, 400};
        Point c = ClampToWorkArea(Point{10, 10}, outer, work);
        check(c.x == 0 && c.y == 0, "oversize window pins to work origin");
    }

    // ---- CenteredResize = CenteredTopLeft then ClampToWorkArea ----
    {
        Rect work{0, 0, 1000, 800};
        // small window near the right edge grows wide; center shifts it left
        // but clamp keeps it fully visible.
        Rect old{880, 300, 1000, 420};   // 120x120 at right edge, center (940,360)
        Size outer{560, 420};
        Point r = CenteredResize(old, outer, work);
        // centered top-left would be (940-280, 360-210) = (660,150);
        // right = 660+560 = 1220 > 1000 -> clamp to 1000-560 = 440.
        check(r.x == 440, "centered+clamped X keeps window on-screen");
        check(r.y == 150, "centered Y within work area is kept");
        check(r.x + outer.cx <= work.right, "right edge inside work area");
        check(r.y + outer.cy <= work.bottom, "bottom edge inside work area");
    }

    if (g_fail) { printf("center_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("center_logic_test: OK\n");
    return 0;
}
