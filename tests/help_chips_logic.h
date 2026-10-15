// help_chips_logic.h
//
// PURE model for the Navigation Help card's section-navigation chips
// (fix thse isseue.md section 3 + add-on section 3). Mirrored byte-for-byte
// into ctrlg-group.wh.cpp (namespace helpchips). No Win32. Single source of
// truth for:
//   * the four chip labels, which are EXACTLY the four section headings
//     (Navigation / New Folder / Group Creation / Auto Group),
//   * the horizontal chip-strip layout math (rects from measured widths + gap),
//   * hit-testing a click against the chip rects,
//   * horizontal strip scrolling (clip + clamp + ensure-a-chip-is-visible) so
//     the row never wraps or grows the card when it overflows the width,
//   * the active chip for a given vertical body scroll (active-section
//     tracking) and the body scroll target when a chip is clicked.
//
// The chip strip is drawn FIXED under the header while the body scrolls; this
// model owns only the pixel decisions, not the Win32 painting/hit wiring.
#pragma once

#include <vector>

namespace helpchips {

// Four chips; labels are the four section headings verbatim so a chip maps 1:1
// to its infohelp Section line.
inline int Count() { return 4; }
inline const wchar_t* Label(int i) {
    switch (i) {
        case 0: return L"Navigation";
        case 1: return L"New Folder";
        case 2: return L"Group Creation";
        case 3: return L"Auto Group";
    }
    return L"";
}

struct Rect { int left; int right; };

// Lay the chips left-to-right from startX given each chip's pixel width and one
// uniform gap. Returns one [left,right) rect per chip, in strip CONTENT coords.
inline std::vector<Rect> Layout(const std::vector<int>& widths, int gap,
                                int startX) {
    std::vector<Rect> out;
    int x = startX;
    for (size_t i = 0; i < widths.size(); ++i) {
        out.push_back(Rect{x, x + widths[i]});
        x += widths[i] + gap;
    }
    return out;
}

// Total strip width (last chip's right edge minus startX==0): sum of widths
// plus the gaps between them.
inline int TotalWidth(const std::vector<int>& widths, int gap) {
    int w = 0;
    for (size_t i = 0; i < widths.size(); ++i) {
        w += widths[i];
        if (i + 1 < widths.size()) w += gap;
    }
    return w;
}

// Hit-test a point X -- already in strip CONTENT coords (the view X plus the
// horizontal scroll offset) -- against the chip rects. Returns the chip index
// or -1 (a gap / outside).
inline int HitTest(const std::vector<Rect>& rects, int x) {
    for (size_t i = 0; i < rects.size(); ++i)
        if (x >= rects[i].left && x < rects[i].right) return static_cast<int>(i);
    return -1;
}

// Largest horizontal scroll so the strip is never scrolled past its content.
inline int MaxScrollX(int stripW, int visibleW) {
    const int m = stripW - visibleW;
    return m > 0 ? m : 0;
}

// Clamp a horizontal strip offset into [0, MaxScrollX].
inline int ClampScrollX(int offset, int stripW, int visibleW) {
    const int m = MaxScrollX(stripW, visibleW);
    if (offset < 0) return 0;
    if (offset > m) return m;
    return offset;
}

// Shift the horizontal offset the MINIMUM needed so chip i is fully visible in
// the window [offset, offset+visibleW) over the strip, then clamp. Used after a
// chip is clicked or becomes active so the active chip is never left off-screen
// (keyboard/active-section accessibility).
inline int EnsureVisible(const std::vector<Rect>& rects, int i, int offset,
                         int visibleW, int stripW) {
    if (i < 0 || i >= static_cast<int>(rects.size()))
        return ClampScrollX(offset, stripW, visibleW);
    const int left  = rects[i].left;
    const int right = rects[i].right;
    if (left < offset)                 offset = left;              // off the left
    else if (right > offset + visibleW) offset = right - visibleW;  // off the right
    return ClampScrollX(offset, stripW, visibleW);
}

// The active chip for a given vertical body scroll: the LAST section whose top
// anchor has scrolled to at or above an activation line a third of the way down
// the viewport ("the corresponding section becomes the primary visible
// section"). anchors are the per-section content-Y offsets (ascending).
inline int ActiveSection(const std::vector<int>& anchors, int scrollY,
                         int viewportH) {
    if (anchors.empty()) return 0;
    const int line = scrollY + viewportH / 3;
    int active = 0;
    for (size_t i = 0; i < anchors.size(); ++i)
        if (anchors[i] <= line) active = static_cast<int>(i);
    return active;
}

// Clicking chip i scrolls the body so section i's top sits at the viewport top,
// clamped to the body's own max vertical scroll (never past the end).
inline int ScrollTargetFor(const std::vector<int>& anchors, int i,
                           int maxScrollY) {
    if (i < 0 || i >= static_cast<int>(anchors.size())) return 0;
    int t = anchors[i];
    if (t < 0) t = 0;
    if (t > maxScrollY) t = maxScrollY;
    return t;
}

// Keyboard chip cycling (wraps). n is the chip count.
inline int NextChip(int cur, int n) { return (n <= 0) ? 0 : (cur + 1) % n; }
inline int PrevChip(int cur, int n) { return (n <= 0) ? 0 : (cur - 1 + n) % n; }

}  // namespace helpchips
