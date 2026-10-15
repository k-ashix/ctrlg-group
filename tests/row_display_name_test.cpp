// row_display_name_test.cpp
// Pins fix thse isseue.md Phase 1B: sel::RowDisplayName renders the filename
// text for a Selected Items row, honoring the Show-extensions toggle:
//   - extensions OFF -> the stem for files (README); folders unchanged;
//   - extensions ON  -> the COMPLETE filename inline (README.md) for files.
// Self-contained: own main(), exit 0 = pass.

#include "selection_logic.h"

#include <cstdio>
#include <string>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    using sel::RowDisplayName;

    // extensions OFF -> stem for files
    check(RowDisplayName(L"README.md", false, false) == L"README",
          "OFF file -> stem");
    check(RowDisplayName(L"codeoverview.md", false, false) == L"codeoverview",
          "OFF file -> stem (2)");

    // extensions ON -> complete filename inline for files (no separate pill)
    check(RowDisplayName(L"README.md", false, true) == L"README.md",
          "ON file -> full name");
    check(RowDisplayName(L"ctrlg-group.wh.cpp", false, true) == L"ctrlg-group.wh.cpp",
          "ON file -> full name (compound ext)");

    // dotless / hidden / trailing-dot files: stem == whole name either way
    check(RowDisplayName(L"Makefile", false, false) == L"Makefile",
          "OFF dotless -> whole name");
    check(RowDisplayName(L"Makefile", false, true) == L"Makefile",
          "ON dotless -> whole name");
    check(RowDisplayName(L".gitignore", false, true) == L".gitignore",
          "ON hidden -> whole name");

    // folders: always the whole folder name, toggle has no effect
    check(RowDisplayName(L"My Folder", true, false) == L"My Folder",
          "OFF folder -> whole name");
    check(RowDisplayName(L"archive.old", true, true) == L"archive.old",
          "ON folder -> whole name (dot in folder kept)");

    if (g_fail) { printf("row_display_name_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("row_display_name_test: OK\n");
    return 0;
}
