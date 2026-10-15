// namefield_d2d_test.cpp
// Pins fix thse isseue.md S11 (v0.12.51): the Folder name field and the
// read-only Destination field are the dominant surfaces the user looks at, so
// their painted rounded SURFACE (fill + hairline border) must now route through
// the Direct2D anti-aliased pipeline instead of raw GDI RoundRect. This test
// locks the aarender POLICY that makes that correct:
//   * the field-surface FILL rides Surface::Button   (anti-aliased)
//   * the field-surface BORDER rides Surface::FocusRing (anti-aliased)
//   * the EDIT control itself stays Surface::EditField (NOT anti-aliased -- it
//     is a plain Win32 control; we never try to D2D-paint its glyphs)
// It also checks the per-DPI float corner radius the D2D path needs so the arc
// stays smooth at 125/150/200% scaling. Self-contained: own main(), 0 == pass.

#include "aa_render_logic.h"

#include <cstdio>
#include <cmath>

using namespace aarender;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}
static bool nearly(float a, float b) { return std::fabs(a - b) < 0.001f; }

int main() {
    // The two surfaces the name/dest field paint now uses MUST be anti-aliased,
    // otherwise S11 would silently fall back to the stair-stepped look.
    check(UsesAntialiasedGeometry(Surface::Button),
          "field-surface FILL (Surface::Button) is anti-aliased");
    check(UsesAntialiasedGeometry(Surface::FocusRing),
          "field-surface BORDER (Surface::FocusRing) is anti-aliased");

    // The EDIT control stays pure Win32: its surface is NOT routed through D2D.
    // This is the guardrail that keeps the input control's text crisp + native.
    check(!UsesAntialiasedGeometry(Surface::EditField),
          "the EDIT control (Surface::EditField) stays Win32 (NOT anti-aliased)");

    // GDI fallback must stay on so an unavailable D2D device degrades to today's
    // RoundRect rather than painting nothing.
    check(kFallbackToGdiOnFailure,
          "GDI RoundRect fallback stays enabled for the field surface");

    // Per-DPI float radius: identity at 96, exact scaling at the common DPIs.
    // The field uses a small design radius; verify the model for a few.
    const int designRadius = 6;  // representative name/dest field corner radius
    check(nearly(DeviceRadius(designRadius, 96), 6.0f),
          "field radius is identity at 96 dpi");
    check(nearly(DeviceRadius(designRadius, 120), 7.5f),
          "field radius scales at 125% (120 dpi)");
    check(nearly(DeviceRadius(designRadius, 144), 9.0f),
          "field radius scales at 150% (144 dpi)");
    check(nearly(DeviceRadius(designRadius, 192), 12.0f),
          "field radius is exactly 2x at 200% (192 dpi)");

    // A 1px hairline stroke is inset by half its width so the AA edge stays
    // inside the field rect (no bleed over the card).
    check(nearly(StrokeInset(1.0f), 0.5f),
          "1px field border is inset by half its width");

    if (g_fail == 0) printf("namefield_d2d_test: OK\n");
    else             printf("namefield_d2d_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
