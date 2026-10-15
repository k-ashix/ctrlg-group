// helpcard_scroll_logic.h
//
// PURE scroll + sizing model for the Navigation Help card
// (fix thse isseue.md "Overflow and Scrollable Help Card"). Mirrored
// byte-for-byte into ctrlg-group.wh.cpp (namespace helpscroll). No Win32, so it
// is unit-testable with plain g++.
//
// The help card is kept COMPACT: it is capped against the current monitor work
// area (CapToWorkArea, DPI-aware because the caller passes device px) and never
// grows without bound as more help content is added. When the content is taller
// than the available content viewport, the content area becomes internally
// scrollable (NeedsScroll) with a theme-aware thumb (ThumbRect); the heading
// band is reserved OUTSIDE the viewport so it stays fixed while the body
// scrolls (ViewportHeight). A mouse wheel over the card scrolls ONLY the card
// and never the Selected Items list or the summary chips (WheelTargetsHelpCard).
//
// The model is PIXEL-based: the caller measures the content/viewport heights
// with the live DC+font (so natural text wrapping is accounted for) and this
// header owns every clamp/overflow/geometry decision.
//
#pragma once

namespace helpscroll {

// Does the content overflow its viewport (so an internal scrollbar is needed)?
// An unmeasured (zero-height) viewport never claims to scroll.
inline bool NeedsScroll(int contentH, int viewportH) {
    return viewportH > 0 && contentH > viewportH;
}

// Largest valid scroll offset in px (0 when everything fits).
inline int MaxScroll(int contentH, int viewportH) {
    const int m = contentH - viewportH;
    return m > 0 ? m : 0;
}

// Clamp a scroll offset into [0, maxScroll].
inline int ClampScroll(int scrollY, int maxScroll) {
    if (scrollY < 0) return 0;
    if (scrollY > maxScroll) return maxScroll;
    return scrollY;
}

// New scroll offset after a mouse-wheel event. wheelDelta is in WHEEL_DELTA
// units (120 per notch); positive = wheel forward = reveal EARLIER content
// (scroll toward the top, i.e. a smaller offset). Clamped to [0, maxScroll].
inline int OnWheel(int scrollY, int wheelDelta, int pxPerNotch, int maxScroll) {
    const int notches = wheelDelta / 120;
    scrollY -= notches * pxPerNotch;
    return ClampScroll(scrollY, maxScroll);
}

// Cap a desired card dimension to the monitor work area minus a margin on BOTH
// sides (so a compact card never touches the screen edges and never enlarges
// past the available space). A degenerate work area (the margin would eat the
// whole dimension) falls back to the raw work extent, so the card is never
// clamped to zero.
inline int CapToWorkArea(int desired, int workExtent, int marginPx) {
    int cap = workExtent - 2 * marginPx;
    if (cap < 1) cap = workExtent;
    return desired > cap ? cap : desired;
}

// Cap a desired card dimension to a FIXED maximum extent (DPI-scaled by the
// caller, so this is pure px). A zero/negative max means "no fixed cap". The
// help card applies this ALONGSIDE CapToWorkArea -- the SMALLER of the two
// always wins -- so the card has a consistent configured maximum size and never
// grows with its content past it; content beyond the resulting viewport scrolls
// INTERNALLY (NeedsScroll) instead of enlarging the card.
inline int CapToMax(int desired, int maxExtent) {
    if (maxExtent > 0 && desired > maxExtent) return maxExtent;
    return desired;
}

// Raise an (already max/work-capped) card dimension to a FIXED minimum so a
// card with little content still reads as a proper card. "Subject to available
// space": the caller passes a minExtent ALREADY clamped to the work area, so
// this never pushes the card off-screen. A zero/negative minimum means "no
// floor".
inline int CapToMin(int desired, int minExtent) {
    if (minExtent > 0 && desired < minExtent) return minExtent;
    return desired;
}

// Height available for the SCROLLABLE content inside a card of total height
// cardH, after reserving the FIXED heading band (heading text + its padding)
// and the content bottom padding. Never negative.
inline int ViewportHeight(int cardH, int headingBandH, int padBottom) {
    const int v = cardH - headingBandH - padBottom;
    return v > 0 ? v : 0;
}

// Vertical scrollbar thumb geometry inside a track of height trackH px. When
// the content fits, the thumb fills the whole track (the caller hides it).
struct Thumb { int y; int h; };
inline Thumb ThumbRect(int scrollY, int contentH, int viewportH, int trackH,
                       int minThumbH) {
    if (contentH <= viewportH || contentH <= 0 || trackH <= 0) {
        return Thumb{0, trackH};
    }
    const long long raw = static_cast<long long>(trackH) * viewportH / contentH;
    int h = static_cast<int>(raw);
    if (h < minThumbH) h = minThumbH;
    if (h > trackH)    h = trackH;
    const int maxScroll = contentH - viewportH;
    const int maxThumbY = trackH - h;
    int y = (maxScroll > 0)
        ? static_cast<int>(static_cast<long long>(scrollY) * maxThumbY / maxScroll)
        : 0;
    if (y < 0)         y = 0;
    if (y > maxThumbY) y = maxThumbY;
    return Thumb{y, h};
}

// Wheel routing: a wheel event while the help card is SHOWN and the pointer is
// over it scrolls the card -- and MUST NOT scroll the Selected Items list or
// pan the summary chips. Both wheel call sites consult this one predicate.
inline bool WheelTargetsHelpCard(bool cardShown, bool pointerOverCard) {
    return cardShown && pointerOverCard;
}

}  // namespace helpscroll
