// chip_model_test.cpp
// Pins fix thse isseue.md #2/#3/#6: the Selected Items summary chip MODEL --
// dynamic [N items]/[N folders]/[count EXT] chips, case-insensitive extension
// grouping, a single "No extension" chip, folder-only and mixed selections, and
// long/unusual extensions. Self-contained: own main(), exit 0 = pass.

#include "chip_model_logic.h"

#include <cstdio>

using namespace chipmodel;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- normalization helpers ----
    check(NormExtKey(L"MD") == L"md", "uppercase label -> lowercase key");
    check(NormExtKey(L".Md") == L"md", "leading dot + mixed case -> md");
    check(UpperExt(L"cpp") == L"CPP", "key -> uppercase label");

    // ---- mixed selection: 5 folders + 7 files (md x3 case-insensitive) ----
    std::vector<sel::Item> items = {
        {L"A", true, -1}, {L"B", true, -1}, {L"C", true, -1},
        {L"D", true, -1}, {L"E", true, -1},
        {L"a.md", false, 10}, {L"b.MD", false, 10}, {L"c.md", false, 10},
        {L"main.cpp", false, 20},
        {L"x.pdf", false, 30}, {L"y.pdf", false, 30},
        {L"z.zip", false, 40},
    };
    auto chips = Build(items);
    check(chips.size() == 6, "items + folders + 4 ext chips");
    check(chips[0].kind == Kind::Items && chips[0].label == L"12 items",
          "items chip first");
    check(chips[1].kind == Kind::Folders && chips[1].label == L"5 folders",
          "folders chip second");
    check(chips[2].kind == Kind::Ext && chips[2].extKey == L"md" &&
          chips[2].count == 3 && chips[2].label == L"3 MD",
          "3 MD (case-insensitive merge)");
    check(chips[3].extKey == L"cpp" && chips[3].label == L"1 CPP", "1 CPP");
    check(chips[4].extKey == L"pdf" && chips[4].count == 2 &&
          chips[4].label == L"2 PDF", "2 PDF");
    check(chips[5].extKey == L"zip" && chips[5].label == L"1 ZIP", "1 ZIP");

    // ---- no folders -> no folders chip ----
    std::vector<sel::Item> filesOnly = { {L"a.txt", false, 1}, {L"b.txt", false, 1} };
    auto c2 = Build(filesOnly);
    check(c2.size() == 2 && c2[0].label == L"2 items" && c2[1].label == L"2 TXT",
          "files only: items + one ext chip, no folders chip");

    // ---- folder-only selection ----
    std::vector<sel::Item> foldersOnly = { {L"x", true, -1} };
    auto c3 = Build(foldersOnly);
    check(c3.size() == 2 && c3[0].label == L"1 item" && c3[1].label == L"1 folder",
          "folder only: singular item + singular folder, no ext chips");

    // ---- extensionless files -> single No extension chip ----
    std::vector<sel::Item> noext = {
        {L"README", false, 1}, {L"LICENSE", false, 1}, {L"a.md", false, 1},
    };
    auto c4 = Build(noext);
    check(c4.size() == 3, "items + No extension + md");
    check(c4[1].kind == Kind::NoExt && c4[1].count == 2 &&
          c4[1].label == L"2 No extension",
          "two extensionless files collapse into one No extension chip");
    check(c4[2].extKey == L"md" && c4[2].label == L"1 MD",
          "md chip after No extension (first-seen order)");

    // ---- unusual / very long extension still produces a chip ----
    std::vector<sel::Item> weird = { {L"data.superlongextension", false, 1} };
    auto c5 = Build(weird);
    check(c5.back().kind == Kind::Ext &&
          c5.back().extKey == L"superlongextension" &&
          c5.back().label == L"1 SUPERLONGEXTENSION",
          "long extension is not truncated in the model");

    // ---- empty selection ----
    auto c6 = Build({});
    check(c6.size() == 1 && c6[0].label == L"0 items",
          "empty selection -> just the items chip");

    if (g_fail) { printf("chip_model_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("chip_model_test: OK\n");
    return 0;
}
