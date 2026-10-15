// chip_active_style_test.cpp
// Pins fix thse isseue.md Phase 2A: the ACTIVE summary chip is indicated ONLY by
// the dotted outline (the same focus/selection styling), never by a second
// accent fill overlay. chipfilter::ChipShowsDottedOutline(isActiveFilter,
// isFocused) decides when a chip draws the dotted outline: when it is the active
// filter OR the keyboard-focused chip. Self-contained: own main(), exit 0 = pass.

#include "chip_filter_logic.h"

#include <cstdio>

using namespace chipfilter;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    check(ChipShowsDottedOutline(true, false),   "active filter -> dotted outline");
    check(ChipShowsDottedOutline(false, true),   "keyboard focus -> dotted outline");
    check(ChipShowsDottedOutline(true, true),    "active + focused -> dotted outline");
    check(!ChipShowsDottedOutline(false, false), "neither -> no outline");

    // The active-filter state is still derived from the tested State model, and
    // the Items chip is the all-items view (NOT a highlighting/active state), so
    // it must not force the selected indicator by itself.
    State s;
    ActivateItems(s);
    check(!s.active(), "Items chip is not an 'active' (highlighting) state");
    ActivateFolders(s);
    check(s.active(), "Folders chip is an active highlighting state");

    if (g_fail) { printf("chip_active_style_test: %d FAILED\n", g_fail); return 1; }
    printf("chip_active_style_test: OK\n");
    return 0;
}
