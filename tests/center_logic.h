// center_logic.h
//
// PURE center-preserving resize geometry (fix thse isseue.md #1), mirrored from
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build (no Win32 runtime). The popup's ApplyPromptLayout() used to resize with
// SWP_NOMOVE, which pins the TOP-LEFT and makes a compact<->expanded (or DPI)
// resize grow only to the right/down. The target UI grows about the window
// CENTER. This header owns that decision:
//   * CenteredTopLeft: new top-left that keeps the current center fixed;
//   * ClampToWorkArea: pull the rectangle back so the whole window stays inside
//     the monitor work area (a window larger than the work area pins to origin);
//   * CenteredResize: the two composed, which is what the mod calls.
//
// Keep byte-for-byte in lockstep with the mod's namespace center. The Win32
// wiring (GetWindowRect + MonitorFromWindow + no SWP_NOMOVE + the resize log) is
// pinned by a source invariant in scripts/regression_check.ps1.
#pragma once

namespace center {

struct Point { int x = 0; int y = 0; };
struct Size  { int cx = 0; int cy = 0; };
struct Rect {
    int left = 0, top = 0, right = 0, bottom = 0;
    int width()  const { return right - left; }
    int height() const { return bottom - top; }
};

// New top-left that keeps `old`'s center fixed while the size becomes `outer`.
inline Point CenteredTopLeft(const Rect& old, const Size& outer) {
    const int cx = (old.left + old.right) / 2;
    const int cy = (old.top + old.bottom) / 2;
    return Point{cx - outer.cx / 2, cy - outer.cy / 2};
}

// Clamp a window of `outer` size at top-left `tl` so it stays fully inside the
// work area. Fit the right/bottom edges first, then the left/top edges -- the
// left/top pass also handles a window that is LARGER than the work area (it
// pins to the work-area origin on that axis instead of going negative).
inline Point ClampToWorkArea(Point tl, const Size& outer, const Rect& work) {
    int left = tl.x;
    int top  = tl.y;
    if (left + outer.cx > work.right)  left = work.right  - outer.cx;
    if (top  + outer.cy > work.bottom) top  = work.bottom - outer.cy;
    if (left < work.left) left = work.left;
    if (top  < work.top)  top  = work.top;
    return Point{left, top};
}

// Center-preserving top-left, clamped to the work area. The mod calls this.
inline Point CenteredResize(const Rect& old, const Size& outer,
                            const Rect& work) {
    return ClampToWorkArea(CenteredTopLeft(old, outer), outer, work);
}

}  // namespace center
