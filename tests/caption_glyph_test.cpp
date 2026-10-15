// caption_glyph_test.cpp
// Pins the PURE caption-button glyph model (fix thse isseue.md C): the verified
// Segoe caption glyph codepoints and the Fluent -> MDL2 -> geometric provider
// fallback. Self-contained: own main(), exit 0 = pass.

#include "caption_glyph_logic.h"

#include <cstdio>

using namespace captionglyph;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- verified Segoe caption glyph codepoints ----
    check(Glyph(Button::Minimize, false) == 0xE921u, "Minimize -> E921 ChromeMinimize");
    check(Glyph(Button::Minimize, true)  == 0xE921u, "Minimize is state-independent (E921)");
    check(Glyph(Button::Close,    false) == 0xE8BBu, "Close -> E8BB ChromeClose");
    check(Glyph(Button::Close,    true)  == 0xE8BBu, "Close is state-independent (E8BB)");
    // The middle button is Maximize in compact, Restore when expanded.
    check(Glyph(Button::Expand,   false) == 0xE922u, "Expand (compact) -> E922 ChromeMaximize");
    check(Glyph(Button::Expand,   true)  == 0xE923u, "Expand (expanded) -> E923 ChromeRestore");

    // ---- provider fallback: Fluent -> MDL2 -> geometric ----
    check(ResolveProvider(true,  true)  == Provider::FluentFont, "Fluent installed -> Segoe Fluent Icons");
    check(ResolveProvider(false, true)  == Provider::Mdl2Font,   "no Fluent, some icon font -> Segoe MDL2 Assets");
    check(ResolveProvider(false, false) == Provider::Geometric,  "no icon font -> deliberate geometric fallback");
    check(UsesFont(Provider::FluentFont), "Fluent provider draws a font glyph");
    check(UsesFont(Provider::Mdl2Font),   "MDL2 provider draws a font glyph");
    check(!UsesFont(Provider::Geometric), "geometric provider draws NO font glyph");

    if (g_fail) { printf("caption_glyph_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("caption_glyph_test: OK\n");
    return 0;
}
