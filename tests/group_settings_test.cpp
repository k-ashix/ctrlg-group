// group_settings_test.cpp
// Task 9: pure mapping of the Ctrl+G Group Selection settings strings
// (groupOperation / groupCollision) to the pure decision types. The Win32
// mod mirrors these mappings when it reads Wh_GetStringSetting. Self-contained:
// exit 0 = pass. Unknown/empty values must fall back to the documented
// defaults (Copy, UseExisting).

#include "group_logic.h"

#include <string>
#include <cstdio>

static int g_fail = 0;
static void expectTrue(bool c, const char* what) {
    if (!c) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- groupOperation -> isMove (Copy is the default) -----------------
    expectTrue(ParseGroupOperationIsMove(L"move") == true,
               "groupOperation 'move' -> Move");
    expectTrue(ParseGroupOperationIsMove(L"copy") == false,
               "groupOperation 'copy' -> Copy");
    expectTrue(ParseGroupOperationIsMove(L"") == false,
               "groupOperation empty -> Copy (default)");
    expectTrue(ParseGroupOperationIsMove(L"bogus") == false,
               "groupOperation unknown -> Copy (default)");

    // ---- groupCollision -> GroupCollision (UseExisting default) ---------
    expectTrue(ParseGroupCollision(L"useExisting") == GroupCollision::UseExisting,
               "groupCollision 'useExisting' -> UseExisting");
    expectTrue(ParseGroupCollision(L"numbered") == GroupCollision::Numbered,
               "groupCollision 'numbered' -> Numbered");
    expectTrue(ParseGroupCollision(L"cancel") == GroupCollision::Cancel,
               "groupCollision 'cancel' -> Cancel");
    expectTrue(ParseGroupCollision(L"") == GroupCollision::UseExisting,
               "groupCollision empty -> UseExisting (default)");
    expectTrue(ParseGroupCollision(L"bogus") == GroupCollision::UseExisting,
               "groupCollision unknown -> UseExisting (default)");

    if (g_fail == 0) {
        printf("group_settings_test: PASS\n");
        return 0;
    }
    printf("group_settings_test: FAIL (%d)\n", g_fail);
    return 1;
}
