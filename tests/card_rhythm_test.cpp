// card_rhythm_test.cpp
//
// Pins fix thse isseue.md #13 (P2 "final internal spacing") / re-review item 8:
// the two "title -> description -> field" cards (Destination and Folder name)
// use ONE grid-snapped internal vertical rhythm instead of the old ad-hoc
// offsets (title->desc was 2, desc->field was 6, field->bottom was 18/16).
//
// Target rhythm (all on the 4px grid, per re-review item 8):
//     title       -> description   4px
//     description -> field         8px
//     field       -> card bottom   12px
//
// NOTE on "icon -> title": that gap is kSectionIconCol(24) - kTileSize(18) = 6.
// It is intentionally NOT forced to 8 here: grid_spacing_test pins
// kSectionIconCol on the 4px grid (26 would be off-grid) and the 18px tile is
// an intrinsic size the grid rhythm explicitly excludes. 6px (on-grid column,
// intrinsic tile) is the honest compromise; see codeoverview.md Pass7 S7.
//
// Self-contained: own main(), exit 0 = pass. RED before layout_logic.h snaps
// the rhythm.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}
static bool g4(int v) { return v % 4 == 0; }

int main() {
    const PromptLayout g = ComputePromptLayout(true);

    // ---- Destination card: title -> description -> read-only field ----
    const int destTitleToDesc = g.destDesc.y   - g.destLabel.bottom();
    const int destDescToField = g.destField.y  - g.destDesc.bottom();
    const int destFieldToBot  = g.destCard.bottom() - g.destField.bottom();
    check(destTitleToDesc == 4,  "dest: title -> description is 4px");
    check(destDescToField == 8,  "dest: description -> field is 8px");
    check(destFieldToBot  >= 12, "dest: field -> card bottom is >= 12px (fix thse isseue.md #2)");
    check(g4(destTitleToDesc) && g4(destDescToField),
          "dest: internal gaps are on the 4px grid");
    check(g.destField.bottom() <= g.destCard.bottom(),
          "dest: field stays inside the card");

    // ---- Folder name card: same title -> description -> edit stack ----
    const int nameTitleToDesc = g.nameDesc.y   - g.nameLabel.bottom();
    const int nameDescToField = g.nameField.y  - g.nameDesc.bottom();
    const int nameFieldToBot  = g.nameCard.bottom() - g.nameField.bottom();
    check(nameTitleToDesc == 4,  "name: title -> description is 4px");
    check(nameDescToField == 8,  "name: description -> field is 8px");
    check(nameFieldToBot  >= 12, "name: field -> card bottom is >= 12px (fix thse isseue.md #2)");
    check(g4(nameTitleToDesc) && g4(nameDescToField),
          "name: internal gaps are on the 4px grid");
    check(g.nameField.bottom() <= g.nameCard.bottom(),
          "name: field stays inside the card");

    // ---- the card GAP itself is unchanged (item 8: do not grow the 12px) ----
    check(g.nameCard.y - g.destCard.bottom() == kCardGap,
          "dest -> name card gap stays kCardGap (12)");

    if (g_fail == 0) printf("card_rhythm_test: OK\n");
    else             printf("card_rhythm_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
