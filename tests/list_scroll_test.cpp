// list_scroll_test.cpp
// Pins the Selected Items scroll model (waht to create.md #1): fixed viewport of
// ~4-6 rows, scrollbar only on overflow, and Up/Down/Home/End/PageUp/PageDown +
// mouse-wheel scrolling, plus proportional thumb geometry and the visible-row
// range. Mirrored from the mod into list_scroll_logic.h.
//
// Self-contained: own main(), exit 0 = pass. RED before the header is
// implemented; GREEN once it mirrors the mod.

#include "list_scroll_logic.h"

#include <cstdio>

using namespace listscroll;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- viewport row math ----
    check(VisibleRows(120, 24) == 5, "120/24 -> 5 rows");
    check(VisibleRows(100, 24) == 4, "100/24 -> 4 rows (floor)");
    check(VisibleRows(50, 0) == 0, "zero row height -> 0 rows (no divide by zero)");

    // ---- overflow / max top row ----
    check(!NeedsScrollbar(5, 5), "exactly fits -> no scrollbar");
    check(NeedsScrollbar(6, 5), "one over -> scrollbar");
    check(!NeedsScrollbar(3, 5), "under -> no scrollbar");
    check(MaxTopRow(12, 5) == 7, "12 items, 5 visible -> max top 7");
    check(MaxTopRow(3, 5) == 0, "fewer items than viewport -> max top 0");

    // ---- clamping ----
    check(ClampTopRow(-3, 7) == 0, "clamp below 0");
    check(ClampTopRow(99, 7) == 7, "clamp above max");
    check(ClampTopRow(4, 7) == 4, "in-range unchanged");

    // ---- keyboard navigation (12 items, 5 visible, max top 7) ----
    check(OnKey(Key::Down, 0, 12, 5) == 1, "Down from top");
    check(OnKey(Key::Up, 0, 12, 5) == 0, "Up at top clamps");
    check(OnKey(Key::Up, 3, 12, 5) == 2, "Up moves one");
    check(OnKey(Key::Home, 5, 12, 5) == 0, "Home to first");
    check(OnKey(Key::End, 0, 12, 5) == 7, "End to last full page");
    check(OnKey(Key::PageDown, 0, 12, 5) == 5, "PageDown one page");
    check(OnKey(Key::PageDown, 5, 12, 5) == 7, "PageDown clamps at max");
    check(OnKey(Key::PageUp, 7, 12, 5) == 2, "PageUp one page");
    check(OnKey(Key::PageUp, 2, 12, 5) == 0, "PageUp clamps at 0");

    // ---- mouse wheel (3 rows per notch) ----
    check(OnWheel(3, +120, 12, 5, 3) == 0, "wheel forward one notch scrolls up 3");
    check(OnWheel(0, -120, 12, 5, 3) == 3, "wheel back one notch scrolls down 3");
    check(OnWheel(0, -1200, 12, 5, 3) == 7, "big wheel clamps at max top");
    check(OnWheel(7, +1200, 12, 5, 3) == 0, "big wheel up clamps at 0");

    // ---- visible range ----
    Range r0 = VisibleRange(0, 12, 5);
    check(r0.first == 0 && r0.count == 5, "range at top");
    Range r7 = VisibleRange(7, 12, 5);
    check(r7.first == 7 && r7.count == 5, "range at bottom is full");
    Range few = VisibleRange(0, 3, 5);
    check(few.first == 0 && few.count == 3, "range shorter than viewport");

    // ---- thumb geometry (track 100 px) ----
    Thumb full = ThumbRect(0, 5, 5, 100, 20);
    check(full.h == 100, "no overflow -> thumb fills track");
    Thumb half = ThumbRect(0, 10, 5, 100, 20);
    check(half.h == 50 && half.y == 0, "half the rows visible -> half thumb at top");
    Thumb halfEnd = ThumbRect(5, 10, 5, 100, 20);   // maxTop = 5
    check(halfEnd.h == 50 && halfEnd.y == 50, "thumb at bottom of track");
    Thumb tiny = ThumbRect(0, 1000, 5, 100, 20);
    check(tiny.h == 20, "min thumb height enforced");

    // ---- input routing: the list must not steal wheel/keys (#3/#4/#12) ----
    // Wheel scrolls the list only when the cursor is over the list region.
    check(WheelScrollsList(true), "wheel over list scrolls list");
    check(!WheelScrollsList(false), "wheel elsewhere does NOT scroll list");
    // Nav keys scroll the list only when NO interactive control owns focus.
    check(KeysScrollList(false, false, false), "keys scroll list when nothing focused");
    check(!KeysScrollList(true, false, false), "edit focus keeps caret keys");
    check(!KeysScrollList(false, true, false), "button focus keeps dialog nav keys");
    check(!KeysScrollList(false, false, true), "Copy/Move focus keeps arrow keys");

    if (g_fail) { printf("list_scroll_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("list_scroll_test: OK\n");
    return 0;
}
