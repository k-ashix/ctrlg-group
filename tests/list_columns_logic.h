// list_columns_logic.h
//
// PURE column geometry for the owner-drawn Selected Items list (Phase 5/C, fix
// thse isseu e.md #3/#14), mirrored into ctrlg-group.wh.cpp. Each
// row is `name .... [EXT] [SIZE]`: the name column is left-aligned and
// ellipsized, the EXT badge and SIZE are fixed-width and right-aligned. This
// header owns the ONE decision that keeps the columns from overlapping -- how
// much width the name column gets for a given row width. Self-contained so a
// plain g++ build unit-tests it (tests\list_columns_logic_test.cpp).
//
#pragma once

namespace listcol {

// Fixed right-aligned columns + gaps, in 96-dpi design px.
inline constexpr int kExtW     = 46;   // uppercase extension badge, e.g. "PDF"
inline constexpr int kSizeW    = 64;   // human size, e.g. "125 MB"
inline constexpr int kColGap   = 8;    // gap between name and the right columns
inline constexpr int kMinNameW = 60;   // never shrink the name below this

// Width available to the (ellipsized) name column so it never overlaps the
// right-aligned EXT + SIZE columns. Clamped so a very narrow row still shows
// some name rather than a negative width.
inline int NameWidth(int rowW) {
    int n = rowW - kExtW - kSizeW - kColGap;
    return n < kMinNameW ? kMinNameW : n;
}

// Name column width from the ACTUAL live content width, with the right-column
// widths supplied by the caller (already DPI-scaled). The owner-draw list uses
// this so the name column tracks the real compact/expanded row width instead of
// the fixed compact content width (fix thse isseue.md #1): a genuinely wider
// expanded window must fill the extra horizontal space. Same clamp contract as
// NameWidth -- never below minNameWpx. Unit-agnostic: pass design px (then it
// equals NameWidth) or device px with scaled columns.
inline int NameWidthFrom(int contentWpx, int extWpx, int sizeWpx,
                         int colGapPx, int minNameWpx) {
    int n = contentWpx - extWpx - sizeWpx - colGapPx;
    return n < minNameWpx ? minNameWpx : n;
}

}  // namespace listcol
