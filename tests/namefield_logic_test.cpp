// namefield_logic_test.cpp
// Pins fix thse isseue.md #7: the Folder name field must read as a real input,
// not a blank region -- an empty+unfocused field shows a secondary-tone
// placeholder, and the field border turns accent while focused. The pure
// decisions (placeholder text; accent-border-when-focused) live here; the Win32
// paint (edit subclass + parent border) is pinned by source invariants.
//
// Self-contained: own main(), exit 0 = pass. RED before namefield_logic.h exists.

#include "namefield_logic.h"

#include <cstdio>
#include <cwchar>

using namespace namefield;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    check(Placeholder() != nullptr && wcslen(Placeholder()) > 0,
          "placeholder is a non-empty string");
    check(wcsstr(Placeholder(), L"folder name") != nullptr,
          "placeholder prompts for a folder name");
    check(BorderIsAccent(true) == true,
          "a focused field uses the accent border");
    check(BorderIsAccent(false) == false,
          "an unfocused field uses the neutral border");

    // fix thse isseue.md #5: the placeholder cue is shown ONLY when the field is
    // empty AND unfocused (the single BeginPaint/EndPaint path). Every other
    // content/focus combination defers to the native EDIT, so typing, deleting
    // and focus changes never leave a stale cue or ghost text.
    check(ShowPlaceholder(true, false) == true,
          "placeholder shows when empty and unfocused");
    check(ShowPlaceholder(true, true) == false,
          "no placeholder when empty but focused (caret shows instead)");
    check(ShowPlaceholder(false, false) == false,
          "no placeholder when the field has text and is unfocused");
    check(ShowPlaceholder(false, true) == false,
          "no placeholder when the field has text and is focused");

    if (g_fail) { printf("namefield_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("namefield_logic_test: OK\n");
    return 0;
}
