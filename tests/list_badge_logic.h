// list_badge_logic.h
//
// PURE extension-BADGE geometry for the owner-drawn Selected Items list (fix
// thse isseue.md #5), mirrored into ctrlg-group.wh.cpp. Each file
// row's extension is shown as a small rounded PILL behind the uppercased ext
// (e.g. [PDF]) instead of plain text, so a row reads as:
//     [icon]  filename ............ [EXT]  size
// This header owns ONLY the arithmetic (does a row get a pill; how wide is it;
// where does it sit) so a plain g++ build can unit-test it; the owner-draw GDI
// paint lives in the mod and is pinned by source invariants.
//
// Coordinate space: unit-agnostic. The renderer passes DEVICE pixels (measured
// text width) together with the DPI-scaled metrics, so the same arithmetic is
// correct at every DPI.
//
#pragma once

#include <string>

namespace listbadge {

// Design-space metrics (96-dpi); the renderer scales these per-DPI.
inline constexpr int kPadX   = 8;   // fix #11: horizontal padding inside the pill (6 -> 8)
inline constexpr int kPadY   = 3;   // fix #11: vertical inset from the row top/bottom (2 -> 3)
inline constexpr int kRadius = 5;   // fix #2 (pass6): pill corner radius 4 -> 5 (soft chip)
inline constexpr int kMinW   = 32;  // fix #11: minimum pill width (24 -> 32) so short exts still read as a chip

struct Box {
    int left = 0, top = 0, right = 0, bottom = 0;
    int w() const { return right - left; }
    int h() const { return bottom - top; }
    bool empty() const { return right <= left || bottom <= top; }
};

// A row shows an extension badge only when there IS an uppercased extension.
// sel::ExtLabel returns "" for folders, extensionless, hidden ("leading dot"),
// and trailing-dot names -- those rows show no pill.
inline bool ShowBadge(const std::wstring& ext) { return !ext.empty(); }

// Pill width for a measured text width + horizontal padding, clamped up so a
// 1-2 character extension still reads as a chip rather than a sliver.
inline int BadgeWidth(int textW, int padX, int minW) {
    const int w = textW + 2 * padX;
    return w < minW ? minW : w;
}

// A right-aligned pill whose right edge is `cellRight`, vertically inset by
// `padY` inside a row [rowTop, rowTop + rowH). All inputs share one coordinate
// space (the renderer passes device px + DPI-scaled metrics).
inline Box BadgeRect(int cellRight, int rowTop, int rowH, int badgeW, int padY) {
    Box b;
    b.right  = cellRight;
    b.left   = cellRight - badgeW;
    b.top    = rowTop + padY;
    b.bottom = rowTop + rowH - padY;
    return b;
}

}  // namespace listbadge
