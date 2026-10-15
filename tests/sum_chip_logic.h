// sum_chip_logic.h
//
// PURE geometry model for the Selected Items SUMMARY CHIPS (fix thse isseue.md
// #1). DEDICATED pill design constants + geometry so the chips are real
// metadata pills -- semibold text, X/Y padding, a pill corner radius, and a
// height DERIVED from the measured text metrics -- NOT the per-file extension
// badge (listbadge) constants and NOT the old hardcoded 16px height.
//
// No Win32: every value is computed from text metrics + padding, so it is
// deterministically unit-testable and mirrored byte-for-byte into
// ctrlg-group.wh.cpp.
#pragma once

namespace sumchip {

// 96-DPI base design tokens. The painter DPI-scales each one via ScaleDpi.
inline constexpr int kFontPx    = 12;  // semibold ~600 (UiFontSemibold helper)
inline constexpr int kPadX      = 8;   // horizontal padding per side
inline constexpr int kPadY      = 4;   // vertical padding per side
inline constexpr int kMinRadius = 10;  // minimum corner radius
inline constexpr int kGap       = 6;   // gap between adjacent chips
inline constexpr int kTitleGap  = 6;   // gap between the title row and chip row

// Chip height = measured text cell height + vertical padding on both sides.
// NEVER the old hardcoded 16px. `padY` is the DPI-scaled kPadY.
inline int ChipHeight(int textH, int padY) { return textH + padY * 2; }

// Chip width = measured text width + horizontal padding on both sides.
// `padX` is the DPI-scaled kPadX.
inline int ChipWidth(int textW, int padX) { return textW + padX * 2; }

// Corner radius = a full pill (half the height) but never below the minimum, so
// a chip always reads as a rounded pill at every DPI. `minRadius` is scaled.
inline int ChipRadius(int chipH, int minRadius) {
    const int r = chipH / 2;
    return (r < minRadius) ? minRadius : r;
}

// X of the next chip after one of width `chipW` starting at `x`, with `gap`
// (the scaled kGap) between adjacent chips.
inline int NextChipX(int x, int chipW, int gap) { return x + chipW + gap; }

}  // namespace sumchip
