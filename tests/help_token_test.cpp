// help_token_test.cpp
// Pins the theme-aware inline keyboard-token colours (fix thse isseue.md add-on
// section 6): dark mode keeps a DARK token surface (never white), light mode a
// muted light surface, both with contrasting text. Self-contained: exit 0 = pass.

#include "help_token_logic.h"

#include <cstdio>
#include <cstdlib>

using namespace helptoken;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const Colors dark  = ForTheme(true);
    const Colors light = ForTheme(false);

    // fix thse isseue.md add-on section 2 FLIP: the token is now a high-contrast
    // INVERSE chip against the card -- DARK theme paints a MUTED LIGHT-GREY
    // surface (~#E8E8E8) with NEAR-BLACK (#171717) text; LIGHT theme paints a
    // DEEP CHARCOAL (#171717) surface with WHITE (#FFFFFF) text. The border is a
    // 0.5 px near-identical neutral (integrated, never a doubled outline) and a
    // restrained hover variant sits a few luma steps off the fill.

    // ---- dark theme: muted light-grey surface (~#E8E8E8), near-black text ----
    check(Luma(dark.bg) > 190, "dark token background is a muted LIGHT-grey surface");
    check(dark.bg != Rgb(255, 255, 255), "dark token background is NEVER pure white");
    check(Luma(dark.text) < 60, "dark token text is near-black (#171717)");
    check(abs(Luma(dark.text) - Luma(dark.bg)) > 120,
          "dark token text strongly contrasts its surface");
    check(dark.border != dark.bg && abs(Luma(dark.border) - Luma(dark.bg)) <= 40,
          "dark token border is a subtle near-identical neutral (no doubled outline)");
    check(dark.hover != dark.bg && abs(Luma(dark.hover) - Luma(dark.bg)) <= 40,
          "dark token hover is a restrained variant of the fill");

    // ---- light theme: deep-charcoal surface (#171717), white text ----
    check(Luma(light.bg) < 60, "light token background is a deep-charcoal surface");
    check(Luma(light.text) > 230, "light token text is white (#FFFFFF)");
    check(abs(Luma(light.text) - Luma(light.bg)) > 120,
          "light token text strongly contrasts its surface");
    check(light.border != light.bg && abs(Luma(light.border) - Luma(light.bg)) <= 40,
          "light token border is a subtle near-identical neutral");
    check(light.hover != light.bg && abs(Luma(light.hover) - Luma(light.bg)) <= 40,
          "light token hover is a restrained variant of the fill");

    // ---- the two themes are INVERTED (dark surface is now LIGHTER) ----
    check(Luma(dark.bg) > Luma(light.bg),
          "the dark-theme token surface is the LIGHT one (inverse chip)");

    // ---- Rgb packs in COLORREF (0x00BBGGRR) order ----
    check(Rgb(1, 2, 3) == (unsigned long)(1 | (2 << 8) | (3 << 16)),
          "Rgb packs channels in COLORREF byte order");

    if (g_fail == 0) printf("help_token_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
