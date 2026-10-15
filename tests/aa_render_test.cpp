// aa_render_test.cpp
// Pins fix thse isseue.md #2/#12: the custom-painted rounded SURFACES move from
// GDI RoundRect (stair-stepped corners) to Direct2D anti-aliased geometry. This
// test owns the PURE policy -- which surfaces are anti-aliased, the per-DPI
// float corner radius, the per-primitive AA mode constant, the stroke inset,
// and the GDI fallback flag. The live Direct2D paint is clang-only and pinned
// by source invariants. Self-contained: own main(), exit 0 = pass.

#include "aa_render_logic.h"

#include <cstdio>
#include <cmath>

using namespace aarender;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}
static bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }

int main() {
    // ---- the custom rounded surfaces are anti-aliased ----
    check(UsesAntialiasedGeometry(Surface::Card),      "Card is anti-aliased");
    check(UsesAntialiasedGeometry(Surface::Button),    "Button is anti-aliased");
    check(UsesAntialiasedGeometry(Surface::Segment),   "Segment is anti-aliased");
    check(UsesAntialiasedGeometry(Surface::Badge),     "Badge is anti-aliased");
    check(UsesAntialiasedGeometry(Surface::Tile),      "Tile is anti-aliased");
    check(UsesAntialiasedGeometry(Surface::Progress),  "Progress is anti-aliased");
    check(UsesAntialiasedGeometry(Surface::FocusRing), "FocusRing is anti-aliased");
    // fix thse isseue.md #5 (RoundRect scan): the Selected Items scrollbar thumb
    // was the last RAW GDI RoundRect; it now routes through the AA layer too.
    check(UsesAntialiasedGeometry(Surface::ScrollThumb), "ScrollThumb is anti-aliased");

    // ---- EDIT controls + the window frame stay pure Win32 ----
    check(!UsesAntialiasedGeometry(Surface::EditField), "EditField stays Win32");
    check(!UsesAntialiasedGeometry(Surface::Window),    "Window frame stays Win32");

    // ---- the per-primitive AA mode constant matches D2D1_ANTIALIAS_MODE_PER_PRIMITIVE (0) ----
    check(kAntialiasPerPrimitive == 0, "per-primitive AA mode constant is 0");

    // ---- GDI fallback keeps the UI safe when Direct2D is unavailable ----
    check(kFallbackToGdiOnFailure, "falls back to GDI on failure");

    // ---- float, DPI-scaled corner radius (the real anti-aliasing win) ----
    check(near(DeviceRadius(8, 96), 8.0f),   "radius identity at 96 dpi");
    check(near(DeviceRadius(8, 192), 16.0f), "radius doubles at 192 dpi");
    check(near(DeviceRadius(5, 120), 6.25f), "radius is a float at 125% dpi");
    check(near(DeviceRadius(0, 192), 0.0f),  "zero radius stays zero");
    check(DeviceRadius(8, 192) > DeviceRadius(8, 96), "radius grows with dpi");
    check(DeviceRadius(8, 150) > DeviceRadius(8, 120), "radius is monotonic in dpi");

    // ---- an AA hairline is inset by half its width to stay inside the rect ----
    check(near(StrokeInset(1.0f), 0.5f), "1px stroke inset is 0.5");
    check(near(StrokeInset(2.0f), 1.0f), "2px stroke inset is 1.0");

    if (g_fail == 0) printf("aa_render_test: OK\n");
    else             printf("aa_render_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
