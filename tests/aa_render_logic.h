// aa_render_logic.h
//
// PURE policy for fix thse isseue.md #2/#12: the custom-painted rounded UI
// SURFACES move off GDI RoundRect (which stair-steps the corners) onto Direct2D
// anti-aliased geometry. This header owns only the DECISIONS a unit test can
// see -- which surfaces are anti-aliased, the per-DPI float corner radius, the
// per-primitive AA-mode constant, the hairline stroke inset, and the GDI
// fallback flag. Byte-for-byte mirror of namespace aarender in
// ctrlg-group.wh.cpp; the live Direct2D paint lives in the mod
// (namespace d2d) and is pinned by source invariants.
//
#pragma once

namespace aarender {

// The custom-painted rounded SURFACES that render with anti-aliased geometry.
// EDIT controls and the window frame stay pure Win32 (the review: "Keep normal
// Win32 controls for EDIT / window input/focus").
enum class Surface { Card, Button, Segment, Badge, Tile, Progress, FocusRing,
                     ScrollThumb, EditField, Window };

inline bool UsesAntialiasedGeometry(Surface s) {
    switch (s) {
        case Surface::Card:
        case Surface::Button:
        case Surface::Segment:
        case Surface::Badge:
        case Surface::Tile:
        case Surface::Progress:
        case Surface::FocusRing:
        case Surface::ScrollThumb:
            return true;
        case Surface::EditField:
        case Surface::Window:
        default:
            return false;
    }
}

// Direct2D's D2D1_ANTIALIAS_MODE_PER_PRIMITIVE == 0. Pinned here so the mod's
// mirror can static_assert it against the real Direct2D enum.
inline constexpr int kAntialiasPerPrimitive = 0;

// If any Direct2D step fails (no factory, lost device, bind failure) the caller
// falls back to the existing GDI RoundRect, so the UI degrades to today's
// behaviour instead of breaking.
inline constexpr bool kFallbackToGdiOnFailure = true;

// Device-pixel corner radius for a design radius at a given DPI. A FLOAT (not
// the integer truncation GDI forces) is what lets Direct2D anti-alias the arc
// cleanly at 125/150/200% scaling.
inline float DeviceRadius(int designRadiusPx, int dpi) {
    return static_cast<float>(designRadiusPx) * static_cast<float>(dpi) / 96.0f;
}

// A hairline centred on the geometry edge is inset by half its width so the
// anti-aliased stroke stays fully inside the surface rect (no bleed).
inline float StrokeInset(float strokeWidthDev) { return strokeWidthDev * 0.5f; }

}  // namespace aarender
