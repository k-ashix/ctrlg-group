// section_icon_coverage_test.cpp
// fix thse isseue.md v0.12.57 (Semantic Section Icon pass): coverage lock --
// ALL FIVE popup sections (Selected items / Destination / Folder name / Status
// / Action) must resolve a Windows-native semantic icon via the pure model
// semanticicon::. If a 6th heading is ever added, the `all[]` list (and the
// Section enum) must grow with it -- that is the "coverage" intent. The live
// paint is in the mod (Selected via DrawSemanticSectionIconDev; the other four
// via DrawSemanticSectionIcon) and is pinned by source invariants; THIS test
// locks the pure model behind every icon. Self-contained: own main().

#include "semantic_icon_logic.h"

#include <cstdio>
#include <cstdint>

using namespace semanticicon;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const Section all[] = { Section::Selected, Section::Destination,
                            Section::Name, Section::Status, Section::Action };
    const int kExpected = 5;
    int seen = 0;

    for (Section s : all) {
        ++seen;
        // Every section resolves on BOTH font paths + the Shell-stock fallback.
        Spec fluent = Resolve(s, true);
        Spec mdl2   = Resolve(s, false);
        Spec stock  = ResolveWithFallback(s, true, false);
        check(fluent.provider == Provider::FluentFont, "Win11 -> Segoe Fluent Icons");
        check(mdl2.provider   == Provider::Mdl2Font,   "Win10 -> Segoe MDL2 Assets");
        check(stock.provider  == Provider::ShellStock, "no icon font -> Shell stock");
        // Font glyph is a real PUA code point; stock id is a real SIID_* value.
        check(fluent.id >= 0xE000u && fluent.id <= 0xF8FFu, "font glyph is a PUA code point");
        check(fluent.id == mdl2.id, "Fluent and MDL2 share the glyph code point");
        check(stock.id == StockId(s), "stock fallback carries the SIID_* id");
        check(stock.id != 0u, "a visible stock icon id is assigned");
    }
    check(seen == kExpected, "all five section headings are covered");

    // The concept mapping the target UI expects (fix thse isseue.md #3).
    check(Glyph(Section::Selected)    == kGlyphList,         "Selected -> List/Stack");
    check(Glyph(Section::Destination) == kGlyphFolder,       "Destination -> Folder");
    check(Glyph(Section::Name)        == kGlyphRename,       "Folder name -> Edit/Rename");
    check(Glyph(Section::Status)      == kGlyphInfo,         "Status -> Info");
    check(Glyph(Section::Action)      == kGlyphMoveToFolder, "Action -> MoveToFolder");

    // The five glyphs are distinct so the hierarchy reads at a glance.
    for (int i = 0; i < kExpected; ++i)
        for (int j = i + 1; j < kExpected; ++j)
            check(Glyph(all[i]) != Glyph(all[j]), "section glyphs are distinct");

    if (g_fail == 0) printf("section_icon_coverage_test: OK\n");
    else             printf("section_icon_coverage_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
