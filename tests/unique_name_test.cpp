// unique_name_test.cpp
// Verifies MakeUniqueFolderName produces Explorer-style "New folder (n)" names,
// skipping any names that already exist. Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <set>
#include <string>
#include <cstdio>

static int g_fail = 0;

static void expectEq(const std::wstring& got,
                     const std::wstring& want,
                     const char* what) {
    if (got != want) {
        printf("[FAIL] %s\n  got : %ls\n  want: %ls\n",
               what, got.c_str(), want.c_str());
        ++g_fail;
    }
}

int main() {
    const std::wstring dir = L"C:\\x";

    // 1. Empty directory -> plain "New folder".
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectEq(MakeUniqueFolderName(dir, exists),
                 L"C:\\x\\New folder",
                 "empty dir yields 'New folder'");
    }

    // 2. "New folder" taken -> "New folder (2)".
    {
        std::set<std::wstring> existing = { L"C:\\x\\New folder" };
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectEq(MakeUniqueFolderName(dir, exists),
                 L"C:\\x\\New folder (2)",
                 "'New folder' taken yields '(2)'");
    }

    // 3. "New folder" and "(2)" taken -> "New folder (3)".
    {
        std::set<std::wstring> existing = {
            L"C:\\x\\New folder",
            L"C:\\x\\New folder (2)",
        };
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectEq(MakeUniqueFolderName(dir, exists),
                 L"C:\\x\\New folder (3)",
                 "'New folder' + '(2)' taken yields '(3)'");
    }

    // 4. Trailing separator on dir must not double the slash.
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectEq(MakeUniqueFolderName(L"C:\\x\\", exists),
                 L"C:\\x\\New folder",
                 "trailing backslash joins cleanly");
    }

    if (g_fail == 0) {
        printf("unique_name_test: PASS\n");
        return 0;
    }

    printf("unique_name_test: FAIL (%d)\n", g_fail);
    return 1;
}
