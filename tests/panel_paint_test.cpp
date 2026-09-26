// panel_paint_test.cpp
// Pins the popup panel-background paint decision (ShouldPaintPanelBackground,
// mirrored from the WM_ERASEBKGND / glass logic in
// explorer-ctrlfn-newfolder.wh.cpp).
//
// Bug reproduced here (RED before the fix): with the acrylic backdrop REQUESTED
// but NOT actually compositing, the old mod skipped the solid fill and left the
// panel unpainted -- it showed through WHITE in dark mode instead of the themed
// #202020. The fix only skips the solid fill when acrylic is CONFIRMED.
//
// Self-contained: own main(), exit 0 = pass.

#include "theme_logic.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

int main() {
    // ---- The bug: glass requested but acrylic NOT confirmed -> MUST paint ----
    // This is the exact failing scenario the user hit: dark mode, glass on,
    // acrylic not really compositing, panel left white. The solid themed panel
    // must be painted so it is #202020, never bare white.
    check(ShouldPaintPanelBackground(/*glass*/ true, /*acrylicConfirmed*/ false),
          "glass requested but acrylic NOT confirmed -> paint solid panel");

    // ---- No glass at all: obviously paint the solid panel ----
    check(ShouldPaintPanelBackground(/*glass*/ false, /*acrylicConfirmed*/ false),
          "no glass -> paint solid panel");
    check(ShouldPaintPanelBackground(/*glass*/ false, /*acrylicConfirmed*/ true),
          "no glass (confirmed irrelevant) -> paint solid panel");

    // ---- Only when acrylic is CONFIRMED do we skip the solid fill ----
    // (Real acrylic composites over nothing; a solid fill would hide the blur.)
    check(!ShouldPaintPanelBackground(/*glass*/ true, /*acrylicConfirmed*/ true),
          "glass requested AND acrylic confirmed -> skip solid fill");

    // ---- The panel colour it paints is the dark themed surface, not white ----
    const ThemePalette dark = ResolveThemePalette(true);
    check(dark.panelBg != kColWhite, "dark panel fill is not white");
    check(dark.panelBg == kColSurfaceDark, "dark panel fill is the #202020 surface");
    check(Luminance(dark.panelBg) < 64, "dark panel fill is actually dark");

    // ---- Light mode still paints the light surface with readable text ----
    const ThemePalette light = ResolveThemePalette(false);
    check(light.panelBg == kColSurfaceLight, "light panel fill is the light surface");
    check(Luminance(light.panelBg) > 191, "light panel fill is actually light");

    if (g_fail == 0) {
        printf("[PASS] panel_paint_test: panel is always painted unless acrylic is confirmed\n");
        return 0;
    }
    printf("[FAIL] panel_paint_test: %d check(s) failed\n", g_fail);
    return 1;
}
