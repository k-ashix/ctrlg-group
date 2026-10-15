// auto_group_footer_test.cpp
// Pins fix thse isseue.md ADD-ON Phase D: the footer row reads
// [Auto Group] [Group] [Cancel], right-aligned, non-overlapping, and fits
// inside the compact AND expanded Group windows. Mirrors the mod's footer math
// (layout constants kWindowW..., kButtonW=74, kButtonGap=8, kAutoGroupBtnW=108).
// Self-contained: own main(), exit 0 = pass.

#include "auto_group_footer_logic.h"

#include <cstdio>

using namespace agfooter;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const int kOuter = 20, kBtnW = 74, kGap = 8, kAutoW = 108;
    const int kCompactW = 410, kExpandedW = 660;

    // ---- compact group window ----
    Row c = Compute(kCompactW, kOuter, kBtnW, kGap, kAutoW);
    check(OrderOk(c), "compact: Auto Group left of Group left of Cancel");
    check(NoOverlap(c), "compact: no button overlaps its neighbour");
    check(FitsInside(c, kCompactW, kOuter), "compact: whole row fits inside content");
    // Cancel is flush to the right content edge.
    check(c.cancel.x + c.cancel.w == kCompactW - kOuter,
          "compact: Cancel right-aligned to content edge");

    // ---- expanded group window ----
    Row e = Compute(kExpandedW, kOuter, kBtnW, kGap, kAutoW);
    check(OrderOk(e), "expanded: order preserved");
    check(NoOverlap(e), "expanded: no overlap");
    check(FitsInside(e, kExpandedW, kOuter), "expanded: fits inside content");

    // ---- the Auto Group button is the wider one (fits "Auto Group (5)") ----
    check(c.autoGroup.w == kAutoW && c.autoGroup.w > kBtnW,
          "Auto Group button is wider than a standard footer button");

    if (g_fail) { printf("auto_group_footer_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("auto_group_footer_test: OK\n");
    return 0;
}
