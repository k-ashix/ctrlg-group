// default_state_test.cpp
// Verifies the shipped DEFAULT of the "Multiple folders in one go" toggle.
// It ships ON so batch counts and folder templates work out of the box (a
// template key is only captured while the count window is open). This pins the
// default so a future edit can't silently flip it back off.
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <cstdio>

static int g_fail = 0;

static void expectTrue(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

static void expectInt(int got, int want, const char* what) {
    if (got != want) {
        printf("[FAIL] %s\n  got : %d\n  want: %d\n", what, got, want);
        ++g_fail;
    }
}

int main() {
    // 1. The toggle ships ON by default (batch + templates work out of the box).
    expectTrue(kMultiFolderDefaultOn == true,
               "multi-folder toggle default must be ON");

    // 2. With the default (on), a typed count creates that many folders...
    expectInt(ResolveFolderCount(5, kMultiFolderDefaultOn, 25), 5,
              "default (on): a typed count of 5 makes 5 folders");
    // ...and no digits still means the classic single folder.
    expectInt(ResolveFolderCount(0, kMultiFolderDefaultOn, 25), 1,
              "default (on): no digits -> 1 folder");

    if (g_fail == 0) {
        printf("default_state_test: PASS\n");
        return 0;
    }
    printf("default_state_test: FAIL (%d)\n", g_fail);
    return 1;
}
