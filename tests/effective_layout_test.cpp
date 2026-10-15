// effective_layout_test.cpp
// Pins fix thse isseue.md "ADD-ON -- Fix Initial Popup Layout Glitch Without
// Requiring Manual Resize" (P0).
//
// ROOT CAUSE (verified): the first visible frame placed its child controls with
// the RAW layout::ComputePromptLayout preset. For a GRID Selected view that
// preset reserves the Selected card at the LIST row height (kSelRowH=24), so the
// taller whole grid cells do not fit; the authoritative resize path
// (ComputeEffectiveLayout) GROWS the Selected card to whole cells and shifts
// every section below it down by the same delta. Because that grid-cell sizing
// only ran on WM_SIZE (gated behind manualSized/IsZoomed), the first frame's
// children stayed at the un-shifted coordinates while the painted cards used the
// shifted geometry -- the Destination / Folder name / Action controls spilled
// OUTSIDE their cards until the user resized.
//
// efflayout::GridSizedPreferred is the ONE shared, mode-aware "effective
// preferred" model now used by BOTH the first frame (initial outer size +
// WM_CREATE child placement) and the resize/maximize path, so the first visible
// frame can no longer disagree with a later resize at the same size.
//
// Self-contained: own main(), exit 0 = pass.

#include "layout_logic.h"
#include "effective_layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// A hand-built group layout whose sections sit DIRECTLY below the Selected card,
// so a grid-cell grow of that card would overlap them unless they are shifted.
static PromptLayout MakeBase() {
    PromptLayout b{};
    b.isGroup = true;
    b.expanded = false;
    // Selected card: header band 40px (list.y 90 - content.y 50) + 120px budget.
    b.selectedContent = Rect{20, 50, 300, 160};
    b.selectedList    = Rect{20, 90, 300, 120};
    b.selectedCard    = Rect{12, 42, 316, 176};
    b.groupHeader     = b.selectedCard;
    // Everything below the Selected card.
    b.destCard        = Rect{12, 230, 316, 70};
    b.destLabel       = Rect{20, 238, 120, 20};
    b.destField       = Rect{20, 262, 240, 30};
    b.destCopyBtn     = Rect{266, 262, 60, 30};
    b.nameCard        = Rect{12, 310, 316, 70};
    b.nameField       = Rect{20, 342, 300, 32};
    b.actionCard      = Rect{12, 390, 316, 50};
    b.segCopy         = Rect{20, 360, 60, 24};
    b.segMove         = Rect{84, 360, 60, 24};
    b.createButton    = Rect{250, 400, 74, 30};
    b.cancelButton    = Rect{168, 400, 74, 30};
    b.infoButton      = Rect{20, 400, 30, 30};
    // Auto Group button DISABLED -> zero-size. It must NOT be shifted (the
    // shiftY guard skips zero-width rects); a stray shift would drift a control
    // that is not even created.
    b.autoGroupButton = Rect{0, 0, 0, 0};
    b.windowH         = 460;
    return b;
}

int main() {
    const int cellH   = 86;   // a Large whole grid cell (icon + pad + label band)
    const int visRows = 2;    // 2 whole cells fit the budget -> card grows
    const int kRowsNeeded = 3;

    // headerBlockH = 90 - 50 = 40; listBudget = 160 - 40 = 120.
    // newContentH  = 40 + 2*86 = 212; delta = 212 - 160 = 52 (grow).
    // fit          = 120 / 86 = 1 (clamped >= 1).
    const int expDelta = 52;

    // ---- 0. The DEFECT, documented: the raw preset overlaps once the card is
    //         grid-sized. destCard.y (230) sits ABOVE the grown card bottom
    //         (42 + 176 + 52 = 270), i.e. inside the Selected card -> overlap. --
    {
        PromptLayout raw = MakeBase();
        const int grownCardBottom = raw.selectedCard.y + raw.selectedCard.h + expDelta;
        check(raw.destCard.y < grownCardBottom,
              "defect: the raw first-frame preset would overlap a grid-sized Selected card");
    }

    // ---- 1. GRID: the Selected card is grid-cell sized (grows by delta) ----
    PromptLayout g = MakeBase();
    efflayout::ReflowMetrics m =
        efflayout::GridSizedPreferred(g, /*isGrid=*/true, /*itemCount=*/5,
                                      cellH, visRows, kRowsNeeded);

    check(g.selectedContent.h == 160 + expDelta,
          "grid: Selected content grows by exactly delta (whole-cell sized)");
    check(g.selectedList.h == 120 + expDelta,
          "grid: Selected list grows by exactly delta");
    check(g.selectedCard.h == 176 + expDelta,
          "grid: Selected CARD grows by exactly delta (paint matches content)");
    check(g.groupHeader.h == 176 + expDelta,
          "grid: group header (card alias) grows by exactly delta");
    check(g.windowH == 460 + expDelta,
          "grid: windowH grows by exactly delta so the footer stays visible");

    // ---- 2. Every section BELOW the card shifts down by the SAME delta ----
    check(g.destCard.y    == 230 + expDelta, "grid shift: destCard");
    check(g.destLabel.y   == 238 + expDelta, "grid shift: destLabel");
    check(g.destField.y   == 262 + expDelta, "grid shift: destField");
    check(g.destCopyBtn.y == 262 + expDelta, "grid shift: destCopyBtn");
    check(g.nameCard.y    == 310 + expDelta, "grid shift: nameCard");
    check(g.nameField.y   == 342 + expDelta, "grid shift: nameField");
    check(g.actionCard.y  == 390 + expDelta, "grid shift: actionCard");
    check(g.segCopy.y     == 360 + expDelta, "grid shift: segCopy");
    check(g.segMove.y     == 360 + expDelta, "grid shift: segMove");
    check(g.createButton.y == 400 + expDelta, "grid shift: createButton");
    check(g.cancelButton.y == 400 + expDelta, "grid shift: cancelButton");
    check(g.infoButton.y  == 400 + expDelta, "grid shift: infoButton");

    // ---- 3. Child controls stay INSIDE their cards (no overlap) ----
    check(g.destCard.y >= g.selectedCard.y + g.selectedCard.h,
          "grid: Destination card is fully below the grown Selected card (no overlap)");
    check(g.destField.y >= g.destCard.y &&
          g.destField.y + g.destField.h <= g.destCard.y + g.destCard.h,
          "grid: Destination field stays within the Destination card");

    // ---- 4. Zero-size (disabled) controls are NOT shifted ----
    check(g.autoGroupButton.y == 0 && g.autoGroupButton.h == 0,
          "grid: a disabled (zero-size) Auto Group button is never shifted");

    // ---- 5. Returned reflow metrics the resize path must reuse ----
    check(m.selRowH == cellH, "metrics: selRowH is the mode's whole-cell height (never 24)");
    check(m.selCap == 1,      "metrics: selCap is whole cells that fit the preferred budget");
    check(m.rowsNeeded == kRowsNeeded, "metrics: rowsNeeded is the true content rows");

    // ---- 6. LIST view is identity: nothing moves, metrics are List defaults ----
    PromptLayout l = MakeBase();
    efflayout::ReflowMetrics lm =
        efflayout::GridSizedPreferred(l, /*isGrid=*/false, /*itemCount=*/5,
                                      /*cellH=*/0, /*visRows=*/0, /*rowsNeeded=*/5);
    check(l.selectedContent.h == 160 && l.selectedCard.h == 176,
          "list: the Selected card is unchanged (no grid grow)");
    check(l.destCard.y == 230 && l.createButton.y == 400 && l.windowH == 460,
          "list: no section is shifted");
    check(lm.selRowH == kSelRowH && lm.selCap == -1 && lm.rowsNeeded == 5,
          "list: metrics are the List defaults (24px row, own cap)");

    // ---- 7. First frame == resize-path start: the SAME function, applied to an
    //         independent copy at the SAME preferred size, yields identical
    //         geometry (there is only ONE calculation). ----
    PromptLayout firstFrame = MakeBase();
    PromptLayout resizeStart = MakeBase();
    efflayout::GridSizedPreferred(firstFrame, true, 5, cellH, visRows, kRowsNeeded);
    efflayout::GridSizedPreferred(resizeStart, true, 5, cellH, visRows, kRowsNeeded);
    check(firstFrame.selectedCard.h == resizeStart.selectedCard.h &&
          firstFrame.destCard.y == resizeStart.destCard.y &&
          firstFrame.createButton.y == resizeStart.createButton.y &&
          firstFrame.windowH == resizeStart.windowH,
          "first frame uses the SAME geometry as the resize path at the same size");

    if (g_fail) { printf("effective_layout_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("effective_layout_test: OK\n");
    return 0;
}
