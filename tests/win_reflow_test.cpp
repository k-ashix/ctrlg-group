// win_reflow_test.cpp
// Pins fix thse isseue.md UI-01 / UI-05 pure logic: when the Group popup is
// resized or maximized, the layout REFLOWS into the ACTUAL client rect instead
// of staying at the fixed compact/expanded size in the upper-left corner.
// layout::ReflowToClient() stretches the content to the client WIDTH, grows the
// Selected Items card by the extra HEIGHT and shifts every lower section down so
// the footer tracks the client bottom and stays visible. New Folder is never
// reflowed. Self-contained: own main(), exit 0 = pass.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const int MINW = 360, MINH = 320;

    // ---- New Folder is NEVER reflowed (its bare geometry is preserved) ----
    PromptLayout nf  = ComputePromptLayout(false);
    PromptLayout nfR = ReflowToClient(nf, 1200, 900, MINW, MINH);
    check(nfR.windowW == nf.windowW && nfR.windowH == nf.windowH,
          "New Folder popup is not reflowed");

    // ---- Group: a MAXIMIZE-sized client (much larger than preferred) ----
    PromptLayout g = ComputePromptLayout(true, false, 20);
    const int CW = g.windowW + 400;   // wider
    const int CH = g.windowH + 300;   // taller
    PromptLayout r = ReflowToClient(g, CW, CH, MINW, MINH);

    check(r.windowW == CW && r.windowH == CH,
          "reflow adopts the live client width and height");

    // content fills the extra WIDTH (no upper-left pinning)
    check(r.destField.w == g.destField.w + 400,
          "destination field widens to the client width");
    check(r.selectedContent.w == g.selectedContent.w + 400,
          "selected card widens to the client width");

    // right-anchored controls stay flush with the new right edge, in bounds
    check(r.cancelButton.x == g.cancelButton.x + 400,
          "Cancel tracks the right edge");
    check(r.cancelButton.right() <= r.windowW,
          "Cancel stays inside the window");
    check(r.chromeCloseBtn.right() <= r.windowW,
          "title-bar Close stays inside the window");

    // Selected Items card absorbs the extra HEIGHT; the footer moves DOWN with it
    check(r.selectedContent.h == g.selectedContent.h + 300,
          "Selected card grows by the extra height");
    check(r.createButton.y == g.createButton.y + 300,
          "Create button tracks the client bottom");
    check(r.createButton.y > g.createButton.y,
          "footer is not left stranded at the old top");

    // UI-05 acceptance: the footer action row stays VISIBLE within the client
    check(r.createButton.bottom() <= r.windowH,
          "Create stays on-screen (footer visible)");
    check(r.cancelButton.bottom() <= r.windowH,
          "Cancel stays on-screen (footer visible)");

    // lower sections shift down together (no overlap with the grown card)
    check(r.destCard.y == g.destCard.y + 300,
          "Destination card follows the taller list");
    check(r.nameField.y == g.nameField.y + 300,
          "Folder-name field follows the taller list");
    check(r.actionCard.y == g.actionCard.y + 300,
          "Action card follows the taller list");

    // ---- Copy/Move segmented control stays LEFT-anchored and fills the card ----
    // Regression: the segments used to be treated as "right-anchored" and were
    // shifted fully right by the extra width, detaching from the card's left
    // inset and leaving a gap. They must stay flush left and WIDEN as one pill.
    check(r.segCopy.x == g.segCopy.x,
          "Copy segment stays flush with the card's left inset (not floated right)");
    check(r.segMove.right() == g.segMove.right() + 400,
          "Move segment's right edge tracks the widened card (full-width pair)");
    check(r.segCopy.right() == r.segMove.x,
          "Copy/Move segments still meet edge-to-edge (one seamless pill)");
    check(r.segCopy.y == g.segCopy.y + 300,
          "Copy/Move segments follow the taller list downward");

    // ---- a too-small client is clamped to the minimum (controls never crushed) ----
    PromptLayout tiny = ReflowToClient(g, 10, 10, MINW, MINH);
    check(tiny.windowW >= MINW && tiny.windowH >= MINH,
          "tiny client clamps to the minimum size");

    // ---- fix thse isseue.md v0.12.65 Bug 1: after reflow the CARD rects still
    // ENCLOSE their child content with the SAME insets as the preferred layout.
    // WM_ERASEBKGND now paints the cards from this exact reflowed layout, so a
    // widened client can never leave a control sticking out past its card border
    // (the "long horizontal bars outside the card borders" in the screenshots). ----
    check(r.selectedContent.x - r.selectedCard.x
              == g.selectedContent.x - g.selectedCard.x,
          "Selected card keeps its left inset around the content after reflow");
    check(r.selectedCard.right() - r.selectedContent.right()
              == g.selectedCard.right() - g.selectedContent.right(),
          "Selected card keeps its right inset around the content after reflow");
    check(r.destCard.x <= r.destField.x && r.destCopyBtn.right() <= r.destCard.right(),
          "Destination field + Copy button stay inside the Destination card");
    check(r.nameCard.x <= r.nameField.x && r.nameField.right() <= r.nameCard.right(),
          "Folder-name field stays inside the Folder-name card");
    check(r.actionCard.x <= r.segCopy.x && r.segMove.right() <= r.actionCard.right(),
          "Copy/Move segments stay inside the Action card");

    // ---- fix thse isseue.md Issue 4: a maximized / zoomed popup must NOT pour
    // all the extra height into the Selected Items card when the selection is
    // SMALL. A selection that already shows every row in the preferred layout
    // (uncapped) keeps its content height, so there is no blank band below the
    // last row and the lower sections keep their natural spacing. A ROW-CAPPED
    // selection (more items than the viewport) still absorbs the height and
    // scrolls (issue 9/10). ----
    {
        // compact caps at kListCompactRows (4); 2 items => uncapped (all shown).
        PromptLayout gs = ComputePromptLayout(true, false, 2);
        PromptLayout rs = ReflowToClient(gs, gs.windowW + 400, gs.windowH + 300,
                                         MINW, MINH);
        check(rs.windowW == gs.windowW + 400 && rs.windowH == gs.windowH + 300,
              "small-selection reflow still adopts the live client size");
        check(rs.selectedContent.w == gs.selectedContent.w + 400,
              "small-selection card still widens to the client width");
        check(rs.selectedContent.h == gs.selectedContent.h,
              "small selection: Selected content does NOT stretch (no dead space)");
        check(rs.selectedCard.h == gs.selectedCard.h,
              "small selection: Selected CARD height is content-sized (unchanged)");
        check(rs.selectedList.h == gs.selectedList.h,
              "small selection: list viewport stays content-sized");
        check(rs.destCard.y == gs.destCard.y,
              "small selection: Destination keeps its natural position (no gap)");
        check(rs.createButton.y == gs.createButton.y,
              "small selection: footer keeps its natural position (not stretched)");
        check(rs.createButton.bottom() <= rs.windowH,
              "small selection: footer still fits inside the client");
        check(rs.selectedContent.x - rs.selectedCard.x
                  == gs.selectedContent.x - gs.selectedCard.x,
              "small selection: card keeps its content inset after reflow");

        // a ROW-CAPPED selection still grows + scrolls (regression guard, issue 10)
        PromptLayout gc = ComputePromptLayout(true, false, 40);
        PromptLayout rc = ReflowToClient(gc, gc.windowW, gc.windowH + 300,
                                         MINW, MINH);
        check(rc.selectedContent.h == gc.selectedContent.h + 300,
              "capped selection still absorbs the extra height (scrolls to reach all)");
        check(rc.createButton.y == gc.createButton.y + 300,
              "capped selection: footer still tracks the client bottom");
    }

    if (g_fail == 0) printf("win_reflow_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
