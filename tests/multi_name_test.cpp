// multi_name_test.cpp
// Verifies MakeUniqueFolderNames: a batch of N Explorer-native names that reserve
// within the batch and skip anything already on disk -- so creating more folders
// when some already exist never collides and never clobbers.
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

    // 1. Empty dir, 3 folders -> New folder, (2), (3).
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(MakeUniqueFolderNames(dir, 3, exists),
                  { L"C:\\x\\New folder", L"C:\\x\\New folder (2)", L"C:\\x\\New folder (3)" },
                  "empty dir, count 3");
    }

    // 2. count 0 -> empty batch.
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(MakeUniqueFolderNames(dir, 0, exists), {}, "count 0 -> empty");
    }

    // 3. count 1 on empty dir -> just "New folder".
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(MakeUniqueFolderNames(dir, 1, exists),
                  { L"C:\\x\\New folder" }, "count 1 -> single 'New folder'");
    }

    // 4. The headline scenario: a tab already has 10 "New folder"s and the user
    //    asks for 2 more -> (11), (12). No error, no clobber.
    {
        std::set<std::wstring> existing = { L"C:\\x\\New folder" };
        for (int i = 2; i <= 10; ++i) {
            wchar_t buf[64] = {0};
            std::swprintf(buf, 64, L"C:\\x\\New folder (%d)", i);
            existing.insert(buf);
        }
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(MakeUniqueFolderNames(dir, 2, exists),
                  { L"C:\\x\\New folder (11)", L"C:\\x\\New folder (12)" },
                  "10 existing + 2 more -> (11), (12)");
    }

    // 5. In-batch reservation: names within one batch must be distinct even when
    //    the directory starts empty (batch item 1 reserves "New folder").
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        auto got = MakeUniqueFolderNames(dir, 4, exists);
        std::set<std::wstring> uniq(got.begin(), got.end());
        if (uniq.size() != got.size()) {
            printf("[FAIL] in-batch names must be unique (got %zu, unique %zu)\n",
                   got.size(), uniq.size());
            ++g_fail;
        }
    }

    // 6. Trailing separator on dir must not double the slash.
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(MakeUniqueFolderNames(L"C:\\x\\", 1, exists),
                  { L"C:\\x\\New folder" }, "trailing backslash joins cleanly");
    }

    if (g_fail == 0) {
        printf("multi_name_test: PASS\n");
        return 0;
    }
    printf("multi_name_test: FAIL (%d)\n", g_fail);
    return 1;
}
