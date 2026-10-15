// segicon_test.cpp
// Pins fix thse isseue.md #9 (UI polish pass 6, PASS 2): the Copy/Move segmented
// control gets a SMALL leading vector glyph before each caption
// ([icon] Copy / [icon] Move), with the icon+label group centred as a UNIT
// inside the segment (not the bare text centred). Pure model mirrored into
// namespace segicon in ctrlg-group.wh.cpp; the GDI paint (which
// glyph, DrawText) is pinned by source invariants. Self-contained: own main().
// RED until tests/segicon_logic.h exists.

#include "segicon_logic.h"

#include <cstdio>

using namespace segicon;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- #9: a small leading glyph + a modest gap ("don't overdo it") ----
    check(kSegIcon == 14, "segment leading glyph is a small 14px (fix #9)");
    check(kSegIconGap == 6, "glyph -> caption gap is 6");

    const int textW = 40;
    const int gw = GroupWidth(textW);
    check(gw == kSegIcon + kSegIconGap + textW, "group width = icon + gap + text");

    // ---- the icon+label group is centred as a unit inside the segment ----
    const int segLeft = 36, segW = 120;
    const int gl = GroupLeft(segLeft, segW, gw);
    check(gl == segLeft + (segW - gw) / 2, "group is centred in the segment");
    check(gl >= segLeft, "group never starts left of the segment");
    check(gl + gw <= segLeft + segW, "centred group fits inside the segment");

    // ---- icon first, then the caption past the icon + gap (no overlap) ----
    check(IconLeft(gl) == gl, "icon sits at the group's left edge");
    check(TextLeft(gl) == gl + kSegIcon + kSegIconGap, "caption starts past the icon + gap");
    check(TextLeft(gl) >= IconLeft(gl) + kSegIcon, "caption never overlaps the icon");

    // ---- a too-small segment clamps the group to the left edge (no negative) ----
    check(GroupLeft(segLeft, 10, gw) == segLeft, "group clamps to left when the segment is too small");

    if (g_fail) { printf("segicon_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("segicon_test: OK\n");
    return 0;
}
