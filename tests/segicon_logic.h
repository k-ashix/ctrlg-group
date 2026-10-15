// segicon_logic.h
//
// PURE icon+label layout for the Copy/Move segmented control (fix thse isseue.md
// #9): each segment shows a SMALL leading vector glyph before its caption
// ([icon] Copy / [icon] Move), and the icon+label group is centred as a UNIT
// inside the segment (not the bare caption centred). Byte-for-byte mirror of
// namespace segicon in ctrlg-group.wh.cpp; the GDI paint (which
// glyph is used, and the DrawText) lives in the mod and is pinned by source
// invariants.
//
// All values are 96-dpi design px; the mod scales them per-control via ScaleDpi.
//
#pragma once

namespace segicon {

inline constexpr int kSegIcon    = 14;  // leading glyph box (small; matches section glyphs)
inline constexpr int kSegIconGap = 6;   // glyph -> caption gap

// Width of the centred [icon + gap + caption] group for a caption whose text
// pixel width is textW.
inline int GroupWidth(int textW) { return kSegIcon + kSegIconGap + textW; }

// Left edge of the group, centred in a segment [segLeft, segLeft+segW). Clamped
// so a group wider than the segment still starts at the segment's left edge
// (never a negative inset).
inline int GroupLeft(int segLeft, int segW, int groupW) {
    int x = segLeft + (segW - groupW) / 2;
    return x < segLeft ? segLeft : x;
}

// Icon box left == group left; caption left == past the icon + gap.
inline int IconLeft(int groupLeft) { return groupLeft; }
inline int TextLeft(int groupLeft) { return groupLeft + kSegIcon + kSegIconGap; }

}  // namespace segicon
