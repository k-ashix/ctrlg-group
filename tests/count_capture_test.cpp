// count_capture_test.cpp
// Verifies the CountDecide digit-entry state machine used after a trigger fires
// while the multi-folder toggle is on: digits accumulate (swallowed), Enter and
// timeout commit, no-digit commits 1, Esc cancels, any other key commits and
// passes through, and the committed count is clamped.
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

int main() {
    const bool multi = true;
    const int  cap   = 25;

    // ---- Digit accumulation (swallowed, stays capturing) ----
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 2, multi, cap);
        check(r1.swallow, "digit is swallowed");
        check(r1.outcome == CountOutcome::Continue, "digit keeps capturing");
        check(r1.next.value == 2 && r1.next.sawDigit, "first digit -> value 2");

        auto r2 = CountDecide(r1.next, CountKey::Digit, 3, multi, cap);
        check(r2.next.value == 23, "second digit -> value 23 (2*10+3)");
    }

    // ---- Enter commits the accumulated count ----
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 5, multi, cap);
        auto r2 = CountDecide(r1.next, CountKey::Enter, 0, multi, cap);
        check(r2.swallow, "Enter is swallowed");
        check(r2.outcome == CountOutcome::Commit, "Enter commits");
        check(r2.count == 5, "Enter commits value 5");
        check(!r2.next.active, "capture ends after Enter");
    }

    // ---- Enter with no digits -> 1 (backward compatible) ----
    {
        auto r = CountDecide(Capturing(), CountKey::Enter, 0, multi, cap);
        check(r.outcome == CountOutcome::Commit && r.count == 1,
              "Enter with no digits -> 1 folder");
    }

    // ---- Timeout commits (no digit -> 1) ----
    {
        auto r = CountDecide(Capturing(), CountKey::Timeout, 0, multi, cap);
        check(r.outcome == CountOutcome::Commit && r.count == 1,
              "timeout with no digits -> 1 folder");
    }

    // ---- Timeout commits accumulated digits ----
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 7, multi, cap);
        auto r2 = CountDecide(r1.next, CountKey::Timeout, 0, multi, cap);
        check(r2.outcome == CountOutcome::Commit && r2.count == 7,
              "timeout commits accumulated 7");
    }

    // ---- Esc cancels: create nothing, key swallowed ----
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 4, multi, cap);
        auto r2 = CountDecide(r1.next, CountKey::Escape, 0, multi, cap);
        check(r2.swallow, "Esc is swallowed");
        check(r2.outcome == CountOutcome::Cancel, "Esc cancels");
        check(!r2.next.active, "capture ends after Esc");
    }

    // ---- Other key (Ctrl still held) commits accumulated and is SWALLOWED ----
    // While Ctrl is held, a stray key must NOT reach Explorer: Ctrl+<key> would
    // fire a shortcut (Ctrl+N new window / Ctrl+F search). It only passes
    // through once Ctrl is released (see ctrl_release_test / count_no_leak_test).
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 4, multi, cap);
        auto r2 = CountDecide(r1.next, CountKey::Other, 0, multi, cap);
        check(r2.swallow, "other key is swallowed while Ctrl held (no Ctrl+key leak)");
        check(r2.outcome == CountOutcome::Commit && r2.count == 4,
              "other key commits accumulated 4");
    }
    {
        // Other key with no digits -> commit 1 (and swallow).
        auto r = CountDecide(Capturing(), CountKey::Other, 0, multi, cap);
        check(r.swallow, "other key with no digits is swallowed while Ctrl held");
        check(r.outcome == CountOutcome::Commit && r.count == 1,
              "other key with no digits -> 1");
    }

    // ---- Committed count is clamped to the cap ----
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 3, multi, cap);  // 3
        auto r2 = CountDecide(r1.next, CountKey::Digit, 0, multi, cap);  // 30
        auto r3 = CountDecide(r2.next, CountKey::Enter, 0, multi, cap);
        check(r3.count == 25, "value 30 clamped to cap 25 on commit");
    }

    // ---- Toggle off forces 1 even if digits typed ----
    {
        CountState st = Capturing();
        auto r1 = CountDecide(st, CountKey::Digit, 9, false, cap);
        auto r2 = CountDecide(r1.next, CountKey::Enter, 0, false, cap);
        check(r2.count == 1, "multi off -> commit 1 regardless of digits");
    }

    // ---- Digit accumulation never exceeds the hard cap while typing ----
    {
        CountState st = Capturing();
        st.value = 50;
        st.sawDigit = true;
        auto r = CountDecide(st, CountKey::Digit, 0, multi, 100);  // 50*10+0 = 500
        check(r.next.value == 100, "typing past hard cap is clamped to 100");
    }

    if (g_fail == 0) {
        printf("count_capture_test: PASS\n");
        return 0;
    }
    printf("count_capture_test: FAIL (%d)\n", g_fail);
    return 1;
}
