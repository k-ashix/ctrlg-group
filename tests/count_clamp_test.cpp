// count_clamp_test.cpp
// Verifies ClampFolderMax + ResolveFolderCount: the multi-folder toggle forces a
// single folder when off, "no digits" means 1, and the per-run count is clamped
// to the configured max with a hard cap of 100.
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <cstdio>

static int g_fail = 0;

static void expectInt(int got, int want, const char* what) {
    if (got != want) {
        printf("[FAIL] %s\n  got : %d\n  want: %d\n", what, got, want);
        ++g_fail;
    }
}

int main() {
    // ---- ClampFolderMax: settings clamp into [1, 100] ----
    expectInt(ClampFolderMax(25),   25,  "ClampFolderMax: default 25 unchanged");
    expectInt(ClampFolderMax(1),    1,   "ClampFolderMax: 1 is allowed");
    expectInt(ClampFolderMax(0),    1,   "ClampFolderMax: 0 -> 1 (floor)");
    expectInt(ClampFolderMax(-5),   1,   "ClampFolderMax: negative -> 1 (floor)");
    expectInt(ClampFolderMax(100),  100, "ClampFolderMax: 100 is the hard cap");
    expectInt(ClampFolderMax(101),  100, "ClampFolderMax: 101 -> 100 (hard cap)");
    expectInt(ClampFolderMax(9999), 100, "ClampFolderMax: huge -> 100 (hard cap)");

    // ---- ResolveFolderCount: toggle OFF always yields 1 ----
    expectInt(ResolveFolderCount(5,  false, 25), 1, "toggle off -> 1 regardless of request");
    expectInt(ResolveFolderCount(99, false, 25), 1, "toggle off -> 1 even for large request");

    // ---- ResolveFolderCount: toggle ON ----
    expectInt(ResolveFolderCount(1,  true, 25), 1,  "requested 1 -> 1");
    expectInt(ResolveFolderCount(5,  true, 25), 5,  "requested 5 within cap -> 5");
    expectInt(ResolveFolderCount(0,  true, 25), 1,  "no digits (0) -> 1");
    expectInt(ResolveFolderCount(-3, true, 25), 1,  "negative -> 1");
    expectInt(ResolveFolderCount(30, true, 25), 25, "requested 30 > cap 25 -> 25");
    expectInt(ResolveFolderCount(50, true, 200), 50,  "setting 200 clamps to 100; 50 < 100 -> 50");
    expectInt(ResolveFolderCount(500, true, 200), 100, "setting 200 clamps to 100; 500 -> 100 hard cap");

    if (g_fail == 0) {
        printf("count_clamp_test: PASS\n");
        return 0;
    }
    printf("count_clamp_test: FAIL (%d)\n", g_fail);
    return 1;
}
