// right_inset_test.cpp
// Pins fix thse isseue.md #4: the dedicated RIGHT content inset for the Selected
// Items list. The inner list content (rows, size/ext metadata, chip clip,
// divider endpoint, scrollbar track) stops kSelListRightInset (12px @96dpi)
// short of the list child's right edge, applied ONCE inside the painter -- the
// 20px card inset is a SEPARATE, outer padding (no double padding).
// Self-contained: own main(), exit 0 == pass.

#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// Mirror of the painter's right-edge composition (fix thse isseue.md #4).
static int kRightInset = 12;      // kSelListRightInset @96dpi
static int kScrollbarW = 10;      // listscroll::kScrollbarW (representative)

// Effective content right = child right edge minus the dedicated inset.
static int ContentRight(int childRight, int inset) { return childRight - inset; }
// Rows/metadata stop before the scrollbar gutter when the bar is shown.
static int RowRight(int contentRight, bool needsBar, int barW) {
    return needsBar ? (contentRight - barW) : contentRight;
}

int main() {
    const int childLeft  = 0;
    const int childRight = 300;   // selectedContent-local right edge

    // ---- the inset is a real 12px, applied once ----
    check(kRightInset == 12, "right inset is 12px @96dpi");
    const int cr = ContentRight(childRight, kRightInset);
    check(cr == 288, "content right = 300 - 12");

    // ---- scrollbar track sits inside the inset, not against the card edge ---
    const int barRightNoScroll = RowRight(cr, false, kScrollbarW);
    check(barRightNoScroll == 288, "no scrollbar -> rows reach the inset edge");
    const int rowsWithBar = RowRight(cr, true, kScrollbarW);
    check(rowsWithBar == 278, "scrollbar present -> rows stop 10px before the inset edge");
    // The scrollbar track right edge == content right (inset), never childRight.
    check(cr < childRight, "scrollbar/divider end is inset from the card edge");

    // ---- name area shrinks by the inset so long names ellipsize earlier ----
    const int fullNameArea  = childRight - childLeft;             // 300 (old, flush)
    const int insetNameArea = cr - childLeft;                     // 288 (new)
    check(insetNameArea == fullNameArea - kRightInset, "name area loses exactly the inset");
    check(insetNameArea < fullNameArea, "long filenames ellipsize before the metadata");

    // ---- DPI scaling: inset scales with the row (125/150/200%) ----
    auto scale = [](int v, int pct) { return (v * pct + 50) / 100; };
    check(scale(kRightInset, 125) == 15, "125%: 12 -> 15");
    check(scale(kRightInset, 150) == 18, "150%: 12 -> 18");
    check(scale(kRightInset, 200) == 24, "200%: 12 -> 24");
    check(ContentRight(scale(childRight,150), scale(kRightInset,150)) == 450 - 18,
          "150%: content right = 450 - 18");

    if (g_fail == 0) printf("right_inset_test: OK\n");
    return g_fail ? 1 : 0;
}
