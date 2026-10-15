// group_dest_test.cpp
// Task 1: CommonParent + path helpers. Self-contained: exit 0 = pass.

#include "group_logic.h"

#include <string>
#include <vector>
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
static void expectTrue(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ParentDirOf
    expectEqW(ParentDirOf(L"C:\\a\\b\\c.txt"), L"C:\\a\\b", "parent of file");
    expectEqW(ParentDirOf(L"C:\\a\\b\\"), L"C:\\a", "parent ignores trailing sep");
    expectEqW(ParentDirOf(L"C:\\a"), L"C:\\", "parent of top-level is root");

    // TrimTrailingSep
    expectEqW(TrimTrailingSep(L"C:\\a\\"), L"C:\\a", "trim trailing backslash");
    expectEqW(TrimTrailingSep(L"C:\\a"), L"C:\\a", "no trailing sep unchanged");

    // CommonParent: all share one parent
    {
        std::vector<std::wstring> sel = {
            L"C:\\shoot\\a.jpg", L"C:\\shoot\\b.jpg", L"C:\\shoot\\sub",
        };
        expectEqW(CommonParent(sel), L"C:\\shoot", "common parent of siblings");
    }
    // CommonParent: mixed parents -> empty
    {
        std::vector<std::wstring> sel = {
            L"C:\\shoot\\a.jpg", L"D:\\other\\b.jpg",
        };
        expectTrue(CommonParent(sel).empty(), "mixed parents -> empty");
    }
    // CommonParent: empty selection -> empty
    expectTrue(CommonParent({}).empty(), "empty selection -> empty");

    // CommonParent: single item still yields its parent
    expectEqW(CommonParent({ L"C:\\shoot\\a.jpg" }), L"C:\\shoot",
              "single item -> its parent");

    // IsSameOrDescendant
    expectTrue(IsSameOrDescendant(L"C:\\a", L"C:\\a"), "same path");
    expectTrue(IsSameOrDescendant(L"C:\\a", L"C:\\a\\b\\c"), "descendant");
    expectTrue(!IsSameOrDescendant(L"C:\\a", L"C:\\ab"), "sibling prefix not child");
    expectTrue(!IsSameOrDescendant(L"C:\\a\\b", L"C:\\a"), "parent is not descendant");

    if (g_fail == 0) { printf("group_dest_test: PASS\n"); return 0; }
    printf("group_dest_test: FAIL (%d)\n", g_fail);
    return 1;
}
