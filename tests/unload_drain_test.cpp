// unload_drain_test.cpp
// Pins the shutdown-race decision for the queued action (ShouldRunQueuedAction,
// mirrored from the worker's guard in explorer-ctrlfn-newfolder.wh.cpp).
//
// The bug: Worker_Exit() sets g_unloading, but the worker loop did not check
// g_unloading before executing a dequeued WM_APP+1. An action queued just
// before/during teardown could therefore START during unload -- in inline mode
// it would create the temporary seed folder before WaitForInlineRenameResult()
// notices unloading, leaving stray state behind at teardown.
//
// The fix threads the unloading flag into the same pure guard the worker already
// consults: if we are unloading, DRAIN the queued action (discard it) instead of
// running it. This test would fail before the fix (the guard had no unloading
// input, so an unload-time action still ran).
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
    const TargetHandle kWin = 0xC0DE;

    // Normal case: not unloading, nothing running, real target -> run.
    check(ShouldRunQueuedAction(QueuedAction{false, false, kWin}),
          "not unloading, idle, real target -> run");

    // The core shutdown race: unloading -> drain even with a valid target.
    check(!ShouldRunQueuedAction(QueuedAction{true, false, kWin}),
          "unloading -> drain (never start during teardown)");

    // Unloading takes precedence regardless of the other fields.
    check(!ShouldRunQueuedAction(QueuedAction{true, true, kWin}),
          "unloading + already running -> drain");
    check(!ShouldRunQueuedAction(QueuedAction{true, false, 0}),
          "unloading + no target -> drain");

    // Existing guards still hold when not unloading.
    check(!ShouldRunQueuedAction(QueuedAction{false, true, kWin}),
          "action already running -> drain");
    check(!ShouldRunQueuedAction(QueuedAction{false, false, 0}),
          "no captured target -> drain");

    if (g_fail == 0) {
        printf("unload_drain_test: PASS\n");
        return 0;
    }
    printf("unload_drain_test: FAIL (%d)\n", g_fail);
    return 1;
}
