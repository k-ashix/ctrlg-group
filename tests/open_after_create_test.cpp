// open_after_create_test.cpp
// Verifies ShouldOpenAfterCreate: the "Create and open" gate.
//
// Rule (v0.8.0): OFF by default. When ON, open the newly created folder in the
// SAME Explorer window (no new window) -- but ONLY for a single-folder create.
// Never open for a batch (>1) and never open when nothing was created.
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
    // ---- Setting OFF: never open, whatever the count ----
    check(!ShouldOpenAfterCreate(false, 1),
          "OFF: does not open even after a single create");
    check(!ShouldOpenAfterCreate(false, 3),
          "OFF: does not open after a batch");
    check(!ShouldOpenAfterCreate(false, 0),
          "OFF: does not open when nothing was created");

    // ---- Setting ON: open ONLY for exactly one created folder ----
    check(ShouldOpenAfterCreate(true, 1),
          "ON: opens after a single folder is created");
    check(!ShouldOpenAfterCreate(true, 2),
          "ON: does NOT open for a 2-folder batch (no single folder to open)");
    check(!ShouldOpenAfterCreate(true, 25),
          "ON: does NOT open for a large batch");
    check(!ShouldOpenAfterCreate(true, 0),
          "ON: does NOT open when nothing was created (cancel / skipped clash)");

    if (g_fail == 0) {
        printf("open_after_create_test: PASS\n");
        return 0;
    }
    printf("open_after_create_test: FAIL (%d)\n", g_fail);
    return 1;
}
