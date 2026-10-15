// vector_icon_contract_test.cpp
// Structural contract for the production icon layer. Row icons (folder/file),
// the resize affordance, and the per-state status marks are drawn from GDI
// primitives (no font-glyph dependency). THREE section headings -- Destination,
// Folder name and Action -- use Windows-native semantic icons (fix thse
// isseue.md v0.12.57): Segoe Fluent Icons (Win11) -> Segoe MDL2 Assets (Win10)
// -> a Shell stock icon when no icon font is installed. Per issue #1 the
// Selected-items + Copy/Move marks are EMOJI and the Status heading shows the
// chosen status symbol in its state tone, NOT the generic semantic glyphs.

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static int failures = 0;
static void check(bool condition, const char* message) {
    if (!condition) {
        std::printf("[FAIL] %s\n", message);
        ++failures;
    }
}

int main() {
    std::ifstream input("ctrlg-group.wh.cpp", std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const std::string source = buffer.str();

    check(!source.empty(), "production source is readable");
    check(source.find("enum class IconId") != std::string::npos,
          "production defines one IconId model");
    check(source.find("static void DrawIcon(") != std::string::npos,
          "production exposes one vector DrawIcon renderer");
    check(source.find("IconId::Expand") != std::string::npos,
          "resize control uses the expand icon");
    check(source.find("IconId::Collapse") != std::string::npos,
          "resize control uses the collapse icon");
    check(source.find("DrawSemanticSectionIcon(hdc, semanticicon::Section::Destination") != std::string::npos,
          "Destination section draws the Windows-native semantic icon");
    check(source.find("DrawSemanticSectionIcon(hdc, semanticicon::Section::Name") != std::string::npos,
          "Folder Name section draws the Windows-native semantic icon");
    check(source.find("DrawSemanticSectionIcon(hdc, semanticicon::Section::Action") != std::string::npos,
          "Action section draws the Windows-native semantic icon");
    check(source.find("DrawSemanticSectionIcon(hdc, semanticicon::Section::Status") == std::string::npos,
          "Status heading reverted from the generic Info semantic glyph (issue #1)");
    check(source.find("DrawSemanticSectionIconDev(hdc, semanticicon::Section::Selected") == std::string::npos,
          "Selected header reverted from the semantic List glyph to an emoji (issue #1)");
    check(source.find("namespace semanticicon") != std::string::npos,
          "mod mirrors the pure semantic-icon model");
    check(source.find("semanticicon::ResolveWithFallback") != std::string::npos,
          "section icons resolve via the tested Fluent/MDL2/Shell fallback model");
    check(source.find("Segoe Fluent Icons") != std::string::npos,
          "section icons prefer Segoe Fluent Icons (Windows 11)");
    check(source.find("Segoe MDL2 Assets") != std::string::npos,
          "section icons fall back to Segoe MDL2 Assets (Windows 10)");
    check(source.find("SHGetStockIconInfo") != std::string::npos,
          "section icons fall back to a Shell stock icon when no icon font exists");
    // The old coloured-tile section-heading system is fully gone (no dead code).
    check(source.find("namespace sectiontile") == std::string::npos,
          "old coloured sectiontile system removed");
    check(source.find("DrawSectionTileDev") == std::string::npos,
          "old section tile device helper removed");
    check(source.find("DrawSectionIcon(") == std::string::npos,
          "old GDI section-tile renderer removed");
    check(source.find("StatusGlyphChar(d->statusState)") == std::string::npos,
          "status control still uses a GDI vector mark, not a Unicode glyph");
    check(source.find("SectionGlyph") == std::string::npos,
          "no stale custom SectionGlyph layer remains in the mod");
    check(source.find("RowGlyph") == std::string::npos,
          "row icons still use GDI vector marks, not Unicode glyphs");
    // fix thse isseue.md #6: one consistent stroke-width rule for every icon.
    check(source.find("IconStrokeWidth(u)") != std::string::npos,
          "icons use one consistent stroke-width rule (IconStrokeWidth), #6");
    check(source.find("u > 12 ? 2 : 1") == std::string::npos,
          "old per-icon stroke-width cliff is gone (#6)");

    if (failures == 0) std::puts("vector_icon_contract_test: PASS");
    return failures == 0 ? 0 : 1;
}
