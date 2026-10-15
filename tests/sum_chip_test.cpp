// sum_chip_test.cpp
// Pins fix thse isseue.md #1: the Selected Items summary-chip geometry -- a
// dedicated pill design (X/Y padding, metric-derived height, pill radius, gaps)
// rather than the per-file listbadge constants or a hardcoded 16px height.
// Self-contained: own main(), exit 0 == pass.

#include "sum_chip_logic.h"

#include <cstdio>

using namespace sumchip;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- 96-DPI base tokens are the dedicated pill design, not listbadge ----
    check(kFontPx == 12, "font size 12px");
    check(kPadX == 8, "horizontal padding 8px per side");
    check(kPadY == 4, "vertical padding 4px per side");
    check(kMinRadius == 10, "minimum corner radius 10px");
    check(kGap == 6, "6px gap between chips");
    check(kTitleGap == 6, "6px gap between title and chip row");

    // ---- height is DERIVED from text metrics + vertical padding, never 16 ----
    check(ChipHeight(16, 4) == 24, "textH16 + 4+4 -> 24 (not the old 16)");
    check(ChipHeight(16, 4) != 16, "chip height is not the hardcoded 16");
    check(ChipHeight(14, 4) == 22, "textH14 -> 22");

    // ---- width is text width + horizontal padding on both sides ----
    check(ChipWidth(30, 8) == 46, "textW30 + 8+8 -> 46");
    check(ChipWidth(0, 8) == 16, "empty label still has 2x padding");

    // ---- radius is a pill (half height), clamped to the >=10 minimum ----
    check(ChipRadius(24, 10) == 12, "h24 -> pill radius 12");
    check(ChipRadius(18, 10) == 10, "small chip clamps up to min 10");
    check(ChipRadius(40, 10) == 20, "tall chip -> half-height 20");

    // ---- advance between chips includes exactly one gap ----
    check(NextChipX(0, 46, 6) == 52, "advance = x + width + gap");
    check(NextChipX(52, 40, 6) == 98, "advance chains across chips");

    // ---- DPI scaling: at 150% padding/min roughly 1.5x; geometry still holds -
    check(ChipHeight(24, 6) == 36, "150%: textH24 padY6 -> 36");
    check(ChipWidth(45, 12) == 69, "150%: textW45 padX12 -> 69");
    check(ChipRadius(36, 15) == 18, "150%: h36 -> 18 (>=15)");
    // 200%:
    check(ChipHeight(32, 8) == 48, "200%: textH32 padY8 -> 48");
    check(ChipRadius(48, 20) == 24, "200%: h48 -> 24 (>=20)");

    if (g_fail == 0) printf("sum_chip_test: OK\n");
    return g_fail ? 1 : 0;
}
