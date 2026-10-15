// scrollbar_autohide_logic.h
//
// PURE auto-hide model for the Selected Items vertical scrollbar
// (fix thse isseue.md Phase B). Mirrored byte-for-byte into ctrlg-group.wh.cpp
// (namespace sbauto). No Win32.
//
// The scrollbar is a display-only indicator (the list is scrolled by wheel and
// keys, there is no draggable thumb), so it auto-hides when idle and reveals
// while the user is interacting:
//   * It is only ever shown when the list actually overflows (needsBar).
//   * It is shown while the pointer is inside the viewport, while a drag is in
//     progress (future hook), and for a short reveal window after the last
//     scroll/interaction -- then it fades using the existing one-shot UI timer
//     (no high-frequency repaint loop).
//   * The GUTTER stays reserved whenever the list overflows, so showing/hiding
//     the thumb never reflows the rows (no layout shift).
//
#pragma once

namespace sbauto {

// Reveal window after the last scroll/interaction before an idle scrollbar
// fades (ms). Scheduled with a single one-shot timer, not a repaint loop.
inline constexpr unsigned kIdleHideMs = 1200;

// Extra pointer hit slop to the LEFT of the visible gutter, so the interaction
// target is a little wider than the (slightly narrower) painted thumb.
inline constexpr int kHitSlopX = 6;

// Still within the reveal window after the last activity tick? A clock that
// appears to go backwards (skew / wrap) keeps the bar visible (fail safe).
inline bool WithinRevealWindow(unsigned long long lastActivityTick,
                               unsigned long long now, unsigned idleMs) {
    if (now < lastActivityTick) return true;
    return (now - lastActivityTick) < idleMs;
}

// The scrollbar is VISIBLE only when the list overflows AND (a drag is in
// progress, OR the pointer is inside the viewport, OR we are still within the
// reveal window). A drag always keeps it visible. No overflow -> never shown.
inline bool Visible(bool needsBar, bool dragging, bool pointerInside,
                    bool withinReveal) {
    if (!needsBar) return false;
    if (dragging) return true;
    return pointerInside || withinReveal;
}

// A pointer x is "on" the scrollbar when it is within the gutter OR within the
// hit slop just left of it (a wider hit target than the painted thumb).
inline bool HitTestX(int px, int gutterLeft, int gutterRight, int hitSlop) {
    return px >= (gutterLeft - hitSlop) && px <= gutterRight;
}

}  // namespace sbauto
