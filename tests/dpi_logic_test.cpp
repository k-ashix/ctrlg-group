// dpi_logic_test.cpp
// Pins the PURE WM_DPICHANGED decisions (waht to create.md #16, Phase 9):
//   * extract the new DPI from the message wParam (the X DPI is the low word);
//   * decide whether a relayout is needed (only when the DPI actually changed
//     to a valid value).
// The actual re-layout uses layout::ScaleForDpi (already tested); this module
// owns the message-decoding decision the handler needs.
//
// Self-contained: own main(), exit 0 = pass. RED before dpi_logic.h exists;
// GREEN once it mirrors the mod.

#include "dpi_logic.h"

#include <cstdio>

using namespace dpiutil;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- new DPI is the low word of wParam (WM_DPICHANGED packs X|Y DPI) ----
    check(NewDpiFromWparam(96u) == 96, "plain 96");
    check(NewDpiFromWparam(120u) == 120, "plain 120");
    // both words hold the DPI: 0x00900090 -> 0x90 = 144
    check(NewDpiFromWparam(0x00900090u) == 144, "packed 144|144 -> 144");
    check(NewDpiFromWparam(0x00C000C0u) == 192, "packed 192|192 -> 192");
    // a differing high word must not leak in
    check(NewDpiFromWparam(0xFFFF0078u) == 120, "high word ignored -> 120");

    // ---- relayout only when the DPI actually changes to a valid value ----
    check(NeedsRelayout(96, 144), "96 -> 144 needs relayout");
    check(NeedsRelayout(192, 96), "192 -> 96 needs relayout");
    check(!NeedsRelayout(96, 96), "same DPI: no relayout");
    check(!NeedsRelayout(96, 0), "invalid new DPI: no relayout");
    check(!NeedsRelayout(120, -10), "negative new DPI: no relayout");

    if (g_fail) { printf("dpi_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("dpi_logic_test: OK\n");
    return 0;
}
