// snapshot_trust_test.cpp
// Pins the worker-busy false-cancel fix for #2 (ShouldRunAfterSnapshot,
// mirrored from PerformNewFolderAction in ctrlg-group.wh.cpp).
//
// Bug (#2, worker-busy false-cancel): when the worker thread is still busy
// finishing a PREVIOUS folder action, the trigger-dir snapshot resolves late.
// A fragile time-based "snapshot resolved too late (worker was busy);
// cancelling" guard treated that late snapshot as untrustworthy and silently
// cancelled the NEXT trigger (0 folders created) -- even though the directory
// and tab had not actually drifted.
//
// Fix (Option C): drop the time-based freshness gate and rely on the
// authoritative same-directory + same-tab drift checks. A busy worker (late
// snapshot) no longer causes a false cancel; true drift is still cancelled.
// ShouldRunAfterSnapshot is the pure decision the worker consults: RUN only
// when the snapshot posted, both dirs resolved, the dirs match, and the tab is
// still valid -- workerWasBusy is informational and never cancels on its own.
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
    // SnapshotTrust fields:
    // {snapshotPosted, snapshotDirResolved, currentDirResolved,
    //  dirsMatch, tabStillValid, workerWasBusy}.

    // Idle worker, posted, both dirs resolved, dirs match, tab valid -> RUN.
    check(ShouldRunAfterSnapshot(
              SnapshotTrust{true, true, true, true, true, false}),
          "idle worker + posted + dirs resolved + match + tab valid -> run");

    // THE BUG: the worker was busy so the snapshot resolved late, but posted +
    // both dirs resolved + dirs match + tab valid -> there is NO real drift, so
    // it must RUN. Previously the time-based freshness gate cancelled this.
    check(ShouldRunAfterSnapshot(
              SnapshotTrust{true, true, true, true, true, true}),
          "workerWasBusy (late snapshot) but no drift -> run (was false-cancel)");

    // Real same-tab drift: the snapshot dir differs from the current dir -> CANCEL.
    check(!ShouldRunAfterSnapshot(
              SnapshotTrust{true, true, true, false, true, false}),
          "dirs differ (same-tab drift) -> cancel");

    // Tab switched between trigger and worker -> CANCEL.
    check(!ShouldRunAfterSnapshot(
              SnapshotTrust{true, true, true, true, false, false}),
          "tab switched -> cancel");

    // The WM_APP+2 snapshot post failed -> no drift protection -> CANCEL.
    check(!ShouldRunAfterSnapshot(
              SnapshotTrust{false, true, true, true, true, false}),
          "snapshot not posted -> cancel");

    // Post ok but snapshot dir did not resolve -> nothing to compare -> CANCEL.
    check(!ShouldRunAfterSnapshot(
              SnapshotTrust{true, false, true, true, true, false}),
          "snapshot dir unresolved -> cancel");

    // Current view dir could not resolve now -> unverifiable -> CANCEL.
    check(!ShouldRunAfterSnapshot(
              SnapshotTrust{true, true, false, true, true, false}),
          "current dir unresolved -> cancel");

    if (g_fail == 0) {
        printf("snapshot_trust_test: PASS\n");
        return 0;
    }
    printf("snapshot_trust_test: FAIL (%d)\n", g_fail);
    return 1;
}
