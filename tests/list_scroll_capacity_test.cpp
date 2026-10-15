// list_scroll_capacity_test.cpp
// Pins fix thse isseue.md Phase 1A: the Selected Items list must reconcile its
// DECLARED per-mode row cap with the TRUE number of rows the viewport can
// physically paint, so a declared-but-unfit row is never silently clipped and
// the scrollbar appears (and scrolling is enabled) when fewer rows physically
// fit than the mode cap. Self-contained: own main(), exit 0 = pass.

#include "list_scroll_logic.h"

#include <cstdio>

using namespace listscroll;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const int rowH = 24;

    // Declared 9 rows, but the viewport only fits 8 -> effective capacity is 8.
    check(EffectiveVisibleRows(9, 8 * rowH, rowH) == 8,
          "9 declared, 8 fit -> 8 effective");
    // Viewport fits all declared rows -> capacity stays at the declared cap.
    check(EffectiveVisibleRows(9, 9 * rowH, rowH) == 9,
          "9 declared, 9 fit -> 9 effective");
    // Viewport fits MORE than declared -> never exceed the declared cap.
    check(EffectiveVisibleRows(9, 20 * rowH, rowH) == 9,
          "declared cap is the ceiling even when more rows fit");
    // Unmeasurable viewport (0 height or 0 rowH) -> fall back to the declared
    // cap so the list never collapses to zero rows.
    check(EffectiveVisibleRows(9, 0, rowH) == 9, "0 height -> declared fallback");
    check(EffectiveVisibleRows(9, 8 * rowH, 0) == 9, "0 rowH -> declared fallback");

    // The regression this phase fixes: 9 items, cap 9, but only 8 physically
    // fit. Old code used the declared cap -> NeedsScrollbar(9,9)=false -> the
    // 9th row was clipped and unreachable. With the effective capacity it is 8,
    // so a scrollbar appears and the final row is reachable by scrolling.
    const int eff = EffectiveVisibleRows(9, 8 * rowH, rowH);
    check(NeedsScrollbar(9, eff), "fewer rows fit than items -> scrollbar shown");
    const int bottom = OnKey(Key::End, 0, 9, eff);
    const Range r = VisibleRange(bottom, 9, eff);
    check(r.first + r.count == 9, "End scroll reaches the last item");

    if (g_fail) { printf("list_scroll_capacity_test: %d FAILED\n", g_fail); return 1; }
    printf("list_scroll_capacity_test: OK\n");
    return 0;
}
