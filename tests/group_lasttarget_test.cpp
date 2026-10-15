// group_lasttarget_test.cpp
// Task 3: ResolveGroupTarget -- typed name vs blank -> last target.
// Self-contained: exit 0 = pass.

#include "group_logic.h"

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
    // 1. Typed name present -> use it, ignore last target.
    {
        auto c = ResolveGroupTarget(L"Prewedding", L"C:\\shoot\\Old");
        expectTrue(c.valid, "typed name valid");
        expectTrue(!c.useLastTarget, "typed name does not use last target");
        expectEqW(c.folderName, L"Prewedding", "typed folder name");
    }

    // 2. Typed name is whitespace -> trimmed to blank -> fall back to last.
    {
        auto c = ResolveGroupTarget(L"  Prewedding  ", L"");
        expectEqW(c.folderName, L"Prewedding", "typed name trimmed");
    }

    // 3. Blank name + valid last target -> reuse last target (key workflow).
    {
        auto c = ResolveGroupTarget(L"   ", L"C:\\shoot\\Prewedding");
        expectTrue(c.valid, "blank + last target is valid");
        expectTrue(c.useLastTarget, "blank uses last target");
        expectEqW(c.lastTargetPath, L"C:\\shoot\\Prewedding", "echoed last target");
    }

    // 4. Blank name + no last target -> not valid (nothing to do).
    {
        auto c = ResolveGroupTarget(L"", L"");
        expectTrue(!c.valid, "blank + no last target -> invalid");
    }

    if (g_fail == 0) { printf("group_lasttarget_test: PASS\n"); return 0; }
    printf("group_lasttarget_test: FAIL (%d)\n", g_fail);
    return 1;
}
