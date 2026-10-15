// dpi_matrix_test.cpp
// Pins the automated half of fix thse isseue.md #15 ("final light/dark +
// 125/150/200 DPI"): the anti-aliased rounded SURFACES must keep crisp,
// correctly-scaled corner radii across 100/125/150/200%. This is a verify+pin
// slice over the already-green aarender::DeviceRadius policy -- it does NOT add
// new behaviour; it locks the full DPI matrix (the per-theme SCREENSHOTS are a
// manual pass on the user's machine, documented in codeoverview.md). Exit 0 = pass.

#include "aa_render_logic.h"

#include <cstdio>
#include <cmath>

using namespace aarender;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}
static bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

int main() {
    struct Scale { int dpi; float factor; };          // 100/125/150/200%
    const Scale M[] = {{96, 1.0f}, {120, 1.25f}, {144, 1.5f}, {192, 2.0f}};
    // representative design radii used by the AA surfaces (cards/buttons/focus
    // 8..14, tiles/badges/progress 3..5).
    const int radii[] = {3, 5, 8, 14};

    for (int r : radii) {
        // identity at 96 dpi
        check(near(DeviceRadius(r, 96), static_cast<float>(r)),
              "radius is identity at 96 dpi");
        float prev = -1.0f;
        for (const Scale& s : M) {
            const float got = DeviceRadius(r, s.dpi);
            // exact float scale -- the Direct2D anti-aliasing win over integer GDI
            check(near(got, r * s.factor), "radius scales by the dpi factor");
            // monotonic non-decreasing as dpi grows
            check(got >= prev, "radius is monotonic across the dpi matrix");
            prev = got;
        }
        // exactly 2x at 200%
        check(near(DeviceRadius(r, 192), DeviceRadius(r, 96) * 2.0f),
              "radius doubles at 200%");
    }

    // the stroke inset is dpi-independent (always half the stroke width)
    check(near(StrokeInset(1.0f), 0.5f), "1px stroke inset stays 0.5");
    check(near(StrokeInset(2.0f), 1.0f), "2px stroke inset stays 1.0");

    // the seven AA surfaces stay anti-aliased regardless of dpi (policy is
    // dpi-independent; the radius above is what scales).
    const Surface aa[] = {Surface::Card, Surface::Button, Surface::Segment,
                          Surface::Badge, Surface::Tile, Surface::Progress,
                          Surface::FocusRing};
    for (Surface s : aa) check(UsesAntialiasedGeometry(s), "surface stays AA");
    check(!UsesAntialiasedGeometry(Surface::EditField), "EDIT stays Win32");
    check(!UsesAntialiasedGeometry(Surface::Window),    "window stays Win32");

    if (g_fail == 0) printf("dpi_matrix_test: OK\n");
    else             printf("dpi_matrix_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
