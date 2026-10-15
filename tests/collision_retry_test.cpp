// collision_retry_test.cpp
// Pins the create-collision decision (ShouldRetryOnCollision, mirrored from the
// folder-create loop in ctrlg-group.wh.cpp).
//
// Bug (#5): naming pre-checks with PathFileExistsW then CreateDirectoryW. Between
// the check and the create another process can create the same directory, so the
// create fails with ERROR_ALREADY_EXISTS. The old code treated that as a final
// failure instead of advancing to the next candidate name. The decision below is
// what the create loop consults: only ERROR_ALREADY_EXISTS means "collision, try
// the next name"; every other error is a real failure.
//
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
    // ERROR_ALREADY_EXISTS (183) is a collision -> retry the next name.
    check(ShouldRetryOnCollision(183), "ERROR_ALREADY_EXISTS -> retry next name");

    // Any other error is NOT a collision -> do not retry, report failure.
    check(!ShouldRetryOnCollision(0), "ERROR_SUCCESS is not a collision");
    check(!ShouldRetryOnCollision(5), "ERROR_ACCESS_DENIED is not a collision");
    check(!ShouldRetryOnCollision(3), "ERROR_PATH_NOT_FOUND is not a collision");
    check(!ShouldRetryOnCollision(32), "ERROR_SHARING_VIOLATION is not a collision");

    if (g_fail == 0) {
        printf("collision_retry_test: PASS\n");
        return 0;
    }
    printf("collision_retry_test: FAIL (%d)\n", g_fail);
    return 1;
}
