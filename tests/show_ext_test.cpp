// show_ext_test.cpp
// Pins fix thse isseue.md Phase 1B: the "Show extensions" toggle is OFF by
// default. Rows always show the size badge (files with a known size). The
// toggle now controls whether a file row renders its COMPLETE filename INLINE
// (e.g. README.md) instead of the stem -- there is NO separate extension
// badge. Folders never show a size badge and are unaffected by the toggle.
// Self-contained: own main(), exit 0 = pass.

#include "show_ext_logic.h"

#include <cstdio>
#include <cwchar>

using namespace showext;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    check(DefaultOn() == true, "toggle defaults ON (Issue 3)");
    check(wcscmp(ToggleLabel(), L"Show extensions") == 0, "toolbar label");

    // size badge: files with a known size only (independent of the toggle)
    check(RowShowsSize(false, true), "file with known size shows size");
    check(!RowShowsSize(false, false), "file with unknown size shows no size");
    check(!RowShowsSize(true, true), "folder never shows size");

    // inline extension: OFF -> stem only (extension is NOT rendered inline)
    check(!RowShowsExtInline(false, false), "OFF -> no inline extension");
    // ON -> a file row renders its complete filename inline (no separate pill)
    check(RowShowsExtInline(true, false), "ON + file -> inline extension");
    // folders are never affected by the toggle
    check(!RowShowsExtInline(true, true), "ON + folder -> no inline extension");
    check(!RowShowsExtInline(false, true), "OFF + folder -> no inline extension");

    // fix thse isseue.md P2 (10249-10250): the custom-drawn switch has NO
    // visible text, so its accessible name/description MUST be a real, STATE
    // AWARE string (wired to a tooltip in the live build) -- not a value that is
    // computed and discarded.
    check(wcscmp(AccessibleName(), L"Show file extensions") == 0,
          "accessible name is the control's purpose");
    check(wcscmp(StateWord(true), L"on") == 0, "state word ON");
    check(wcscmp(StateWord(false), L"off") == 0, "state word OFF");
    check(AccessibleLabel(true) == L"Show file extensions (on)",
          "accessible label announces the ON state");
    check(AccessibleLabel(false) == L"Show file extensions (off)",
          "accessible label announces the OFF state");

    if (g_fail) { printf("show_ext_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("show_ext_test: OK\n");
    return 0;
}
