// redraw_logic_test.cpp
// Pins the popup relayout REDRAW policy (fix thse isseue.md "Next exact fix
// order" #1-#4): a compact/expanded +/- resize must be ATOMIC. Suppress
// intermediate painting while the window resizes and EVERY child moves, then
// issue ONE final repaint that invalidates, erases, updates now, AND includes
// every child. No per-child repaint (the cause of the wiggly/staggered +/-
// transition) and -- because that one repaint already covers every child -- no
// separate pre-relayout invalidate of the size toggle.
//
// Self-contained: own main(), exit 0 = pass. RED while redraw_logic.h is absent
// or the flag set is wrong; GREEN once the policy header matches.

#include "redraw_logic.h"

#include <cstdio>

using namespace redraw;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- a child moved during the batch must NOT repaint itself ----
    // (DeferWindowPos / no bRepaint == TRUE); the single final repaint paints.
    check(kRepaintChildOnMove == false,
          "children are moved without a per-child repaint");

    // ---- the single final repaint covers window + ALL children ----
    check((FinalRepaintFlags() & kRdwInvalidate)  != 0, "final repaint invalidates");
    check((FinalRepaintFlags() & kRdwErase)       != 0, "final repaint erases");
    check((FinalRepaintFlags() & kRdwUpdateNow)   != 0, "final repaint updates now");
    check((FinalRepaintFlags() & kRdwAllChildren) != 0, "final repaint includes all children");

    // ---- exactly those four flags, nothing stray ----
    check(FinalRepaintFlags() ==
              (kRdwInvalidate | kRdwErase | kRdwUpdateNow | kRdwAllChildren),
          "final repaint flag set == INVALIDATE|ERASE|UPDATENOW|ALLCHILDREN");

    // ---- because the final repaint already covers every child, a separate
    //      pre-relayout invalidate of the size toggle is redundant ----
    check(FinalRepaintCoversChildren() == true,
          "final repaint covers children -> no separate size-toggle invalidate");

    if (g_fail) { printf("redraw_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("redraw_logic_test: OK\n");
    return 0;
}
