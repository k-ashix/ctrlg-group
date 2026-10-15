// view_switch_repaint_test.cpp
// Pins fix thse isseue.md Phase 4 (eliminate unnecessary view-switch repainting).
// A Ctrl+wheel view-mode switch must NOT reposition every child and issue a
// full-window RDW_ALLCHILDREN erase (which flashes the title bar, destination
// card, folder-name card and footer) when the surrounding SECTIONS do not move.
// layout::ViewSwitchRepaintFor compares the OLD and NEW effective layouts:
//   - every non-Selected section identical  -> SelectedOnly (repaint just the
//     Selected Items viewport, no erase, no child reposition -> no flash),
//   - any section moved / the window resized -> FullRelayout (one atomic pass).
// Self-contained: own main(), exit 0 = pass.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    PromptLayout a = ComputePromptLayout(true, false, 20);

    // Same sections (a maximized/capped switch where selGrow is identical in
    // both modes) -> only the Selected viewport content changed -> SelectedOnly.
    PromptLayout same = a;
    check(ViewSwitchRepaintFor(a, same) == ViewSwitchRepaint::SelectedOnly,
          "identical sections -> repaint Selected Items only (no flash)");

    // A change CONFINED to the Selected card's inner height (lower sections did
    // not move) still only needs a Selected-Items repaint.
    PromptLayout selOnly = a;
    selOnly.selectedList.h += 120;
    check(ViewSwitchRepaintFor(a, selOnly) == ViewSwitchRepaint::SelectedOnly,
          "selected-viewport-only delta -> SelectedOnly");

    // The preferred popup genuinely resizes for the taller mode -> FullRelayout.
    PromptLayout grew = a;
    grew.windowH += 60;
    check(ViewSwitchRepaintFor(a, grew) == ViewSwitchRepaint::FullRelayout,
          "window height changed -> full atomic relayout");

    // Lower sections shifted down (cards restacked) -> FullRelayout.
    PromptLayout shifted = a;
    shifted.destCard.y   += 40;
    shifted.nameCard.y   += 40;
    shifted.actionCard.y += 40;
    check(ViewSwitchRepaintFor(a, shifted) == ViewSwitchRepaint::FullRelayout,
          "destination/name/action cards moved -> full relayout");

    // Footer moved -> FullRelayout (the footer must never be left stranded).
    PromptLayout footer = a;
    footer.createButton.y += 40;
    footer.cancelButton.y += 40;
    check(ViewSwitchRepaintFor(a, footer) == ViewSwitchRepaint::FullRelayout,
          "footer buttons moved -> full relayout");

    // Width change (card widths) -> FullRelayout.
    PromptLayout wider = a;
    wider.windowW += 80;
    wider.destCard.w += 80;
    check(ViewSwitchRepaintFor(a, wider) == ViewSwitchRepaint::FullRelayout,
          "window/section width changed -> full relayout");

    if (g_fail) { printf("view_switch_repaint_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("view_switch_repaint_test: OK\n");
    return 0;
}
