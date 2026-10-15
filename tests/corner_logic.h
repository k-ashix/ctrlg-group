// corner_logic.h
//
// PURE rounded-corner decisions (waht to create.md #17), mirrored from
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build (no Win32 runtime). Per plan section 19 this covers only the
// method/geometry DECISIONS -- not the Win32 painting itself:
//   * choose the native Windows DWM corner preference on Win11, a clipped
//     region only as a fallback;
//   * the DWM corner-preference value is DWMWCP_ROUND (2);
//   * DPI-scale the corner radius with the same MulDiv rounding the rest of the
//     popup uses (matches layout::ScaleForDpi / the mod's ScaleDpi).
//
#pragma once

namespace corner {

enum class Method { DwmRound, RegionFallback };

// Win11 composits real rounded window corners via DWM; older builds fall back
// to a clipped region.
inline Method ChooseMethod(bool isWin11) {
    return isWin11 ? Method::DwmRound : Method::RegionFallback;
}

inline bool UsesDwm(Method m) { return m == Method::DwmRound; }
inline bool NeedsRegionClip(Method m) { return m == Method::RegionFallback; }

// DWMWA_WINDOW_CORNER_PREFERENCE value for rounded corners.
inline int DwmCornerPreference() { return 2; }  // DWMWCP_ROUND

// Corner radius in 96-dpi design pixels, and its DPI-scaled device value.
inline int CornerRadius96() { return 8; }

inline int ScaledRadius(int radius96, int dpi) {
    return static_cast<int>(
        (static_cast<long long>(radius96) * dpi + 48) / 96);
}

}  // namespace corner
