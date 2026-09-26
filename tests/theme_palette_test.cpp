// theme_palette_test.cpp
// Pins the popup light/dark palette (ResolveThemePalette, mirrored from the mod
// into theme_logic.h). These are the decisions behind "compatible with light
// AND dark mode": the accent stays the muted #005FB8 in both themes, every
// text/background pair keeps a real luminance gap, dark is darker than light,
// and no slot silently collapses to the same colour in both themes.
//
// Self-contained: own main(), exit 0 = pass. RED before the header exists /
// has the right values; GREEN once theme_logic.h mirrors the mod.

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
    const ThemePalette light = ResolveThemePalette(false);
    const ThemePalette dark = ResolveThemePalette(true);

    // ---- RGB32 packs like the Win32 COLORREF macro (0x00BBGGRR) ----
    check(RGB32(0, 95, 184) == 0x00B85F00u, "RGB32(0,95,184) packs to 0x00BBGGRR");
    check(ColR(RGB32(10, 20, 30)) == 10 && ColG(RGB32(10, 20, 30)) == 20 &&
          ColB(RGB32(10, 20, 30)) == 30, "ColR/G/B unpack the channels");

    // ---- Accent: Create is the muted accent blue #005FB8 in BOTH themes ----
    check(light.createFill == RGB32(0, 95, 184), "light Create fill is #005FB8");
    check(dark.createFill == RGB32(0, 95, 184), "dark Create fill is #005FB8");
    check(light.createFill == dark.createFill, "accent is identical light/dark");
    check(light.createText == RGB32(255, 255, 255) &&
          dark.createText == RGB32(255, 255, 255), "Create caption is white");

    // ---- Create pressed is a touch DARKER than resting (visible feedback) ----
    check(Luminance(light.createFillPressed) < Luminance(light.createFill),
          "light Create pressed darker than resting");
    check(Luminance(dark.createFillPressed) < Luminance(dark.createFill),
          "dark Create pressed darker than resting");

    // ---- Text vs background keeps a real contrast gap in BOTH themes ----
    auto gap = [](Color a, Color b) { int d = Luminance(a) - Luminance(b);
                                      return d < 0 ? -d : d; };

    check(gap(light.panelText, light.panelBg) >= 120, "light panel text/bg contrast");
    check(gap(dark.panelText, dark.panelBg) >= 120, "dark panel text/bg contrast");
    check(gap(light.editText, light.editBg) >= 120, "light edit text/bg contrast");
    check(gap(dark.editText, dark.editBg) >= 120, "dark edit text/bg contrast");
    check(gap(light.createText, light.createFill) >= 100, "light Create caption contrast");
    check(gap(dark.createText, dark.createFill) >= 100, "dark Create caption contrast");
    check(gap(light.cancelText, light.cancelFill) >= 100, "light Cancel caption contrast");
    check(gap(dark.cancelText, dark.cancelFill) >= 100, "dark Cancel caption contrast");

    // ---- Cancel keeps a VISIBLE border (distinct from its fill) both themes ----
    check(light.cancelBorder != light.cancelFill, "light Cancel border != fill");
    check(dark.cancelBorder != dark.cancelFill, "dark Cancel border != fill");

    // ---- Dark theme really is darker than light for the surfaces ----
    check(Luminance(dark.panelBg) < Luminance(light.panelBg), "dark panel darker than light");
    check(Luminance(dark.editBg) < Luminance(light.editBg), "dark edit darker than light");
    check(Luminance(dark.cancelFill) < Luminance(light.cancelFill), "dark Cancel darker than light");

    // ---- Theme-dependent slots must actually differ between light and dark ----
    // (createFill/Pressed/Text/Border and cancelText are intentionally shared
    // across themes -- the accent and the white button captions -- so excluded.)
    check(light.panelBg != dark.panelBg, "panelBg differs light vs dark");
    check(light.panelText != dark.panelText, "panelText differs light vs dark");
    check(light.editBg != dark.editBg, "editBg differs light vs dark");
    check(light.editText != dark.editText, "editText differs light vs dark");
    check(light.cancelFill != dark.cancelFill, "cancelFill differs light vs dark");
    check(light.cancelBorder != dark.cancelBorder, "cancelBorder differs light vs dark");

    // ---- Dark input field is the requested #545454; captions are white ----
    check(dark.editBg == RGB32(84, 84, 84), "dark input field is #545454");
    check(light.cancelText == RGB32(255, 255, 255) &&
          dark.cancelText == RGB32(255, 255, 255), "Cancel caption is white");

    // ---- Cancel is unmistakably RED in both themes; Create stays BLUE ----
    check(ColR(light.cancelFill) > ColG(light.cancelFill) &&
          ColR(light.cancelFill) > ColB(light.cancelFill),
          "light Cancel fill is red-dominant");
    check(ColR(dark.cancelFill) > ColG(dark.cancelFill) &&
          ColR(dark.cancelFill) > ColB(dark.cancelFill),
          "dark Cancel fill is red-dominant");
    check(ColB(light.createFill) > ColR(light.createFill) &&
          ColB(light.createFill) > ColG(light.createFill),
          "Create fill stays blue-dominant");

    if (g_fail == 0) {
        printf("theme_palette_test: PASS\n");
        return 0;
    }
    printf("theme_palette_test: FAIL (%d)\n", g_fail);
    return 1;
}
