// auto_group_repaint_test.cpp
// Pins fix thse isseue.md Phase 4: the Auto Group countdown repaint timer must
// invalidate the button ONLY when the VISIBLE whole-second value OR the
// enabled/disabled state changes -- never once per tick. agcd::ShouldRepaint is
// that single tested decision; repeated same-second ticks must NOT repaint
// (the real cause of the flashing). Self-contained: own main(), exit 0 = pass.

#include "auto_group_countdown_logic.h"

#include <cstdio>

using namespace agcd;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // Nothing visibly changed -> NO repaint (the anti-flicker contract).
    check(!ShouldRepaint(3, 3, false, false), "same second + same state -> no repaint");
    // The visible whole-second ticked down -> repaint once.
    check(ShouldRepaint(2, 3, false, false), "second changed -> repaint");
    // The button unlocked (Waiting -> Enabled) -> repaint.
    check(ShouldRepaint(0, 0, true, false), "enabled flipped -> repaint");
    // Both changed at once -> repaint.
    check(ShouldRepaint(1, 2, true, false), "second + enabled changed -> repaint");

    // Many sub-second (~100ms) ticks WITHIN the same second must never repaint
    // -- this is exactly the per-tick erase/repaint that used to flash.
    for (int i = 0; i < 25; ++i) {
        check(!ShouldRepaint(4, 4, false, false),
              "repeated same-second tick -> no repaint");
    }

    if (g_fail) { printf("auto_group_repaint_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("auto_group_repaint_test: OK\n");
    return 0;
}
