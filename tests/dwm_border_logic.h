// dwm_border_logic.h
//
// PURE DWM border-colour decision (fix thse isseue.md P0 #5), mirrored into
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build (no Win32 runtime). The mod keeps a byte-for-byte copy and calls
// DwmSetWindowAttribute with these values.
//
// ApplyWindowPolish() already sets immersive dark mode, rounded DWM corners and
// the backdrop. It used to set DWMWA_BORDER_COLOR to the popup's OWN panel
// surface colour, but a surface-matched border STILL left a faint ~1-2px
// perimeter that tracked the light/dark theme (the outline the issue reports).
// fix thse isseue.md #4: set the border to the documented sentinel
// DWMWA_COLOR_NONE instead, so the compositor draws NO border at all -- a
// genuinely transparent frame, independent of theme. Only the custom borderless
// Group popup gets it; the native-caption New Folder popup keeps the system
// default (unchanged).
//
#pragma once

namespace dwmborder {

// DWMWA_BORDER_COLOR (dwmapi.h). A COLORREF attribute: setting it to a real
// colour overrides the default system border; setting it to DWMWA_COLOR_NONE
// removes the border entirely.
inline constexpr int kAttrBorderColor = 34;

// DWMWA_COLOR_NONE (dwmapi.h): the documented COLORREF sentinel that tells DWM
// to draw NO visible border for the window. Passing this removes the stray
// theme-coloured perimeter completely -- the "transparent" border the issue
// asks for.
inline constexpr unsigned long kColorNone = 0xFFFFFFFEUL;

// The border is now TRANSPARENT (no perimeter) -- deliberately NOT the system
// default and NOT the panel surface colour (which still showed a faint edge).
enum class Source { None };
inline Source BorderSource() { return Source::None; }
inline bool IsTransparent() { return BorderSource() == Source::None; }
// Kept so existing call-sites/tests can assert the border no longer tracks the
// (theme-dependent) panel surface.
inline bool MatchesSurface() { return false; }

// Only the custom borderless (WS_POPUP) Group popup gets a deliberate border
// policy; the native-caption New Folder popup is left untouched.
inline bool AppliesTo(bool isGroup) { return isGroup; }

// fix thse isseue.md P0 #2: DwmSetWindowAttribute(DWMWA_BORDER_COLOR) is
// best-effort -- older Win10 builds (pre-22000) reject it outright. The caller
// must CHECK the returned HRESULT and record acceptance, NEVER assume
// DWMWA_COLOR_NONE took effect. This mirrors the SUCCEEDED(hr) macro: an HRESULT
// has its high (severity) bit clear on success and set on failure.
inline bool BorderColorAccepted(unsigned long hr) {
    return (hr & 0x80000000UL) == 0UL;
}

}  // namespace dwmborder
