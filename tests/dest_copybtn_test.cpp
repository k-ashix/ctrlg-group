// dest_copybtn_test.cpp
// Pins the Destination card's "copy path" button geometry (fix thse isseue.md
// #2, path pass): "no icon path ... a button to copy path - refined reduced
// rect properly". The read-only path field is a REDUCED rect that leaves room
// for a compact copy-path button on the SAME row, to its right, inside the
// card -- so the path reads plainly and the button never overlaps the text.
// Self-contained: own main(), exit 0 = pass.

#include "layout_logic.h"

#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    layout::PromptLayout g = layout::ComputePromptLayout(true);

    // ---- the copy-path button is a real control on the Destination card ----
    check(g.destCopyBtn.w > 0 && g.destCopyBtn.h > 0, "dest card has a copy-path button rect");

    // ---- it shares the path field's row and height ----
    check(g.destCopyBtn.y == g.destField.y, "copy button shares the path field's row");
    check(g.destCopyBtn.h == g.destField.h, "copy button matches the field height");

    // ---- it sits to the RIGHT of the field, with a gap, never overlapping ----
    check(g.destCopyBtn.x >= g.destField.right(), "copy button sits to the right of the path field");
    check(g.destField.right() <= g.destCopyBtn.x, "path field never overlaps the copy button");
    check(g.destCopyBtn.x - g.destField.right() >= 4, "a visible gap separates field and button");

    // ---- the field is a REDUCED rect (narrower than the full card) ----
    check(g.destField.w > 0, "path field keeps a positive width");
    check(g.destField.w < g.destCard.w, "path field is a reduced rect (narrower than the card)");

    // ---- both stay inside the card's content width (no artifact spilling out) ----
    check(g.destCopyBtn.right() <= g.destCard.right(), "copy button stays inside the card width");

    // ---- New Folder (non-group) popup has no copy-path button ----
    layout::PromptLayout nf = layout::ComputePromptLayout(false);
    check(nf.destCopyBtn.w == 0, "New Folder popup has no copy-path button");

    if (g_fail) { printf("dest_copybtn_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("dest_copybtn_test: OK\n");
    return 0;
}
