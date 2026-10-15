// group_opplan_test.cpp
// Task 6a: GroupOperationPlan -- the pure decision that turns a resolved
// destination + a Move/Copy choice into the operation the worker will run via
// IFileOperation. Copy is the default. This does NOT touch the shell; it only
// decides WHAT to do so the Win32 side (Task 6b) can mirror it.
//
// Self-contained: exit 0 = pass. RED before GroupOperationPlan exists.

#include "group_logic.h"

#include <cstdio>

static int g_fail = 0;
static void expect(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // 1. Copy is the default when nothing is specified.
    {
        GroupOperationPlan p = MakeGroupOperationPlan(
            L"C:\\shoot\\Prewedding", /*isMove*/false, GroupResolveKind::Create);
        expect(p.valid, "copy plan is valid");
        expect(!p.isMove, "default operation is Copy");
        expect(p.createDestFirst, "Create resolution creates the dest first");
        expect(p.destPath == L"C:\\shoot\\Prewedding", "copy plan keeps dest path");
    }

    // 2. Move is honored when requested.
    {
        GroupOperationPlan p = MakeGroupOperationPlan(
            L"C:\\shoot\\Prewedding", /*isMove*/true, GroupResolveKind::UseExisting);
        expect(p.valid, "move plan is valid");
        expect(p.isMove, "move requested -> move");
        expect(!p.createDestFirst, "UseExisting reuses the dest, no create");
    }

    // 3. A Cancel resolution yields an invalid (no-op) plan regardless of move/copy.
    {
        GroupOperationPlan pc = MakeGroupOperationPlan(
            L"", /*isMove*/false, GroupResolveKind::Cancel);
        expect(!pc.valid, "cancel resolution -> invalid plan (copy)");
        GroupOperationPlan pm = MakeGroupOperationPlan(
            L"", /*isMove*/true, GroupResolveKind::Cancel);
        expect(!pm.valid, "cancel resolution -> invalid plan (move)");
    }

    // 4. An empty destination path is never a valid plan (safety).
    {
        GroupOperationPlan p = MakeGroupOperationPlan(
            L"", /*isMove*/false, GroupResolveKind::Create);
        expect(!p.valid, "empty dest -> invalid plan");
    }

    if (g_fail == 0) { printf("group_opplan_test: PASS\n"); return 0; }
    printf("group_opplan_test: FAIL (%d)\n", g_fail);
    return 1;
}
