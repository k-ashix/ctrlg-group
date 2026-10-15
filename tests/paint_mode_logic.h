// paint_mode_logic.h
//
// PURE background-paint-mode decision for the Ctrl+F+N / Ctrl+G popups (fix thse
// isseue.md P0 #1/#2/#3), mirrored byte-for-byte into ctrlg-group.wh.cpp so it
// can be unit tested with a plain g++ build (no Win32 runtime).
//
// The popup must decide HONESTLY how it paints its own background in
// WM_ERASEBKGND. Two earlier bugs came from ASSUMING a DWM backdrop would
// composite when it actually had not:
//   * Ctrl+F+N (New Folder): WM_ERASEBKGND returned without painting its client
//     area whenever a backdrop was merely REQUESTED/accepted -- but the New
//     Folder popup keeps a native caption and never extends its frame, so
//     nothing composited and the body showed through WHITE in dark mode.
//   * Ctrl+G (Group): the custom title band was left unpainted on the strength
//     of the DwmSetWindowAttribute result alone, IGNORING whether
//     DwmExtendFrameIntoClientArea actually succeeded -- so the band showed a
//     WHITE strip when the frame extension failed.
//
// This namespace is the single source of truth for that decision. The Win32
// wiring (DwmSetWindowAttribute + DwmExtendFrameIntoClientArea + the
// WM_ERASEBKGND FillRect) lives in the mod and is pinned by source invariants.
//
#pragma once

namespace paintmode {

// How the popup paints its own client-area background in WM_ERASEBKGND. The
// name is also the DIAGNOSTIC "backgroundPaintMode" value logged per popup.
enum class PaintMode {
    SolidTheme,    // fill the ENTIRE client area (title band included) with the
                   // themed panel brush -- the always-readable fallback.
    DwmTitleBand,  // leave ONLY the custom Group title band for the CONFIRMED
                   // DWM system backdrop; paint the themed panel/cards below it.
};

// The New Folder popup (isGroup == false) ALWAYS paints its full themed client
// area: it has a native caption and never calls DwmExtendFrameIntoClientArea, so
// there is NO verified surface for a backdrop to composite into. Only the Group
// popup may use the DWM title-band path, and ONLY when every condition holds:
//   isWin11           -- the backdrop + frame-extension APIs exist
//   transparencyOn    -- the user's "Transparency effects" setting is on
//   backdropRequested -- glass was requested for this window
//   backdropAccepted  -- DwmSetWindowAttribute(DWMSBT) returned SUCCEEDED
//   frameExtended     -- DwmExtendFrameIntoClientArea returned SUCCEEDED
// Any failure (Win10 / transparency off / missing DWM entry point / a failed
// HRESULT) -> SolidTheme (paint everything: no white strip, no white body).
inline PaintMode ChoosePaintMode(bool isGroup, bool isWin11, bool transparencyOn,
                                 bool backdropRequested, bool backdropAccepted,
                                 bool frameExtended) {
    if (!isGroup) {
        return PaintMode::SolidTheme;
    }
    const bool policy = isWin11 && backdropRequested && transparencyOn;
    const bool dwmConfirmed = backdropAccepted && frameExtended;
    return (policy && dwmConfirmed) ? PaintMode::DwmTitleBand
                                    : PaintMode::SolidTheme;
}

// Convenience: the New Folder popup / any fallback paints the WHOLE client area.
inline bool PaintsFullClient(PaintMode m) { return m == PaintMode::SolidTheme; }

// Convenience: the Group popup leaves ONLY its title band for the DWM backdrop.
inline bool UsesTitleBandBackdrop(PaintMode m) {
    return m == PaintMode::DwmTitleBand;
}

// The Group title band may use the DWM backdrop ONLY when BOTH required DWM
// calls succeeded -- the frame-extension HRESULT must NOT be discarded.
inline bool GroupBandCanUseBackdrop(bool isGroup, bool backdropAccepted,
                                    bool frameExtended) {
    return isGroup && backdropAccepted && frameExtended;
}

}  // namespace paintmode
