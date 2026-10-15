// winplace_logic.h
//
// PURE model for fix thse isseue.md Issue 6 -- remembering the Group popup's
// last size / position / maximize state. No Win32. Mirrored byte-for-byte into
// ctrlg-group.wh.cpp (namespace winplace). Single source of truth for:
//   * clamping a remembered window rect onto the current monitor work area
//     (shrink if larger, nudge in-bounds; nearest-monitor fallback is the
//     caller passing that monitor's work rect),
//   * whether a saved placement is at all visible on a given work area,
//   * the rule that the remembered NORMAL rect is captured ONLY from a
//     non-maximized window (so a maximize never overwrites the restore size).
#pragma once

namespace winplace {

struct Rect {
    int left = 0, top = 0, right = 0, bottom = 0;
    int w() const { return right - left; }
    int h() const { return bottom - top; }
};

// Clamp `r` so it is fully inside `work`: shrink it to the work area if it is
// larger, then nudge it in-bounds. A rect that already fits is returned with its
// size preserved and only its origin adjusted if needed.
inline Rect ClampToWork(Rect r, Rect work) {
    int w = r.w();
    int h = r.h();
    const int ww = work.w();
    const int wh = work.h();
    if (w > ww) w = ww;
    if (h > wh) h = wh;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    int x = r.left;
    int y = r.top;
    if (x + w > work.right)  x = work.right - w;
    if (y + h > work.bottom) y = work.bottom - h;
    if (x < work.left) x = work.left;
    if (y < work.top)  y = work.top;
    return Rect{x, y, x + w, y + h};
}

// Any overlap with the work area means the saved window is at least partly
// visible there (so it does not need to be relocated to another monitor).
inline bool IntersectsWork(Rect r, Rect work) {
    return r.left < work.right && r.right > work.left &&
           r.top < work.bottom && r.bottom > work.top;
}

// The remembered NORMAL (restored) rect must be captured ONLY while the window
// is NOT maximized, so maximizing never overwrites the size the popup restores
// to. (The maximize STATE is remembered separately.)
inline bool ShouldCaptureNormal(bool isZoomed) { return !isZoomed; }

}  // namespace winplace
