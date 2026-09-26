// trigger_no_crossleak_test.cpp
//
// Guards against LEAKAGE between the three chord modes (Ctrl+F+N, Ctrl+N,
// Ctrl+N+F). Only ONE mode is ever active (the shortcut setting is single-
// select), so the risk is a pending chord in one mode being completed by the
// WRONG second key -- e.g. CtrlFN is pending on F and the user presses F again,
// or CtrlNF is pending on N and N is pressed again. A wrong second key must
// NOT create a folder and must NOT stay wrongly pending.
//
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        std::printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

static KeyEvent Down(int vk, bool ctrl, unsigned long long now) {
    KeyEvent e;
    e.vk = vk;
    e.isDown = true;
    e.ctrlDown = ctrl;
    e.altDown = false;
    e.shiftDown = false;
    e.explorerForeground = true;
    e.nowMs = now;
    return e;
}

int main() {
    // CtrlFN pending on F, then the WRONG key (its own F is auto-repeat -> stay
    // pending & swallow; but a DIFFERENT non-N key must abandon, not create).
    {
        auto rF = TriggerDecide(TriggerMode::CtrlFN, TriggerState{},
                                Down(kVkF, /*ctrl=*/true, 1000));
        check(rF.next.pending, "CtrlFN: F sets pending");

        // A stray letter that is NOT N: chord broken, no folder created.
        auto rWrong = TriggerDecide(TriggerMode::CtrlFN, rF.next,
                                    Down('K', /*ctrl=*/true, 1010));
        check(rWrong.action != TriggerAction::CreateFolder,
              "CtrlFN: a wrong second key ('K') must NOT create a folder");
        check(!rWrong.next.pending,
              "CtrlFN: pending is cleared after a wrong second key (no leak)");
    }

    // CtrlNF pending on N, then N pressed again (auto-repeat) stays pending and
    // swallows -- it must NOT fire CreateFolder (that needs F).
    {
        auto rN = TriggerDecide(TriggerMode::CtrlNF, TriggerState{},
                                Down(kVkN, true, 2000));
        check(rN.next.pending, "CtrlNF: N sets pending");

        auto rN2 = TriggerDecide(TriggerMode::CtrlNF, rN.next,
                                 Down(kVkN, true, 2010));
        check(rN2.action != TriggerAction::CreateFolder,
              "CtrlNF: repeating N must NOT create a folder (F completes it)");
    }

    // CtrlNF pending on N, then F WITHOUT Ctrl held: chord requires Ctrl, so it
    // must not silently fire.
    {
        auto rN = TriggerDecide(TriggerMode::CtrlNF, TriggerState{},
                                Down(kVkN, true, 3000));
        auto rF = TriggerDecide(TriggerMode::CtrlNF, rN.next,
                                Down(kVkF, /*ctrl=*/false, 3010));
        check(rF.action != TriggerAction::CreateFolder,
              "CtrlNF: F with Ctrl released must NOT create a folder");
        check(!rF.next.pending,
              "CtrlNF: pending cleared once the chord is broken (no leak)");
    }

    if (g_fail == 0) {
        std::printf("trigger_no_crossleak_test: all assertions passed\n");
        return 0;
    }
    std::printf("trigger_no_crossleak_test: %d assertion(s) failed\n", g_fail);
    return 1;
}
