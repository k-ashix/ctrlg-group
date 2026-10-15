// toggle_switch_test.cpp
// Pins fix thse isseue.md Phase 3: the Show-extensions toolbar control is a
// COMPACT ON/OFF switch with NO inline text label. It has an accessible name
// ("Show file extensions") and a 2:1 pill track whose knob sits LEFT when OFF
// and RIGHT when ON, giving clear state feedback without any text.
// Self-contained: own main(), exit 0 = pass.

#include "show_ext_logic.h"

#include <cstdio>
#include <cwchar>

using namespace showext;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    check(wcscmp(AccessibleName(), L"Show file extensions") == 0,
          "accessible name is the descriptive phrase (no visible text label)");

    const int h = 20, inset = 2;
    const int w = SwitchTrackW(h);
    check(w == 40, "track is a 2:1 pill");
    const int d = SwitchKnobDiameter(h, inset);
    check(d == 16, "knob is inset on all sides");

    // OFF -> knob at the left; ON -> knob at the right.
    check(SwitchKnobLeft(100, w, h, inset, false) == 102, "OFF knob sits left");
    check(SwitchKnobLeft(100, w, h, inset, true) == 100 + 40 - 2 - 16,
          "ON knob sits right");
    check(SwitchKnobLeft(100, w, h, inset, false) !=
              SwitchKnobLeft(100, w, h, inset, true),
          "knob moves between states (visible ON/OFF without a label)");

    // Degenerate heights never produce a non-positive knob.
    check(SwitchKnobDiameter(2, 2) >= 1, "knob diameter clamped to >= 1");

    if (g_fail) { printf("toggle_switch_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("toggle_switch_test: OK\n");
    return 0;
}
