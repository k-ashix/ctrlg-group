// auto_group_countdown_test.cpp
// Pins fix thse isseue.md ADD-ON section 2 + Phase C: the five-second Auto
// Group safety countdown. Covers state boundaries, the monotonic-clock rule
// (delayed/early ticks can't unlock), popup recreation (fresh start), the
// settings-disabled state, the stable M:SS countdown text (fix thse isseue.md
// #5: no milliseconds), and that expiry NEVER implies execution (only Clickable
// gates the action).
// Self-contained: own main(), exit 0 = pass.

#include "auto_group_countdown_logic.h"

#include <cstdio>

using namespace agcd;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const uint64_t start = 1'000'000;  // arbitrary monotonic origin

    // ---- disabled in settings -> Unavailable, not visible, not clickable ----
    check(Resolve(false, start, start) == State::Unavailable, "disabled -> Unavailable");
    check(!Visible(State::Unavailable), "Unavailable button not shown");
    check(!Clickable(State::Unavailable), "Unavailable never clickable");

    // ---- boundaries of the 8s window ----
    check(Resolve(true, start, start) == State::Waiting, "t=0 -> Waiting");
    check(Resolve(true, start, start + 1) == State::Waiting, "t=1ms -> Waiting");
    check(Resolve(true, start, start + 7999) == State::Waiting, "t=7999ms -> Waiting");
    check(Resolve(true, start, start + 8000) == State::Enabled, "t=8000ms -> Enabled");
    check(Resolve(true, start, start + 12000) == State::Enabled, "t>8s -> stays Enabled");

    // ---- whole-second countdown label: 8,7,6,5,4,3,2,1 then "Auto Group" ----
    auto labelAt = [&](uint64_t dt) {
        State s = Resolve(true, start, start + dt);
        return Label(s, RemainingSeconds(true, start, start + dt));
    };
    check(labelAt(0)    == L"Auto Group (8)", "t=0 label (8)");
    check(labelAt(999)  == L"Auto Group (8)", "t=999ms still (8)");
    check(labelAt(1000) == L"Auto Group (7)", "t=1000ms label (7)");
    check(labelAt(2000) == L"Auto Group (6)", "t=2000ms label (6)");
    check(labelAt(3000) == L"Auto Group (5)", "t=3000ms label (5)");
    check(labelAt(4000) == L"Auto Group (4)", "t=4000ms label (4)");
    check(labelAt(5000) == L"Auto Group (3)", "t=5000ms label (3)");
    check(labelAt(6000) == L"Auto Group (2)", "t=6000ms label (2)");
    check(labelAt(7000) == L"Auto Group (1)", "t=7000ms label (1)");
    check(labelAt(7999) == L"Auto Group (1)", "final second still (1), never (0)");
    check(labelAt(8000) == L"Auto Group",     "t=8000ms plain label, no number");

    // ---- stable M:SS countdown (fix thse isseue.md #5): minutes:seconds,
    // seconds zero-padded, NO milliseconds, driven by the whole-second value so
    // it changes at most once per second (0:08,0:07,...,0:01). ----
    auto cd = [&](uint64_t dt) {
        return CountdownText(RemainingSeconds(true, start, start + dt));
    };
    check(cd(0)    == L"0:08", "t=0 countdown is 0:08");
    check(cd(999)  == L"0:08", "t=999ms still 0:08");
    check(cd(1000) == L"0:07", "t=1000ms 0:07");
    check(cd(2000) == L"0:06", "t=2000ms 0:06");
    check(cd(3000) == L"0:05", "t=3000ms 0:05");
    check(cd(4000) == L"0:04", "t=4000ms 0:04");
    check(cd(5000) == L"0:03", "t=5000ms 0:03");
    check(cd(6000) == L"0:02", "t=6000ms 0:02");
    check(cd(7000) == L"0:01", "t=7000ms 0:01");
    check(cd(7999) == L"0:01", "final second still 0:01, never 0:00");
    // Format invariants: always M:SS, exactly one colon, seconds two digits,
    // minutes roll over, and there is NEVER a millisecond suffix.
    check(CountdownText(5)   == L"0:05", "5s -> 0:05");
    check(CountdownText(65)  == L"1:05", "65s -> 1:05 (minutes roll over)");
    check(CountdownText(600) == L"10:00", "600s -> 10:00");
    check(CountdownText(-3)  == L"0:00", "negative clamps to 0:00");
    {
        std::wstring t = CountdownText(3);
        check(t.find(L'.') == std::wstring::npos, "no millisecond dot in countdown");
        check(t.find(L':') != std::wstring::npos &&
              t.rfind(L':') == t.find(L':'), "exactly one colon");
        check(t == L"0:03" && t.size() == 4, "exactly M:SS, no ms suffix");
    }
    check(ShowOverlay(State::Waiting), "countdown shown while Waiting");
    check(!ShowOverlay(State::Enabled), "no countdown once Enabled");

    // ---- monotonic rule: a delayed 'now' simply reports the true elapsed; a
    // backwards clock is clamped so it cannot unlock early ----
    check(Resolve(true, start, start - 100) == State::Waiting,
          "now < start (clock skew) stays Waiting, never early-unlocks");
    check(RemainingSeconds(true, start, start - 100) == 8,
          "clock skew keeps the full 8s remaining");
    // A long gap between ticks (busy loop) still unlocks exactly at >=8000ms,
    // not after a count of received ticks.
    check(Resolve(true, start, start + 100000) == State::Enabled,
          "huge elapsed -> Enabled (time-based, not tick-count based)");

    // ---- popup recreation: a fresh start tick restarts the full countdown ----
    const uint64_t restart = start + 20000;  // popup reopened later
    check(Resolve(true, restart, restart) == State::Waiting,
          "reopened popup starts Waiting again");
    check(RemainingSeconds(true, restart, restart) == 8,
          "reopened popup shows the full 8s again");
    check(Resolve(true, restart, restart + 7999) == State::Waiting,
          "reopened popup honors its own 8s window");

    // ---- expiry does NOT imply execution: only Clickable gates the action ----
    check(Clickable(State::Enabled), "Enabled is clickable");
    check(!Clickable(State::Waiting), "Waiting is NOT clickable (no early run)");
    // There is deliberately no 'ShouldAutoRun'/'Fire' in the model: reaching
    // Enabled only makes the button clickable; nothing here triggers the op.

    if (g_fail) { printf("auto_group_countdown_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("auto_group_countdown_test: OK\n");
    return 0;
}
