// adaptive_list_test.cpp
// Pins fix thse isseue.md "Production UI pass" item #1 (P0): the Selected Items
// list is CONTENT-SIZED, not a fixed viewport. The popup must size the list to
// min(itemCount, rowsForMode) rows so a 5-item selection no longer reserves a
// 9-row viewport (the big empty area in the expanded screenshot).
//
//     compactVisibleRows  = min(itemCount, 4)
//     expandedVisibleRows = min(itemCount, 9)
//     selectedList.h      = visibleRows * rowHeight + listPadding
//
// Self-contained: own main(), exit 0 = pass. RED before mode::VisibleRowsFor /
// layout::kSelRowH exist; GREEN once the header mirrors the mod.

#include "layout_logic.h"
#include "mode_logic.h"

#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- item count drives the visible row count (capped per mode) ----
    check(mode::VisibleRowsFor(0, mode::Mode::Compact) == 0, "0 items -> 0 rows");
    check(mode::VisibleRowsFor(3, mode::Mode::Compact) == 3, "3 items compact -> 3 rows");
    check(mode::VisibleRowsFor(4, mode::Mode::Compact) == 4, "4 items compact -> 4 rows");
    check(mode::VisibleRowsFor(10, mode::Mode::Compact) == 4, "compact caps at 4 rows");
    check(mode::VisibleRowsFor(5, mode::Mode::Expanded) == 5, "5 items expanded -> 5 rows");
    check(mode::VisibleRowsFor(20, mode::Mode::Expanded) == 9, "expanded caps at 9 rows");
    check(mode::VisibleRowsFor(9, mode::Mode::Expanded) == 9, "9 items expanded -> 9 rows");

    // ---- layout list height is content-sized ----
    using namespace layout;
    check(kListCompactRows == 4 && kListExpandedRows == 9,
          "layout row caps mirror the mode model");
    check(kSelRowH > 0 && kSelListPadV >= 0, "row height + padding are sane");

    PromptLayout g2 = ComputePromptLayout(true, false, 2);
    PromptLayout g4 = ComputePromptLayout(true, false, 4);
    check(g2.selectedList.h < g4.selectedList.h, "fewer items -> shorter list");
    check(g4.selectedList.h == 4 * kSelRowH,
          "4-item compact list == 4 rows (content-sized; 16px card inset)");

    // The #1 win: a small selection in EXPANDED mode must NOT reserve 9 rows.
    PromptLayout e5  = ComputePromptLayout(true, true, 5);
    PromptLayout e20 = ComputePromptLayout(true, true, 20);
    check(e5.selectedList.h < e20.selectedList.h, "5 items expanded shorter than a full list");
    check(e20.selectedList.h == kListExpandedRows * kSelRowH,
          "full expanded list == 9 rows (content-sized; 16px card inset)");

    // A small selection sizes to its CONTENT, so expanded and compact agree on
    // the list height (expanded still differs in WIDTH, tested elsewhere).
    PromptLayout c3 = ComputePromptLayout(true, false, 3);
    PromptLayout x3 = ComputePromptLayout(true, true, 3);
    check(c3.selectedList.h == x3.selectedList.h,
          "#1 small selection does not balloon in expanded");
    check(c3.windowH == x3.windowH,
          "#1 small selection window height is unchanged by mode");

    // The window grows with the selection (until the per-mode cap).
    check(ComputePromptLayout(true, false, 4).windowH >
          ComputePromptLayout(true, false, 1).windowH,
          "more items -> taller window");

    // Defensive: never a zero-height list even for a degenerate 0 count.
    check(ComputePromptLayout(true, false, 0).selectedList.h >= kSelRowH,
          "list is at least one row tall");

    if (g_fail) { printf("adaptive_list_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("adaptive_list_test: OK\n");
    return 0;
}
