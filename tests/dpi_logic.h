// dpi_logic.h
//
// PURE WM_DPICHANGED decisions (waht to create.md #16), mirrored from
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build (no Win32 runtime). The mod keeps a byte-for-byte copy: its
// WM_DPICHANGED handler decodes the new DPI with these helpers, then relayouts
// with layout::ScaleForDpi (already tested) and adopts the OS-suggested rect.
//
// Named `dpiutil` (not `dpi`) so it never collides with the popup's local `dpi`
// variable inside the mod.
//
#pragma once

namespace dpiutil {

// WM_DPICHANGED packs the new X DPI in the low word and the Y DPI in the high
// word of wParam; they are equal in practice. Take the low word.
inline int NewDpiFromWparam(unsigned long wParam) {
    return static_cast<int>(wParam & 0xFFFFu);
}

// Relayout only when the DPI actually changed to a valid, positive value.
inline bool NeedsRelayout(int oldDpi, int newDpi) {
    return newDpi > 0 && newDpi != oldDpi;
}

}  // namespace dpiutil
