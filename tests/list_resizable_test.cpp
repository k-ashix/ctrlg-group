// list_resizable_test.cpp
// Pins fix thse isseue.md v0.12.69 spec section 2 (P0 -- eliminate dead space in
// Selected Items, List mode). When the popup is MAXIMIZED or MANUALLY RESIZED,
// the List viewport must show as many WHOLE rows as physically fit (filling the
// enlarged card -> no blank tail), NOT the compact 4-row / expanded 9-row cap.
// When the popup has NOT been user-sized, the declared compact/expanded cap is
// preserved exactly as before. Self-contained: own main(), exit 0 = pass.

#include "list_scroll_logic.h"

#include <cstdio>

using namespace listscroll;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const int rowH = 24;

    // ---- NOT user-sized: identical to the existing EffectiveVisibleRows cap --
    // Compact cap 4, a tall viewport that physically fits ~20 rows -> still 4.
    check(EffectiveVisibleRowsResizable(4, 20 * rowH, rowH, /*userSized=*/false) == 4,
          "preferred compact: declared 4-row cap preserved in a tall viewport");
    check(EffectiveVisibleRowsResizable(9, 20 * rowH, rowH, false) == 9,
          "preferred expanded: declared 9-row cap preserved");
    // Behaves exactly like the legacy helper when not user-sized.
    check(EffectiveVisibleRowsResizable(4, 20 * rowH, rowH, false)
              == EffectiveVisibleRows(4, 20 * rowH, rowH),
          "not user-sized == legacy EffectiveVisibleRows");
    // A short viewport still clamps to physical capacity (never a clipped row).
    check(EffectiveVisibleRowsResizable(9, 3 * rowH, rowH, false) == 3,
          "preferred: physical capacity still wins when it is below the cap");

    // ---- USER-SIZED (maximized / manually resized): FILL the viewport --------
    // A tall maximized List viewport fits ~20 rows -> show 20, not 4/9 -> no dead space.
    check(EffectiveVisibleRowsResizable(4, 20 * rowH, rowH, /*userSized=*/true) == 20,
          "maximized List: fill the viewport (20 rows), not the 4-row cap");
    check(EffectiveVisibleRowsResizable(9, 20 * rowH, rowH, true) == 20,
          "maximized List: fill the viewport, not the 9-row cap");
    // A manually resized medium viewport fits 7 rows -> show 7.
    check(EffectiveVisibleRowsResizable(4, 7 * rowH + 10, rowH, true) == 7,
          "resized List: whole rows that fit (7), never a clipped row");

    // ---- not-yet-measured viewport: fall back to the declared cap -----------
    check(EffectiveVisibleRowsResizable(4, 0, rowH, true) == 4,
          "unmeasured viewport -> declared cap (never zero rows), even user-sized");
    check(EffectiveVisibleRowsResizable(9, 100, 0, false) == 9,
          "zero row height -> declared cap (never divide-by-zero)");

    if (g_fail) { printf("list_resizable_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("list_resizable_test: OK\n");
    return 0;
}
