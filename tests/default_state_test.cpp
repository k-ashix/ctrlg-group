// default_state_test.cpp
// Verifies the shipped DEFAULT of the "Multiple folders in one go" toggle.
// The classic single-folder behaviour is the least-surprising default, so the
// multi-folder feature must ship OFF (opt-in). This pins the default so a future
// edit can't silently flip it back on.
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
    // 1. The toggle ships OFF by default.
    expectTrue(kMultiFolderDefaultOn == false,
               "multi-folder toggle default must be OFF (opt-in)");

    // 2. With the default (off), even a requested count collapses to a single
    //    folder -- the classic behaviour a first-time user expects.
    expectInt(ResolveFolderCount(5, kMultiFolderDefaultOn, 25), 1,
              "default (off): a typed count still makes exactly 1 folder");
    expectInt(ResolveFolderCount(0, kMultiFolderDefaultOn, 25), 1,
              "default (off): no digits -> 1 folder");

    if (g_fail == 0) {
        printf("default_state_test: PASS\n");
        return 0;
    }
    printf("default_state_test: FAIL (%d)\n", g_fail);
    return 1;
}
