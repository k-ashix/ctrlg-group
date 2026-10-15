// caption_fg_test.cpp
// Pins the shared theme-aware caption foreground (captionfg::, mirroring
// CaptionForeground() in ctrlg-group.wh.cpp). fix thse isseue.md #2/#5:
// near-black title text + glyphs in light mode, near-white in dark mode, and
// NEVER a hard-coded black in dark mode.
//
// Self-contained: own main(), exit 0 = pass.

#include "caption_fg_logic.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

int main() {
    using captionfg::Rgb;

    // ---- The EXACT theme-aware values the issue specifies. ----
    check(captionfg::Foreground(false) == Rgb{24, 24, 24},
          "light theme -> near-black RGB(24,24,24)");
    check(captionfg::Foreground(true) == Rgb{245, 245, 245},
          "dark theme -> near-white RGB(245,245,245)");

    // ---- Adapts to the theme; never a hard-coded black in dark mode. ----
    check(captionfg::AdaptsToTheme(),
          "caption foreground differs between light and dark");
    check(!(captionfg::Foreground(true) == Rgb{0, 0, 0}),
          "dark caption foreground is NOT hard-coded black");
    check(captionfg::IsLegibleInDark(),
          "dark caption foreground is near-white (legible on dark material)");
    check(captionfg::IsLegibleInLight(),
          "light caption foreground is near-black (legible on light material)");

    if (g_fail == 0) {
        printf("[PASS] caption_fg_test: shared caption foreground is theme-aware "
               "(near-black light / near-white dark)\n");
        return 0;
    }
    printf("[FAIL] caption_fg_test: %d check(s) failed\n", g_fail);
    return 1;
}
