// grid_fit_test.cpp
// Pins fix thse isseue.md v0.12.68: the PURE decisions behind the mode-aware
// layout fix. (1) Blank-grid prevention -- a grid viewport always reserves at
// least one WHOLE cell (SelectedGridVisibleRows >= 1) and the layout can derive
// the minimum Selected-Items content height from the header plus one complete
// cell/row (MinSelectedContentH), replacing the old fixed 40px floor. (2) When
// the window is too small for the requested icon mode, LargestModeThatFits
// picks the biggest mode whose cell still fits -- never a blank grid. (3) Scroll
// position is preserved across a view switch by mapping the first visible ITEM
// through FirstVisibleItem/TopRowForItem. Self-contained: own main(), exit 0 = pass.

#include "view_mode_logic.h"

#include <cstdio>

using namespace viewmode;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // CellH sanity (icon + 2*padY(6) + labelBand(30) = icon + 42 for grids).
    check(CellH(Mode::Large) == 90, "Large cell height is icon(48)+42");
    check(CellH(Mode::ExtraLarge) == 138, "Extra large cell height is icon(96)+42");

    // ---- MinCellH: one whole cell (grid) or one list row ----
    check(MinCellH(Mode::List) == 24, "List minimum is one 24px row");
    check(MinCellH(Mode::Large) == 90, "Large minimum is one whole cell");
    check(MinCellH(Mode::ExtraLarge) == 138, "Extra large minimum is one whole cell");

    // ---- LargestModeThatFits: biggest mode whose cell fits, never blank ----
    check(LargestModeThatFits(200) == Mode::ExtraLarge, "roomy viewport keeps Extra large");
    check(LargestModeThatFits(138) == Mode::ExtraLarge, "exactly one XL cell still fits XL");
    check(LargestModeThatFits(137) == Mode::Large, "one px short of XL downgrades to Large");
    check(LargestModeThatFits(90)  == Mode::Large, "exactly one Large cell fits Large");
    check(LargestModeThatFits(89)  == Mode::List, "too small for any grid falls back to List");
    check(LargestModeThatFits(0)   == Mode::List, "a zero viewport falls back to List (never blank)");

    // ---- MinSelectedContentH: header + ONE whole cell/row (no fixed 40 floor) ----
    check(MinSelectedContentH(Mode::ExtraLarge, 60) == 198, "XL min content = header(60)+cell(138)");
    check(MinSelectedContentH(Mode::List, 60) == 84, "List min content = header(60)+row(24)");
    check(MinSelectedContentH(Mode::Large, 0) == 90, "a zero header still reserves one whole cell");

    // ---- CellFits: overlap guard -- does one whole cell fit the rows area? ----
    check(CellFits(Mode::ExtraLarge, 138), "an XL cell fits in exactly its own height");
    check(!CellFits(Mode::ExtraLarge, 137), "an XL cell does not fit one px short");
    check(CellFits(Mode::List, 24), "a list row fits in 24px");

    // ---- Blank-grid prevention: a too-small grid viewport still shows >= 1 ----
    check(SelectedGridVisibleRows(Mode::ExtraLarge, 20, 3, 50) >= 1,
          "a viewport smaller than one XL cell still reserves one whole row (never blank)");
    check(SelectedGridVisibleRows(Mode::ExtraLarge, 20, 3, 300) >= 1,
          "a roomy grid viewport shows at least one row");
    check(SelectedGridVisibleRows(Mode::List, 20, 1, 0) == 0,
          "List mode is not a grid (0 grid rows)");

    // ---- Scroll preservation across a view switch (Bug #2) ----
    {
        // List scrolled to top row 10 -> first visible item is item 10.
        const int firstItem = FirstVisibleItem(Mode::List, 10, 1);
        check(firstItem == 10, "list first-visible item equals its top row");

        // Switch to a 4-column grid: item 10 lives on grid row 2 (items 8..11).
        const int gridTop = TopRowForItem(Mode::Large, firstItem, 4);
        check(gridTop == 2, "the captured item maps to the correct grid row");
        check(FirstVisibleItem(Mode::Large, gridTop, 4) <= firstItem,
              "the grid row's first item is at or above the captured item (still visible)");

        // Return to List: land on the grid row's first item (item 8), not 0.
        const int backItem = FirstVisibleItem(Mode::Large, gridTop, 4);
        const int listTop  = TopRowForItem(Mode::List, backItem, 1);
        check(listTop == 8, "returning to List preserves the visible context (item 8)");
    }

    if (g_fail == 0) printf("grid_fit_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
