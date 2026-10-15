// focus_logic_test.cpp
// Pins the PURE focus-ring + tab-order model (waht to create.md #13, Phase 9):
//   * initial focus is the name field;
//   * Tab walks a logical order (Group includes the Copy/Move toggle; New
//     Folder skips it), Shift+Tab reverses, both wrap;
//   * every interactive control shows a focus ring, and the ring is a real
//     DPI-scaled outline (visible without relying on color alone).
//
// Self-contained: own main(), exit 0 = pass. RED before focus_logic.h exists;
// GREEN once it mirrors the mod.

#include "focus_logic.h"

#include <cstdio>

using namespace focus;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    check(First() == Ctl::Name, "initial focus is the name field");

    // ---- Group tab order: Name -> OpToggle -> Create -> Cancel -> (wrap) ----
    check(Next(Ctl::Name, true) == Ctl::OpToggle, "group: Name -> OpToggle");
    check(Next(Ctl::OpToggle, true) == Ctl::Create, "group: OpToggle -> Create");
    check(Next(Ctl::Create, true) == Ctl::Cancel, "group: Create -> Cancel");
    check(Next(Ctl::Cancel, true) == Ctl::Name, "group: Cancel wraps to Name");
    check(Prev(Ctl::OpToggle, true) == Ctl::Name, "group: Shift+Tab OpToggle -> Name");
    check(Prev(Ctl::Name, true) == Ctl::Cancel, "group: Shift+Tab Name wraps to Cancel");
    check(Prev(Next(Ctl::Create, true), true) == Ctl::Create, "group: Next then Prev round-trips");

    // ---- New Folder tab order skips the (absent) Copy/Move toggle ----
    check(Next(Ctl::Name, false) == Ctl::Create, "nf: Name -> Create (no toggle)");
    check(Next(Ctl::Create, false) == Ctl::Cancel, "nf: Create -> Cancel");
    check(Next(Ctl::Cancel, false) == Ctl::Name, "nf: Cancel wraps to Name");
    check(Prev(Ctl::Create, false) == Ctl::Name, "nf: Shift+Tab Create -> Name");
    check(Prev(Next(Ctl::Name, false), false) == Ctl::Name, "nf: Next then Prev round-trips");

    // ---- ring shown on every interactive control ----
    check(ShowsFocusRing(Ctl::Name), "ring on Name");
    check(ShowsFocusRing(Ctl::OpToggle), "ring on OpToggle");
    check(ShowsFocusRing(Ctl::Create), "ring on Create");
    check(ShowsFocusRing(Ctl::Cancel), "ring on Cancel");

    // fix thse isseue.md #4: the painted ring (DPI-scaled outline) was removed
    // in favour of a focus-COLOUR change, so the ring-geometry helpers are gone.
    // Keyboard-focus-visibility is pinned by focus_visible_test; the focus
    // COLOUR tint is pinned by hover_tint_test (kFocusPct) + the source gate.

    if (g_fail) { printf("focus_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("focus_logic_test: OK\n");
    return 0;
}
