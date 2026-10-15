// thumb_lifecycle_test.cpp
// Pins fix thse isseue.md P1 "Thumbnail worker can outlive its popup state"
// (lines 8429-8552, 8509-8516): the PURE lifecycle decisions behind the
// IShellItemImageFactory worker --
//   * EnsureThumbThread publishes thumbInit ONLY when BOTH the wake event and
//     the worker thread were created (a failed init stays retryable);
//   * StopThumbThread releases thread-owned resources (critical section, cached
//     bitmaps) ONLY after the worker has CONFIRMED it exited;
//   * a thumbnail bitmap is accepted ONLY when GetObject succeeded with a
//     positive width and height, otherwise it is dropped (icon fallback).
// The live Win32 glue in ctrlg-group.wh.cpp (namespace thumblife) mirrors these.
// Self-contained: own main(), exit 0 = pass.

#include "thumb_lifecycle_logic.h"

#include <cstdio>

using namespace thumblife;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- init publishing: both creations must succeed ----
    check(ShouldMarkInitialized(true, true),   "init done only when event+thread both created");
    check(!ShouldMarkInitialized(true, false), "thread-create failure -> NOT initialized");
    check(!ShouldMarkInitialized(false, true), "event-create failure -> NOT initialized");
    check(!ShouldMarkInitialized(false, false),"both failed -> NOT initialized");

    // ---- a failed init must remain retryable on a later paint ----
    check(CanRetryInit(false), "failed init (thumbInit=false) can be retried");
    check(!CanRetryInit(true),  "successful init is never re-created");

    // ---- teardown ordering: free only after CONFIRMED worker exit ----
    check(SafeToReleaseAfterWait(true),   "confirmed thread exit -> safe to release CS + cache");
    check(!SafeToReleaseAfterWait(false), "timed-out wait -> must NOT release (use-after-free)");

    // ---- bitmap metadata validation ----
    check(IsValidThumbnailMetadata(true, 64, 64),  "valid bitmap (GetObject ok, +w, +h) accepted");
    check(!IsValidThumbnailMetadata(false, 64, 64),"GetObject failure rejected");
    check(!IsValidThumbnailMetadata(true, 0, 64),  "zero width rejected");
    check(!IsValidThumbnailMetadata(true, 64, 0),  "zero height rejected");
    check(!IsValidThumbnailMetadata(true, -1, 64), "negative width rejected");

    if (g_fail) { printf("thumb_lifecycle: %d check(s) FAILED\n", g_fail); return 1; }
    printf("thumb_lifecycle: all checks passed\n");
    return 0;
}
