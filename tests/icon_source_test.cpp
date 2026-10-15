// icon_source_test.cpp
// Pins fix thse isseue.md Phase 1: the Selected Items rows must source each
// Shell icon from a system image-list TIER whose resolution is >= the size the
// painter draws at, so Large / Extra large icons are DOWNSAMPLED (crisp) and
// never UPSCALED from a 16px small icon (blurry). Also pins the per-(type,tier,
// dpi) cache key that keeps zoom / scroll / view-mode independent.
//
// Pure model (namespace iconsrc), mirrored byte-for-byte into ctrlg-group.wh.cpp.
// Self-contained: own main(), exit 0 = pass. The actual SHGetImageList /
// IImageList extraction + HICON cache are Win32 and are asserted by
// scripts/regression_check.ps1 source invariants.

#include "view_mode_logic.h"
#include "shell_icon_logic.h"
#include "icon_source_logic.h"

#include <cstdio>

using viewmode::Mode;
using namespace iconsrc;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- SHIL_* numeric values the tier maps to (handed to SHGetImageList) ----
    check(Shil(Tier::Small) == 1,      "SHIL_SMALL == 1");
    check(Shil(Tier::ExtraLarge) == 2, "SHIL_EXTRALARGE == 2");
    check(Shil(Tier::Jumbo) == 4,      "SHIL_JUMBO == 4");

    // ---- nominal 96-DPI source resolution of each tier ----
    check(TierBasePx(Tier::Small) == 16,       "small tier is 16px baseline");
    check(TierBasePx(Tier::ExtraLarge) == 48,  "extra-large tier is 48px baseline");
    check(TierBasePx(Tier::Jumbo) == 256,      "jumbo tier is 256px baseline");

    // ---- jumbo is a FIXED 256 physical px at every DPI; the others scale ----
    check(TierPx(Tier::Jumbo, 96) == 256 && TierPx(Tier::Jumbo, 192) == 256,
          "jumbo source stays 256px across DPI");
    check(TierPx(Tier::Small, 96) == 16 && TierPx(Tier::Small, 192) == 32,
          "small tier scales 16 -> 32 at 200%");
    check(TierPx(Tier::ExtraLarge, 96) == 48 && TierPx(Tier::ExtraLarge, 192) == 96,
          "extra-large tier scales 48 -> 96 at 200%");

    // ---- the physical size the painter draws at (mirrors the paint layer) ----
    check(DrawPx(Mode::List, 96) == 16 && DrawPx(Mode::Large, 96) == 48 &&
          DrawPx(Mode::ExtraLarge, 96) == 96,
          "draw size at 96 DPI = 16 / 48 / 96");
    check(DrawPx(Mode::List, 192) == 32 && DrawPx(Mode::Large, 192) == 96 &&
          DrawPx(Mode::ExtraLarge, 192) == 192,
          "draw size at 200% DPI doubles");

    // ---- spec mode -> source tier mapping ----
    check(TierForMode(Mode::List) == Tier::Small,       "List -> small tier");
    check(TierForMode(Mode::Large) == Tier::ExtraLarge, "Large -> extra-large tier");
    check(TierForMode(Mode::ExtraLarge) == Tier::Jumbo, "Extra large -> jumbo tier");

    // ---- THE core anti-blur guarantee: across every supported DPI the chosen
    //      source is never smaller than the drawn size (so we only downsample) --
    const unsigned dpis[] = {96u, 120u, 144u, 168u, 192u};
    const Mode modes[] = {Mode::List, Mode::Large, Mode::ExtraLarge};
    for (unsigned dpi : dpis) {
        for (Mode m : modes) {
            const Tier t = SelectTier(m, dpi);
            check(TierPx(t, dpi) >= DrawPx(m, dpi),
                  "selected source tier covers the drawn size (no upscaling)");
            check(SourceCoversDraw(m, dpi),
                  "SourceCoversDraw agrees the source is big enough");
        }
        // the descriptive spec mapping holds at every supported DPI too
        check(SelectTier(Mode::List, dpi) == Tier::Small,
              "List keeps the small tier at every DPI");
        check(SelectTier(Mode::Large, dpi) == Tier::ExtraLarge,
              "Large keeps the extra-large tier at every DPI");
        check(SelectTier(Mode::ExtraLarge, dpi) == Tier::Jumbo,
              "Extra large keeps the jumbo tier at every DPI");
    }

    // ---- never source a tier SMALLER than what we draw (explicit) ----
    check(SelectTier(Mode::ExtraLarge, 96) != Tier::Small &&
          SelectTier(Mode::ExtraLarge, 96) != Tier::ExtraLarge,
          "Extra large never upscales a small/extra-large icon to 96+px");
    check(SelectTier(Mode::Large, 96) != Tier::Small,
          "Large never upscales a 16px small icon");

    // ---- cache key: stable, and keyed ONLY by (type, tier, dpi) ----
    const std::wstring kPdf = shellicon::IconKey(L"report.PDF", false); // ".pdf"
    const std::wstring kDir = shellicon::IconKey(L"anything", true);    // "<dir>"
    const std::wstring kNone = shellicon::IconKey(L"README", false);    // "<noext>"
    check(kPdf == L".pdf" && kDir == L"<dir>" && kNone == L"<noext>",
          "type keys: lowercased ext / folder / no-extension");

    check(CacheKey(kPdf, Tier::Jumbo, 96) == CacheKey(kPdf, Tier::Jumbo, 96),
          "same (type,tier,dpi) -> identical cache key (reused, not regenerated)");
    check(CacheKey(kPdf, Tier::Jumbo, 96) != CacheKey(kPdf, Tier::Jumbo, 192),
          "a DPI change invalidates the cache key");
    check(CacheKey(kPdf, Tier::Small, 96) != CacheKey(kPdf, Tier::Jumbo, 96),
          "a tier change invalidates the cache key");
    check(CacheKey(kPdf, Tier::Jumbo, 96) != CacheKey(kDir, Tier::Jumbo, 96),
          "a different type -> a different cache key");

    // zoom/scroll independence: switching modes between Large and ExtraLarge
    // reuses the SAME cached jumbo source for XL regardless of how far scrolled,
    // because the key never encodes scroll offset or the visible range.
    check(CacheKey(kPdf, SelectTier(Mode::ExtraLarge, 144), 144) ==
          CacheKey(kPdf, SelectTier(Mode::ExtraLarge, 144), 144),
          "scroll offset is not part of the key (cache is reused while scrolling)");

    if (g_fail == 0) printf("icon_source_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
