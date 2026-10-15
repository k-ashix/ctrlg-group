// help_footer_test.cpp
// Pins the GitHub author footer gesture (fix thse isseue.md section 6): the
// k-ashix URL, the footer hit rect, and the Ctrl+left-click-only open gate.
// Self-contained: own main(), exit 0 = pass.

#include "help_footer_logic.h"

#include <cwchar>
#include <cstdio>

using namespace helpfooter;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- author URL ----
    check(wcscmp(Url(), L"https://github.com/k-ashix") == 0,
          "footer opens the k-ashix GitHub profile");

    // ---- footer hit rect ----
    check(InsideFooter(20, 105, 10, 300, 100, 118),
          "a point inside the footer rect is inside");
    check(!InsideFooter(5, 105, 10, 300, 100, 118),
          "a point left of the footer is outside");
    check(!InsideFooter(20, 99, 10, 300, 100, 118),
          "a point above the footer is outside");
    check(!InsideFooter(20, 118, 10, 300, 100, 118),
          "the bottom edge is exclusive (outside)");

    // ---- open ONLY on Ctrl + left-click inside the footer ----
    check(ShouldOpen(true, true, true), "Ctrl + left-click inside opens the link");
    check(!ShouldOpen(false, true, true), "a plain left-click (no Ctrl) never opens");
    check(!ShouldOpen(true, false, true), "a non-left click never opens");
    check(!ShouldOpen(true, true, false), "Ctrl + click outside the footer never opens");

    // ---- add-on section 4: a plain DOUBLE left-click inside also opens ----
    check(ShouldOpenDouble(true, true), "a double left-click inside opens the link");
    check(!ShouldOpenDouble(true, false), "a double-click outside the footer never opens");
    check(!ShouldOpenDouble(false, true), "a single (non-double) click never opens via the double path");

    if (g_fail == 0) printf("help_footer_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
