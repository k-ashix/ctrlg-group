// grid_spacing_test.cpp
// Pins fix thse isseue.md #13 ("final 4/8 px spacing cleanup"): every
// inter-element SPACING token in the layout system sits on the 4px grid, and
// the one straggler -- the action-row / chrome gap -- is cleaned from 6 to 8.
//
// Intentionally NOT grid-bound (and therefore not asserted): control intrinsic
// sizes (label/field/button heights such as 22/30) and total window dimensions
// (344/410/660/178), which follow standard Win32 control metrics rather than
// the spacing rhythm. Self-contained: own main(), exit 0 = pass.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}
static bool g4(int v) { return v % 4 == 0; }

int main() {
    // ---- the spacing SCALE is a strict 4px grid ----
    check(g4(kUnit) && g4(kSpace2) && g4(kSpace3) && g4(kSpace4) &&
          g4(kSpace5) && g4(kSpace6), "spacing scale is on the 4px grid");

    // ---- every inter-element spacing / padding / gap token is on the grid ----
    check(g4(kOuterMargin),           "outer margin on grid");
    check(g4(kButtonGap),             "button/chrome gap on grid (fix #13: was 6)");
    check(g4(kCardInset),             "card inset on grid");
    check(g4(kCardGap),               "card gap on grid");
    check(g4(kGroupHeaderTop),        "group header top on grid");
    check(g4(kGroupHeaderBottomGap),  "group header bottom gap on grid");
    check(g4(kGroupBottomMargin),     "group bottom margin on grid");
    check(g4(kSelListPadV),           "selected-list vertical padding on grid");
    check(g4(kSelRowH),               "selected row height on grid");
    check(g4(kSectionIconCol),        "section icon column on grid");
    check(g4(kChromeBtnSize),         "chrome button size on grid");
    check(g4(kDescH),                 "description row height on grid");

    // ---- the straggler is specifically cleaned to 8 ----
    check(kButtonGap == 8, "fix #13: action-row / chrome gap cleaned 6 -> 8");

    // ---- the resulting action-row gap equals the grid gap, both popups ----
    PromptLayout nf = ComputePromptLayout(false);
    check(nf.cancelButton.x - nf.createButton.right() == kButtonGap,
          "nf Create/Cancel gap == button gap");
    check(nf.cancelButton.x - nf.createButton.right() == 8, "nf action gap is 8");
    PromptLayout g = ComputePromptLayout(true);
    check(g.cancelButton.x - g.createButton.right() == 8, "group action gap is 8");

    // ---- key composed positions land on the grid (visual rhythm) ----
    check(g4(g.groupHeader.x) && g4(g.groupHeader.y), "group header origin on grid");
    check(g4(g.selectedList.x), "selected list left on grid");

    if (g_fail == 0) printf("grid_spacing_test: OK\n");
    else             printf("grid_spacing_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
