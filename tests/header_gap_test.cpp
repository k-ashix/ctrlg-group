// header_gap_test.cpp
//
// fix thse isseue.md #2: there must be a DELIBERATE gap between the custom
// title band (TitleBandH) and the first (Selected Items) card. The layout model
// introduces a separate design-space constant kHeaderToSelectedGap (16px @96dpi,
// fix thse isseue.md #3: raised from 12 for more separation from the title bar)
// and sets cardTop = TitleBandH() + kHeaderToSelectedGap, so the first card no
// longer butts straight up against the header band.
//
// Mirrors layout_logic.h (which the mod copies byte-for-byte). Self-contained:
// own main(), exit 0 == pass. RED before the gap constant exists; GREEN once the
// layout model adds it.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // The gap is its OWN design-space constant, 16px at 96 dpi (fix thse
    // isseue.md #3: raised from 12).
    check(kHeaderToSelectedGap == 16, "header-to-selected gap is 16px @96dpi");

    // Group: the Selected Items card starts a full gap BELOW the title band.
    PromptLayout g = ComputePromptLayout(true);
    check(g.selectedCard.y == TitleBandH() + kHeaderToSelectedGap,
          "Selected card top = TitleBandH() + kHeaderToSelectedGap");
    check(g.selectedCard.y > TitleBandH(),
          "there is a real gap between the title band and the first card");

    // The title band itself is UNCHANGED (the gap must not grow the frame).
    check(TitleBandH() >= 32 && TitleBandH() <= 36,
          "title band height is unchanged (32-36px)");

    // Expanded keeps the exact same gap (band -> first card is mode-independent).
    PromptLayout gx = ComputePromptLayout(true, true);
    check(gx.selectedCard.y == TitleBandH() + kHeaderToSelectedGap,
          "expanded: Selected card still a gap below the title band");

    // New Folder has no title band / cards, so it is unaffected.
    PromptLayout nf = ComputePromptLayout(false);
    check(nf.selectedCard.empty(), "New Folder has no Selected card");

    if (g_fail) { printf("header_gap_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("header_gap_test: OK\n");
    return 0;
}
