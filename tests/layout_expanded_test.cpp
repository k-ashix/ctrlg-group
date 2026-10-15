// layout_expanded_test.cpp
// Pins the EXPANDED popup geometry added in Phase 7 (waht to create.md #15).
// Expanded gives the Group popup's Selected Items list more vertical room; the
// form below it (name label/field + action row) shifts down by the same
// amount, and the window grows by that amount. Compact geometry is UNCHANGED
// (still pinned by layout_spacing_test.cpp) and New Folder -- which has no
// list -- is unaffected by the expanded flag.
//
// Self-contained: own main(), exit 0 = pass. RED before layout_logic.h grows
// the expanded parameter; GREEN once it mirrors the mod.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- default arg == compact ----
    PromptLayout gCompactDefault = ComputePromptLayout(true);
    PromptLayout gCompact = ComputePromptLayout(true, false);
    check(gCompactDefault.windowH == gCompact.windowH, "default arg is compact");
    check(!gCompact.expanded, "compact flag is false");
    check(gCompact.windowH == 667, "compact group height is 667 (fix thse isseue.md A: 32px title band, -4 from the 36px band; Selected card breathing room)");

    // ---- expanded group grows by the extra list rows (content-sized, #1) ----
    // With a FULL default selection, expanded shows ExpandedRows and compact
    // shows CompactRows, so the window grows by exactly that many extra rows.
    PromptLayout gExp = ComputePromptLayout(true, true);
    check(gExp.expanded, "expanded flag is true");
    const int extra = (kListExpandedRows - kListCompactRows) * kSelRowH;
    check(extra > 0, "expanded extra is positive");
    check(gExp.windowH == gCompact.windowH + extra, "expanded group height grows by extra");
    check(gExp.groupHeader.h == gCompact.groupHeader.h + extra,
          "expanded list band grows by extra");

    // ---- the form shifts down by exactly the extra, order preserved ----
    check(gExp.nameLabel.y == gCompact.nameLabel.y + extra, "name label shifts down");
    check(gExp.nameField.y == gCompact.nameField.y + extra, "name field shifts down");
    check(gExp.createButton.y == gCompact.createButton.y + extra, "buttons shift down");
    check(gExp.groupHeader.bottom() <= gExp.nameLabel.y, "header above name label");
    check(gExp.nameLabel.bottom() <= gExp.nameField.y, "label above field");
    check(gExp.nameField.bottom() <= gExp.createButton.y, "field above action row");
    // fix thse isseu e.md #6: segments live in the Action card; Create/Cancel are
    // on a bottom row below it. Both shift down by extra together.
    check(gExp.opToggle.y == gCompact.opToggle.y + extra, "segments shift down with the card");
    check(gExp.actionCard.y == gCompact.actionCard.y + extra, "Action card shifts down");
    check(gExp.opToggle.y < gExp.createButton.y, "segments still above the bottom row");
    // fix thse isseue.md #9/#10: expanded mode now changes WIDTH too (not just
    // vertical room). Compact stays 344; expanded is a WIDER 560 and every
    // content width (fields, list, segments) grows with it (#11/#12). Left
    // margins are fixed, so x is unchanged while w grows.
    check(gCompact.windowW == 410, "compact group width is 410 (fix #7)");
    check(gExp.windowW == 660, "expanded group width is 660 (fix #8)");
    check(gExp.windowW > gCompact.windowW, "expanded is WIDER than compact (fix #9/#10)");
    check(gExp.nameField.x == gCompact.nameField.x, "field left edge unchanged (fixed margins)");
    check(gExp.nameField.w > gCompact.nameField.w, "name field widens when expanded (#11/#12)");
    check(gExp.destField.w > gCompact.destField.w, "destination field widens when expanded (#12)");
    check(gExp.selectedList.w > gCompact.selectedList.w, "Selected Items list widens when expanded (#12)");
    check(gExp.segMove.right() > gCompact.segMove.right(), "segments grow horizontally when expanded (#12)");
    check(gExp.cancelButton.right() == gExp.windowW - kOuterMargin,
          "Cancel stays right-aligned at the expanded width");

    // ---- New Folder has no list, so expanded == compact ----
    PromptLayout nfCompact = ComputePromptLayout(false, false);
    PromptLayout nfExp = ComputePromptLayout(false, true);
    check(nfCompact.windowH == 178 && nfExp.windowH == 178,
          "New Folder height unaffected by expanded");
    check(nfExp.nameField.y == nfCompact.nameField.y,
          "New Folder field position unaffected by expanded");

    if (g_fail) { printf("layout_expanded_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("layout_expanded_test: OK\n");
    return 0;
}
