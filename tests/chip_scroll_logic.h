// chip_scroll_logic.h
//
// PURE horizontal scroll model for the summary chip strip (fix thse isseue.md
// #4). The strip has a fixed height and scrolls horizontally with the mouse
// wheel (and Shift+wheel / horizontal wheel) while the pointer is over it, plus
// Left/Right/Home/End keyboard navigation that keeps the active chip in view.
// No visible scrollbar is required. Mirrored byte-for-byte into
// ctrlg-group.wh.cpp.
#pragma once

namespace chipscroll {

// Largest horizontal offset: how far the content extends past the viewport.
inline int MaxOffset(int contentW, int viewportW) {
    const int m = contentW - viewportW;
    return m > 0 ? m : 0;
}

// Scrolling is only possible/needed when the chips overflow the viewport.
inline bool NeedsScroll(int contentW, int viewportW) {
    return contentW > viewportW;
}

// Keep an offset within [0, maxOffset].
inline int Clamp(int offset, int maxOffset) {
    if (offset < 0) return 0;
    if (offset > maxOffset) return maxOffset;
    return offset;
}

// Mouse wheel -> horizontal pan. Positive wheelDelta (wheel away from the user)
// pans toward the start (offset 0), matching the vertical list convention.
inline int OnWheel(int offset, int wheelDelta, int maxOffset, int pxPerNotch) {
    const int notches = wheelDelta / 120;
    offset -= notches * pxPerNotch;
    return Clamp(offset, maxOffset);
}

// WM_MOUSEHWHEEL / tilt wheels use the OPPOSITE sign of WM_MOUSEWHEEL (positive
// means pan right). Normalize a horizontal-wheel delta to the OnWheel sign.
inline int WheelDeltaFromHorizontal(int hwheelDelta) { return -hwheelDelta; }

// Smallest offset change that brings [chipLeft, chipRight) fully into view.
inline int BringIntoView(int chipLeft, int chipRight, int offset, int viewportW) {
    if (chipLeft < offset) return chipLeft;
    if (chipRight > offset + viewportW) return chipRight - viewportW;
    return offset;
}

// Keyboard navigation across the chips; returns the clamped active index.
enum class Key { Left, Right, Home, End };
inline int OnKey(Key k, int activeIndex, int chipCount) {
    if (chipCount <= 0) return 0;
    switch (k) {
        case Key::Left:  activeIndex -= 1; break;
        case Key::Right: activeIndex += 1; break;
        case Key::Home:  activeIndex = 0; break;
        case Key::End:   activeIndex = chipCount - 1; break;
    }
    if (activeIndex < 0) return 0;
    if (activeIndex > chipCount - 1) return chipCount - 1;
    return activeIndex;
}

}  // namespace chipscroll
