// help_chips_test.cpp
// Pins the Navigation Help card's section-navigation chip model (fix thse
// isseue.md section 3 + add-on section 3): labels, strip layout, hit-testing,
// horizontal scroll clamp + ensure-visible, active-section tracking and the
// click-to-section scroll target. Self-contained: own main(), exit 0 = pass.

#include "help_chips_logic.h"

#include <cwchar>
#include <cstdio>
#include <vector>

using namespace helpchips;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- four chips, labelled exactly like the four sections ----
    check(Count() == 4, "there are exactly four section chips");
    check(wcscmp(Label(0), L"Navigation") == 0,     "chip 0 is Navigation");
    check(wcscmp(Label(1), L"New Folder") == 0,      "chip 1 is New Folder");
    check(wcscmp(Label(2), L"Group Creation") == 0,  "chip 2 is Group Creation");
    check(wcscmp(Label(3), L"Auto Group") == 0,      "chip 3 is Auto Group");
    check(Label(4)[0] == L'\0', "out-of-range chip label is empty");

    // ---- strip layout: left-to-right from startX with a uniform gap ----
    std::vector<int> widths = {50, 60, 70, 80};
    const int gap = 6;
    auto rects = Layout(widths, gap, 0);
    check(rects.size() == 4, "one rect per chip");
    check(rects[0].left == 0  && rects[0].right == 50,  "chip 0 spans [0,50)");
    check(rects[1].left == 56 && rects[1].right == 116, "chip 1 starts after width+gap");
    check(rects[2].left == 122 && rects[2].right == 192, "chip 2 placed after the gap");
    check(rects[3].left == 198 && rects[3].right == 278, "chip 3 placed after the gap");
    check(TotalWidth(widths, gap) == 278, "total strip width is widths + inner gaps");

    // ---- hit-testing (content coords); gaps and outside return -1 ----
    check(HitTest(rects, 25) == 0,  "a point inside chip 0 hits chip 0");
    check(HitTest(rects, 50) == -1, "a point in the gap after chip 0 misses");
    check(HitTest(rects, 56) == 1,  "the gap end is the start of chip 1");
    check(HitTest(rects, 277) == 3, "the last pixel of chip 3 hits chip 3");
    check(HitTest(rects, 278) == -1, "a point past the strip misses");
    check(HitTest(rects, -1) == -1,  "a negative point misses");

    // ---- horizontal strip scroll clamp (overflow case) ----
    const int visibleW = 200, stripW = 278;
    check(MaxScrollX(stripW, visibleW) == 78, "max scroll is stripW - visibleW");
    check(MaxScrollX(150, 200) == 0, "no scroll when the strip fits");
    check(ClampScrollX(-5, stripW, visibleW) == 0,  "offset clamps up to 0");
    check(ClampScrollX(999, stripW, visibleW) == 78, "offset clamps down to max");
    check(ClampScrollX(40, stripW, visibleW) == 40,  "an in-range offset is kept");

    // ---- ensure a chip is visible: minimal shift, both directions ----
    check(EnsureVisible(rects, 3, 0, visibleW, stripW) == 78,
          "revealing the last chip scrolls so its right edge is visible");
    check(EnsureVisible(rects, 0, 78, visibleW, stripW) == 0,
          "revealing the first chip scrolls back so its left edge is visible");
    check(EnsureVisible(rects, 1, 0, visibleW, stripW) == 0,
          "an already-visible chip does not move the strip");

    // ---- active-section tracking from the body scroll ----
    std::vector<int> anchors = {0, 100, 250, 400};
    const int vh = 150;  // activation line = scrollY + 50
    check(ActiveSection(anchors, 0, vh) == 0,   "top of the body selects Navigation");
    check(ActiveSection(anchors, 60, vh) == 1,  "scrolling into New Folder selects it");
    check(ActiveSection(anchors, 210, vh) == 2, "scrolling into Group Creation selects it");
    check(ActiveSection(anchors, 400, vh) == 3, "the bottom selects Auto Group");

    // ---- click-to-section scroll target, clamped to the body's max scroll ----
    const int maxScrollY = 380;
    check(ScrollTargetFor(anchors, 0, maxScrollY) == 0,   "clicking Navigation scrolls to top");
    check(ScrollTargetFor(anchors, 2, maxScrollY) == 250, "clicking a chip scrolls that section to the top");
    check(ScrollTargetFor(anchors, 3, maxScrollY) == 380, "the last chip clamps to the body max scroll");
    check(ScrollTargetFor(anchors, 9, maxScrollY) == 0,   "an out-of-range chip is a no-op (top)");

    // ---- keyboard cycling wraps ----
    check(NextChip(3, 4) == 0, "Next from the last chip wraps to the first");
    check(PrevChip(0, 4) == 3, "Prev from the first chip wraps to the last");
    check(NextChip(0, 4) == 1, "Next advances");
    check(PrevChip(2, 4) == 1, "Prev retreats");

    if (g_fail == 0) printf("help_chips_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
