// card_padding_test.cpp
// Pins fix thse isseue.md #3 (UI polish pass 6): "the biggest overall spacing
// improvement" -- more horizontal padding INSIDE every card (card outer ->
// content 12 -> 16), while the 12px gap BETWEEN cards is kept. The internal
// description -> field breathing also moves into the 6-8 range (was 4), and the
// title -> description gap stays tight (2-4). Self-contained: own main().
// RED while kCardInset is still kSpace3 (12) / the field offset is 4.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- #3: card outer -> content padding is 16 (was 12) ----
    check(kCardInset == kSpace4, "card inner padding is 16 (fix #3)");
    check(kCardInset % 4 == 0, "card inset stays on the 4px grid");
    // ---- the 12px gap BETWEEN cards is preserved (only internal air grows) ----
    check(kCardGap == kSpace4, "inter-card gap stays 12 (fix #3: do not change the card gap)");

    PromptLayout g = ComputePromptLayout(true);
    const int innerX = kOuterMargin + kCardInset;   // 20 + 16 = 36
    // fix thse isseue.md #3: the Selected card uses its OWN larger content pad
    // (kSelectedPad = 20) for extra breathing room, so its list left edge is
    // kOuterMargin + kSelectedPad, NOT the generic kCardInset used by the other
    // cards.
    check(g.selectedList.x == kOuterMargin + kSelectedPad, "selected list left uses the 20px Selected pad (#3)");
    check(g.destDesc.x == innerX,     "dest content uses the 16px inset");
    check(g.nameDesc.x == innerX,     "name content uses the 16px inset");
    check(g.segCopy.x == innerX,      "action segments use the 16px inset");

    // ---- description -> field breathing is 8 (P2 #13: snapped to the 4px grid,
    //      top of the documented 6-8 range) ----
    check(g.destField.y - g.destDesc.bottom() == 8, "dest description -> field gap is 8 (P2 #13)");
    check(g.nameField.y - g.nameDesc.bottom() == 8, "name description -> field gap is 8 (P2 #13)");

    // ---- title -> description stays tight (2-4) ----
    const int dt = g.destDesc.y - g.destLabel.bottom();
    check(dt >= 2 && dt <= 4, "dest title -> description gap is 2-4");

    if (g_fail) { printf("card_padding_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("card_padding_test: OK\n");
    return 0;
}
