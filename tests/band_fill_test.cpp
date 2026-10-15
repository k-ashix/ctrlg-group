// band_fill_test.cpp
// Pins the Ctrl+G title-band fill decision (bandfill::, modelling the
// WM_ERASEBKGND groupBandBackdrop branch in ctrlg-group.wh.cpp).
//
// Bug reproduced here (RED before the fix): with the DWM frame extended, the old
// mod left the title band UNPAINTED. Untouched pixels are not transparent, so
// the band showed a WHITE strip instead of the Win11 material. The fix fills the
// band with the transparent/black brush when the backdrop is CONFIRMED, and with
// the themed panel otherwise -- never leaving it unpainted.
//
// Self-contained: own main(), exit 0 = pass.

#include "band_fill_logic.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

int main() {
    using bandfill::Brush;

    // ---- The bug: backdrop CONFIRMED -> band must be TRANSPARENT black, ----
    // ---- never left unpainted (leaving it untouched was the white strip). ----
    check(bandfill::TitleBandBrush(true) == Brush::TransparentBlack,
          "backdrop confirmed -> title band filled transparent BLACK");
    check(bandfill::TitleBandBrush(true) != Brush::Unpainted,
          "backdrop confirmed -> title band is NEVER left unpainted");
    check(bandfill::TitleBandIsPainted(true),
          "backdrop confirmed -> title band receives real pixels");

    // ---- No confirmed backdrop -> solid themed panel across the band. ----
    check(bandfill::TitleBandBrush(false) == Brush::Panel,
          "no confirmed backdrop -> title band filled with themed panel");
    check(bandfill::TitleBandIsPainted(false),
          "no confirmed backdrop -> title band still painted (solid fallback)");

    // ---- Area below the band is ALWAYS the themed panel (both paths). ----
    check(bandfill::BelowBandBrush(true) == Brush::Panel,
          "below band (confirmed) -> themed panel keeps content readable");
    check(bandfill::BelowBandBrush(false) == Brush::Panel,
          "below band (fallback) -> themed panel");

    // ---- Band-height clamp: never exceeds the client, never negative. ----
    check(bandfill::ClampBandHeight(48, 400) == 48,
          "band shorter than client is unchanged");
    check(bandfill::ClampBandHeight(500, 400) == 400,
          "band taller than client clamps to client height");
    check(bandfill::ClampBandHeight(-5, 400) == 0,
          "negative band height clamps to 0");

    if (g_fail == 0) {
        printf("[PASS] band_fill_test: extended title band is filled "
               "transparent-black, never unpainted\n");
        return 0;
    }
    printf("[FAIL] band_fill_test: %d check(s) failed\n", g_fail);
    return 1;
}
