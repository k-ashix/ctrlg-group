// caption_glyph_logic.h
//
// PURE caption-button glyph model (fix thse isseue.md C). Decides WHICH Windows
// caption glyph each title-bar button shows and WHICH font provider renders it,
// mirrored from ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build. The mod keeps a byte-for-byte copy and feeds the codepoint to a
// Segoe Fluent Icons / Segoe MDL2 Assets DrawTextW, with a deliberate GEOMETRIC
// fallback when no icon font is installed (never an empty / tofu glyph).
//
// Verified Segoe caption glyphs (Microsoft "Segoe Fluent Icons" / "Segoe MDL2
// Assets"): E921 ChromeMinimize, E922 ChromeMaximize, E923 ChromeRestore,
// E8BB ChromeClose.
//
#pragma once

namespace captionglyph {

enum class Button { Minimize, Expand, Close };

// Font provider for the glyph: Segoe Fluent Icons (Win11) -> Segoe MDL2 Assets
// (Win10) -> a deliberate GEOMETRIC fallback (neither font installed).
enum class Provider { FluentFont, Mdl2Font, Geometric };

// The caption glyph codepoint. The middle (Expand) button is state-dependent:
// compact mode shows Maximize (the popup will expand); expanded mode shows
// Restore (the popup will collapse). Minimize and Close are fixed.
inline unsigned Glyph(Button b, bool expanded) {
    switch (b) {
        case Button::Minimize: return 0xE921u;  // ChromeMinimize
        case Button::Close:    return 0xE8BBu;  // ChromeClose
        case Button::Expand:   return expanded ? 0xE923u   // ChromeRestore
                                               : 0xE922u;  // ChromeMaximize
    }
    return 0xE8BBu;
}

// Resolve the font provider with the SAME fallback order the section icons use:
// prefer Segoe Fluent Icons, else Segoe MDL2 Assets, else a geometric mark.
inline Provider ResolveProvider(bool fluentAvailable, bool anyFontAvailable) {
    if (fluentAvailable)  return Provider::FluentFont;
    if (anyFontAvailable) return Provider::Mdl2Font;
    return Provider::Geometric;
}

// True when a real icon font will render the glyph; false -> geometric fallback.
inline bool UsesFont(Provider p) { return p != Provider::Geometric; }

}  // namespace captionglyph
