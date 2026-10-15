// scrollbar_autohide_test.cpp
// Pins fix thse isseue.md Phase B: the Selected Items scrollbar auto-hide model.
// Only shown on overflow; revealed while interacting / within the reveal window;
// kept visible while dragging; hidden when idle; wider hit target than the thumb.
// Self-contained: own main(), exit 0 = pass.

#include "scrollbar_autohide_logic.h"

#include <cstdio>

using namespace sbauto;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- never shown when the list does not overflow ----
    check(!Visible(false, true,  true,  true),  "no overflow -> hidden even if dragging/hovering");
    check(!Visible(false, false, false, false), "no overflow -> hidden");

    // ---- reveal reasons when the list overflows ----
    check(Visible(true, false, true,  false), "pointer inside -> visible");
    check(Visible(true, false, false, true),  "within reveal window -> visible");
    check(Visible(true, true,  false, false), "dragging -> visible");
    check(!Visible(true, false, false, false), "overflow but idle + no pointer -> hidden");

    // ---- dragging always wins ----
    check(Visible(true, true, false, false), "drag keeps it visible even when idle");

    // ---- reveal window timing ----
    check(WithinRevealWindow(1000, 1000, kIdleHideMs), "exact same tick is within window");
    check(WithinRevealWindow(1000, 1000 + kIdleHideMs - 1, kIdleHideMs),
          "just before idle expiry is within window");
    check(!WithinRevealWindow(1000, 1000 + kIdleHideMs, kIdleHideMs),
          "at idle expiry the window has closed");
    check(!WithinRevealWindow(1000, 5000, kIdleHideMs), "long idle -> window closed");
    check(WithinRevealWindow(5000, 1000, kIdleHideMs),
          "clock going backwards keeps it visible (fail safe)");

    // ---- hit test is wider than the gutter on the left ----
    const int gL = 100, gR = 114;   // 14px gutter
    check(HitTestX(107, gL, gR, kHitSlopX), "inside the gutter hits");
    check(HitTestX(gL - kHitSlopX, gL, gR, kHitSlopX), "left slop edge hits");
    check(!HitTestX(gL - kHitSlopX - 1, gL, gR, kHitSlopX), "just left of the slop misses");
    check(!HitTestX(gR + 1, gL, gR, kHitSlopX), "right of the gutter misses");
    check(kHitSlopX > 0, "hit slop is positive (wider target than the thumb)");

    if (g_fail == 0) printf("scrollbar_autohide_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
