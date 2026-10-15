// icon_source_logic.h
//
// PURE icon-SOURCE selection model for fix thse isseue.md Phase 1: the Selected
// Items rows (List / Large icons / Extra large icons) must draw each Windows
// Shell icon from a system image-list TIER whose physical resolution is >= the
// size the painter draws at, so a Large / Extra large icon is DOWNSAMPLED from
// a crisp high-resolution source instead of being UPSCALED from a 16px small
// icon (the old blur). The real Win32 extraction (SHGetFileInfoW with
// SHGFI_SYSICONINDEX -> SHGetImageList(SHIL_*) -> IImageList::GetIcon) and the
// per-(type,tier,dpi) HICON cache are asserted by scripts/regression_check.ps1
// source invariants; this header is the byte-for-byte mirror of namespace
// iconsrc in the mod, unit-tested with a plain g++ build (no Win32 runtime).
//
// Depends on viewmode:: (the presentation mode) and composes with
// shellicon::IconKey (the per-type cache key). Microsoft documents the system
// image-list sizes: SHIL_SMALL (16), SHIL_EXTRALARGE (48, DPI-scalable with
// DPI awareness) and SHIL_JUMBO (a fixed 256 physical px).
//
//   SHIL_SMALL       1
//   SHIL_EXTRALARGE  2
//   SHIL_JUMBO       4
//
#pragma once

#include <string>

#include "view_mode_logic.h"

namespace iconsrc {

// Shell system image-list tiers. The numeric values ARE the real SHIL_*
// constants handed to SHGetImageList.
enum class Tier { Small = 1, ExtraLarge = 2, Jumbo = 4 };

inline int Shil(Tier t) { return static_cast<int>(t); }

// Nominal 96-DPI source resolution each tier supplies.
inline int TierBasePx(Tier t) {
    switch (t) {
        case Tier::Small:      return 16;
        case Tier::ExtraLarge: return 48;
        case Tier::Jumbo:      return 256;
    }
    return 16;
}

// MulDiv-rounded DPI scale (matches the mod's ScaleDpi(px, dpi): 96->x1).
inline int ScalePx(int basePx, unsigned dpi) {
    return static_cast<int>((static_cast<unsigned>(basePx) * dpi + 48u) / 96u);
}

// The physical resolution a tier actually supplies at the active DPI. JUMBO is
// a FIXED 256px bitmap regardless of DPI; SMALL / EXTRALARGE scale with DPI
// awareness (Per-Monitor-V2).
inline int TierPx(Tier t, unsigned dpi) {
    if (t == Tier::Jumbo) return 256;
    return ScalePx(TierBasePx(t), dpi);
}

// The physical pixel size the painter draws the row icon at, for a mode + DPI.
// Mirrors the paint layer: the 96-base per-mode IconPx (16 / 48 / 96) scaled by
// the active window DPI.
inline int DrawPx(viewmode::Mode m, unsigned dpi) {
    return ScalePx(viewmode::IconPx(m), dpi);
}

// The descriptive spec mapping: List -> small, Large -> extra-large, Extra
// large -> jumbo.
inline Tier TierForMode(viewmode::Mode m) {
    switch (m) {
        case viewmode::Mode::List:       return Tier::Small;
        case viewmode::Mode::Large:      return Tier::ExtraLarge;
        case viewmode::Mode::ExtraLarge: return Tier::Jumbo;
    }
    return Tier::Small;
}

// DPI-aware selection the painter uses: the SMALLEST tier whose physical
// resolution covers drawPx, so we always downsample (crisp) and never upscale
// (blur). Falls back to the 256px jumbo when nothing smaller covers. At every
// supported DPI (96..192) this reduces to the descriptive TierForMode mapping.
inline Tier SelectTier(viewmode::Mode m, unsigned dpi) {
    const int need = DrawPx(m, dpi);
    if (TierPx(Tier::Small, dpi) >= need)      return Tier::Small;
    if (TierPx(Tier::ExtraLarge, dpi) >= need) return Tier::ExtraLarge;
    return Tier::Jumbo;
}

// True when the selected source is at least as large as the drawn size (the
// Phase 1 anti-blur guarantee).
inline bool SourceCoversDraw(viewmode::Mode m, unsigned dpi) {
    return TierPx(SelectTier(m, dpi), dpi) >= DrawPx(m, dpi);
}

// A stable cache key for ONE extracted icon source: the per-type key (from
// shellicon::IconKey -- folder / lowercased extension / <noext>), the requested
// source tier, and the DPI. Deliberately independent of the scroll offset, the
// visible range and the view mode, so scrolling and zooming reuse the SAME
// cached source instead of re-extracting a lower-resolution copy.
inline std::wstring CacheKey(const std::wstring& typeKey, Tier t, unsigned dpi) {
    return typeKey + L"|t" + std::to_wstring(static_cast<int>(t)) +
           L"|d" + std::to_wstring(dpi);
}

}  // namespace iconsrc
