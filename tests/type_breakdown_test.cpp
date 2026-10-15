// type_breakdown_test.cpp
// Slice 1 / fix thse isseu e.md #11: the Selected Items summary should be
// ACTIONABLE -- a file-type breakdown ("4 items - 3 MP3 - 1 JPG - 125 MB")
// instead of only "N items - X files - Y folders". Pins the pure
// sel::TypeBreakdown() model, mirrored byte-for-byte into the mod.
//
// Self-contained: own main(), exit 0 = pass. RED before sel::TypeBreakdown
// exists; GREEN once selection_logic.h (and the mod) mirror it.

#include "selection_logic.h"

#include <cstdio>

using namespace sel;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const std::wstring DOT = L" \u00b7 ";  // space + middle dot + space

    // ---- extension groups ordered by first appearance, uppercased, bare count ----
    std::vector<Item> media = {
        {L"song1.mp3", false, 2 * 1024 * 1024},
        {L"song2.mp3", false, 2 * 1024 * 1024},
        {L"song3.MP3", false, 2 * 1024 * 1024},   // case-insensitive -> same MP3 group
        {L"photo.jpg", false, 1 * 1024 * 1024},
    };
    // 4 items; 3 MP3 then 1 JPG (order of first appearance); total size 7 MB.
    check(TypeBreakdown(media) ==
              L"4 items" + DOT + L"3 MP3" + DOT + L"1 JPG" + DOT + L"7.0 MB",
          "media: counts per type in first-seen order + total size");

    // ---- folders use the folder/folders group, no size contribution ----
    std::vector<Item> withFolders = {
        {L"a.txt", false, 512},
        {L"docs", true, -1},
        {L"pics", true, -1},
    };
    check(TypeBreakdown(withFolders) ==
              L"3 items" + DOT + L"1 TXT" + DOT + L"2 folders" + DOT + L"512 B",
          "files + folders: folder group + total known size");

    // ---- extensionless files group under FILE / FILES ----
    std::vector<Item> noext = {
        {L"README", false, 100},
        {L"LICENSE", false, 100},
    };
    check(TypeBreakdown(noext) == L"2 items" + DOT + L"2 FILES" + DOT + L"200 B",
          "extensionless files grouped as FILES");

    std::vector<Item> oneNoext = { {L"Makefile", false, 10} };
    check(TypeBreakdown(oneNoext) == L"1 item" + DOT + L"1 FILE" + DOT + L"10 B",
          "single extensionless file -> FILE singular");

    // ---- singular item noun + single-folder singular ----
    std::vector<Item> oneFolder = { {L"only", true, -1} };
    check(TypeBreakdown(oneFolder) == L"1 item" + DOT + L"1 folder",
          "one folder: no size segment, folder singular");

    // ---- no known sizes -> no trailing size segment ----
    std::vector<Item> unknownSizes = {
        {L"a.txt", false, -1}, {L"b.txt", false, -1},
    };
    check(TypeBreakdown(unknownSizes) == L"2 items" + DOT + L"2 TXT",
          "all sizes unknown -> omit size segment");

    // ---- empty selection ----
    check(TypeBreakdown({}) == L"0 items", "empty selection -> just the item count");

    if (g_fail) { printf("type_breakdown_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("type_breakdown_test: OK\n");
    return 0;
}
