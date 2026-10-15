// queue_drain_test.cpp
// Pins the queue-drain decision for #2 (ShouldRunQueuedAction, mirrored from
// the worker's guard in ctrlg-group.wh.cpp).
//
// Bug (#2): the name popup (PromptForFolderName) runs a NESTED GetMessageW loop
// on the worker thread while PerformNewFolderAction is still in flight. The
// "already posted" latch is cleared before the handler runs, so a second
// WM_APP+1 posted during the modal popup queues up and fires a stale duplicate
// action the instant the popup closes. ShouldRunQueuedAction is the guard the
// worker consults before executing a dequeued action: run it only when no
// action is already in progress AND the message carried a captured target.
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
    const TargetHandle kWin = 0xB0B;

    // (QueuedAction fields: {unloading, actionInProgress, target}.)

    // Nothing running yet + a real captured target -> run it.
    check(ShouldRunQueuedAction(QueuedAction{false, false, kWin}),
          "idle worker + captured target -> run");

    // An action is already in flight (popup open) -> DRAIN the duplicate that
    // was posted during the modal loop instead of running it. This is the core
    // of the "popup works only once / fires twice" bug.
    check(!ShouldRunQueuedAction(QueuedAction{false, true, kWin}),
          "action already in progress -> drain duplicate");

    // Idle worker but the message carried no captured target (stale/legacy
    // post) -> drain, don't act on an unknown window.
    check(!ShouldRunQueuedAction(QueuedAction{false, false, 0}),
          "no captured target -> drain");

    // Both bad -> still drain.
    check(!ShouldRunQueuedAction(QueuedAction{false, true, 0}),
          "in progress AND no target -> drain");

    if (g_fail == 0) {
        printf("queue_drain_test: PASS\n");
        return 0;
    }
    printf("queue_drain_test: FAIL (%d)\n", g_fail);
    return 1;
}
