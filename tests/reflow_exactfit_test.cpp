// reflow_exactfit_test.cpp
// Pins fix thse isseue.md P1 "Selected Items dead-space logic treats an exact
// fit as capped" (lines 3054-3100 / 7582-7622). layout::ReflowToClient must
// decide "row-capped" (absorb extra height + scroll) from whether MORE rows are
// actually NEEDED than the preferred viewport shows -- NOT from equality with
// the declared row cap. An EXACT fit (rowsNeeded == visibleRows) is NOT
// overflow and must stay content-sized (no dead space).
//
// The caller supplies the true rows-needed via the new rowsNeededOverride arg;
// when it is omitted the legacy cap heuristic is unchanged (so older callers /
// tests keep their behavior). Self-contained: own main(), exit 0 = pass.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// A Group base whose Selected list shows exactly 2 whole grid cells.
static PromptLayout TwoCellBase(int cellH, int gHeaderH) {
    PromptLayout base{};
    base.isGroup = true;
    base.expanded = false;
    base.selectedContent = Rect{20, 50, 300, gHeaderH + 2 * cellH};
    base.selectedList    = Rect{20, 50 + gHeaderH, 300, 2 * cellH};
    base.windowW = 360;
    base.windowH = base.selectedList.bottom() + 200;   // footer below the list
    return base;
}

int main() {
    const int MINW = 360, MINH = 320, cellH = 86, gHeaderH = 60, GROW = 240;

    // Overflow: 5 rows needed, 2 visible -> capped -> absorb the extra height.
    {
        PromptLayout base = TwoCellBase(cellH, gHeaderH);
        PromptLayout r = ReflowToClient(base, base.windowW, base.windowH + GROW,
                                        MINW, MINH, cellH, /*cap*/2, /*rowsNeeded*/5);
        check(r.selectedList.h == base.selectedList.h + GROW,
              "overflow (rowsNeeded > visible): card absorbs extra height + scrolls");
    }

    // EXACT FIT: 2 rows needed, 2 visible -> NOT capped -> no growth (the bug).
    {
        PromptLayout base = TwoCellBase(cellH, gHeaderH);
        PromptLayout r = ReflowToClient(base, base.windowW, base.windowH + GROW,
                                        MINW, MINH, cellH, /*cap*/2, /*rowsNeeded*/2);
        check(r.selectedList.h == base.selectedList.h,
              "exact fit (rowsNeeded == visible): no dead space, content-sized");
    }

    // Under-fill: 1 row needed, 2 visible -> not capped -> no growth.
    {
        PromptLayout base = TwoCellBase(cellH, gHeaderH);
        PromptLayout r = ReflowToClient(base, base.windowW, base.windowH + GROW,
                                        MINW, MINH, cellH, /*cap*/2, /*rowsNeeded*/1);
        check(r.selectedList.h == base.selectedList.h,
              "under-fill (rowsNeeded < visible): content-sized");
    }

    // rowsNeeded takes precedence over the legacy cap==visible heuristic.
    {
        PromptLayout base = TwoCellBase(cellH, gHeaderH);
        PromptLayout rLegacy = ReflowToClient(base, base.windowW, base.windowH + GROW,
                                              MINW, MINH, cellH, /*cap*/2);  // no rowsNeeded
        check(rLegacy.selectedList.h == base.selectedList.h + GROW,
              "legacy path (no rowsNeeded) still grows at cap==visible (unchanged)");

        PromptLayout rNeeded = ReflowToClient(base, base.windowW, base.windowH + GROW,
                                              MINW, MINH, cellH, /*cap*/2, /*rowsNeeded*/2);
        check(rNeeded.selectedList.h == base.selectedList.h,
              "rowsNeeded overrides the legacy cap -> exact fit does not grow");
    }

    if (g_fail) { printf("reflow_exactfit_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("reflow_exactfit_test: OK\n");
    return 0;
}
