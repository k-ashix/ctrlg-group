// chip_scroll_test.cpp
// Pins fix thse isseue.md #4: the summary chip strip scrolls horizontally with
// the mouse wheel (and Shift/horizontal wheel), stays clamped, brings the
// active chip into view, and supports Left/Right/Home/End keyboard navigation
// even with dozens of chips. Self-contained: own main(), exit 0 = pass.

#include "chip_scroll_logic.h"

#include <cstdio>

using namespace chipscroll;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- overflow math (dozens of chips) ----
    check(MaxOffset(2000, 300) == 1700, "content wider than viewport");
    check(MaxOffset(200, 300) == 0, "content fits -> no scroll room");
    check(NeedsScroll(2000, 300), "overflow needs scroll");
    check(!NeedsScroll(200, 300), "fits -> no scroll");

    // ---- clamping ----
    check(Clamp(-50, 1700) == 0, "clamp below 0");
    check(Clamp(9999, 1700) == 1700, "clamp above max");
    check(Clamp(500, 1700) == 500, "in range unchanged");

    // ---- wheel (40 px per notch) ----
    check(OnWheel(500, -120, 1700, 40) == 540, "wheel toward user pans right");
    check(OnWheel(500, +120, 1700, 40) == 460, "wheel away pans left");
    check(OnWheel(10, +1200, 1700, 40) == 0, "big wheel left clamps at 0");
    check(OnWheel(1690, -1200, 1700, 40) == 1700, "big wheel right clamps at max");

    // ---- horizontal wheel normalizes to the vertical sign ----
    check(WheelDeltaFromHorizontal(120) == -120, "hwheel sign is inverted");

    // ---- bring active chip into view ----
    check(BringIntoView(50, 120, 200, 300) == 50, "chip left of view -> scroll to its left");
    check(BringIntoView(900, 980, 200, 300) == 680, "chip right of view -> align its right edge");
    check(BringIntoView(250, 320, 200, 300) == 200, "already visible -> no change");

    // ---- keyboard nav over 50 chips ----
    check(OnKey(Key::Right, 0, 50) == 1, "right advances");
    check(OnKey(Key::Left, 0, 50) == 0, "left at start clamps");
    check(OnKey(Key::End, 0, 50) == 49, "End -> last chip");
    check(OnKey(Key::Right, 49, 50) == 49, "right at end clamps");
    check(OnKey(Key::Home, 49, 50) == 0, "Home -> first chip");
    check(OnKey(Key::Right, 0, 0) == 0, "no chips -> index 0");

    if (g_fail) { printf("chip_scroll_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("chip_scroll_test: OK\n");
    return 0;
}
