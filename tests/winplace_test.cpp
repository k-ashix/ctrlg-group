// winplace_test.cpp
// Pins fix thse isseue.md Issue 6: the PURE remembered-placement model --
// work-area clamping (shrink + nudge + nearest-monitor fallback), visibility
// test, and the "capture NORMAL only when not maximized" rule.
// Self-contained: own main(), exit 0 = pass.

#include "winplace_logic.h"

#include <cstdio>

using namespace winplace;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const Rect work{0, 0, 1920, 1040};   // a 1920x1080 monitor minus the taskbar

    // ---- a rect that already fits is unchanged ----
    {
        Rect r{100, 100, 500, 460};
        Rect c = ClampToWork(r, work);
        check(c.left == 100 && c.top == 100 && c.w() == 400 && c.h() == 360,
              "a fitting rect keeps its size and position");
    }

    // ---- off the right / bottom edge -> nudged fully in-bounds, size kept ----
    {
        Rect r{1800, 1000, 2200, 1200};   // 400x200, hanging off the bottom-right
        Rect c = ClampToWork(r, work);
        check(c.w() == 400 && c.h() == 200, "size is preserved when nudging in-bounds");
        check(c.right <= work.right && c.bottom <= work.bottom,
              "nudged inside the right/bottom edges");
        check(c.right == work.right && c.bottom == work.bottom,
              "flush against the edges it overhung");
    }

    // ---- negative origin (off the top-left) -> pinned to the work origin ----
    {
        Rect r{-200, -50, 200, 350};
        Rect c = ClampToWork(r, work);
        check(c.left == 0 && c.top == 0, "negative origin pinned to the work origin");
        check(c.w() == 400 && c.h() == 400, "size preserved when pinning the origin");
    }

    // ---- larger than the work area -> shrunk to the work area ----
    {
        Rect r{-100, -100, 3000, 2000};
        Rect c = ClampToWork(r, work);
        check(c.w() == work.w() && c.h() == work.h(), "an oversized rect shrinks to the work area");
        check(c.left == work.left && c.top == work.top, "shrunk rect sits at the work origin");
    }

    // ---- nearest-monitor fallback: a rect on a vanished monitor, clamped to
    // the (passed) nearest monitor's work area, becomes visible there ----
    {
        Rect saved{-1920 + 50, 100, -1920 + 450, 460};   // on a monitor to the left that is now gone
        check(!IntersectsWork(saved, work), "the saved rect does not intersect the surviving monitor");
        Rect c = ClampToWork(saved, work);               // caller passes the nearest monitor
        check(IntersectsWork(c, work), "clamping relocates the window onto the nearest monitor");
        check(c.left >= work.left && c.top >= work.top, "relocated window is inside the work area");
    }

    // ---- visibility test ----
    check(IntersectsWork(Rect{10, 10, 20, 20}, work), "an on-screen rect intersects the work area");
    check(!IntersectsWork(Rect{5000, 5000, 5100, 5100}, work), "a far off-screen rect does not");

    // ---- capture-normal rule: only when NOT maximized ----
    check(ShouldCaptureNormal(false), "capture the normal rect when not maximized");
    check(!ShouldCaptureNormal(true), "never capture (overwrite) the normal rect while maximized");

    if (g_fail) { printf("winplace_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("winplace_test: OK\n");
    return 0;
}
