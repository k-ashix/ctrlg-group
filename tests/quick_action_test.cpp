// quick_action_test.cpp
// Pins fix thse isseue.md Phase 4: the OPT-IN two-stroke "Ctrl+G then C/M"
// quick Copy/Move. Pure, deterministic state machine (monotonic tick based,
// never blocks a hook): arming only happens when the feature is enabled AND a
// remembered valid target exists; a quick action fires ONLY on an explicit C/M
// within a bounded window; any other key or a timeout opens the ordinary popup
// WITHOUT consuming/losing that key; cancel/modifier-release/unload disarm with
// no action; and nothing can fire twice.
// Self-contained: own main(), exit 0 = pass.

#include "quick_action_logic.h"

#include <cstdio>

using namespace qseq;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const uint64_t t0 = 1'000'000;

    // ---- feature OFF: Ctrl+G opens the popup immediately, never arms ----
    {
        Decision d = OnCtrlG(/*enabled=*/false, /*hasValidTarget=*/true, t0);
        check(d.action == Action::OpenPopup, "disabled -> OpenPopup");
        check(!Armed(d.next), "disabled -> not armed");
    }

    // ---- enabled but NO remembered valid target: popup immediately ----
    {
        Decision d = OnCtrlG(true, /*hasValidTarget=*/false, t0);
        check(d.action == Action::OpenPopup, "no target -> OpenPopup immediately");
        check(!Armed(d.next), "no target -> not armed");
    }

    // ---- enabled + valid target: ARM and wait (Ctrl+G is consumed, no popup) --
    State armed;
    {
        Decision d = OnCtrlG(true, true, t0);
        check(d.action == Action::None, "armed: no immediate action");
        check(Armed(d.next), "armed after Ctrl+G with a valid target");
        check(d.consumeKey, "Ctrl+G is consumed while arming");
        armed = d.next;
    }

    // ---- 'C' within the window -> QuickCopy, consumed, disarmed ----
    {
        Decision d = OnKey(armed, Key::C, t0 + 200);
        check(d.action == Action::QuickCopy, "armed + C -> QuickCopy");
        check(d.consumeKey, "quick Copy consumes the C key");
        check(!Armed(d.next), "firing disarms (back to Idle)");
        // no double fire: a second key does nothing now.
        Decision again = OnKey(d.next, Key::C, t0 + 220);
        check(again.action == Action::None, "no action once disarmed (no double fire)");
    }

    // ---- 'M' within the window -> QuickMove, consumed ----
    {
        Decision d = OnKey(armed, Key::M, t0 + 300);
        check(d.action == Action::QuickMove, "armed + M -> QuickMove");
        check(d.consumeKey, "quick Move consumes the M key");
    }

    // ---- another key within the window -> popup, key PRESERVED (not consumed) --
    {
        Decision d = OnKey(armed, Key::Other, t0 + 300);
        check(d.action == Action::OpenPopup, "armed + other key -> OpenPopup");
        check(!d.consumeKey, "the other key is preserved (not consumed)");
        check(!Armed(d.next), "disarmed after falling back to the popup");
    }

    // ---- C/M AFTER the window has lapsed -> popup, key NOT consumed ----
    {
        Decision d = OnKey(armed, Key::C, t0 + WindowMs());
        check(d.action == Action::OpenPopup, "C after the window -> OpenPopup, too late to quick-copy");
        check(!d.consumeKey, "late key is not swallowed");
        check(!Armed(d.next), "disarmed after expiry");
    }

    // ---- timeout: armed + expired -> popup; armed + not-expired -> keep waiting
    {
        Decision exp = OnTimeout(armed, t0 + WindowMs());
        check(exp.action == Action::OpenPopup, "expired timeout -> OpenPopup");
        check(!Armed(exp.next), "expired timeout disarms");
        Decision early = OnTimeout(armed, t0 + WindowMs() - 1);
        check(early.action == Action::None, "not-yet-expired timeout keeps waiting");
        check(Armed(early.next), "still armed before the window elapses");
    }

    // ---- explicit cancel / modifier release / unload: disarm, NO action ----
    {
        Decision c = Reset(armed);
        check(c.action == Action::None, "cancel/unload fires no action");
        check(!Armed(c.next), "cancel/unload disarms");
    }

    // ---- a stale key when NOT armed never fires a quick action ----
    {
        State idle;
        Decision d = OnKey(idle, Key::C, t0 + 5);
        check(d.action == Action::None, "C while idle -> nothing (no accidental quick action)");
        check(!d.consumeKey, "idle key is not consumed by this module");
    }

    if (g_fail) { printf("quick_action_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("quick_action_test: OK\n");
    return 0;
}
