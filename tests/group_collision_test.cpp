// group_collision_test.cpp
// Task 2: GroupCollisionResolve -- UseExisting (default) / Numbered / Cancel.
// Self-contained: exit 0 = pass.

#include "group_logic.h"

#include <set>
#include <string>
#include <cstdio>

static int g_fail = 0;
static void expectEqW(const std::wstring& got, const std::wstring& want,
                      const char* what) {
    if (got != want) {
        printf("[FAIL] %s\n  got : %ls\n  want: %ls\n",
               what, got.c_str(), want.c_str());
        ++g_fail;
    }
}
static void expectTrue(bool c, const char* what) {
    if (!c) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const std::wstring dir = L"C:\\shoot";

    // 1. Name free -> Create at <dir>\<name>.
    {
        std::set<std::wstring> ex;
        auto exists = [&](const std::wstring& p) { return ex.count(p) > 0; };
        auto r = GroupCollisionResolve(dir, L"Prewedding",
                                       GroupCollision::UseExisting, exists);
        expectTrue(r.kind == GroupResolveKind::Create, "free name -> Create");
        expectEqW(r.path, L"C:\\shoot\\Prewedding", "free name path");
    }

    // 2. Name exists + UseExisting (default) -> reuse it (the key workflow).
    {
        std::set<std::wstring> ex = { L"C:\\shoot\\Prewedding" };
        auto exists = [&](const std::wstring& p) { return ex.count(p) > 0; };
        auto r = GroupCollisionResolve(dir, L"Prewedding",
                                       GroupCollision::UseExisting, exists);
        expectTrue(r.kind == GroupResolveKind::UseExisting,
                   "existing + UseExisting -> reuse");
        expectEqW(r.path, L"C:\\shoot\\Prewedding", "reuse path");
    }

    // 3. Name exists + Numbered -> next free numbered sibling "Prewedding 2".
    {
        std::set<std::wstring> ex = { L"C:\\shoot\\Prewedding" };
        auto exists = [&](const std::wstring& p) { return ex.count(p) > 0; };
        auto r = GroupCollisionResolve(dir, L"Prewedding",
                                       GroupCollision::Numbered, exists);
        expectTrue(r.kind == GroupResolveKind::Create, "numbered -> Create");
        expectEqW(r.path, L"C:\\shoot\\Prewedding 2", "numbered sibling");
    }

    // 3b. Numbered skips already-taken numbers.
    {
        std::set<std::wstring> ex = {
            L"C:\\shoot\\Prewedding", L"C:\\shoot\\Prewedding 2",
        };
        auto exists = [&](const std::wstring& p) { return ex.count(p) > 0; };
        auto r = GroupCollisionResolve(dir, L"Prewedding",
                                       GroupCollision::Numbered, exists);
        expectEqW(r.path, L"C:\\shoot\\Prewedding 3", "numbered skips taken");
    }

    // 4. Name exists + Cancel -> Cancel, empty path.
    {
        std::set<std::wstring> ex = { L"C:\\shoot\\Prewedding" };
        auto exists = [&](const std::wstring& p) { return ex.count(p) > 0; };
        auto r = GroupCollisionResolve(dir, L"Prewedding",
                                       GroupCollision::Cancel, exists);
        expectTrue(r.kind == GroupResolveKind::Cancel, "existing + Cancel");
        expectTrue(r.path.empty(), "cancel has empty path");
    }

    if (g_fail == 0) { printf("group_collision_test: PASS\n"); return 0; }
    printf("group_collision_test: FAIL (%d)\n", g_fail);
    return 1;
}
