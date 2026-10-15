// semantic_icon_logic.h
//
// PURE semantic-section-icon model for fix thse isseue.md v0.12.57 (Semantic
// Section Icon pass). The five popup section headings -- Selected items /
// Destination / Folder name / Status / Action -- must stop using the mod's
// hand-drawn GDI glyph inside a coloured rounded tile and instead render a
// WINDOWS-NATIVE icon:
//
//   * Windows 11  -> Segoe Fluent Icons   (Provider::FluentFont)
//   * Windows 10  -> Segoe MDL2 Assets    (Provider::Mdl2Font)   [fallback]
//   * no icon font-> Shell stock icon     (Provider::ShellStock) [safe fallback]
//                    drawn via SHGetStockIconInfo(SIID_*).
//
// Segoe Fluent Icons and Segoe MDL2 Assets share the SAME Private-Use-Area code
// points for these concepts, so ONE glyph table serves both fonts; only the
// chosen font face differs. This header is the byte-for-byte mirror of namespace
// semanticicon in ctrlg-group.wh.cpp. It makes ONLY the pure
// provider/mapping decision -- the HDC painting (DrawTextW with the icon font,
// or DrawIconEx for the stock icon) lives in the mod and is pinned by source
// invariants. The SIID_* numeric mirrors are tied to the real ShellApi.h values
// by static_asserts in the mod.
//
#pragma once

#include <cstdint>

namespace semanticicon {

// The five popup section headings that carry a semantic icon.
enum class Section { Selected, Destination, Name, Status, Action };

// Where the painted icon comes from. Fluent (Win11) is preferred, MDL2 (Win10)
// is the font fallback, and the Shell stock icon is the last-resort fallback
// when NEITHER icon font is installed (so a heading never shows a tofu box).
enum class Provider { FluentFont, Mdl2Font, ShellStock };

// A resolved icon source. For the two font providers `id` is a PUA glyph code
// point; for ShellStock `id` is a SHSTOCKICONID (SIID_*) value.
struct Spec {
    Provider provider;
    uint32_t id;
};

// --- Segoe Fluent Icons / Segoe MDL2 Assets shared PUA code points ----------
// List (Selected), Folder (Destination), Rename (Folder name), Info (Status),
// MoveToFolder (Action). These are standard, Microsoft-documented symbols -- we
// do NOT invent custom artwork.
inline constexpr uint32_t kGlyphList         = 0xE8FDu;  // List
inline constexpr uint32_t kGlyphFolder       = 0xE8B7u;  // Folder
inline constexpr uint32_t kGlyphRename       = 0xE8ACu;  // Rename / Edit
inline constexpr uint32_t kGlyphInfo         = 0xE946u;  // Info
inline constexpr uint32_t kGlyphMoveToFolder = 0xE8DEu;  // MoveToFolder

// --- SHSTOCKICONID (ShellApi.h) values for the no-icon-font fallback --------
inline constexpr uint32_t kStockStack  = 55u;  // SIID_STACK
inline constexpr uint32_t kStockFolder = 3u;   // SIID_FOLDER
inline constexpr uint32_t kStockRename = 83u;  // SIID_RENAME
inline constexpr uint32_t kStockInfo   = 79u;  // SIID_INFO

// The icon-font glyph for a section (identical code point in Fluent & MDL2).
inline uint32_t Glyph(Section s) {
    switch (s) {
        case Section::Selected:    return kGlyphList;
        case Section::Destination: return kGlyphFolder;
        case Section::Name:        return kGlyphRename;
        case Section::Status:      return kGlyphInfo;
        case Section::Action:      return kGlyphMoveToFolder;
    }
    return kGlyphInfo;  // defensive (unreachable)
}

// The Shell stock icon for a section when NO icon font is available. Action
// (MoveToFolder) has no exact stock peer, so it falls back to the folder icon.
inline uint32_t StockId(Section s) {
    switch (s) {
        case Section::Selected:    return kStockStack;
        case Section::Destination: return kStockFolder;
        case Section::Name:        return kStockRename;
        case Section::Status:      return kStockInfo;
        case Section::Action:      return kStockFolder;
    }
    return kStockFolder;  // defensive (unreachable)
}

// Preferred provider: Segoe Fluent Icons on Windows 11, Segoe MDL2 Assets on
// Windows 10. The glyph code point is the same either way.
inline Spec Resolve(Section s, bool fluentAvailable) {
    return Spec{ fluentAvailable ? Provider::FluentFont : Provider::Mdl2Font,
                 Glyph(s) };
}

// Full resolution with the safe fallback: when NEITHER icon font is installed
// (no Segoe Fluent Icons AND no Segoe MDL2 Assets) drop to a real Shell stock
// icon so a heading never paints a missing-glyph box.
inline Spec ResolveWithFallback(Section s, bool fluentAvailable,
                                bool anyIconFontAvailable) {
    if (!anyIconFontAvailable) return Spec{ Provider::ShellStock, StockId(s) };
    return Resolve(s, fluentAvailable);
}

}  // namespace semanticicon
