// count_no_leak_test.cpp
//
// Prove-It repro + regression guard for the reported bug:
//   "Using Ctrl+F+N <number> ALSO opens a new Explorer window (plus the popup)."
//
// Root cause: during count entry, a non-count key was PASSED THROUGH to
// Explorer while Ctrl was STILL held. Ctrl down + that key = a Ctrl+<key>
// Explorer shortcut -- Ctrl+N opens a new window, Ctrl+F opens search. The count
// window must instead SWALLOW every key while Ctrl is held (digits accumulate;
// a stray key commits the typed count but is never leaked). Only AFTER Ctrl is
// released may a key reach Explorer -- and by then Ctrl is not held, so it is
// safe.
//
// Because CountDecide runs the same after ANY of the three triggers fire, this
// also pins that Ctrl+N and Ctrl+N+F (the N-based modes) do not regress: the
// trigger key is swallowed and no follow-up key leaks a new window.
//
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

static KeyEvent Down(int vk, bool ctrl, bool fg, unsigned long long now) {
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

    // ---- THE BUG: a stray key mid-count, Ctrl still held, must be SWALLOWED ----
    // (Ctrl is down, so leaking it = Ctrl+<key> = new window / search.)
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 7, multi, cap, /*ctrlHeld=*/true);
        auto r2 = CountDecide(r1.next, CountKey::Other, 0, multi, cap, /*ctrlHeld=*/true);
        check(r2.swallow,
              "stray key during count entry (Ctrl held) is swallowed, never leaked");
        check(r2.outcome == CountOutcome::Commit && r2.count == 7,
              "stray key still commits the typed count (7)");
        check(!r2.next.active, "capture ends after the stray key");
    }

    // ---- Stray key with no digits yet, Ctrl held -> commit default 1, swallowed ----
    {
        auto r = CountDecide(Capturing(), CountKey::Other, 0, multi, cap, /*ctrlHeld=*/true);
        check(r.swallow, "stray key with no digits (Ctrl held) is swallowed");
        check(r.outcome == CountOutcome::Commit && r.count == 1,
              "stray key with no digits commits the default single folder");
    }

    // ---- Only AFTER Ctrl is released may a key pass through (safe: no Ctrl) ----
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 2, multi, cap, /*ctrlHeld=*/true);
        auto r2 = CountDecide(r1.next, CountKey::Other, 0, multi, cap, /*ctrlHeld=*/false);
        check(!r2.swallow,
              "after Ctrl release the key passes through (no Ctrl held, so safe)");
        check(r2.outcome == CountOutcome::Commit && r2.count == 2,
              "Ctrl release commits the typed count (2)");
    }

    // ---- Regression: each trigger still fires AND swallows its own trigger key ----
    // ctrl_n (mode 1): Ctrl+N creates now and swallows N (never a new window).
    {
        auto r = TriggerDecide(TriggerMode::CtrlN, TriggerState{},
                               Down(kVkN, /*ctrl=*/true, /*fg=*/true, 1000));
        check(r.action == TriggerAction::CreateFolder,
              "ctrl_n: Ctrl+N fires CreateFolder");
        check(r.swallow, "ctrl_n: the N is swallowed (no default new-window)");
        check(r.next.swallowUpVk == kVkN, "ctrl_n: N key-up is swallowed too");
    }

    // ctrl_n_f (mode 2): hold Ctrl, tap N (pending, swallowed), tap F (fires).
    {
        auto rN = TriggerDecide(TriggerMode::CtrlNF, TriggerState{},
                                Down(kVkN, true, true, 1000));
        check(rN.swallow && rN.next.pending,
              "ctrl_n_f: N is pending and swallowed (no default new-window)");
        auto rF = TriggerDecide(TriggerMode::CtrlNF, rN.next,
                                Down(kVkF, true, true, 1100));
        check(rF.action == TriggerAction::CreateFolder && rF.swallow,
              "ctrl_n_f: F completes the chord and fires");
    }

    // ctrl_f_n (mode 0): hold Ctrl, tap F (pending, swallowed), tap N (fires).
    {
        auto rF = TriggerDecide(TriggerMode::CtrlFN, TriggerState{},
                                Down(kVkF, true, true, 1000));
        check(rF.swallow && rF.next.pending, "ctrl_f_n: F is pending and swallowed");
        auto rN = TriggerDecide(TriggerMode::CtrlFN, rF.next,
                                Down(kVkN, true, true, 1100));
        check(rN.action == TriggerAction::CreateFolder && rN.swallow,
              "ctrl_f_n: N completes the chord, fires, and is swallowed");
    }

    if (g_fail == 0) {
        printf("count_no_leak_test: PASS\n");
        return 0;
    }
    printf("count_no_leak_test: FAIL (%d)\n", g_fail);
    return 1;
}
