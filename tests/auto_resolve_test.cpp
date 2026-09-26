// auto_resolve_test.cpp
// Verifies the autoResolve flag on MakeUniqueFolderNames:
//   autoResolve = true  -> number clashes away ("New folder (2)", ...)
//   autoResolve = false -> only the plain "New folder"; a clash is skipped, never
//                          numbered (so a run yields fewer folders).
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <set>
#include <vector>
#include <string>
#include <cstdio>

static int g_fail = 0;

static void expectVec(const std::vector<std::wstring>& got,
                      const std::vector<std::wstring>& want,
                      const char* what) {
    bool ok = got.size() == want.size();
    for (size_t i = 0; ok && i < want.size(); ++i) {
        if (got[i] != want[i]) ok = false;
    }
    if (!ok) {
        printf("[FAIL] %s\n  got (%zu):\n", what, got.size());
        for (const auto& s : got) printf("    %ls\n", s.c_str());
        printf("  want (%zu):\n", want.size());
        for (const auto& s : want) printf("    %ls\n", s.c_str());
        ++g_fail;
    }
}

int main() {
    const std::wstring dir = L"C:\\x";

    // autoResolve = false, empty dir, count 3 -> just one "New folder"
    // (the other two slots clash in-batch and are skipped, not numbered).
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(MakeUniqueFolderNames(dir, 3, exists, false),
                  { L"C:\\x\\New folder" },
                  "autoResolve off: count 3 -> single 'New folder'");
    }

    // autoResolve = false, "New folder" already exists -> nothing created.
    {
        std::set<std::wstring> existing = { L"C:\\x\\New folder" };
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(MakeUniqueFolderNames(dir, 2, exists, false),
                  {},
                  "autoResolve off: base exists -> empty (no numbering)");
    }

    // autoResolve = true still numbers clashes (default path regression guard).
    {
        std::set<std::wstring> existing = { L"C:\\x\\New folder" };
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(MakeUniqueFolderNames(dir, 2, exists, true),
                  { L"C:\\x\\New folder (2)", L"C:\\x\\New folder (3)" },
                  "autoResolve on: base exists -> (2), (3)");
    }

    // autoResolve = false, base free, count 1 -> exactly "New folder".
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(MakeUniqueFolderNames(dir, 1, exists, false),
                  { L"C:\\x\\New folder" },
                  "autoResolve off: count 1, base free -> 'New folder'");
    }

    if (g_fail == 0) {
        printf("auto_resolve_test: PASS\n");
        return 0;
    }
    printf("auto_resolve_test: FAIL (%d)\n", g_fail);
    return 1;
}
