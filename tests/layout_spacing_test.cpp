// layout_spacing_test.cpp
// Pins the popup spacing system + geometry (ComputePromptLayout / ScaleForDpi,
// mirrored from the mod into layout_logic.h). This is waht to create.md #9:
// a single 4px scale and one layout function instead of scattered absolute
// coordinates. The geometry values are pinned to the CURRENT build so the
// refactor is provably faithful (New Folder + Group look unchanged), and the
// DPI scaling is pinned to MulDiv rounding so 96/125/150/175/200% stay correct.
//
// Self-contained: own main(), exit 0 = pass. RED before layout_logic.h is
// implemented; GREEN once it mirrors the mod.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- spacing scale is a real 4px system ----
    check(kUnit == 4, "base unit is 4px");
    check(kSpace2 == 8 && kSpace3 == 12 && kSpace4 == 16 && kSpace5 == 20 &&
          kSpace6 == 24, "scale tokens have the expected values");
    check(kSpace2 % kUnit == 0 && kSpace3 % kUnit == 0 && kSpace4 % kUnit == 0 &&
          kSpace5 % kUnit == 0 && kSpace6 % kUnit == 0,
          "every scale token is a multiple of the base unit");

    // ---- New Folder (non-group) ----
    PromptLayout nf = ComputePromptLayout(false);
    check(!nf.isGroup, "nf is not a group layout");
    check(nf.windowW == 344, "nf window width is 344");
    check(nf.windowH == 178, "nf window height is 178");
    check(nf.groupHeader.empty(), "nf has no group header");
    check(nf.opToggle.empty(), "nf has no Move/Copy toggle");
    // vertical order with no overlap
    check(nf.nameLabel.bottom() <= nf.nameField.y, "nf label sits above field");
    check(nf.nameField.bottom() <= nf.createButton.y, "nf field sits above buttons");
    // action row
    check(nf.createButton.y == nf.cancelButton.y, "nf Create/Cancel share a row");
    check(nf.createButton.right() <= nf.cancelButton.x, "nf Create is left of Cancel");
    check(nf.cancelButton.right() == nf.windowW - 20, "nf Cancel is right-aligned to the margin");
    // faithful to the current build
    check(nf.nameLabel.x == 20 && nf.nameLabel.y == 18 && nf.nameLabel.w == 304 &&
          nf.nameLabel.h == 22, "nf label coords match current build");
    check(nf.nameField.x == 20 && nf.nameField.y == 46 && nf.nameField.w == 304 &&
          nf.nameField.h == 30, "nf field coords match current build");
    check(nf.createButton.x == 168 && nf.createButton.y == 92 &&
          nf.createButton.w == 74 && nf.createButton.h == 30, "nf Create coords match current build");
    check(nf.cancelButton.x == 250 && nf.cancelButton.y == 92, "nf Cancel coords match current build");

    // ---- Group ----
    PromptLayout g = ComputePromptLayout(true);
    check(g.isGroup, "g is a group layout");
    check(g.windowW == 410, "g window width is 410 (fix #7 compact ~400-420)");
    check(g.windowH == 667, "g window height is 671 (fix thse isseue.md #3: Selected card breathing room +15, was 656)");
    check(!g.groupHeader.empty(), "g has a group header block");
    check(g.groupHeader.x == 20 && g.groupHeader.y == 48 && g.groupHeader.w == 370 &&
          g.groupHeader.h == 189, "g header (Selected Items card) coords match build (24px rows + 20px inset, fix thse isseue.md #3)");
    check(!g.opToggle.empty(), "g has a Move/Copy toggle");
    // sections stack top -> bottom, no overlap
    check(g.groupHeader.bottom() <= g.nameLabel.y, "g header sits above the name label");
    check(g.nameLabel.bottom() <= g.nameField.y, "g label sits above field");
    check(g.nameField.bottom() <= g.opToggle.y, "g field sits above the action segments");
    // fix thse isseu e.md #6: the Copy/Move segments now live INSIDE the Action
    // card; Create/Cancel are on their OWN bottom action row BELOW that card.
    check(!g.actionCard.empty(), "g Action card exists");
    check(g.opToggle.y < g.createButton.y,
          "g segments sit ABOVE the Create/Cancel bottom row");
    check(g.createButton.y == g.cancelButton.y, "g Create/Cancel share the bottom row");
    check(g.actionCard.bottom() <= g.createButton.y, "g Action card sits above the bottom row");
    // faithful to the sectioned card stack (Phase 3a + description rows, fix #1)
    // Each info card now stacks title -> description -> field, so the field sits
    // BELOW a dedicated description row instead of the description being painted
    // under the field. Cards grew (dest 64->96, name 72->100) and everything
    // below shifts down.
    check(!g.destDesc.empty(), "g Destination description row exists");
    check(!g.nameDesc.empty(), "g Folder name description row exists");
    check(g.destLabel.bottom() <= g.destDesc.y, "g dest description sits below the label");
    check(g.destDesc.bottom() <= g.destField.y, "g dest description sits ABOVE the field");
    check(g.nameLabel.bottom() <= g.nameDesc.y, "g name description sits below the label");
    check(g.nameDesc.bottom() <= g.nameField.y, "g name description sits ABOVE the field");
    // Coordinates shifted down by kChromeBand (22px) vs the pre-#5 build: the
    // group card stack now drops below a window-chrome strip that holds the
    // compact/expand size control (fix thse isseu e.md #5).
    // fix thse isseue.md #3: every coordinate below the Selected card shifts
    // DOWN by +15 (cardTop +4 from kHeaderToSelectedGap 12->16, selCardH +11
    // from kSelHdrGap 3->4 and kSelectedPad 16->20).
    check(g.destLabel.y == 265, "g dest label y (fix thse isseue.md #3: +15 Selected breathing cascade)");
    check(g.destDesc.y == 287, "g dest description y (title + 4px, P2 #13)");
    check(g.destField.y == 311, "g dest field y (8px below description, P2 #13)");
    check(g.nameLabel.y == 381, "g name label y (inside the Folder name card)");
    check(g.nameDesc.y == 403, "g name description y");
    check(g.nameField.y == 427, "g name field y (8px below the description row, P2 #13)");
    // Action card holds the segments; Create/Cancel are on the bottom row.
    check(g.actionCard.x == 20 && g.actionCard.y == 493 && g.actionCard.w == 370 &&
          g.actionCard.h == 116, "g Action card coords match build (+15 Selected breathing cascade, fix thse isseue.md #3)");
    check(!g.actionTitle.empty() && !g.actionDesc.empty() && !g.keyboardHint.empty(),
          "g Action card has title + description + keyboard hint rows");
    check(g.opToggle.x == 36 && g.opToggle.y == 551 && g.opToggle.w == 74 &&
          g.opToggle.h == 30, "g segments sit inside the Action card at y=555 (+15, fix thse isseue.md #3)");
    check(g.segCopy.y == 551 && g.segMove.y == 551, "g Copy/Move segments at y=555");
    check(g.createButton.x == 234 && g.createButton.y == 621, "g Create is on the bottom row (+15, fix thse isseue.md #3)");
    check(g.cancelButton.x == 316 && g.cancelButton.y == 621, "g Cancel is on the bottom row (+15, fix thse isseue.md #3)");
    // size control in the chrome strip ABOVE the first card (fix #5).
    check(!g.chromeSizeBtn.empty(), "g has a chrome size button");
    check(g.chromeSizeBtn.bottom() <= g.groupHeader.y,
          "g size button sits above the first card (in the chrome)");

    // ---- DPI scaling is linear and matches MulDiv rounding ----
    check(ScaleForDpi(100, 96) == 100, "scale @96 is identity");
    check(ScaleForDpi(344, 96) == 344, "scale window @96 is identity");
    check(ScaleForDpi(100, 192) == 200, "scale @192 (200%) doubles");
    check(ScaleForDpi(100, 144) == 150, "scale @144 (150%) is 1.5x");
    check(ScaleForDpi(100, 120) == 125, "scale @120 (125%) is 1.25x");
    check(ScaleForDpi(100, 168) == 175, "scale @168 (175%) is 1.75x");
    // monotonic non-decreasing across DPI steps
    int prev = 0;
    const int dpis[5] = {96, 120, 144, 168, 192};
    for (int i = 0; i < 5; ++i) {
        int s = ScaleForDpi(nf.windowW, dpis[i]);
        check(s >= prev, "window width scales monotonically across DPI");
        prev = s;
    }
    // ScaleRect scales every edge
    Rect r = ScaleRect(nf.nameField, 192);
    check(r.x == ScaleForDpi(20, 192) && r.y == ScaleForDpi(46, 192) &&
          r.w == ScaleForDpi(304, 192) && r.h == ScaleForDpi(30, 192),
          "ScaleRect scales x/y/w/h");

    if (g_fail) { printf("layout_spacing_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("layout_spacing_test: OK\n");
    return 0;
}
