// mode_logic_test.cpp
// Pins the PURE compact/expanded mode model for the popup (waht to create.md
// #15, plan Phase 7):
//   * a toggle that flips Compact <-> Expanded (remembered for the session),
//   * compact shows fewer selected-item rows, expanded gives the list more
//     room (more rows),
//   * a large selection auto-opens expanded so more of it is visible without
//     forcing the user to resize first.
//
// Self-contained: own main(), exit 0 = pass. RED before mode_logic.h exists;
// GREEN once it mirrors the mod.

#include "mode_logic.h"

#include <cstdio>

using namespace mode;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- toggle ----
    check(Toggle(Mode::Compact) == Mode::Expanded, "compact toggles to expanded");
    check(Toggle(Mode::Expanded) == Mode::Compact, "expanded toggles to compact");
    check(Toggle(Toggle(Mode::Compact)) == Mode::Compact, "double toggle is identity");
    check(IsExpanded(Mode::Expanded), "expanded is expanded");
    check(!IsExpanded(Mode::Compact), "compact is not expanded");

    // ---- rows per mode: expanded shows strictly more ----
    check(VisibleRows(Mode::Compact) == CompactRows(), "compact rows");
    check(VisibleRows(Mode::Expanded) == ExpandedRows(), "expanded rows");
    check(ExpandedRows() > CompactRows(), "expanded shows more rows than compact");
    check(CompactRows() >= 4, "compact keeps a usable viewport (>=4)");

    // ---- auto-mode: small fits compact, large auto-expands ----
    check(AutoMode(0) == Mode::Compact, "empty -> compact");
    check(AutoMode(1) == Mode::Compact, "1 item -> compact");
    check(AutoMode(static_cast<size_t>(CompactRows())) == Mode::Compact,
          "exactly compact-rows -> compact");
    check(AutoMode(static_cast<size_t>(CompactRows()) + 1) == Mode::Expanded,
          "one over compact -> expanded");
    check(AutoMode(100) == Mode::Expanded, "large selection -> expanded");

    // ---- initial mode: session memory wins, else auto-expand (#11/#15) ----
    // No session choice yet -> behave exactly like AutoMode.
    check(InitialMode(false, Mode::Compact, 1) == Mode::Compact,
          "no session, small -> compact (auto)");
    check(InitialMode(false, Mode::Compact, 100) == Mode::Expanded,
          "no session, large -> expanded (auto)");
    // A remembered session mode overrides the auto default in BOTH directions.
    check(InitialMode(true, Mode::Compact, 100) == Mode::Compact,
          "session compact overrides auto-expand for a large selection");
    check(InitialMode(true, Mode::Expanded, 1) == Mode::Expanded,
          "session expanded overrides auto-compact for a small selection");

    // ---- fix thse isseue.md #4: fresh popup auto-sizes by count; no leak ----
    // A FRESH popup (hasSessionMode == false) auto-sizes PURELY by count, so a
    // stale remembered value passed in does NOT leak: 4 items opens compact even
    // if a prior session was Expanded, and 5 opens expanded. (The mod sets the
    // session flag ONLY on a MANUAL toggle -- button id 103 / Ctrl+E.)
    check(InitialMode(false, Mode::Expanded, 4) == Mode::Compact,
          "#4 fresh 4-item popup is compact (stale Expanded does not leak)");
    check(InitialMode(false, Mode::Expanded, 5) == Mode::Expanded,
          "#4 fresh 5-item popup is expanded");
    check(CompactRows() == 4, "#4 compact viewport boundary is 4 rows");
    check(AutoMode(4) == Mode::Compact && AutoMode(5) == Mode::Expanded,
          "#4 boundary: 4 -> compact, 5 -> expanded");
    // A manual choice is still honoured across the session (remembered).
    check(InitialMode(true, Mode::Expanded, 4) == Mode::Expanded,
          "#4 a manually-chosen Expanded is remembered for a later 4-item popup");

    if (g_fail) { printf("mode_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("mode_logic_test: OK\n");
    return 0;
}
