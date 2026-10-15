// view_mode_logic.h
//
// PURE presentation-mode model for the Ctrl+G "Selected Items" viewport
// (fix thse isseue.md Phase A). Mirrored byte-for-byte into ctrlg-group.wh.cpp
// (namespace viewmode). No Win32 -- unit-tested with plain g++.
//
// Three Windows-Explorer-like modes: List (compact rows), Large icons, Extra
// large icons. Ctrl + Mouse Wheel cycles between them. Direction maps wheel-up
// (forward, positive delta) -> a LARGER view and wheel-down -> a SMALLER view,
// CLAMPED at the two ends (no wrap-around) -- documented in the help popup.
// Normal wheel scrolls the list and Shift + wheel pans the chip strip; the
// routing helpers below keep those three gestures mutually exclusive so a zoom
// is never also a scroll or a chip pan.
//
// Geometry (icon px, cell px, row height, responsive columns, content height)
// is derived per mode at a 96-DPI base; the painter DPI-scales the results. The
// selection model/order never changes with the mode -- only presentation does.
//
#pragma once

namespace viewmode {

enum class Mode { List = 0, Large = 1, ExtraLarge = 2 };

inline int ModeCount() { return 3; }

// ---- Ctrl+wheel direction mapping (clamped, no wrap) ----------------------
// Step by whole mode positions, clamped to [List, ExtraLarge].
inline Mode Step(Mode m, int steps) {
    int v = static_cast<int>(m) + steps;
    if (v < 0) v = 0;
    if (v > 2) v = 2;
    return static_cast<Mode>(v);
}

// wheelDelta is in WHEEL_DELTA units (120 per notch). Positive (wheel up /
// forward) -> a LARGER view; negative -> a SMALLER view. Multiple notches in
// one event step multiple times, always clamping at the ends.
inline Mode OnCtrlWheel(Mode m, int wheelDelta) {
    const int notches = wheelDelta / 120;   // truncates toward zero
    return Step(m, notches);                // wheel up (+notches) -> larger
}

// ---- input routing: exactly one meaning per wheel event -------------------
// Ctrl (with or without Shift) zooms the view; Shift alone pans chips; no
// modifier scrolls the list. Ctrl beats Shift so an accidental Ctrl+Shift wheel
// zooms (never also pans). These three are mutually exclusive and exhaustive.
inline bool ZoomsOnWheel(bool ctrlDown) { return ctrlDown; }
inline bool PansChipsOnWheel(bool ctrlDown, bool shiftDown) {
    return !ctrlDown && shiftDown;
}
inline bool ScrollsListOnWheel(bool ctrlDown, bool shiftDown) {
    return !ctrlDown && !shiftDown;
}

// fix thse isseue.md Issue 5: LOCATION-aware wheel routing. The pointer position
// (over the summary-chip strip vs the item rows) decides pan-vs-scroll, so a
// PLAIN wheel over the strip pans the chips WITHOUT requiring Shift (Shift +
// wheel and a horizontal wheel over the strip still pan -- they are also "over
// the strip"). Ctrl keeps zoom priority. `horizontal` is a WM_MOUSEHWHEEL event.
// At most ONE of {zoom, pan, scroll} is ever true for the same event.
inline bool WheelZooms(bool ctrlDown, bool horizontal) {
    return ctrlDown && !horizontal;
}
inline bool WheelPansChips(bool ctrlDown, bool overStrip, bool horizontal) {
    return !WheelZooms(ctrlDown, horizontal) && overStrip;
}
inline bool WheelScrollsRows(bool ctrlDown, bool overStrip, bool overRows,
                             bool horizontal) {
    if (WheelZooms(ctrlDown, horizontal)) return false;
    if (overStrip) return false;              // the strip pans, never scrolls rows
    return overRows && !horizontal;           // only a vertical wheel scrolls rows
}

// ---- per-mode metrics (96-DPI base px) ------------------------------------
inline bool IsGrid(Mode m) { return m != Mode::List; }

inline int IconPx(Mode m) {
    switch (m) {
        case Mode::List:       return 16;
        case Mode::Large:      return 48;
        case Mode::ExtraLarge: return 96;
    }
    return 16;
}

// Compact List row height (unchanged from the existing S(24) list); grid modes
// do not use a flat row height.
inline int RowHeight(Mode m) { return m == Mode::List ? 24 : 0; }

inline int CellPadX(Mode m) { return IsGrid(m) ? 8 : 0; }
inline int CellPadY(Mode m) { return IsGrid(m) ? 6 : 0; }

// Room under the icon for up to two lines of wrapped filename.
inline int LabelBandH(Mode m) { return IsGrid(m) ? 30 : 0; }

// Square-ish grid cell. Horizontal breathing is a CONSTANT 8px for EVERY grid
// mode and the label band is fixed, so the Ctrl+wheel zoom grows ONLY the icon
// (IconPx) and the cell that wraps it -- never the padding or the text area.
inline int CellW(Mode m) {
    if (!IsGrid(m)) return 0;
    return IconPx(m) + 2 * CellPadX(m) + 8;
}
inline int CellH(Mode m) {
    if (!IsGrid(m)) return 0;
    return IconPx(m) + 2 * CellPadY(m) + LabelBandH(m);
}

// Responsive column count that fits availW; always >= 1 so an item is never
// clipped (spec: prefer fewer columns over clipped icons/labels).
inline int Columns(Mode m, int availW) {
    if (!IsGrid(m)) return 1;
    const int cw = CellW(m);
    if (cw <= 0 || availW <= 0) return 1;
    const int c = availW / cw;
    return c < 1 ? 1 : c;
}

// Grid rows needed to lay itemCount items into `cols` columns (ceil division).
inline int GridRows(int itemCount, int cols) {
    if (cols < 1) cols = 1;
    if (itemCount <= 0) return 0;
    return (itemCount + cols - 1) / cols;
}

// Whole grid rows that physically fit in viewportH (paging + scrollbar).
inline int VisibleGridRows(Mode m, int viewportH) {
    const int ch = CellH(m);
    if (ch <= 0 || viewportH <= 0) return 0;
    return viewportH / ch;
}

// Total scrollable content height for a mode + selection, used to decide
// whether the scrollbar is needed and how tall the thumb is.
inline int ContentHeight(Mode m, int itemCount, int availW) {
    if (!IsGrid(m)) return itemCount * RowHeight(m);
    const int cols = Columns(m, availW);
    return GridRows(itemCount, cols) * CellH(m);
}

// fix thse isseue.md v0.12.67 (Issue #1 B): how many GRID rows the Selected
// viewport should show for `itemCount` items at `cols` columns, given a
// vertical pixel budget. Whole cells only (never a clipped row), always at
// least one row, and never more rows than the items actually need -- so a small
// selection content-sizes (no dead space) while a large one is capped to the
// budget and the remainder scrolls. List mode returns 0 (it is not a grid).
inline int SelectedGridVisibleRows(Mode m, int itemCount, int cols,
                                   int budgetPx) {
    if (!IsGrid(m)) return 0;
    const int ch = CellH(m);
    if (ch <= 0) return 0;
    int fit = budgetPx / ch;          // whole cells only -> no clipped row
    if (fit < 1) fit = 1;             // always at least one row visible
    const int need = GridRows(itemCount, cols);
    const int want = need < 1 ? 1 : need;
    return want < fit ? want : fit;
}

// ---- scroll preservation across a mode change -----------------------------
// Keep the first currently-visible ITEM anchored when the mode changes, then
// the caller clamps the new top row. In List mode a "row" is one item; in grid
// mode a "row" is `cols` items, so conversion goes through the item index.
inline int FirstVisibleItem(Mode oldMode, int topRow, int oldCols) {
    if (oldMode == Mode::List) return topRow;
    return topRow * (oldCols < 1 ? 1 : oldCols);
}
inline int TopRowForItem(Mode newMode, int itemIndex, int newCols) {
    if (itemIndex < 0) itemIndex = 0;
    if (newMode == Mode::List) return itemIndex;
    const int c = newCols < 1 ? 1 : newCols;
    return itemIndex / c;
}

// fix thse isseue.md v0.12.68: minimum WHOLE-cell height for a mode -- one grid
// cell for the icon grids, one list row for List. Used to reserve space for at
// least one complete cell (never a blank grid) and to derive the Selected-Items
// minimum content height instead of a fixed pixel floor.
inline int MinCellH(Mode m) { return IsGrid(m) ? CellH(m) : RowHeight(m); }

// The LARGEST view mode whose one whole cell fits in `availRowsH` px of rows
// viewport. Falls back to List (which always shows at least one short row), so
// a too-small window degrades gracefully to a mode that can still show a cell
// rather than painting nothing. Order: Extra large > Large > List.
inline Mode LargestModeThatFits(int availRowsH) {
    if (availRowsH >= CellH(Mode::ExtraLarge)) return Mode::ExtraLarge;
    if (availRowsH >= CellH(Mode::Large))      return Mode::Large;
    return Mode::List;
}

// Minimum Selected-Items CONTENT height for a mode: the header block plus ONE
// whole cell/row. Replaces the old fixed 40px reflow floor so the lower cards
// and footer are never overlapped by a half-painted cell.
inline int MinSelectedContentH(Mode m, int headerBlockH) {
    return headerBlockH + MinCellH(m);
}

// Overlap guard: does one whole cell/row of this mode fit in `availRowsH` px?
inline bool CellFits(Mode m, int availRowsH) { return MinCellH(m) <= availRowsH; }

}  // namespace viewmode
