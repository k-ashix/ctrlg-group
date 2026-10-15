// view_mode_test.cpp
// Pins fix thse isseue.md Phase A: the Selected Items presentation-mode model.
// Ctrl+wheel direction mapping (clamped), mutually-exclusive wheel routing,
// per-mode geometry (icon/cell sizes, responsive columns, grid rows, content
// height) and scroll-position preservation across a mode change.
// Self-contained: own main(), exit 0 = pass.

#include "view_mode_logic.h"

#include <cstdio>

using namespace viewmode;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- mode ordering (smaller -> larger) ----
    check(static_cast<int>(Mode::List) < static_cast<int>(Mode::Large) &&
          static_cast<int>(Mode::Large) < static_cast<int>(Mode::ExtraLarge),
          "modes ordered List < Large < ExtraLarge");
    check(ModeCount() == 3, "three presentation modes");

    // ---- Ctrl+wheel up -> larger, down -> smaller, clamped (no wrap) ----
    check(OnCtrlWheel(Mode::List, 120) == Mode::Large, "wheel up: List -> Large");
    check(OnCtrlWheel(Mode::Large, 120) == Mode::ExtraLarge,
          "wheel up: Large -> ExtraLarge");
    check(OnCtrlWheel(Mode::ExtraLarge, 120) == Mode::ExtraLarge,
          "wheel up at max clamps (no wrap)");
    check(OnCtrlWheel(Mode::ExtraLarge, -120) == Mode::Large,
          "wheel down: ExtraLarge -> Large");
    check(OnCtrlWheel(Mode::Large, -120) == Mode::List,
          "wheel down: Large -> List");
    check(OnCtrlWheel(Mode::List, -120) == Mode::List,
          "wheel down at min clamps (no wrap)");
    check(OnCtrlWheel(Mode::List, 0) == Mode::List, "no notch -> no change");
    // multi-notch in one event steps multiple times then clamps
    check(OnCtrlWheel(Mode::List, 240) == Mode::ExtraLarge, "two notches up");
    check(OnCtrlWheel(Mode::List, 600) == Mode::ExtraLarge, "many notches clamp up");
    check(OnCtrlWheel(Mode::ExtraLarge, -600) == Mode::List, "many notches clamp down");

    // ---- wheel routing: exactly one gesture per (ctrl,shift) combination ----
    for (int c = 0; c < 2; ++c) {
        for (int s = 0; s < 2; ++s) {
            const bool ctrl = c != 0, shift = s != 0;
            const int n = (ZoomsOnWheel(ctrl) ? 1 : 0) +
                          (PansChipsOnWheel(ctrl, shift) ? 1 : 0) +
                          (ScrollsListOnWheel(ctrl, shift) ? 1 : 0);
            check(n == 1, "exactly one wheel gesture per modifier combo");
        }
    }
    check(ZoomsOnWheel(true), "ctrl zooms");
    check(PansChipsOnWheel(false, true), "shift (no ctrl) pans chips");
    check(ScrollsListOnWheel(false, false), "no modifier scrolls list");
    check(ZoomsOnWheel(true) && !PansChipsOnWheel(true, true),
          "ctrl beats shift: ctrl+shift zooms, never pans");

    // ---- Issue 5: LOCATION-aware wheel routing (no Shift needed over strip) --
    // at most one gesture per (ctrl, overStrip, overRows, horizontal) combo
    for (int c = 0; c < 2; ++c)
        for (int st = 0; st < 2; ++st)
            for (int rw = 0; rw < 2; ++rw)
                for (int h = 0; h < 2; ++h) {
                    if (st && rw) continue;   // the strip and rows never overlap
                    const bool ctrl = c != 0, strip = st != 0,
                               rows = rw != 0, horiz = h != 0;
                    const int n = (WheelZooms(ctrl, horiz) ? 1 : 0) +
                                  (WheelPansChips(ctrl, strip, horiz) ? 1 : 0) +
                                  (WheelScrollsRows(ctrl, strip, rows, horiz) ? 1 : 0);
                    check(n <= 1, "at most one location-aware wheel gesture per combo");
                }
    // the key new behavior: a PLAIN wheel over the strip pans (NO Shift)
    check(WheelPansChips(false, true, false), "plain wheel over the strip pans chips (no Shift)");
    check(!WheelScrollsRows(false, true, false, false), "a wheel over the strip never scrolls rows");
    check(WheelPansChips(false, true, true), "horizontal wheel over the strip pans chips");
    check(WheelScrollsRows(false, false, true, false), "plain wheel over the rows scrolls the list");
    check(!WheelPansChips(false, false, false), "a wheel off the strip does not pan chips");
    check(WheelZooms(true, false) && !WheelPansChips(true, true, false),
          "Ctrl+wheel zooms, never pans, even over the strip");
    check(!WheelPansChips(false, false, true) && !WheelScrollsRows(false, false, true, true),
          "a horizontal wheel over the rows is ignored");

    // ---- per-mode metrics ----
    check(!IsGrid(Mode::List), "List is not a grid");
    check(IsGrid(Mode::Large) && IsGrid(Mode::ExtraLarge), "Large/XL are grids");
    check(IconPx(Mode::List) < IconPx(Mode::Large) &&
          IconPx(Mode::Large) < IconPx(Mode::ExtraLarge),
          "icon size grows with the mode");
    check(RowHeight(Mode::List) > 0 && RowHeight(Mode::Large) == 0,
          "only List uses a flat row height");
    check(CellW(Mode::List) == 0 && CellH(Mode::List) == 0,
          "List has no grid cell");
    check(CellW(Mode::Large) > 0 && CellH(Mode::Large) > 0, "Large has a cell");
    check(CellW(Mode::ExtraLarge) > CellW(Mode::Large) &&
          CellH(Mode::ExtraLarge) > CellH(Mode::Large),
          "ExtraLarge cell is bigger than Large");

    // ---- Ctrl+wheel zoom scales ONLY the icon content ----
    // fix thse isseue.md: "only zoom the actual contents, not the text and
    // chips and their padding". The non-icon part of every grid cell (the
    // paddings and the fixed label band that holds the DPI-only, mode-INDEPENDENT
    // filename text) must be IDENTICAL across the zoom levels, so zooming grows
    // only the icon (and the cell that wraps it) -- never the text, never the
    // padding. The chip strip is a separate gesture (Shift+wheel) and the label
    // font is UiFont(dpi), so neither scales with the Ctrl+wheel zoom.
    check(CellPadX(Mode::Large) == CellPadX(Mode::ExtraLarge),
          "horizontal cell padding is constant across zoom");
    check(CellPadY(Mode::Large) == CellPadY(Mode::ExtraLarge),
          "vertical cell padding is constant across zoom");
    check(LabelBandH(Mode::Large) == LabelBandH(Mode::ExtraLarge),
          "label band height is constant across zoom (text area never zooms)");
    check(CellW(Mode::Large) - IconPx(Mode::Large) ==
          CellW(Mode::ExtraLarge) - IconPx(Mode::ExtraLarge),
          "only the icon drives cell width across zoom (fixed horizontal padding)");
    check(CellH(Mode::Large) - IconPx(Mode::Large) ==
          CellH(Mode::ExtraLarge) - IconPx(Mode::ExtraLarge),
          "only the icon drives cell height across zoom (fixed padding + label band)");

    // fix thse isseue.md UI-07: a Large / Extra large grid cell reserves a REAL
    // filename band BELOW the icon (icon + vertical padding + the label band),
    // so the centred, wrapped + ellipsized name is never clipped the way the old
    // bare icon-height cell (IconPx + 16, no label band) clipped it. CellH must
    // exceed the icon by at least the label band on EVERY grid mode.
    check(CellH(Mode::Large) - IconPx(Mode::Large) >= LabelBandH(Mode::Large),
          "Large cell reserves a filename band beneath the icon (no clip)");
    check(CellH(Mode::ExtraLarge) - IconPx(Mode::ExtraLarge) >= LabelBandH(Mode::ExtraLarge),
          "ExtraLarge cell reserves a filename band beneath the icon (no clip)");

    // ---- responsive columns ----
    check(Columns(Mode::List, 1000) == 1, "List is single-column");
    check(Columns(Mode::Large, 0) == 1, "zero width still yields >= 1 column");
    check(Columns(Mode::Large, 5) == 1, "too-narrow width clamps to 1 column");
    {
        const int wide = CellW(Mode::Large) * 4 + 3;
        check(Columns(Mode::Large, wide) == 4, "fits exactly four Large columns");
        // a bigger cell yields FEWER columns in the same width
        check(Columns(Mode::ExtraLarge, wide) <= Columns(Mode::Large, wide),
              "ExtraLarge never has more columns than Large at a given width");
    }

    // ---- grid rows (ceil division) + content height ----
    check(GridRows(0, 4) == 0, "no items -> no rows");
    check(GridRows(1, 4) == 1, "one item -> one row");
    check(GridRows(4, 4) == 1, "exactly one full row");
    check(GridRows(5, 4) == 2, "overflow spills to a second row");
    check(GridRows(9, 4) == 3, "ceil division across three rows");
    check(GridRows(10, 0) == 10, "cols<1 treated as 1 column");
    check(VisibleGridRows(Mode::Large, CellH(Mode::Large) * 3 + 2) == 3,
          "whole visible grid rows that fit");
    check(ContentHeight(Mode::List, 10, 500) == 10 * RowHeight(Mode::List),
          "List content height = rows * rowH");
    {
        const int cols = Columns(Mode::Large, 500);
        check(ContentHeight(Mode::Large, 50, 500) ==
              GridRows(50, cols) * CellH(Mode::Large),
              "grid content height = gridRows * cellH");
    }

    // ---- fix thse isseue.md v0.12.67 (Issue #1 B): mode-aware Selected
    //      viewport sizing. List is not a grid (0 rows); a grid viewport shows
    //      WHOLE cells only (never a clipped row), always at least one row,
    //      content-sizes a small selection and caps a large one to the budget. --
    check(SelectedGridVisibleRows(Mode::List, 50, 1, 1000) == 0,
          "List mode is not a grid -> 0 grid rows");
    {
        const int cellH = CellH(Mode::Large);
        // small selection that FITS: 3 items, 4 columns -> 1 grid row; the
        // generous budget never forces more rows than the items need.
        check(SelectedGridVisibleRows(Mode::Large, 3, 4, cellH * 5) == 1,
              "small grid selection is content-sized (fits in one row, no dead space)");
        // a selection that EXCEEDS the budget caps to the whole cells that fit,
        // and the capped region never exceeds the budget (no clipped cell).
        const int budget = cellH * 2 + cellH / 2;   // 2.5 cells
        const int vis = SelectedGridVisibleRows(Mode::Large, 50, 4, budget);
        check(vis == 2, "overflowing grid caps to the whole cells that fit (2.5 -> 2)");
        check(vis * cellH <= budget,
              "capped grid rows never exceed the budget (no clipped cell)");
        // never zero rows even when a single cell is taller than the budget.
        check(SelectedGridVisibleRows(Mode::ExtraLarge, 20, 2, 1) == 1,
              "always at least one grid row, even with a tiny budget");
        // exact fit: budget == N whole cells, more items than that -> N rows.
        check(SelectedGridVisibleRows(Mode::Large, 99, 3, cellH * 3) == 3,
              "exact whole-cell budget shows exactly that many rows");
    }

    // ---- scroll preservation across a mode change ----
    // List topRow 7 -> grid: the same item 7 stays visible, clamped by cols.
    {
        const int item = FirstVisibleItem(Mode::List, 7, 1);
        check(item == 7, "List top row maps directly to item index");
        const int newCols = 4;
        const int newTop = TopRowForItem(Mode::Large, item, newCols);
        check(newTop == 7 / 4, "item 7 lands on grid row 1 with 4 columns");
        // round-trip: grid row back to an item still refers to the same block
        const int backItem = FirstVisibleItem(Mode::Large, newTop, newCols);
        check(backItem <= item && item < backItem + newCols,
              "the anchored item stays within the first visible grid row");
    }
    check(TopRowForItem(Mode::List, -5, 1) == 0, "negative item clamps to row 0");

    if (g_fail == 0) printf("view_mode_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
