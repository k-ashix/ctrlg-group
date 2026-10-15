// icon_logic.h
//
// PURE icon-glyph model for the Ctrl+G popup (fix thse isseu e.md #3 / #10 / #13):
// a SMALL, CONSISTENT vector-glyph layer -- NOT emoji -- used for the section
// titles (Selected Items / Destination / Folder name / Action / Status) and for
// each Selected Items row (folder vs file). Mirrored byte-for-byte into
// ctrlg-group.wh.cpp; the mod draws these codepoints with the UI
// font at one consistent size/baseline. Keeping the mapping here means the
// glyph choices are unit-tested and cannot silently drift.
//
// Glyphs are geometric marks available in common UI fonts (Segoe UI etc.):
//   folder  U+1F5C0 is an emoji -> AVOID. We use U+25A3 (filled square in square)
//           for a folder-like mark and U+25A1 (empty square) for a file, both
//           plain geometric shapes, so the layer stays emoji-free and renders
//           identically light/dark.
//
#pragma once

namespace icons {

// Leading row-icon column (96-dpi design px): width + gap before the name.
inline constexpr int kRowIconW   = 16;
inline constexpr int kRowIconGap = 8;   // fix #1: icon -> text gap is 8

// Section identities that get a title glyph.
enum class Section { Selected, Destination, Name, Action, Status };

// Section title glyph (geometric, never emoji). Distinct per section so the
// hierarchy reads at a glance.
inline wchar_t SectionGlyph(Section s) {
    switch (s) {
        case Section::Selected:    return L'\u2630';  // trigram / stacked lines (list)
        case Section::Destination: return L'\u25A3';  // square-in-square (folder-ish)
        case Section::Name:        return L'\u270E';  // lower-right pencil (rename)
        case Section::Action:      return L'\u21C4';  // left-right arrows (move/copy)
        case Section::Status:      return L'\u25CB';  // ring (matches status Ready glyph)
    }
    return L'\u2022';  // defensive: plain bullet (unreachable)
}

// Per-row glyph: a folder mark for folders, a file mark otherwise. One glyph
// each -- we deliberately do NOT fake per-extension icons (we have no icon font
// here); the EXT badge already carries the type, the icon carries folder/file.
inline wchar_t RowGlyph(bool isFolder) {
    return isFolder ? L'\u25A3'   // square-in-square = folder
                    : L'\u25A1';  // empty square = file
}

// True when every section/row glyph is a non-emoji geometric codepoint
// (BMP, < U+1F000). A guard so a future edit can't slip an emoji in.
inline bool IsGeometric(wchar_t g) { return g != 0 && g < 0xF000; }

// Consistent icon stroke width (fix thse isseue.md #6): ONE rule for every icon
// instead of the old per-icon box-size threshold cliff that made some marks
// look heavier/lighter than others. boxPx is the icon's DPI-scaled box (min w/h,
// device px), so the stroke tracks DPI uniformly across ALL icons: ~1px around
// 96 dpi, 2px once the box is clearly larger (>=150%), scaling up higher.
// Floored at 1 so a tiny box still draws.
inline int IconStrokeWidth(int boxPx) {
    int w = (boxPx + 8) / 16;
    return w < 1 ? 1 : w;
}

}  // namespace icons
