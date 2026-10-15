// effective_layout_logic.h
//
// efflayout::GridSizedPreferred -- the ONE shared, mode-aware "effective
// preferred" layout model, mirrored byte-for-byte in ctrlg-group.wh.cpp (just
// above ComputeEffectiveLayout). Extracted so BOTH the first visible frame
// (initial outer size + WM_CREATE child placement) and the resize / maximize
// reflow start from the SAME geometry -- fix thse isseue.md P0: the first frame
// can no longer disagree with a later resize at the same size.
//
// ComputePromptLayout reserves the Selected list at the LIST row height
// (kSelRowH=24). In a GRID Selected view the rows are much taller WHOLE cells,
// so a list-sized card clips/crams the cells and leaves every section below it
// at the wrong Y. This re-sizes the Selected card to the current mode's whole
// grid cells -- bounded by the SAME vertical budget the list used -- then shifts
// every section below by the same delta and grows windowH. List mode is
// identity (delta 0). The caller passes the mode's grid metrics as plain ints
// (exactly as layout::ReflowToClient already does), so this stays a pure,
// Win32-free, unit-tested model with no viewmode dependency.
#ifndef EFFECTIVE_LAYOUT_LOGIC_H
#define EFFECTIVE_LAYOUT_LOGIC_H

#include "layout_logic.h"

namespace efflayout {

struct ReflowMetrics {
    int selRowH;     // row height handed to layout::ReflowToClient
    int selCap;      // whole-cell cap (-1 = ReflowToClient's own cap)
    int rowsNeeded;  // true content rows the reflow must know about
};

// base      : the preferred layout from layout::ComputePromptLayout (mutated).
// isGrid    : the Selected view is a grid (List -> identity, delta 0).
// itemCount : selected item count (List metrics default rowsNeeded).
// cellH     : whole grid cell height in 96-dpi design pixels.
// visRows   : whole grid rows the preferred budget shows (SelectedGridVisibleRows).
// rowsNeeded: true grid rows the content needs (GridRows).
inline ReflowMetrics GridSizedPreferred(layout::PromptLayout& base, bool isGrid,
                                        int itemCount, int cellH, int visRows,
                                        int rowsNeeded) {
    ReflowMetrics m{ layout::kSelRowH, -1, itemCount };
    if (!isGrid) return m;
    const int headerBlockH = base.selectedList.y - base.selectedContent.y;
    const int listBudget   = base.selectedContent.h - headerBlockH;
    int fit = (cellH > 0) ? listBudget / cellH : 0;  // whole cells in the budget
    if (fit < 1) fit = 1;
    const int newContentH = headerBlockH + visRows * cellH;
    const int delta = newContentH - base.selectedContent.h;
    if (delta != 0) {
        base.selectedContent.h += delta;
        base.selectedList.h    += delta;
        base.selectedCard.h    += delta;
        base.groupHeader.h     += delta;
        const auto shiftY = [delta](layout::Rect& r) {
            if (r.w > 0) r.y += delta;
        };
        shiftY(base.destCard);     shiftY(base.destLabel);
        shiftY(base.destDesc);     shiftY(base.destField);
        shiftY(base.destCopyBtn);
        shiftY(base.nameCard);     shiftY(base.nameLabel);
        shiftY(base.nameDesc);     shiftY(base.nameField);
        shiftY(base.actionCard);   shiftY(base.actionTitle);
        shiftY(base.actionDesc);
        shiftY(base.segCopy);      shiftY(base.segMove);
        shiftY(base.opToggle);     shiftY(base.keyboardHint);
        shiftY(base.createButton); shiftY(base.cancelButton);
        shiftY(base.autoGroupButton); shiftY(base.infoButton);
        base.windowH += delta;
    }
    m.selRowH    = cellH;
    m.selCap     = fit;
    m.rowsNeeded = rowsNeeded;
    return m;
}

}  // namespace efflayout

#endif  // EFFECTIVE_LAYOUT_LOGIC_H
