// chip_toolbar_test.cpp
// Pins fix thse isseue.md #4/#5: the summary row gives remaining width to the
// scrollable chip strip and keeps the action toolbar (Show extensions + future
// actions) visible to its right, wrapping the toolbar to a second row at narrow
// widths instead of clipping it. Self-contained: own main(), exit 0 = pass.

#include "chip_toolbar_logic.h"

#include <cstdio>

using namespace chiptoolbar;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // Wide row: single row, strip takes the remaining width, toolbar on the right.
    Layout wide = Compute(0, 0, 500, 28, /*toolbarW*/120, /*gap*/8, /*minStripW*/120, /*rowH*/28);
    check(!wide.wrapped && wide.rows == 1, "wide -> single row");
    check(wide.strip.left == 0 && wide.strip.right == 372, "strip gets remaining width");
    check(wide.toolbar.left == 380 && wide.toolbar.right == 500, "toolbar sits to the right");
    check(wide.strip.w() >= 120, "strip never below min");

    // Narrow row: toolbar wraps to a second row, strip keeps full width row 1.
    Layout narrow = Compute(0, 0, 200, 28, 120, 8, 120, 28);
    check(narrow.wrapped && narrow.rows == 2, "narrow -> wraps to 2 rows");
    check(narrow.strip.left == 0 && narrow.strip.right == 200, "strip full width on row 1");
    check(narrow.toolbar.top == 28 && narrow.toolbar.bottom == 56, "toolbar on row 2");
    check(narrow.toolbar.right == 200, "toolbar spans width on row 2");

    // Button placement preserves order and never zero-width.
    Box tb{0, 0, 300, 28};
    std::vector<int> widths = {120, 90};  // Show extensions, future action
    auto boxes = ButtonBoxes(tb, widths, 8);
    check(boxes.size() == 2, "two buttons");
    check(boxes[0].left == 0 && boxes[0].right == 120, "first button = Show extensions");
    check(boxes[1].left == 128 && boxes[1].w() == 90, "second button after gap, order preserved");

    if (g_fail) { printf("chip_toolbar_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("chip_toolbar_test: OK\n");
    return 0;
}
