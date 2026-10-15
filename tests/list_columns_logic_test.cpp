// list_columns_logic_test.cpp
// Pins the PURE column geometry for the owner-drawn Selected Items list
// (Phase 5/C): the name column must fill the row minus the fixed right-aligned
// EXT + SIZE columns, and must never collapse to a negative/overlapping width.
//
// Self-contained: own main(), exit 0 = pass.

#include "list_columns_logic.h"

#include <cstdio>

using namespace listcol;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- name gets the row minus the fixed right columns + gap ----
    check(NameWidth(280) == 280 - kExtW - kSizeW - kColGap, "name = row - ext - size - gap");
    check(NameWidth(280) == 162, "concrete: 280 -> 162");

    // ---- name never overlaps the right columns on a narrow row ----
    check(NameWidth(100) == kMinNameW, "narrow row clamps to min name width");
    check(NameWidth(0) == kMinNameW, "zero row still clamps to min");

    // ---- wider row => wider (or equal) name column (monotonic) ----
    check(NameWidth(400) >= NameWidth(280), "wider row, wider name");

    // ---- the fixed columns are real, positive widths ----
    check(kExtW > 0 && kSizeW > 0, "ext + size columns are positive");

    // ---- NameWidthFrom: name column from the LIVE row width (fix thse isseue.md #1) ----
    // The owner-draw list must size the name column from the ACTUAL current
    // content width (which grows when the window expands), not the fixed
    // compact content width. The caller passes already-DPI-scaled column
    // widths, so the helper is unit-agnostic.
    //
    // At 1x (design px) it matches NameWidth exactly.
    check(NameWidthFrom(280, kExtW, kSizeW, kColGap, kMinNameW) == NameWidth(280),
          "NameWidthFrom at 1x == NameWidth");
    // A WIDER live row => a WIDER name column: this is the whole point of the
    // fix -- the genuinely wider expanded window must use the extra width.
    check(NameWidthFrom(560, kExtW, kSizeW, kColGap, kMinNameW)
              > NameWidthFrom(304, kExtW, kSizeW, kColGap, kMinNameW),
          "wider live row -> wider name column (expanded uses the extra width)");
    // DPI-scaled columns are honoured (not the design-px constants).
    check(NameWidthFrom(600, kExtW * 2, kSizeW * 2, kColGap * 2, kMinNameW * 2)
              == 600 - (kExtW + kSizeW + kColGap) * 2,
          "NameWidthFrom honours DPI-scaled columns");
    // Still clamps to the minimum on a narrow live row.
    check(NameWidthFrom(40, kExtW, kSizeW, kColGap, kMinNameW) == kMinNameW,
          "narrow live row clamps to min name width");

    if (g_fail) { printf("list_columns_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("list_columns_logic_test: OK\n");
    return 0;
}
