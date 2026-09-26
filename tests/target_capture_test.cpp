// target_capture_test.cpp
// Pins the origin-target decision for #1 (ShouldActOnTarget, mirrored from the
// worker's guard in explorer-ctrlfn-newfolder.wh.cpp).
//
// Bug (#1): the Explorer target HWND was resolved LATE on the worker thread
// (inside PerformNewFolderAction), long after the shortcut fired -- the
// multi-folder count timer alone is up to 1200ms. By then foreground/cursor
// could have moved, so the worker created folders in the WRONG window (or
// none). The fix captures the HWND at trigger time, carries it through the
// queued WM_APP+1, and the worker acts only when that same target is still a
// live Explorer window. ShouldActOnTarget is that guard.
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
    const TargetHandle kWin = 0xA11CE;  // some captured HWND value

    // Happy path: captured a real window, still valid, still Explorer -> act.
    check(ShouldActOnTarget(TargetCheck{kWin, true, true}),
          "valid live Explorer target -> act");

    // Nothing captured at trigger time -> never act (this was the silent
    // "resolve whatever is foreground now" fallback we are removing).
    check(!ShouldActOnTarget(TargetCheck{0, true, true}),
          "no captured target -> cancel");
    check(!ShouldActOnTarget(TargetCheck{0, false, false}),
          "null capture stays cancel regardless of other facts");

    // Captured window was destroyed before the worker got to it -> cancel.
    check(!ShouldActOnTarget(TargetCheck{kWin, false, true}),
          "captured window no longer valid -> cancel");

    // Window still alive but no longer an Explorer target (user navigated it
    // away / it became something else) -> cancel rather than act on it.
    check(!ShouldActOnTarget(TargetCheck{kWin, true, false}),
          "valid but non-Explorer target -> cancel");

    if (g_fail == 0) {
        printf("target_capture_test: PASS\n");
        return 0;
    }
    printf("target_capture_test: FAIL (%d)\n", g_fail);
    return 1;
}
