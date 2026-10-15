// chip_toolbar_logic.h
//
// PURE layout model for the summary row: a horizontally scrollable chip strip
// on the left and a reusable action toolbar on the right (fix thse isseue.md
// #4/#5). The toolbar (starting with "Show extensions") stays visible; when the
// strip would get too narrow on one row, the toolbar wraps to a second row so
// the buttons are never clipped. Mirrored byte-for-byte into ctrlg-group.wh.cpp.
#pragma once

#include <vector>
#include <cstddef>

namespace chiptoolbar {

struct Box {
    int left = 0, top = 0, right = 0, bottom = 0;
    int w() const { return right - left; }
    int h() const { return bottom - top; }
};

struct Layout {
    bool wrapped = false;   // true -> toolbar moved to a second row
    int  rows = 1;
    Box  strip;             // chip-strip viewport
    Box  toolbar;           // region holding the action buttons
};

// Lay out the summary row in [left,top,right,bottom]. toolbarW is the total
// width the toolbar buttons need (including their internal gaps), gap is the
// space between the strip and the toolbar, minStripW is the smallest usable
// chip-strip width, and rowH is the height of one summary row. The strip takes
// the remaining width on a single row when it stays >= minStripW; otherwise the
// toolbar wraps below the strip.
inline Layout Compute(int left, int top, int right, int bottom,
                      int toolbarW, int gap, int minStripW, int rowH) {
    Layout out;
    const int singleStripW = (right - left) - toolbarW - gap;
    if (singleStripW >= minStripW) {
        out.wrapped = false;
        out.rows = 1;
        out.strip = Box{left, top, left + singleStripW, bottom};
        out.toolbar = Box{left + singleStripW + gap, top, right, bottom};
    } else {
        out.wrapped = true;
        out.rows = 2;
        out.strip = Box{left, top, right, top + rowH};
        out.toolbar = Box{left, top + rowH, right, top + rowH + rowH};
    }
    return out;
}

// Place buttons left-to-right inside the toolbar box in order (Show extensions
// first, future actions after). Returns one box per width.
inline std::vector<Box> ButtonBoxes(const Box& toolbar,
                                    const std::vector<int>& widths, int gap) {
    std::vector<Box> out;
    int x = toolbar.left;
    for (std::size_t i = 0; i < widths.size(); ++i) {
        out.push_back(Box{x, toolbar.top, x + widths[i], toolbar.bottom});
        x += widths[i] + gap;
    }
    return out;
}

}  // namespace chiptoolbar
