// selection_summary_test.cpp
// Pins the Selected Items summary (waht to create.md #1): a title line
// "Selected items (N)" and a detail line "N items - X files - Y folders" using a
// middle-dot separator, with correct singular/plural for each noun. Mirrored
// from the mod into selection_logic.h.
//
// Self-contained: own main(), exit 0 = pass. RED before selection_logic.h is
// implemented; GREEN once it mirrors the mod.

#include "selection_logic.h"

#include <cstdio>

using namespace sel;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const std::wstring DOT = L" \u00b7 ";  // space + middle dot + space

    // ---- file / folder counting ----
    std::vector<Item> mixed = {
        {L"a.txt", false, 10}, {L"b.mp3", false, 20},
        {L"docs", true, -1},   {L"pics", true, -1},
    };
    check(FileCount(mixed) == 2, "file count ignores folders");
    check(FolderCount(mixed) == 2, "folder count ignores files");
    check(FileCount({}) == 0 && FolderCount({}) == 0, "empty selection counts are zero");

    // ---- title line ----
    check(SummaryTitle(12) == L"Selected items -- 12", "title with 12 (dash, no parens)");
    check(SummaryTitle(0) == L"Selected items -- 0", "title with 0 (dash, no parens)");
    check(SummaryTitle(1) == L"Selected items -- 1", "title with 1 (dash, no parens)");

    // ---- detail line: 8 files + 4 folders = 12 items ----
    std::vector<Item> big;
    for (int i = 0; i < 8; ++i) big.push_back({L"f", false, 1});
    for (int i = 0; i < 4; ++i) big.push_back({L"d", true, -1});
    check(SummaryCounts(big) == L"12 items" + DOT + L"8 files" + DOT + L"4 folders",
          "mixed 8 files + 4 folders");

    // ---- singular/plural per noun ----
    std::vector<Item> oneFile = { {L"x.txt", false, 1} };
    check(SummaryCounts(oneFile) == L"1 item" + DOT + L"1 file" + DOT + L"0 folders",
          "one file: item + file singular, folders plural(0)");

    std::vector<Item> oneFolder = { {L"x", true, -1} };
    check(SummaryCounts(oneFolder) == L"1 item" + DOT + L"0 files" + DOT + L"1 folder",
          "one folder: item + folder singular, files plural(0)");

    std::vector<Item> twoFolders = { {L"x", true, -1}, {L"y", true, -1} };
    check(SummaryCounts(twoFolders) == L"2 items" + DOT + L"0 files" + DOT + L"2 folders",
          "two folders only");

    check(SummaryCounts({}) == L"0 items" + DOT + L"0 files" + DOT + L"0 folders",
          "empty selection");

    if (g_fail) { printf("selection_summary_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("selection_summary_test: OK\n");
    return 0;
}
