// backdrop_logic.h
//
// PURE Win11 "system backdrop" decision (fix thse isseue.md #7), mirrored into
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++ build (no Win32
// runtime). The popup REQUESTS the Win11 acrylic backdrop, but it may only let
// that TRANSLUCENT material show through -- instead of painting the flat, opaque
// "boxed" panel -- when the material will actually composite. Otherwise it keeps
// the solid themed panel (the safe fallback that avoids the white-panel bug on
// Win10, with transparency effects off, or in a remote session).
//
// The mod keeps a byte-for-byte copy of this namespace; the Win32 wiring
// (DwmSetWindowAttribute + the WM_ERASEBKGND fill) lives in the mod and is
// pinned by source invariants.
//
#pragma once

namespace backdrop {

// DWM system-backdrop materials (DWM_SYSTEMBACKDROP_TYPE). Acrylic
// (DWMSBT_TRANSIENTWINDOW) is the translucent, blurred material Windows 11 uses
// for transient surfaces (flyouts / popups) -- the "transparent, premium,
// minimal" look the issue asks for, as opposed to Mica, which is meant for
// persistent main windows.
inline constexpr int kBackdropAcrylic = 3;   // DWMSBT_TRANSIENTWINDOW

// What fills the popup background.
enum class Fill { SolidPanel, SystemBackdrop };

// The popup may DROP its solid fill and let the Win11 backdrop show through ONLY
// when every condition holds: the OS is Windows 11 (the backdrop API exists),
// the backdrop was requested for this window, AND the user has Transparency
// effects enabled. Any failure -> keep the solid themed panel (safe fallback).
inline Fill ChooseFill(bool isWin11, bool backdropRequested, bool transparencyOn) {
    const bool show = isWin11 && backdropRequested && transparencyOn;
    return show ? Fill::SystemBackdrop : Fill::SolidPanel;
}

// Convenience: true when the solid themed fill should be SKIPPED (backdrop wins).
inline bool SkipSolidFill(bool isWin11, bool backdropRequested, bool transparencyOn) {
    return ChooseFill(isWin11, backdropRequested, transparencyOn) == Fill::SystemBackdrop;
}

// fix thse isseue.md #1: the HONEST decision. The popup may only DROP its solid
// fill when the Win11 policy holds (ChooseFill above) AND the real
// DwmSetWindowAttribute backdrop request actually SUCCEEDED (backdropApplied).
// The OS version + the Transparency setting are NOT proof the compositor
// accepted the backdrop -- requesting it and it working are different things, so
// a failed/ignored request must fall back to the solid themed panel.
inline Fill ChooseFillChecked(bool isWin11, bool backdropRequested,
                              bool transparencyOn, bool backdropApplied) {
    const bool policyWantsBackdrop =
        ChooseFill(isWin11, backdropRequested, transparencyOn) == Fill::SystemBackdrop;
    return (policyWantsBackdrop && backdropApplied) ? Fill::SystemBackdrop
                                                    : Fill::SolidPanel;
}

// Convenience: true when the solid fill should be SKIPPED under the honest,
// HRESULT-checked decision (backdrop wins only when it genuinely composited).
inline bool SkipSolidFillChecked(bool isWin11, bool backdropRequested,
                                 bool transparencyOn, bool backdropApplied) {
    return ChooseFillChecked(isWin11, backdropRequested, transparencyOn,
                             backdropApplied) == Fill::SystemBackdrop;
}

}  // namespace backdrop
