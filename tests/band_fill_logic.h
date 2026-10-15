// band_fill_logic.h
//
// PURE title-band fill decision for the Ctrl+G Group popup (fix thse isseue.md
// P0 #1). Kept test-only, exactly like theme_logic.h's ShouldPaintPanelBackground
// -- the WM_ERASEBKGND Win32 fill in ctrlg-group.wh.cpp is pinned separately by
// a source invariant (regression_check.ps1), this header pins the DECISION.
//
// THE BUG (RED before the fix): when the DWM frame was extended into the custom
// title band, the mod LEFT the band's pixels untouched. Untouched pixels are
// NOT transparent -- an extended DWM frame needs TRANSPARENT pixel data, and GDI
// supplies that only via a BLACK (0,0,0) fill (Microsoft "Custom Window Frame
// Using DWM"). So the band showed a WHITE strip instead of the Win11 material.
//
// THE FIX: when the backdrop is CONFIRMED, fill the band with the transparent
// (black) brush; otherwise it is part of the solid themed client fill. Either
// way the band is NEVER left unpainted, and the area below is always the panel.
#pragma once

namespace bandfill {

// The brush used to erase a region of the popup client area in WM_ERASEBKGND.
enum class Brush {
    Unpainted,         // the OLD bug: region left untouched (NOT transparent)
    Panel,             // the themed solid surface (#202020 dark / #F3F3F3 light)
    TransparentBlack,  // GDI black -> transparent pixels for the extended frame
};

// Which brush fills the DPI-scaled title band.
// backdropConfirmed == groupBandBackdrop (d->isGroup && d->acrylicActive).
inline Brush TitleBandBrush(bool backdropConfirmed) {
    if (backdropConfirmed) {
        // The frame is extended: hand the band TRANSPARENT (black) pixels so the
        // Win11 system material composites through. NEVER leave it Unpainted.
        return Brush::TransparentBlack;
    }
    // No confirmed backdrop: the band is part of the solid themed client fill.
    return Brush::Panel;
}

// Which brush fills the area BELOW the title band -- always the themed panel so
// the cards, fields and text stay solid and readable in both paint modes.
inline Brush BelowBandBrush(bool /*backdropConfirmed*/) {
    return Brush::Panel;
}

// The title band must ALWAYS receive real pixels -- never Unpainted. This is the
// exact invariant the bug violated.
inline bool TitleBandIsPainted(bool backdropConfirmed) {
    return TitleBandBrush(backdropConfirmed) != Brush::Unpainted;
}

// Clamp the DPI-scaled band height to the client height (defensive; mirrors the
// mod so a band taller than the client never produces a negative below-rect).
inline int ClampBandHeight(int bandH, int clientH) {
    if (bandH < 0) {
        return 0;
    }
    return (bandH < clientH) ? bandH : clientH;
}

}  // namespace bandfill
