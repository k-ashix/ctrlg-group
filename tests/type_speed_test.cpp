// type_speed_test.cpp
// Verifies the type-speed presets (v0.9.0): one setting scales the count-entry
// window AND the inline-rename watch together, with three sane presets. An
// unknown value falls back to Normal (never zero / never unbounded).
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

int main() {
    // ---- Normal (default) = today's behavior, unchanged ----
    check(CountWindowMsForSpeed(0) == 1200,
          "Normal: count window stays 1200 ms (no regression)");
    check(RenameWatchMsForSpeed(0) == 30000,
          "Normal: rename watch stays 30 s (no regression)");

    // ---- Relaxed = roughly double ----
    check(CountWindowMsForSpeed(1) == 2500, "Relaxed: count window 2500 ms");
    check(RenameWatchMsForSpeed(1) == 60000, "Relaxed: rename watch 60 s");

    // ---- Patient = most generous ----
    check(CountWindowMsForSpeed(2) == 4000, "Patient: count window 4000 ms");
    check(RenameWatchMsForSpeed(2) == 120000, "Patient: rename watch 120 s");

    // ---- Unknown value falls back to Normal (guards against a bad setting) --
    check(CountWindowMsForSpeed(99) == 1200,
          "Unknown speed: count window falls back to Normal");
    check(RenameWatchMsForSpeed(-1) == 30000,
          "Unknown speed: rename watch falls back to Normal");

    // ---- Every preset is strictly increasing and never zero/unbounded ----
    check(CountWindowMsForSpeed(0) < CountWindowMsForSpeed(1) &&
          CountWindowMsForSpeed(1) < CountWindowMsForSpeed(2),
          "Count windows increase Normal < Relaxed < Patient");
    check(RenameWatchMsForSpeed(0) < RenameWatchMsForSpeed(1) &&
          RenameWatchMsForSpeed(1) < RenameWatchMsForSpeed(2),
          "Rename watches increase Normal < Relaxed < Patient");
    check(CountWindowMsForSpeed(0) > 0 && RenameWatchMsForSpeed(0) > 0,
          "No preset is zero (the gesture would be unusable)");

    if (g_fail == 0) {
        printf("type_speed_test: PASS\n");
        return 0;
    }
    printf("type_speed_test: FAIL (%d)\n", g_fail);
    return 1;
}
