// layout_outer_size_test.cpp
// Phase 3a bug fix (fix thse isseu e.md #1): the Group / New Folder popup was
// vertically CLIPPED because ComputePromptLayout().windowH is a CLIENT (content)
// height, but it was passed to CreateWindowExW / SetWindowPos as the OUTER
// window height. With WS_CAPTION the title bar + borders consume client space,
// so the bottom action buttons fell OUTSIDE the client area. The mod now
// converts client -> outer via AdjustWindowRectExForDpi; these helpers own the
// arithmetic and, crucially, the invariant that a captioned window's outer
// height is strictly greater than its client height.
//
// Self-contained: own main(), exit 0 = pass. RED before layout::Outer*FromClient
// exists; GREEN once layout_logic.h mirrors the mod.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // Representative WS_CAPTION | WS_SYSMENU frame at 96 dpi (device px):
    //   ~8px border on each side, ~31px title bar on top, ~8px bottom border.
    // AdjustWindowRectExForDpi() reports these; the helpers just add them.
    const int ncExtraW = 8 + 8;    // left + right borders
    const int ncExtraH = 31 + 8;   // caption + bottom border

    // ---- outer = client + non-client extents ----
    check(OuterWidthFromClient(344, ncExtraW) == 360, "width adds side borders");
    check(OuterHeightFromClient(178, ncExtraH) == 217, "height adds caption + bottom border");

    // ---- THE invariant that fixes the clip: a caption makes the OUTER height
    //      strictly greater than the CLIENT height, so bottom controls that sit
    //      at y up to windowH remain inside the client area. ----
    check(OuterHeightFromClient(178, ncExtraH) > 178, "caption => outer taller than client (no clip)");
    check(OuterHeightFromClient(560, ncExtraH) > 560, "tall expanded Group popup also unclipped");

    // ---- the ORIGINAL bug is exactly ncExtra == 0 (client used verbatim as
    //      the outer size): the identity case, documented so it can't creep back. ----
    check(OuterHeightFromClient(178, 0) == 178, "degenerate: no frame => outer == client");

    if (g_fail) { printf("layout_outer_size_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("layout_outer_size_test: OK\n");
    return 0;
}
