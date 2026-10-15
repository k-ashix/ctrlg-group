// win_resize_test.cpp
// Pins fix thse isseue.md Phase 3 pure logic: resize-frame hit testing, the
// Maximize/Restore glyph following the ACTUAL maximized state, maximize-to-
// work-area, restore-to-previous-bounds, minimum client size, and Ctrl+E
// density staying independent of maximize. Self-contained: own main().

#include "win_resize_logic.h"

#include <cstdio>

using namespace winresize;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- HT codes must equal the Win32 constants (returned directly) ----
    check(Client == 1 && Left == 10 && Right == 11 && Top == 12 &&
          TopLeft == 13 && TopRight == 14 && Bottom == 15 &&
          BottomLeft == 16 && BottomRight == 17,
          "Ht enum mirrors the Win32 HT* numeric codes");

    // ---- resize-frame hit test (client coords, 6px grab border) ----
    const int W = 400, H = 300, B = 6;
    check(HitTest(200, 150, W, H, B) == Client, "centre is HTCLIENT");
    check(HitTest(1, 150, W, H, B) == Left, "left edge");
    check(HitTest(W - 1, 150, W, H, B) == Right, "right edge");
    check(HitTest(200, 1, W, H, B) == Top, "top edge");
    check(HitTest(200, H - 1, W, H, B) == Bottom, "bottom edge");
    check(HitTest(1, 1, W, H, B) == TopLeft, "top-left corner");
    check(HitTest(W - 1, 1, W, H, B) == TopRight, "top-right corner");
    check(HitTest(1, H - 1, W, H, B) == BottomLeft, "bottom-left corner");
    check(HitTest(W - 1, H - 1, W, H, B) == BottomRight, "bottom-right corner");
    check(HitTest(B, B, W, H, B) == Client, "just inside the border is client");

    // ---- maximize/restore glyph tracks the real window state ----
    check(ShowRestoreGlyph(true), "maximized -> show restore glyph");
    check(!ShowRestoreGlyph(false), "normal -> show maximize glyph");

    // ---- maximize occupies the WORK AREA (not full screen) ----
    const Rect work = {0, 0, 1920, 1040};   // 1080 screen minus a 40px taskbar
    const Rect maxed = MaximizeTo(work);
    check(maxed.left == 0 && maxed.top == 0 && maxed.right == 1920 &&
          maxed.bottom == 1040,
          "maximize frame == monitor work area");
    check(Valid(maxed) && Width(maxed) == 1920 && Height(maxed) == 1040,
          "maximized rect is valid and work-area sized");

    // ---- restore returns the PREVIOUS bounds, not a hard-coded size ----
    const Rect prev = {120, 80, 120 + 660, 80 + 520};
    const Rect fallback = {0, 0, 410, 480};
    check(RestoreTo(prev, fallback).left == 120 &&
          Width(RestoreTo(prev, fallback)) == 660,
          "restore returns the saved previous bounds");
    const Rect noPrev = {0, 0, 0, 0};
    check(Width(RestoreTo(noPrev, fallback)) == 410,
          "restore falls back when there are no saved bounds (no hard-coded W)");

    // ---- minimum client size clamps a too-small resize ----
    check(MinClientW() > 0 && MinClientH() > 0, "min client size is positive");
    check(ClampW(100, MinClientW()) == MinClientW(), "width clamps up to min");
    check(ClampH(50, MinClientH()) == MinClientH(), "height clamps up to min");
    check(ClampW(900, MinClientW()) == 900, "a large width is not clamped");

    // ---- Ctrl+E density stays independent of maximize ----
    check(DensityMayResizeWindow(false),
          "not maximized -> density toggle may resize the window");
    check(!DensityMayResizeWindow(true),
          "maximized -> density toggle must NOT resize/unmaximize");

    // ---- WM_GETMINMAXINFO: maximize is clamped to the monitor WORK AREA ----
    // A borderless WS_POPUP maximized with the DEFAULT MINMAXINFO covers the
    // whole monitor (over the taskbar). ComputeMinMax derives the MINMAXINFO
    // fields from the monitor + work-area rects so maximize fills the work area
    // (taskbar respected) and a drag-resize can never shrink below the min.
    {
        const Rect monitor = {0, 0, 1920, 1080};
        const Rect workA   = {0, 0, 1920, 1040};   // 40px taskbar at the bottom
        const MinMax mm = ComputeMinMax(monitor, workA, 500, 460);
        check(mm.maxPos.cx == 0 && mm.maxPos.cy == 0,
              "primary monitor: maximized top-left == work-area origin");
        check(mm.maxSize.cx == 1920 && mm.maxSize.cy == 1040,
              "maximized size == work area (taskbar respected, not full screen)");
        check(mm.minTrack.cx == 500 && mm.minTrack.cy == 460,
              "minimum track size is the supplied outer min size");

        // secondary monitor at NEGATIVE virtual coords, taskbar docked on top
        const Rect mon2  = {-1920, 0, 0, 1080};
        const Rect work2 = {-1920, 40, 0, 1080};
        const MinMax mm2 = ComputeMinMax(mon2, work2, 500, 460);
        check(mm2.maxPos.cx == 0 && mm2.maxPos.cy == 40,
              "secondary monitor: max position is RELATIVE to the monitor origin");
        check(mm2.maxSize.cx == 1920 && mm2.maxSize.cy == 1040,
              "secondary monitor: maximized size still equals its work area");
    }

    if (g_fail == 0) printf("win_resize_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
