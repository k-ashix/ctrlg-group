// focus_route_test.cpp
// Pins fix thse isseue.md P1:
//   * "Keyboard navigation does not exclude every interactive control"
//     (lines 12283-12319 / 12354-12369): chip Left/Right AND list
//     Up/Down/Home/End/PageUp/PageDown must be driven by ONE centralized
//     focus classification. A keystroke drives the chip strip / Selected list
//     ONLY when NONE of the interactive controls owns focus -- including the
//     ones the old ad-hoc checks forgot (destCopyBtn, autoGroupBtn, infoBtn,
//     sizeToggle, minBtn, closeBtn).
//   * "Wheel routing ... not whether it lies over the actual item-row viewport"
//     (lines 12222-12351): a vertical wheel scrolls the rows ONLY over the real
//     item-row viewport -- never over the title / chip strip / toggle, never
//     under Ctrl (zoom), and never for a horizontal wheel.
// Self-contained: own main(), exit 0 = pass.

#include "focus_route_logic.h"
#include "view_mode_logic.h"

#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// Helper: classify with exactly ONE control focused (by 0-based index 0..10).
static bool interactiveWithOnly(int idx) {
    bool b[11] = {false,false,false,false,false,false,false,false,false,false,false};
    if (idx >= 0 && idx < 11) b[idx] = true;
    return focusroute::IsInteractiveFocus(b[0], b[1], b[2], b[3], b[4], b[5],
                                          b[6], b[7], b[8], b[9], b[10]);
}

int main() {
    using namespace focusroute;

    // ---- nothing interactive focused -> nav keys drive chips/list ----
    check(!IsInteractiveFocus(false,false,false,false,false,false,
                              false,false,false,false,false),
          "no interactive control focused -> not interactive");
    check(NavKeysDriveChipsOrList(
              IsInteractiveFocus(false,false,false,false,false,false,
                                 false,false,false,false,false)),
          "no interactive control -> nav keys drive chips/list");

    // ---- EACH interactive control swallows the nav keys ----
    const char* names[11] = {
        "edit", "createBtn", "cancelBtn", "copySegment", "moveSegment",
        "destCopyBtn", "autoGroupBtn", "infoBtn", "sizeToggle", "minBtn", "closeBtn"};
    for (int i = 0; i < 11; ++i) {
        check(interactiveWithOnly(i), names[i]);  // classified interactive
        check(!NavKeysDriveChipsOrList(interactiveWithOnly(i)),
              names[i]);                          // nav keys do NOT drive chips/list
    }

    // ---- the SIX controls the old ad-hoc checks forgot are now covered ----
    check(interactiveWithOnly(5), "destCopyBtn now swallows nav keys");
    check(interactiveWithOnly(6), "autoGroupBtn now swallows nav keys");
    check(interactiveWithOnly(7), "infoBtn now swallows nav keys");
    check(interactiveWithOnly(8), "sizeToggle now swallows nav keys");
    check(interactiveWithOnly(9), "minBtn now swallows nav keys");
    check(interactiveWithOnly(10), "closeBtn now swallows nav keys");

    // ---- wheel routing by the REAL item-row viewport ----
    // overRows true + plain vertical wheel -> scroll the rows.
    check(viewmode::WheelScrollsRows(/*ctrl*/false, /*overStrip*/false,
                                     /*overRows*/true, /*horizontal*/false),
          "wheel over the item-row viewport scrolls the rows");
    // over the chip strip (not rows) -> never scroll rows (strip pans).
    check(!viewmode::WheelScrollsRows(false, /*overStrip*/true,
                                      /*overRows*/false, false),
          "wheel over the chip strip never scrolls the rows");
    // over neither (title / toggle / header) -> never scroll rows.
    check(!viewmode::WheelScrollsRows(false, false, /*overRows*/false, false),
          "wheel over a non-row area never scrolls the rows");
    // Ctrl (zoom) wins even over the rows.
    check(!viewmode::WheelScrollsRows(/*ctrl*/true, false, /*overRows*/true, false),
          "Ctrl+wheel zooms, never scrolls the rows");
    // horizontal wheel never scrolls rows (it pans).
    check(!viewmode::WheelScrollsRows(false, false, /*overRows*/true, /*horizontal*/true),
          "a horizontal wheel never scrolls the rows");

    if (g_fail) { printf("focus_route_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("focus_route_test: OK\n");
    return 0;
}
