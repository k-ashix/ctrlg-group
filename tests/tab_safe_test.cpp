// tab_safe_test.cpp
// Pins the tab/path-safety decision for #3 (ShouldActOnTab, mirrored from the
// worker's guard in explorer-ctrlfn-newfolder.wh.cpp).
//
// #3: capturing only the Explorer WINDOW HWND is not enough -- one window hosts
// many tabs. Triggering in tab A then switching to tab B while the action is
// queued/waiting let the worker resolve the window's currently-active tab and
// create folders in B. We now also capture the active TAB identity at trigger
// time and require it to be unchanged on the worker; a zero captured tab means
// no tab could be identified (Desktop / single-tab shell) so there is nothing
// to drift and we proceed.
//
// Self-contained: own main(), exit 0 = pass.

#include "capture_logic.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

int main() {
    const TargetHandle kTabA = 0xAAAA;
    const TargetHandle kTabB = 0xBBBB;

    // Same tab active now as at trigger time -> safe to act.
    check(ShouldActOnTab(TabCheck{kTabA, kTabA}),
          "same active tab -> act");

    // The core #3 bug: user switched to a different tab while waiting -> cancel.
    check(!ShouldActOnTab(TabCheck{kTabA, kTabB}),
          "active tab changed (A->B) -> cancel");

    // Tab went away entirely (closed) -> not the same -> cancel.
    check(!ShouldActOnTab(TabCheck{kTabA, 0}),
          "captured tab no longer active (closed) -> cancel");

    // No tab identity captured (Desktop / single-tab shell): nothing to drift.
    check(ShouldActOnTab(TabCheck{0, 0}),
          "no captured tab identity -> proceed");
    check(ShouldActOnTab(TabCheck{0, kTabB}),
          "no captured tab identity, some tab now -> still proceed");

    if (g_fail == 0) {
        printf("tab_safe_test: PASS\n");
        return 0;
    }
    printf("tab_safe_test: FAIL (%d)\n", g_fail);
    return 1;
}
