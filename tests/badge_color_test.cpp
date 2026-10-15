// badge_color_test.cpp
// Pins fix thse isseue.md #12: the extension badge gets a SOFT, category-based
// colour (soft fill + darker ink for text/border); unknown types keep the
// themed neutral chip (inherit). Self-contained: own main(), exit 0 = pass.

#include "badge_color_logic.h"

#include <cstdio>

using namespace badgecolor;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- label normalisation (ExtLabel gives UPPERCASE, no dot) ----
    check(Norm(L"PDF") == L"pdf", "uppercase label -> lowercase");
    check(Norm(L".JPG") == L"jpg", "leading dot tolerated");
    check(Norm(L"Mp3") == L"mp3", "mixed case -> lowercase");

    // ---- categories (ExtLabel is uppercase; mapping is case-insensitive) ----
    check(CategoryFor(L"JPG")  == Cat::Image,    "JPG -> Image");
    check(CategoryFor(L"PNG")  == Cat::Image,    "PNG -> Image");
    check(CategoryFor(L"MP3")  == Cat::Audio,    "MP3 -> Audio");
    check(CategoryFor(L"FLAC") == Cat::Audio,    "FLAC -> Audio");
    check(CategoryFor(L"MP4")  == Cat::Video,    "MP4 -> Video");
    check(CategoryFor(L"PDF")  == Cat::Document, "PDF -> Document");
    check(CategoryFor(L"DOCX") == Cat::Document, "DOCX -> Document");
    check(CategoryFor(L"ZIP")  == Cat::Archive,  "ZIP -> Archive");
    check(CategoryFor(L"CPP")  == Cat::Code,     "CPP -> Code");
    check(CategoryFor(L"JSON") == Cat::Code,     "JSON -> Code");
    check(CategoryFor(L"XYZ")  == Cat::Neutral,  "unknown -> Neutral");
    check(CategoryFor(L"")     == Cat::Neutral,  "empty -> Neutral");

    // ---- known categories are tinted; neutral inherits the themed chip ----
    for (bool dark : {false, true}) {
        check(ChipFor(Cat::Neutral, dark).inherit, "Neutral inherits themed chip");
        const Cat known[] = {Cat::Image, Cat::Audio, Cat::Video,
                             Cat::Document, Cat::Archive, Cat::Code};
        for (Cat c : known) {
            Chip ch = ChipFor(c, dark);
            check(!ch.inherit, "known category has its own colour");
            check((ch.fill & 0xFF000000u) == 0u, "fill packs into 24 bits");
            check((ch.ink  & 0xFF000000u) == 0u, "ink packs into 24 bits");
            check(ch.fill != ch.ink, "fill differs from ink (soft bg + darker ink)");
        }
    }

    // ---- categories are visually distinct from one another (light) ----
    check(ChipFor(Cat::Image, false).fill    != ChipFor(Cat::Audio, false).fill,
          "image fill != audio fill");
    check(ChipFor(Cat::Document, false).fill  != ChipFor(Cat::Code, false).fill,
          "document fill != code fill");
    check(ChipFor(Cat::Archive, false).fill   != ChipFor(Cat::Video, false).fill,
          "archive fill != video fill");

    // ---- light vs dark differ for the same category ----
    check(ChipFor(Cat::Image, false).fill != ChipFor(Cat::Image, true).fill,
          "image fill differs by theme");

    // ---- end-to-end convenience resolver ----
    check(!ChipForLabel(L"PNG", false).inherit, "PNG label resolves to a tinted chip");
    check(ChipForLabel(L"QWE", false).inherit,  "unknown label inherits");

    if (g_fail == 0) printf("badge_color_test: OK\n");
    else             printf("badge_color_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
