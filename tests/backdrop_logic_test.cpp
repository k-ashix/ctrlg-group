// backdrop_logic_test.cpp
// Pins the PURE Win11 system-backdrop decision (fix thse isseue.md #7): the
// popup only lets the translucent acrylic backdrop show through (a transparent,
// premium, minimal Win11 window) INSTEAD of the flat, opaque "boxed" panel when
// it is Win11 + the backdrop was requested + the user's Transparency effects are
// ON. Any failure keeps the solid themed panel (the safe fallback that avoids
// the white-panel bug on Win10 / transparency-off / remote sessions).
// Self-contained: own main(), exit 0 = pass. RED before backdrop_logic.h exists.

#include "backdrop_logic.h"

#include <cstdio>

using namespace backdrop;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- material is the Win11 transient acrylic backdrop ----
    check(kBackdropAcrylic == 3, "acrylic backdrop is DWMSBT_TRANSIENTWINDOW (3)");

    // ---- all three conditions true -> show the backdrop (transparent look) ----
    check(ChooseFill(true, true, true) == Fill::SystemBackdrop,
          "win11 + requested + transparency -> system backdrop");
    check(SkipSolidFill(true, true, true),
          "skip the solid fill when the backdrop will composite");

    // ---- any condition false -> keep the solid themed panel (safe fallback) ----
    check(ChooseFill(false, true, true)  == Fill::SolidPanel, "non-win11 keeps the solid panel");
    check(ChooseFill(true, false, true)  == Fill::SolidPanel, "no backdrop requested keeps the solid panel");
    check(ChooseFill(true, true, false)  == Fill::SolidPanel, "transparency OFF keeps the solid panel");
    check(!SkipSolidFill(false, true, true), "non-win11 does NOT skip the solid fill");
    check(!SkipSolidFill(true, false, true), "no request does NOT skip the solid fill");
    check(!SkipSolidFill(true, true, false), "transparency OFF does NOT skip the solid fill");

    // ---- fix thse isseue.md #1: the HONEST-HRESULT decision ----
    // Dropping the solid fill now ALSO requires the real DwmSetWindowAttribute
    // backdrop request to have SUCCEEDED (backdropApplied). The OS version + the
    // Transparency setting are NOT proof on their own -- that was the bug.
    check(ChooseFillChecked(true, true, true, true) == Fill::SystemBackdrop,
          "win11 + requested + transparency + HRESULT ok -> system backdrop");
    check(SkipSolidFillChecked(true, true, true, true),
          "skip the solid fill only when the backdrop ACTUALLY applied");
    // Reproduction of the bug: everything "looks" right (win11 + requested +
    // transparency) but the HRESULT FAILED -> keep the solid panel.
    check(ChooseFillChecked(true, true, true, false) == Fill::SolidPanel,
          "backdrop request FAILED keeps the solid panel (no assuming success)");
    check(!SkipSolidFillChecked(true, true, true, false),
          "HRESULT failure does NOT skip the solid fill");
    // The other inputs still gate exactly like the 3-arg policy, even with a
    // successful HRESULT.
    check(ChooseFillChecked(false, true, true, true) == Fill::SolidPanel,
          "non-win11 keeps the solid panel (checked)");
    check(ChooseFillChecked(true, false, true, true) == Fill::SolidPanel,
          "no backdrop requested keeps the solid panel (checked)");
    check(ChooseFillChecked(true, true, false, true) == Fill::SolidPanel,
          "transparency OFF keeps the solid panel (checked)");

    if (g_fail) { printf("backdrop_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("backdrop_logic_test: OK\n");
    return 0;
}
