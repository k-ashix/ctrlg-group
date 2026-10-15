// card_spacing_test.cpp
// Pins the FINAL card-spacing polish (fix thse isseue.md #7, "exact order" item
// 7): the gap BETWEEN stacked Group cards is aligned to the 4px spacing scale
// (kSpace3 = 12) and is UNIFORM across the whole stack, so the card stack
// breathes consistently instead of using an ad-hoc 10px gap. The geometry is
// the same tested layout model the controls are placed from, so the polish
// cannot drift away from what is painted.
//
// Self-contained: own main(), exit 0 = pass. RED while kCardGap is still 10;
// GREEN once it is kSpace3.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- the gap is a 4px-scale token, not an ad-hoc value ----
    check(kCardGap == kSpace4, "inter-card gap is the 4px scale token kSpace3 (12)");
    check(kCardGap % kUnit == 0, "inter-card gap is a multiple of the 4px base unit");
    check(kCardGap >= kSpace2, "inter-card gap is at least kSpace2");

    // ---- every inter-card gap in the compact Group stack is the SAME ----
    PromptLayout g = ComputePromptLayout(true);
    check(g.destCard.y   - g.selectedCard.bottom() == kCardGap,
          "Selected -> Destination gap == kCardGap");
    check(g.nameCard.y   - g.destCard.bottom()     == kCardGap,
          "Destination -> Name gap == kCardGap");
    check(g.actionCard.y - g.nameCard.bottom()     == kCardGap,
          "Name -> Action gap == kCardGap (Status card removed, #1)");

    // ---- expanded keeps the SAME uniform gaps (only the list grows) ----
    PromptLayout e = ComputePromptLayout(true, true);
    check(e.nameCard.y   - e.destCard.bottom()     == kCardGap,
          "expanded Destination -> Name gap == kCardGap");
    check(e.actionCard.y - e.nameCard.bottom()     == kCardGap,
          "expanded Name -> Action gap == kCardGap (Status card removed, #1)");

    if (g_fail) { printf("card_spacing_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("card_spacing_test: OK\n");
    return 0;
}
