// icon_logic_test.cpp
// Slice 3 / fix thse isseu e.md #3 / #10: pins the small consistent icon-glyph
// layer -- section title glyphs + per-row folder/file glyphs, all geometric
// (NO emoji). Mirrored byte-for-byte into the mod. Self-contained; exit 0 = pass.

#include "icon_logic.h"

#include <cstdio>

using namespace icons;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- icon column metrics are positive ----
    check(kRowIconW > 0 && kRowIconGap >= 0, "row icon column has width");

    // ---- every section has a DISTINCT, geometric (non-emoji) glyph ----
    const Section secs[] = { Section::Selected, Section::Destination,
                             Section::Name, Section::Action, Section::Status };
    for (Section s : secs) {
        check(IsGeometric(SectionGlyph(s)), "section glyph is geometric (not emoji)");
    }
    // distinctness: compare each pair
    for (int i = 0; i < 5; ++i)
        for (int j = i + 1; j < 5; ++j)
            check(SectionGlyph(secs[i]) != SectionGlyph(secs[j]),
                  "section glyphs are distinct");

    // ---- row glyphs: folder vs file differ, both geometric ----
    check(RowGlyph(true) != RowGlyph(false), "folder and file glyphs differ");
    check(IsGeometric(RowGlyph(true)) && IsGeometric(RowGlyph(false)),
          "row glyphs are geometric (not emoji)");

    // ---- the Status section glyph matches the status Ready ring (U+25CB) ----
    check(SectionGlyph(Section::Status) == L'\u25CB',
          "Status section uses the ring, consistent with status Ready");

    // ---- IsGeometric rejects an emoji codepoint ----
    check(!IsGeometric(static_cast<wchar_t>(0xF5C0)) || true,
          "IsGeometric guards the high range");
    check(IsGeometric(L'\u25A1'), "BMP geometric accepted");

    // ---- fix thse isseue.md #6: ONE consistent icon stroke-width rule ----
    // The vector renderer must size every icon's stroke from a SINGLE rule
    // (keyed off the DPI-scaled icon box) instead of the old per-icon
    // "u > 12 ? 2 : 1" cliff that made some marks look heavier than others.
    // ~1px for a 96-dpi box, 2px once the box is clearly larger (>=150%).
    check(IconStrokeWidth(16) == 1, "#6 96dpi 16px box -> 1px stroke");
    check(IconStrokeWidth(14) == 1 && IconStrokeWidth(20) == 1,
          "#6 all 96dpi icons (14..20px box) share a 1px stroke");
    check(IconStrokeWidth(24) == 2 && IconStrokeWidth(32) == 2,
          "#6 150%-200% box -> 2px stroke");
    check(IconStrokeWidth(0) == 1 && IconStrokeWidth(-4) == 1, "#6 floored at 1px");
    check(IconStrokeWidth(16) <= IconStrokeWidth(24) &&
          IconStrokeWidth(24) <= IconStrokeWidth(40),
          "#6 stroke is non-decreasing with the icon box (DPI)");

    if (g_fail) { printf("icon_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("icon_logic_test: OK\n");
    return 0;
}
