// dir_safe_test.cpp
// Pins the directory-safety decision for #2 (ShouldActOnDir, mirrored from the
// worker's guard in ctrlg-group.wh.cpp).
//
// #2 (directory race): capturing the Explorer WINDOW + active TAB is still not
// enough. The user can navigate the SAME tab (A -> B) between the trigger and
// the moment the worker resolves the path, so the tab guard (#3) passes yet the
// resolved directory is B, not A. Because the trigger-time dir snapshot is
// resolved on the worker (not truly at keypress time), it too can end up = B if
// the nav happened before the snapshot ran. Either way, if the directory we
// captured differs from the directory live NOW, we cannot trust that we would
// create "where you were browsing" AND drive the inline rename against the
// right live view -- so the safe decision is to CANCEL, create nothing.
//
// This is the pure decision. `capturedDir` is the browse dir snapshot taken for
// the trigger; `currentDir` is the dir the active view resolves to now, on the
// worker. Hardened rule: we ACT only when the snapshot POST succeeded AND the
// snapshot was resolved fresh (close to the keypress -- #2) AND it actually
// resolved a dir (#3) AND that dir still matches `currentDir`. Any other case
// (post failed, stale/late resolve, empty snapshot, empty current dir, or a
// mismatch) CANCELS -- we never fall back to the current dir with no verified
// snapshot. Comparison is case-insensitive (Windows paths).
//
// SnapshotIsFresh(keypressTick, resolveTick, maxMs) is the freshness helper the
// worker uses to compute the snapshotFresh flag; it is pinned here too.
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
    // DirCheck fields: {snapshotExpected, snapshotFresh, capturedDir, currentDir}.
    //   snapshotExpected == the WM_APP+2 snapshot POST succeeded.
    //   snapshotFresh    == the snapshot was RESOLVED close enough to the keypress
    //                       to be trusted as a true trigger-time capture (#2).

    // ---- ShouldActOnDir (unified #2 + #3 rule) ----

    // Fresh, resolved, same dir now as at trigger time -> safe to act.
    check(ShouldActOnDir(DirCheck{true, true, L"C:\\A", L"C:\\A"}),
          "fresh + same dir -> act");

    // The core #2 bug: user navigated the same tab C:\\A -> C:\\B -> cancel.
    check(!ShouldActOnDir(DirCheck{true, true, L"C:\\A", L"C:\\B"}),
          "dir changed (A->B) same tab -> cancel");

    // Case differences on Windows must NOT be treated as a change.
    check(ShouldActOnDir(DirCheck{true, true, L"C:\\Users\\Me", L"c:\\users\\me"}),
          "case-insensitive same dir -> act");

    // #3 (post-failure hole): the WM_APP+2 post failed -> no protection at all
    // -> CANCEL rather than run an unprotected action.
    check(!ShouldActOnDir(DirCheck{false, false, L"", L"C:\\B"}),
          "snapshot post failed -> cancel (no unprotected action)");

    // #2 (true trigger-time capture): post succeeded and the dirs match, but the
    // snapshot resolved too late (worker was busy) to be a keypress-time capture
    // -- it may itself hold a drifted-to dir -> CANCEL even though it "matches".
    check(!ShouldActOnDir(DirCheck{true, false, L"C:\\A", L"C:\\A"}),
          "stale snapshot (resolved too late) -> cancel even if dirs match");

    // #3 (resolution-failure hole): "post ok" != "snapshot ok". Post succeeded
    // and the snapshot is fresh, but the captured dir is EMPTY (ResolveActive-
    // ShellView failed) -> nothing trustworthy to compare -> CANCEL. This is the
    // remaining hole where the action used to proceed on the current dir with no
    // drift check.
    check(!ShouldActOnDir(DirCheck{true, true, L"", L"C:\\B"}),
          "post ok but snapshot did not resolve -> cancel");

    // Fresh, snapshot resolved, but the CURRENT dir could not be resolved ->
    // unverifiable -> CANCEL.
    check(!ShouldActOnDir(DirCheck{true, true, L"C:\\A", L""}),
          "empty current dir -> cancel (unverifiable)");

    // ---- SnapshotIsFresh (the #2 freshness helper) ----

    // Resolved within the window -> fresh.
    check(SnapshotIsFresh(1000, 1000, 250), "same tick -> fresh");
    check(SnapshotIsFresh(1000, 1200, 250), "200ms latency (<=250) -> fresh");
    check(SnapshotIsFresh(1000, 1250, 250), "exactly 250ms latency -> fresh");

    // Resolved too late -> not fresh (worker was busy; possible drift).
    check(!SnapshotIsFresh(1000, 1251, 250), "251ms latency (>250) -> not fresh");
    check(!SnapshotIsFresh(1000, 5000, 250), "4s latency (worker busy) -> not fresh");

    // Never stamped / clock skew -> not fresh (cannot prove freshness).
    check(!SnapshotIsFresh(0, 1200, 250), "keypress tick unset -> not fresh");
    check(!SnapshotIsFresh(1000, 0, 250), "resolve tick unset -> not fresh");
    check(!SnapshotIsFresh(2000, 1000, 250), "resolve before keypress -> not fresh");

    if (g_fail == 0) {
        printf("dir_safe_test: PASS\n");
        return 0;
    }
    printf("dir_safe_test: FAIL (%d)\n", g_fail);
    return 1;
}
