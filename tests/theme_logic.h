// theme_logic.h
//
// PURE mirror of the popup theme palette in explorer-ctrlfn-newfolder.wh.cpp
// (ResolveThemePalette). Colours are packed as 0x00BBGGRR to match the Win32
// COLORREF layout produced by RGB(r, g, b), so the numeric values here are
// IDENTICAL to the mod's -- editing a theme colour means editing it in both
// places (the mod handler reads the mod copy; these tests read this copy).
//
// No <windows.h>, so it compiles in the plain g++ test harness. This is the
// single source of truth for the light/dark palette under test.
//
#pragma once

#include <cstdint>

// A colour packed as 0x00BBGGRR -- same layout as a Win32 COLORREF.
using Color = uint32_t;

// Mirror of the Win32 RGB(r, g, b) macro: r | (g << 8) | (b << 16).
inline constexpr Color RGB32(int r, int g, int b) {
    return (Color)((r & 0xFF) | ((g & 0xFF) << 8) | ((b & 0xFF) << 16));
}

inline constexpr int ColR(Color c) { return (int)(c & 0xFF); }
inline constexpr int ColG(Color c) { return (int)((c >> 8) & 0xFF); }
inline constexpr int ColB(Color c) { return (int)((c >> 16) & 0xFF); }

// Perceived luminance (Rec. 601 weights), 0..255. Tests use it to assert a
// real contrast gap between a foreground and its background.
inline int Luminance(Color c) {
    return (299 * ColR(c) + 587 * ColG(c) + 114 * ColB(c)) / 1000;
}

// Every colour the popup uses, for one Windows theme.
struct ThemePalette {
    Color panelBg;           // window / panel background
    Color panelText;         // label text on the panel
    Color editBg;            // edit-box background
    Color editText;          // edit-box text
    Color createFill;        // Create button fill (accent)
    Color createFillPressed; // Create button fill while pressed
    Color createText;        // Create button caption
    Color createBorder;      // Create button border
    Color cancelFill;        // Cancel button fill
    Color cancelFillPressed; // Cancel button fill while pressed
    Color cancelText;        // Cancel button caption
    Color cancelBorder;      // Cancel button border
};

// ---- Design colour tokens ----
// Mirror of the mod's token set (same names, same values). ResolveThemePalette
// maps each slot to a token; no palette slot is a bare RGB32() literal.
inline constexpr Color kColWhite            = RGB32(255, 255, 255);
// Accent (Create) -- shared across both themes.
inline constexpr Color kColAccent           = RGB32(0, 95, 184);   // #005FB8
inline constexpr Color kColAccentPressed    = RGB32(0, 78, 152);
// Light-theme neutrals.
inline constexpr Color kColSurfaceLight     = RGB32(243, 243, 243);
inline constexpr Color kColTextLight        = RGB32(26, 26, 26);
inline constexpr Color kColInputLight       = RGB32(255, 255, 255);
inline constexpr Color kColInputTextLight   = RGB32(20, 20, 20);
// Dark-theme neutrals.
inline constexpr Color kColSurfaceDark      = RGB32(32, 32, 32);
inline constexpr Color kColTextDark         = RGB32(240, 240, 240);
inline constexpr Color kColInputDark        = RGB32(84, 84, 84);    // #545454
inline constexpr Color kColInputTextDark    = RGB32(240, 240, 240);
// Danger (Cancel) reds -- per theme.
inline constexpr Color kColDangerLight        = RGB32(196, 43, 28); // #C42B1C
inline constexpr Color kColDangerLightPressed = RGB32(176, 34, 22);
inline constexpr Color kColDangerLightBorder  = RGB32(150, 32, 20);
inline constexpr Color kColDangerDark         = RGB32(160, 42, 34);
inline constexpr Color kColDangerDarkPressed  = RGB32(130, 34, 28);
inline constexpr Color kColDangerDarkBorder   = RGB32(110, 28, 22);

// Resolve the full palette for the active theme. MUST stay byte-for-byte in
// sync with ResolveThemePalette() in explorer-ctrlfn-newfolder.wh.cpp.
inline ThemePalette ResolveThemePalette(bool dark) {
    ThemePalette p = {};
    // Create = the muted accent blue in BOTH themes (pressed darker); white
    // caption. Every assignment below is a token, not a raw colour.
    p.createFill = kColAccent;
    p.createFillPressed = kColAccentPressed;
    p.createText = kColWhite;
    p.createBorder = kColAccent;
    if (dark) {
        p.panelBg = kColSurfaceDark;
        p.panelText = kColTextDark;
        p.editBg = kColInputDark;
        p.editText = kColInputTextDark;
        p.cancelFill = kColDangerDark;
        p.cancelFillPressed = kColDangerDarkPressed;
        p.cancelText = kColWhite;
        p.cancelBorder = kColDangerDarkBorder;
    } else {
        p.panelBg = kColSurfaceLight;
        p.panelText = kColTextLight;
        p.editBg = kColInputLight;
        p.editText = kColInputTextLight;
        p.cancelFill = kColDangerLight;
        p.cancelFillPressed = kColDangerLightPressed;
        p.cancelText = kColWhite;
        p.cancelBorder = kColDangerLightBorder;
    }
    return p;
}

// ---- Panel background paint decision (mirror of the WM_ERASEBKGND / glass
// logic in explorer-ctrlfn-newfolder.wh.cpp) ----
//
// Bug: when the popup requested the acrylic backdrop (glass) the mod skipped
// filling the panel entirely and returned 1, trusting DWM to composite the
// acrylic. On machines where the acrylic backdrop does NOT actually render
// (transparency effects off, DWM rejects DWMSBT_TRANSIENTWINDOW, remote
// session, ...) nothing was painted and the panel showed through WHITE in dark
// mode instead of the themed #202020.
//
// Rule: only skip the solid fill when the acrylic backdrop is CONFIRMED to be
// compositing. Requesting glass is not the same as glass working. When it is
// not confirmed we must paint the solid themed panel brush -- acrylic (when it
// does work) simply composites over that fill, so painting is always safe.
inline bool ShouldPaintPanelBackground(bool glassRequested,
                                       bool acrylicConfirmed) {
    if (glassRequested && acrylicConfirmed) {
        return false;  // real acrylic will show through; skip the solid fill
    }
    return true;  // paint the solid themed panel (never leave it unpainted)
}
