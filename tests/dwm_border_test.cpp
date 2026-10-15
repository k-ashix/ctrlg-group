// dwm_border_test.cpp
// Pins the PURE DWM border-colour decision (fix thse isseue.md #4): the custom
// borderless Group popup must set DWMWA_BORDER_COLOR to the documented sentinel
// DWMWA_COLOR_NONE (0xFFFFFFFE) so the compositor draws NO perimeter at all --
// removing the stray ~1-2px white/black outline that otherwise tracks the
// theme. Setting the border to the panel surface (the old behaviour) still left
// a faint theme-dependent edge, so the border is now TRANSPARENT instead.
// Like corner_logic_test, this pins only the attribute id + policy (NOT Win32
// painting); the mod mirrors dwmborder:: and calls DwmSetWindowAttribute.
//
// Self-contained: own main(), exit 0 = pass. RED before dwm_border_logic.h.

#include "dwm_border_logic.h"

#include <cstdio>

using namespace dwmborder;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- the attribute is DWMWA_BORDER_COLOR (34) ----
    check(kAttrBorderColor == 34, "border attribute is DWMWA_BORDER_COLOR (34)");

    // ---- the border is TRANSPARENT: DWMWA_COLOR_NONE, no perimeter ----
    check(BorderSource() == Source::None, "border colour source is None (transparent)");
    check(IsTransparent(), "border is deliberately transparent (no perimeter)");
    check(kColorNone == 0xFFFFFFFEUL, "transparent sentinel is DWMWA_COLOR_NONE (0xFFFFFFFE)");

    // ---- it is NOT the theme-dependent panel surface and NOT the system default ----
    check(!MatchesSurface(), "border no longer matches the (theme-dependent) panel surface");

    // ---- only the custom borderless Group popup gets it ----
    check(AppliesTo(true), "custom Group popup sets a deliberate transparent border");
    check(!AppliesTo(false), "native-caption New Folder keeps the system default border");

    // ---- the border-colour call is best-effort: its HRESULT is CHECKED, not
    //      assumed to have succeeded (P0 #2). Older Win10 rejects the attribute. ----
    check(BorderColorAccepted(0x00000000UL), "S_OK border result is accepted");
    check(BorderColorAccepted(0x00000001UL), "S_FALSE (success) border result is accepted");
    check(!BorderColorAccepted(0x80004005UL), "E_FAIL border result is rejected (not assumed OK)");
    check(!BorderColorAccepted(0x80070057UL), "E_INVALIDARG border result is rejected");

    if (g_fail) { printf("dwm_border_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("dwm_border_test: OK\n");
    return 0;
}
