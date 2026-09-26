// capture_resolve_test.cpp
// Pins the pure name-capture resolution shared by BOTH forks
// (inline_rename_test.wh.cpp and dailogbox_rename_test.wh.cpp):
//   - SanitizeFolderBase : trims + strips forbidden chars from typed input
//   - MakeUniqueNamedFolder : single "<base>" / "<base> (2)" collision handling
//   - ResolveCaptureNames : empty base -> default flow; count 1 -> "<base>";
//                           count N -> "<base> 1".."<base> N" (skips existing)
//
// Self-contained: own main(), exit 0 = pass.

#include "capture_logic.h"

#include <cstdio>
#include <set>
#include <string>
#include <vector>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

int main() {
    const std::wstring dir = L"C:\\tmp";

    // ---- SanitizeFolderBase ----
    check(SanitizeFolderBase(L"  BIG  ") == L"BIG", "trims surrounding spaces");
    check(SanitizeFolderBase(L"My Folder") == L"My Folder", "keeps inner spaces");
    check(SanitizeFolderBase(L"a/b:c*d?") == L"abcd", "drops forbidden chars");
    check(SanitizeFolderBase(L"Report.") == L"Report", "strips trailing dot");
    check(SanitizeFolderBase(L"   ") == L"", "all-whitespace -> empty");
    check(SanitizeFolderBase(L"") == L"", "empty -> empty");
    check(SanitizeFolderBase(L"??") == L"", "only-forbidden -> empty");
    // #12: strip ALL C0 control chars (< 0x20), not just \r \n \t. A bell,
    // SOH, or unit-separator embedded in the typed name would otherwise
    // survive into CreateDirectoryW and make it fail.
    check(SanitizeFolderBase(L"a\x01" L"b\x1f" L"c") == L"abc",
          "strips embedded control chars SOH/US");
    check(SanitizeFolderBase(L"be\x07ll") == L"bell", "strips bell (0x07)");
    check(SanitizeFolderBase(L"tab\ttab") == L"tabtab", "still strips tab");

    // ---- MakeUniqueNamedFolder (single-name collisions) ----
    {
        std::set<std::wstring> present;
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };
        check(MakeUniqueNamedFolder(dir, L"BIG", exists) == L"C:\\tmp\\BIG",
              "free single -> 'BIG'");
    }
    {
        std::set<std::wstring> present = { L"C:\\tmp\\BIG" };
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };
        check(MakeUniqueNamedFolder(dir, L"BIG", exists) == L"C:\\tmp\\BIG (2)",
              "taken single -> 'BIG (2)'");
    }

    // ---- ResolveCaptureNames: empty base -> default "New folder" flow ----
    {
        std::set<std::wstring> present;
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };
        auto n = ResolveCaptureNames(dir, L"", 3, exists, true);
        check(n.size() == 3, "empty base, count 3 -> 3 defaults");
        check(n.size() == 3 && n[0] == L"C:\\tmp\\New folder", "default[0] New folder");
        check(n.size() == 3 && n[1] == L"C:\\tmp\\New folder (2)", "default[1] (2)");
        check(n.size() == 3 && n[2] == L"C:\\tmp\\New folder (3)", "default[2] (3)");
    }

    // ---- ResolveCaptureNames: whitespace/forbidden base falls back too ----
    {
        std::set<std::wstring> present;
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };
        auto n = ResolveCaptureNames(dir, L"??", 2, exists, true);
        check(n.size() == 2 && n[0] == L"C:\\tmp\\New folder",
              "forbidden-only base falls back to default flow");
    }

    // ---- ResolveCaptureNames: base + count 1 -> exact "<base>" ----
    {
        std::set<std::wstring> present;
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };
        auto n = ResolveCaptureNames(dir, L"BIG", 1, exists, true);
        check(n.size() == 1 && n[0] == L"C:\\tmp\\BIG", "base + count 1 -> 'BIG'");
    }
    {
        std::set<std::wstring> present = { L"C:\\tmp\\BIG" };
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };
        auto n = ResolveCaptureNames(dir, L"BIG", 1, exists, true);
        check(n.size() == 1 && n[0] == L"C:\\tmp\\BIG (2)",
              "base + count 1, taken -> 'BIG (2)'");
    }

    // ---- ResolveCaptureNames: base + count N -> "<base> 1".."<base> N" ----
    {
        std::set<std::wstring> present;
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };
        auto n = ResolveCaptureNames(dir, L"BIG", 3, exists, true);
        check(n.size() == 3 && n[0] == L"C:\\tmp\\BIG 1", "count 3 -> BIG 1");
        check(n.size() == 3 && n[1] == L"C:\\tmp\\BIG 2", "count 3 -> BIG 2");
        check(n.size() == 3 && n[2] == L"C:\\tmp\\BIG 3", "count 3 -> BIG 3");
    }

    // ---- ResolveCaptureNames: sanitizes then batches; skips existing ----
    {
        std::set<std::wstring> present = { L"C:\\tmp\\BIG 1" };
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };
        auto n = ResolveCaptureNames(dir, L"  BIG  ", 2, exists, true);
        check(n.size() == 2 && n[0] == L"C:\\tmp\\BIG 2",
              "trimmed base skips existing BIG 1 -> BIG 2");
        check(n.size() == 2 && n[1] == L"C:\\tmp\\BIG 3", "then BIG 3");
    }

    // ---- #9: autoResolveNames is honored for CUSTOM names, not just default ----
    // autoResolve=false + single custom name whose exact name is taken must NOT
    // silently number to "(2)" -- it returns the bare "<base>" (the create layer
    // then reports the collision instead of quietly picking a different name).
    {
        std::set<std::wstring> present = { L"C:\\tmp\\BIG" };
        auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };
        auto on  = ResolveCaptureNames(dir, L"BIG", 1, exists, /*autoResolve*/ true);
        auto off = ResolveCaptureNames(dir, L"BIG", 1, exists, /*autoResolve*/ false);
        check(on.size() == 1 && on[0] == L"C:\\tmp\\BIG (2)",
              "autoResolve on: taken custom single -> 'BIG (2)'");
        check(off.size() == 1 && off[0] == L"C:\\tmp\\BIG",
              "autoResolve off: taken custom single -> bare 'BIG' (no (2))");
    }

    if (g_fail == 0) {
        printf("capture_resolve_test: PASS\n");
        return 0;
    }
    printf("capture_resolve_test: FAIL (%d)\n", g_fail);
    return 1;
}
