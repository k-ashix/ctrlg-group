// destfield_logic_test.cpp
// Pins the read-only Destination "information field" model (fix thse isseu e.md
// #2), mirrored from the mod into destfield_logic.h. The field must read as a
// proper read-only info field: folder icon on the left, path text inset to the
// right of it, no keyboard focus / no caret, and path ellipsis for long paths.
//
// Self-contained: own main(), exit 0 = pass. RED before destfield_logic.h
// exists; GREEN once it mirrors the mod.

#include "destfield_logic.h"

#include <cstdio>

using namespace destfield;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // Field as laid out by the Destination card: 280 wide, 26 tall (design px).
    const int fieldW = 280, fieldH = 26;

    // ---- NO icon: the path shows plainly, text begins at the edge pad ----
    // fix thse isseue.md #2 (path pass): "no icon path" -- the read-only field
    // no longer reserves a folder-glyph column; the path text starts right at
    // the edge pad so the field reads as a clean, un-decorated path.
    check(TextLeftInset() == kEdgePad, "text begins at the edge pad (no icon column)");
    check(TextLeftInset() == 10, "text left inset is 10 (edge pad only, no icon)");

    // ---- text cell: inset by the edge pad on both sides; right-padded ----
    Cell text = TextCell(fieldW, fieldH);
    check(!text.empty(), "text cell exists");
    check(text.x == kEdgePad, "text starts at the edge pad (no icon offset)");
    check(text.x == TextLeftInset(), "text inset matches TextLeftInset()");
    check(text.right() <= fieldW - kEdgePad + 1, "text is right-padded from the border");
    check(text.h == fieldH, "text cell spans the field height");

    // ---- read-only info-field policy: no focus, no caret, path ellipsis ----
    check(NoKeyboardFocus(), "field takes no keyboard focus");
    check(!ShowCaret(), "field shows no caret");
    check(UsePathEllipsis(), "long paths use a path ellipsis");

    // fix thse isseue.md #6: a read-only INFO field has a steady NEUTRAL border
    // -- it never shows a focus accent (unlike the editable Folder name field).
    check(!BorderIsAccent(false), "read-only field border is neutral when unfocused");
    check(!BorderIsAccent(true), "read-only field never shows a focus accent");

    // ---- geometry scales sanely for a narrower field (no negative widths) ----
    Cell narrow = TextCell(60, fieldH);
    check(narrow.w >= 0, "narrow field never yields a negative text width");

    if (g_fail) { printf("destfield_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("destfield_logic_test: OK\n");
    return 0;
}
