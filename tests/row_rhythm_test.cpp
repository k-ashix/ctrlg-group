// row_rhythm_test.cpp
// Pins fix thse isseue.md #1 (UI polish pass 6): the Selected Items list rows
// get more vertical air -- the row height moves 20 -> 24 while the file icon
// stays 16 and the icon -> text gap becomes 8. The list viewport height tracks
// the new row height, so a row reads with real top/bottom breathing instead of
// a cramped 20px stripe. Self-contained: own main(), exit 0 = pass. RED while
// kSelRowH is still 20 / kRowIconGap is still 6; GREEN once the tokens move.

#include "layout_logic.h"
#include "icon_logic.h"

#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    using namespace layout;

    // ---- #1: taller row for vertical breathing (was 20) ----
    check(kSelRowH == 24, "selected row height is 24 (fix #1)");
    check(kSelRowH % 4 == 0, "row height stays on the 4px grid");

    // ---- file icon stays 16; icon -> text gap is 8 (#1) ----
    check(icons::kRowIconW == 16, "row icon is 16 (fix #1)");
    check(icons::kRowIconGap == 8, "icon -> text gap is 8 (fix #1)");

    // ---- the list viewport tracks the new row height ----
    PromptLayout g4 = ComputePromptLayout(true, false, 4);
    check(g4.selectedList.h == 4 * kSelRowH,
          "4-row compact list uses the new row height (content-sized)");

    // ---- a row is taller than its 16px icon, so there is real top/bottom air ----
    check(kSelRowH > icons::kRowIconW, "row is taller than the icon (vertical air)");

    if (g_fail) { printf("row_rhythm_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("row_rhythm_test: OK\n");
    return 0;
}
