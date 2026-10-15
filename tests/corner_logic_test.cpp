// corner_logic_test.cpp
// Pins the PURE rounded-corner decisions (waht to create.md #17, plan Phase 8):
//   * prefer the native Windows DWM corner preference on Win11, fall back to a
//     clipped region only where DWM is unavailable;
//   * the DWM corner-preference value is DWMWCP_ROUND (2);
//   * the corner radius is DPI-scaled with the same MulDiv rounding the rest of
//     the popup uses.
// Per plan section 19 this does NOT try to unit test Win32 painting -- only the
// method/geometry decisions the mod mirrors.
//
// Self-contained: own main(), exit 0 = pass. RED before corner_logic.h exists;
// GREEN once it mirrors the mod.

#include "corner_logic.h"

#include <cstdio>

using namespace corner;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- method selection ----
    check(ChooseMethod(true) == Method::DwmRound, "Win11 -> DWM round");
    check(ChooseMethod(false) == Method::RegionFallback, "older -> region fallback");
    check(UsesDwm(Method::DwmRound), "DwmRound uses DWM");
    check(!UsesDwm(Method::RegionFallback), "fallback does not use DWM");
    check(NeedsRegionClip(Method::RegionFallback), "fallback needs region clip");
    check(!NeedsRegionClip(Method::DwmRound), "DWM does not need region clip");

    // ---- DWM corner preference is DWMWCP_ROUND (2) ----
    check(DwmCornerPreference() == 2, "corner preference is DWMWCP_ROUND (2)");

    // ---- radius is sane and DPI-scaled (MulDiv rounding) ----
    check(CornerRadius96() >= 4 && CornerRadius96() <= 16, "radius is 4-16 design px");
    check(ScaledRadius(8, 96) == 8, "radius @96 identity");
    check(ScaledRadius(8, 192) == 16, "radius @192 doubles");
    check(ScaledRadius(8, 144) == 12, "radius @144 is 1.5x");
    check(ScaledRadius(12, 120) == 15, "radius scales 12@125% -> 15");
    // monotonic non-decreasing across DPI
    int prev = 0;
    const int dpis[5] = {96, 120, 144, 168, 192};
    for (int i = 0; i < 5; ++i) {
        int s = ScaledRadius(CornerRadius96(), dpis[i]);
        check(s >= prev, "radius scales monotonically across DPI");
        prev = s;
    }

    if (g_fail) { printf("corner_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("corner_logic_test: OK\n");
    return 0;
}
