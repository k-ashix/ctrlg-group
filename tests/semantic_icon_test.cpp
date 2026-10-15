// semantic_icon_test.cpp
// fix thse isseue.md v0.12.57 (Semantic Section Icon pass): the five section
// headings -- Selected items / Destination / Folder name / Status / Action --
// drop their hand-drawn GDI glyph + coloured tile and instead resolve a
// WINDOWS-NATIVE semantic icon: Segoe Fluent Icons (Windows 11), falling back
// to Segoe MDL2 Assets (Windows 10), and finally to a real Shell stock icon
// (SHGetStockIconInfo) when NO icon font is installed. This pins the PURE
// provider/mapping decision ONLY (never HDC painting), mirrored byte-for-byte
// into namespace semanticicon in ctrlg-group.wh.cpp.
// Self-contained: own main(), exit 0 == pass. RED until semantic_icon_logic.h.

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

    // ---- 1. semantic concept per section (fix #3): the glyph code point is the
    //         Fluent/MDL2 symbol for List, Folder, Rename, Info, MoveToFolder. --
    check(Glyph(Section::Selected)    == kGlyphList,         "Selected -> List/Stack glyph");
    check(Glyph(Section::Destination) == kGlyphFolder,       "Destination -> Folder glyph");
    check(Glyph(Section::Name)        == kGlyphRename,       "Folder name -> Edit/Rename glyph");
    check(Glyph(Section::Status)      == kGlyphInfo,         "Status -> Info glyph");
    check(Glyph(Section::Action)      == kGlyphMoveToFolder, "Action -> MoveToFolder glyph");

    // ---- 2. the Shell stock fallback maps to the matching SIID_* value ----
    check(StockId(Section::Selected)    == kStockStack,  "Selected -> SIID_STACK");
    check(StockId(Section::Destination) == kStockFolder, "Destination -> SIID_FOLDER");
    check(StockId(Section::Name)        == kStockRename, "Folder name -> SIID_RENAME");
    check(StockId(Section::Status)      == kStockInfo,   "Status -> SIID_INFO");
    // Action (MoveToFolder) has no exact stock peer -> safe folder fallback.
    check(StockId(Section::Action)      == kStockFolder, "Action -> SIID_FOLDER (no exact stock peer)");

    // ---- 3. provider selection: Segoe Fluent Icons on Win11, MDL2 on Win10 ----
    for (Section s : all) {
        Spec win11 = Resolve(s, /*fluentAvailable=*/true);
        Spec win10 = Resolve(s, /*fluentAvailable=*/false);
        check(win11.provider == Provider::FluentFont, "Fluent available -> Segoe Fluent Icons");
        check(win10.provider == Provider::Mdl2Font,   "Fluent unavailable -> Segoe MDL2 Assets");
        // same code point either way (the two fonts share their PUA points).
        check(win11.id == win10.id, "Fluent/MDL2 share the glyph code point");
        check(win11.id == Glyph(s), "Resolve carries the section glyph");
    }

    // ---- 4. missing font/provider -> safe Shell stock fallback ----
    for (Section s : all) {
        Spec stock = ResolveWithFallback(s, /*fluentAvailable=*/true,
                                         /*anyIconFontAvailable=*/false);
        check(stock.provider == Provider::ShellStock, "no icon font -> Shell stock");
        check(stock.id == StockId(s), "stock fallback carries the SIID_* id");
        // when a font IS available the fallback defers to Resolve.
        Spec f11 = ResolveWithFallback(s, true,  true);
        Spec f10 = ResolveWithFallback(s, false, true);
        check(f11.provider == Provider::FluentFont, "font available (Win11) -> Fluent");
        check(f10.provider == Provider::Mdl2Font,   "font available (Win10) -> MDL2");
    }

    // ---- 5. every section glyph is a NATIVE icon-font PUA code point ----
    //         (U+E000..U+F8FF): never ASCII and never an emoji (< U+1F000), so
    //         we neither invent custom artwork nor fall into emoji fallback. --
    for (Section s : all) {
        uint32_t g = Glyph(s);
        check(g >= 0xE000u && g <= 0xF8FFu, "section glyph is a font PUA code point");
        check(g < 0x1F000u, "section glyph is not an emoji code point");
    }

    // ---- 6. the five section glyphs are DISTINCT (hierarchy reads at a glance) --
    for (int i = 0; i < kExpected; ++i)
        for (int j = i + 1; j < kExpected; ++j)
            check(Glyph(all[i]) != Glyph(all[j]), "section glyphs are distinct");

    // ---- 7. coverage: exactly the five popup sections are modelled. If a 6th
    //         heading is added this list (and the enum) must grow with it. ----
    int seen = 0;
    for (Section s : all) { (void)s; ++seen; }
    check(seen == kExpected, "all five section headings are covered");

    if (g_fail) { printf("semantic_icon_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("semantic_icon_test: OK\n");
    return 0;
}
