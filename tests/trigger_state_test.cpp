// trigger_state_test.cpp
// Verifies the TriggerDecide state machine for all three trigger modes:
//   CtrlN, CtrlFN (hold Ctrl, F then N), CtrlNF (hold Ctrl, N then F).
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

static KeyEvent Down(int vk, bool ctrl, bool fg, unsigned long long now = 1000) {
    KeyEvent e;
    e.vk = vk;
    e.isDown = true;
    e.ctrlDown = ctrl;
    e.explorerForeground = fg;
    e.nowMs = now;
    return e;
}

static KeyEvent Timeout(unsigned long long now) {
    KeyEvent e;
    e.isTimeout = true;
    e.nowMs = now;
    return e;
}

// Ctrl+Shift+<vk> key-down (shift held). Used for the Ctrl+Shift+N mode, which
// overlays Windows' own native "new folder" shortcut.
static KeyEvent ShiftDown(int vk, bool ctrl, bool shift, bool fg,
                          unsigned long long now = 1000) {
    KeyEvent e;
    e.vk = vk;
    e.isDown = true;
    e.ctrlDown = ctrl;
    e.shiftDown = shift;
    e.explorerForeground = fg;
    e.nowMs = now;
    return e;
}

int main() {
    // ---- Mode CtrlN: Ctrl+N fires immediately, swallowing new-window ----
    {
        auto r = TriggerDecide(TriggerMode::CtrlN, TriggerState{},
                               Down(kVkN, true, true));
        check(r.swallow, "CtrlN: Ctrl+N is swallowed");
        check(r.action == TriggerAction::CreateFolder, "CtrlN: Ctrl+N creates folder");
    }
    {
        auto r = TriggerDecide(TriggerMode::CtrlN, TriggerState{},
                               Down(kVkN, true, false));
        check(!r.swallow && r.action == TriggerAction::None,
              "CtrlN: no action when Explorer not foreground");
    }
    {
        auto r = TriggerDecide(TriggerMode::CtrlN, TriggerState{},
                               Down(kVkN, false, true));
        check(!r.swallow && r.action == TriggerAction::None,
              "CtrlN: no action without Ctrl held");
    }

    // ---- Mode CtrlFN: F pending, N completes; timeout replays Ctrl+F ----
    {
        auto rF = TriggerDecide(TriggerMode::CtrlFN, TriggerState{},
                                Down(kVkF, true, true, 1000));
        check(rF.swallow, "CtrlFN: Ctrl+F is swallowed (search suppressed while pending)");
        check(rF.action == TriggerAction::None, "CtrlFN: Ctrl+F alone does not create");
        check(rF.next.pending && rF.next.pendingVk == kVkF,
              "CtrlFN: Ctrl+F enters pending on F");

        auto rN = TriggerDecide(TriggerMode::CtrlFN, rF.next,
                                Down(kVkN, true, true, 1100));
        check(rN.swallow && rN.action == TriggerAction::CreateFolder,
              "CtrlFN: N after F creates folder");
        check(!rN.next.pending, "CtrlFN: pending cleared after create");

        auto rT = TriggerDecide(TriggerMode::CtrlFN, rF.next,
                                Timeout(1000 + kChordTimeoutMs + 1));
        check(rT.action == TriggerAction::ReplayCtrlF,
              "CtrlFN: timeout replays Ctrl+F so search still opens");
        check(!rT.next.pending, "CtrlFN: pending cleared after timeout");
    }
    {
        auto r = TriggerDecide(TriggerMode::CtrlFN, TriggerState{},
                               Down(kVkF, true, false));
        check(!r.swallow && !r.next.pending,
              "CtrlFN: Ctrl+F ignored when Explorer not foreground");
    }

    // ---- Mode CtrlNF: N pending (new-window suppressed), F completes ----
    {
        auto rN = TriggerDecide(TriggerMode::CtrlNF, TriggerState{},
                                Down(kVkN, true, true, 2000));
        check(rN.swallow, "CtrlNF: Ctrl+N is swallowed (new-window suppressed)");
        check(rN.action == TriggerAction::None, "CtrlNF: Ctrl+N alone does not create");
        check(rN.next.pending && rN.next.pendingVk == kVkN,
              "CtrlNF: Ctrl+N enters pending on N");

        auto rF = TriggerDecide(TriggerMode::CtrlNF, rN.next,
                                Down(kVkF, true, true, 2100));
        check(rF.swallow && rF.action == TriggerAction::CreateFolder,
              "CtrlNF: F after N creates folder");

        auto rT = TriggerDecide(TriggerMode::CtrlNF, rN.next,
                                Timeout(2000 + kChordTimeoutMs + 1));
        check(rT.action == TriggerAction::None,
              "CtrlNF: timeout does NOT replay Ctrl+F (new-window already suppressed)");
        check(!rT.next.pending, "CtrlNF: pending cleared after timeout");
    }

    // ---- Mode CtrlShiftN: Ctrl+Shift+N fires immediately (overlays the
    //      native Windows "new folder" shortcut; nothing to replay) ----
    {
        auto r = TriggerDecide(TriggerMode::CtrlShiftN, TriggerState{},
                               ShiftDown(kVkN, true, true, true));
        check(r.swallow, "CtrlShiftN: Ctrl+Shift+N is swallowed");
        check(r.action == TriggerAction::CreateFolder,
              "CtrlShiftN: Ctrl+Shift+N creates folder");
        check(!r.next.pending, "CtrlShiftN: single-press, never pending");
    }
    {
        // Explorer not foreground -> pass through untouched.
        auto r = TriggerDecide(TriggerMode::CtrlShiftN, TriggerState{},
                               ShiftDown(kVkN, true, true, false));
        check(!r.swallow && r.action == TriggerAction::None,
              "CtrlShiftN: no action when Explorer not foreground");
    }
    {
        // Plain Ctrl+N (no Shift) must NOT trigger this mode.
        auto r = TriggerDecide(TriggerMode::CtrlShiftN, TriggerState{},
                               ShiftDown(kVkN, true, false, true));
        check(!r.swallow && r.action == TriggerAction::None,
              "CtrlShiftN: Ctrl+N without Shift does not fire");
    }
    {
        // Shift without Ctrl must NOT trigger this mode.
        auto r = TriggerDecide(TriggerMode::CtrlShiftN, TriggerState{},
                               ShiftDown(kVkN, false, true, true));
        check(!r.swallow && r.action == TriggerAction::None,
              "CtrlShiftN: Shift+N without Ctrl does not fire");
    }
    {
        // A DIFFERENT key with Ctrl+Shift held must NOT fire.
        auto r = TriggerDecide(TriggerMode::CtrlShiftN, TriggerState{},
                               ShiftDown(kVkF, true, true, true));
        check(!r.swallow && r.action == TriggerAction::None,
              "CtrlShiftN: Ctrl+Shift+F does not fire (wrong key)");
    }
    {
        // Ctrl+Shift+N must never replay Ctrl+F on a timeout (it is single-press
        // and owes nothing back to Explorer).
        auto r = TriggerDecide(TriggerMode::CtrlShiftN, TriggerState{},
                               Timeout(1000 + kChordTimeoutMs + 1));
        check(r.action == TriggerAction::None,
              "CtrlShiftN: timeout is a no-op (no replay)");
    }
    {
        // Ctrl+Shift+N does NOT trigger the plain Ctrl+N mode (that mode requires
        // a clean Ctrl with no Shift).
        auto r = TriggerDecide(TriggerMode::CtrlN, TriggerState{},
                               ShiftDown(kVkN, true, true, true));
        check(!r.swallow && r.action == TriggerAction::None,
              "CtrlN: Ctrl+Shift+N does not fire the plain Ctrl+N mode");
    }

    // ---- Injected events are ignored (no feedback loop) ----
    {
        KeyEvent inj = Down(kVkN, true, true);
        inj.injected = true;
        auto r = TriggerDecide(TriggerMode::CtrlN, TriggerState{}, inj);
        check(!r.swallow && r.action == TriggerAction::None,
              "injected events are ignored");
    }

    if (g_fail == 0) {
        printf("trigger_state_test: PASS\n");
        return 0;
    }

    printf("trigger_state_test: FAIL (%d)\n", g_fail);
    return 1;
}
