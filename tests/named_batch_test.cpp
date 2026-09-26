// named_batch_test.cpp
// Auto-resolve name clashes OFF + a number: after the user types a base name,
// the batch is named "<base> 1", "<base> 2", ... skipping any that already
// exist on disk, so it never clobbers and still yields the requested count.
// This pins the pure naming core (MakeNamedFolderNames) that both name-capture
// paths (inline-rename or popup) will feed.
//
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"
#include "capture_logic.h"  // ResolveCaptureNames (single-name vs named-batch)

#include <cstdio>
#include <set>
#include <string>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

int main() {
    const std::wstring dir = L"C:\\tmp";

    // ---- Clean directory: BIG 1, BIG 2, BIG 3 ----
    {
        std::set<std::wstring> present;
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };

        auto names = MakeNamedFolderNames(dir, L"BIG", 3, exists);
        check(names.size() == 3, "3 requested -> 3 names");
        check(names.size() == 3 && names[0] == L"C:\\tmp\\BIG 1", "first is 'BIG 1'");
        check(names.size() == 3 && names[1] == L"C:\\tmp\\BIG 2", "second is 'BIG 2'");
        check(names.size() == 3 && names[2] == L"C:\\tmp\\BIG 3", "third is 'BIG 3'");
    }

    // ---- Skip an existing number, still produce 3 NEW folders ----
    {
        std::set<std::wstring> present = { L"C:\\tmp\\BIG 2" };
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };

        auto names = MakeNamedFolderNames(dir, L"BIG", 3, exists);
        check(names.size() == 3, "3 requested with a clash -> still 3 names");
        check(names.size() == 3 && names[0] == L"C:\\tmp\\BIG 1", "clash: BIG 1");
        check(names.size() == 3 && names[1] == L"C:\\tmp\\BIG 3",
              "clash: skips existing BIG 2 -> BIG 3");
        check(names.size() == 3 && names[2] == L"C:\\tmp\\BIG 4", "clash: BIG 4");
    }

    // ---- Trailing-slash dir is joined cleanly (no double separator) ----
    {
        std::set<std::wstring> present;
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };

        auto names = MakeNamedFolderNames(L"C:\\tmp\\", L"Report", 1, exists);
        check(names.size() == 1 && names[0] == L"C:\\tmp\\Report 1",
              "trailing slash -> 'C:\\tmp\\Report 1'");
    }

    // ---- Guards: count < 1 or empty base -> nothing ----
    {
        std::set<std::wstring> present;
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };

        check(MakeNamedFolderNames(dir, L"BIG", 0, exists).empty(),
              "count 0 -> empty");
        check(MakeNamedFolderNames(dir, L"", 5, exists).empty(),
              "empty base -> empty");
    }

    // ---- Named-batch collision retry picks the next "<base> N", not bare
    //      "<base>". Reproduces the dialog-mode bug: planned Project 1/2/3, a
    //      race creates Project 2 first, and the retry for that slot must land
    //      on the next free numbered name (Project 4 here), NEVER the bare
    //      single-name "Project" that ResolveCaptureNames(count=1) would give,
    //      which would break the sequence into "Project 1 / Project / Project 3".
    {
        std::set<std::wstring> present = {
            L"C:\\tmp\\Project 1",  // already created earlier in the batch
            L"C:\\tmp\\Project 2",  // lost the race to another process
            L"C:\\tmp\\Project 3",  // already created earlier in the batch
        };
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };

        // CORRECT retry: next free numbered name.
        auto retry = MakeNamedFolderNames(dir, L"Project", 1, exists);
        check(retry.size() == 1 && retry[0] == L"C:\\tmp\\Project 4",
              "named-batch retry -> next free 'Project 4'");

        // WRONG retry (the old code): a single custom name resolves to bare
        // "Project" (auto-resolve would even give "Project (2)"), NOT "Project N".
        auto wrong = ResolveCaptureNames(dir, L"Project", 1, exists, true);
        check(!wrong.empty() && wrong[0] != L"C:\\tmp\\Project 4",
              "ResolveCaptureNames(count=1) is the WRONG retry for a batch");
    }

    if (g_fail == 0) {
        printf("named_batch_test: PASS\n");
        return 0;
    }
    printf("named_batch_test: FAIL (%d)\n", g_fail);
    return 1;
}
