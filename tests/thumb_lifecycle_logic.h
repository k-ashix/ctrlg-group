// thumb_lifecycle_logic.h
//
// PURE model for the fix thse isseue.md P1 finding "Thumbnail worker can outlive
// its popup state" (ctrlg-group.wh.cpp lines ~8429-8552, ~8509-8516). No Win32.
// Mirrored byte-for-byte into ctrlg-group.wh.cpp (namespace thumblife) and
// consumed by EnsureThumbThread / StopThumbThread / ThumbThreadProc so the
// worker lifecycle decisions are a single, tested source of truth:
//
//   * EnsureThumbThread must publish thumbInit=true ONLY when BOTH the auto-reset
//     wake event and the worker thread were actually created. A partial failure
//     must stay "not initialized" so a later paint RETRIES instead of being
//     permanently stuck on the Shell-icon fallback.
//   * StopThumbThread may free thread-owned resources (the critical section and
//     cached HBITMAPs) ONLY after the worker has CONFIRMED termination. A
//     timed-out wait means the worker may still touch them, so freeing then is a
//     use-after-free.
//   * A thumbnail HBITMAP is usable ONLY when GetObject succeeded and reported a
//     positive width and height; otherwise the result is dropped and the caller
//     keeps the icon fallback.
#pragma once

namespace thumblife {

// EnsureThumbThread: mark initialization complete only when every handle the
// worker depends on exists.
inline bool ShouldMarkInitialized(bool eventCreated, bool threadCreated) {
    return eventCreated && threadCreated;
}

// A later EnsureThumbThread call may retry creation only while init is not done.
inline bool CanRetryInit(bool thumbInit) {
    return !thumbInit;
}

// StopThumbThread: releasing the critical section / cached bitmaps is safe only
// once the worker thread has confirmed it exited.
inline bool SafeToReleaseAfterWait(bool threadConfirmedExited) {
    return threadConfirmedExited;
}

// ThumbThreadProc: accept an extracted bitmap only with valid, positive metadata.
inline bool IsValidThumbnailMetadata(bool getObjectOk, int w, int h) {
    return getObjectOk && w > 0 && h > 0;
}

}  // namespace thumblife
