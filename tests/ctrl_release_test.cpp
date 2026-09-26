// ctrl_release_test.cpp
// Ctrl must be held for the ENTIRE gesture. This pins the two release paths:
//   1. CountDecide with ctrlHeld == false ends capture:
//        - a digit was seen  -> Commit that many
//        - no digit yet       -> Commit 1 (default single folder)
//   2. TriggerDecide: releasing Ctrl mid-chord abandons the pending chord.
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

static CountState Capturing() {
    CountState s;
    s.active = true;
    return s;
}

static KeyEvent Down(int vk, bool ctrl, bool fg, unsigned long long now = 1000) {
    KeyEvent e;
    e.vk = vk;
    e.isDown = true;
    e.ctrlDown = ctrl;
    e.explorerForeground = fg;
    e.nowMs = now;
    return e;
}

int main() {
    const bool multi = true;
    const int  cap   = 25;

    // ---- 1a. Ctrl released mid-count with digits typed -> commit them ----
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 5, multi, cap, /*ctrlHeld=*/true);
        // Next key comes in while Ctrl is NO LONGER held.
        auto r2 = CountDecide(r1.next, CountKey::Digit, 0, multi, cap, /*ctrlHeld=*/false);
        check(r2.outcome == CountOutcome::Commit,
              "Ctrl released after a digit -> commit");
        check(r2.count == 5, "Ctrl released after '5' -> create 5");
        check(!r2.next.active, "capture ends when Ctrl released");
    }

    // ---- 1b. Ctrl released before any digit -> default single folder ----
    // A bare trigger + release (no number typed) must create exactly ONE folder.
    // The releasing key here is NOT a digit (Other): no count was expressed.
    {
        CountState st = Capturing();
        auto r = CountDecide(st, CountKey::Other, 0, multi, cap, /*ctrlHeld=*/false);
        check(r.outcome == CountOutcome::Commit,
              "Ctrl released with no digit -> commit the default single folder");
        check(r.count == 1, "Ctrl released with no digit -> exactly 1 folder");
        check(!r.next.active, "capture ends when Ctrl released");
    }

    // ---- 1b-bis. Digit arrives on the SAME event Ctrl reads released ----
    // Real-world race (see log: "sawDigit=0 value=0 -> requested=0" after the
    // user pressed Ctrl+F+N then 9): the '9' keydown is delivered while
    // GetAsyncKeyState(VK_CONTROL) already reports released. A digit is explicit
    // intent -- it must be recorded and committed, NOT collapsed to 1.
    {
        CountState st = Capturing();
        auto r = CountDecide(st, CountKey::Digit, 9, multi, cap, /*ctrlHeld=*/false);
        check(r.outcome == CountOutcome::Commit,
              "digit with Ctrl released on same event -> commit");
        check(r.count == 9, "typed '9' with Ctrl released -> create 9, not 1");
        check(!r.next.active, "capture ends after committing the digit");
    }

    // ---- 1c. Enter only commits while Ctrl is held ----
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 3, multi, cap, /*ctrlHeld=*/true);
        // User let go of Ctrl, THEN pressed Enter: release already decided it.
        auto r2 = CountDecide(r1.next, CountKey::Enter, 0, multi, cap, /*ctrlHeld=*/false);
        check(r2.outcome == CountOutcome::Commit && r2.count == 3,
              "Ctrl released then Enter still commits the typed 3");
    }

    // ---- 2. Releasing Ctrl mid-chord abandons the pending chord ----
    {
        // mode 0 (Ctrl+F+N): F pending.
        auto rF = TriggerDecide(TriggerMode::CtrlFN, TriggerState{},
                                Down(kVkF, true, true, 1000));
        check(rF.next.pending, "CtrlFN: F is pending");

        // N arrives but Ctrl is NOT held -> must NOT create; chord abandoned.
        auto rN = TriggerDecide(TriggerMode::CtrlFN, rF.next,
                                Down(kVkN, /*ctrl=*/false, true, 1100));
        check(rN.action != TriggerAction::CreateFolder,
              "CtrlFN: N without Ctrl held must NOT create a folder");
        check(!rN.next.pending, "CtrlFN: chord abandoned when Ctrl released");
    }

    if (g_fail == 0) {
        printf("ctrl_release_test: PASS\n");
        return 0;
    }
    printf("ctrl_release_test: FAIL (%d)\n", g_fail);
    return 1;
}
