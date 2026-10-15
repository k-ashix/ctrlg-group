// list_scroll_logic.h
//
// PURE scroll model for the Ctrl+G "Selected Items" list (waht to create.md #1):
// a fixed-height viewport showing ~4-6 rows, a vertical scrollbar only when the
// selection overflows, and keyboard + wheel scrolling (Up/Down, Home/End,
// PageUp/PageDown, mouse wheel). Mirrored from the mod so it can be unit tested
// with plain g++ (no Win32). The mod keeps a byte-for-byte copy and drives its
// custom-drawn list + thumb from these functions.
//
// The model is ROW-QUANTIZED: state is a top-row index, and pixel geometry
// (thumb) is derived. Rows are uniform height, so this is exact and trivially
// testable.
//
#pragma once

namespace listscroll {

// Whole rows that fit in a viewport of `viewportH` px at `rowH` px per row.
inline int VisibleRows(int viewportH, int rowH) {
    if (rowH <= 0) return 0;
    return viewportH / rowH;
}

// Reconcile the per-mode DECLARED row cap with the TRUE capacity of the live
// viewport (fix thse isseue.md Phase 1A): never show MORE rows than physically
// fit (so the last row is never silently clipped), never exceed the declared
// cap, and fall back to the declared cap when the viewport is not yet
// measurable (height or rowH is 0) so the list never collapses to zero rows.
inline int EffectiveVisibleRows(int declaredCap, int viewportH, int rowH) {
    const int phys = VisibleRows(viewportH, rowH);
    if (phys <= 0) return declaredCap;
    return phys < declaredCap ? phys : declaredCap;
}

// fix thse isseue.md v0.12.69 spec section 2 (P0 dead space): when the popup is
// USER-SIZED (maximized or manually resized), the List viewport should show as
// many WHOLE rows as PHYSICALLY FIT -- filling the enlarged Selected Items card
// instead of leaving a blank tail below the compact 4-row / expanded 9-row cap.
// When NOT user-sized this is byte-identical to EffectiveVisibleRows (the
// declared compact/expanded cap is preserved). An unmeasured viewport (height
// or rowH 0) still falls back to the declared cap so the list never collapses.
inline int EffectiveVisibleRowsResizable(int declaredCap, int viewportH,
                                         int rowH, bool userSized) {
    const int phys = VisibleRows(viewportH, rowH);
    if (phys <= 0) return declaredCap;        // not yet measurable
    if (userSized) return phys;               // fill the enlarged viewport (no cap)
    return phys < declaredCap ? phys : declaredCap;
}

// Highest valid top-row index (so the last page is full). 0 when everything fits.
inline int MaxTopRow(int itemCount, int visibleRows) {
    int m = itemCount - visibleRows;
    return m > 0 ? m : 0;
}

// True only when the list overflows its viewport (scrollbar shown).
inline bool NeedsScrollbar(int itemCount, int visibleRows) {
    return itemCount > visibleRows;
}

// Clamp a top-row index into [0, maxTop].
inline int ClampTopRow(int topRow, int maxTop) {
    if (topRow < 0) return 0;
    if (topRow > maxTop) return maxTop;
    return topRow;
}

enum class Key { Up, Down, Home, End, PageUp, PageDown };

// New top-row after a navigation key, clamped to valid range.
inline int OnKey(Key k, int topRow, int itemCount, int visibleRows) {
    const int maxTop = MaxTopRow(itemCount, visibleRows);
    const int page = visibleRows > 0 ? visibleRows : 1;
    switch (k) {
        case Key::Up:       topRow -= 1; break;
        case Key::Down:     topRow += 1; break;
        case Key::PageUp:   topRow -= page; break;
        case Key::PageDown: topRow += page; break;
        case Key::Home:     topRow = 0; break;
        case Key::End:      topRow = maxTop; break;
    }
    return ClampTopRow(topRow, maxTop);
}

// New top-row after a mouse-wheel event. wheelDelta is in WHEEL_DELTA units
// (120 per notch); positive = wheel forward = show earlier rows.
inline int OnWheel(int topRow, int wheelDelta, int itemCount, int visibleRows,
                   int rowsPerNotch) {
    const int maxTop = MaxTopRow(itemCount, visibleRows);
    const int notches = wheelDelta / 120;
    topRow -= notches * rowsPerNotch;
    return ClampTopRow(topRow, maxTop);
}

// The half-open range of item indices currently visible: [first, first+count).
struct Range { int first; int count; };
inline Range VisibleRange(int topRow, int itemCount, int visibleRows) {
    const int maxTop = MaxTopRow(itemCount, visibleRows);
    topRow = ClampTopRow(topRow, maxTop);
    int remaining = itemCount - topRow;
    int count = remaining < visibleRows ? remaining : visibleRows;
    if (count < 0) count = 0;
    return Range{topRow, count};
}

// Scrollbar thumb geometry inside a vertical track of height trackH px.
struct Thumb { int y; int h; };
inline Thumb ThumbRect(int topRow, int itemCount, int visibleRows, int trackH,
                       int minThumbH) {
    if (itemCount <= visibleRows || itemCount <= 0) {
        return Thumb{0, trackH};   // no overflow: caller hides it or fills track
    }
    int h = static_cast<int>(
        static_cast<long long>(trackH) * visibleRows / itemCount);
    if (h < minThumbH) h = minThumbH;
    if (h > trackH) h = trackH;
    const int maxTop = MaxTopRow(itemCount, visibleRows);
    topRow = ClampTopRow(topRow, maxTop);
    int y = 0;
    if (maxTop > 0) {
        y = static_cast<int>(
            static_cast<long long>(trackH - h) * topRow / maxTop);
    }
    return Thumb{y, h};
}

// ---- Visible scrollbar geometry (fix thse isseu e.md #4) ----
// The list reserves a fixed-width gutter column on its RIGHT for a visible
// scrollbar track; rows must not draw into that gutter. The thumb (ThumbRect)
// is positioned inside the track. These are 96-dpi design widths; the mod
// applies its own DPI scale. The gutter is reserved ONLY when the list
// overflows -- when everything fits there is no bar and rows use the full width.
// fix thse isseue.md Phase B: a more substantial thumb. The gutter is 14 design
// px and the thumb insets 1px each side -> a ~12px visible thumb at 96 DPI
// (spec: 10-12px), still rounded via the shared AA layer. kMetaScrollGap keeps
// ~8px breathing room between the rightmost size metadata and the gutter.
inline constexpr int kScrollbarW = 14;    // gutter/track width (design px)
inline constexpr int kScrollbarPad = 1;   // inset of the thumb inside the gutter
inline constexpr int kMetaScrollGap = 8;  // breathing room: size text -> gutter

// A simple design-space rectangle (left/top/right/bottom in px).
struct Box { int left; int top; int right; int bottom; };

// The scrollbar TRACK rect inside a list viewport [left,top,right,bottom], given
// the gutter width `barW`. Occupies the right edge, full viewport height.
inline Box TrackRect(int left, int top, int right, int bottom, int barW) {
    return Box{right - barW, top, right, bottom};
}

// The width available for row CONTENT once the gutter is reserved. When the
// list does not overflow, the full width is returned (no gutter needed).
inline int ContentWidth(int viewportW, int barW, bool needsScrollbar) {
    return needsScrollbar ? (viewportW - barW) : viewportW;
}

// ---- Input routing (waht to create.md #3 / #4 / #12) ----
// The Selected Items viewport must not steal input that belongs elsewhere:
//   * The mouse wheel scrolls the list ONLY when the cursor is over the list
//     region -- hovering the name field, the buttons or the Copy/Move control
//     must not scroll the list.
//   * The navigation keys (Up/Down/Home/End/PageUp/PageDown) scroll the list
//     ONLY when no interactive control owns focus. When the EDIT, a button, or
//     the Copy/Move toggle has focus, those keys belong to normal caret / dialog
//     navigation and must fall through unchanged.
inline bool WheelScrollsList(bool cursorOverList) {
    return cursorOverList;
}

inline bool KeysScrollList(bool focusOnEdit, bool focusOnButton,
                           bool focusOnToggle) {
    return !focusOnEdit && !focusOnButton && !focusOnToggle;
}

}  // namespace listscroll
