// capture_selection_test.cpp
// Task 5: capture Explorer selection, validate via ValidateGroup, then invalidate.
// This mirrors the pure decisions the mod's CaptureAndValidateGroupSelection
// makes after capturing an IShellItemArray into a std::vector<GroupItem>.
// Self-contained: exit 0 = pass.

#include "group_logic.h"

#include <vector>
#include <string>
#include <cstdio>

static int g_fail = 0;
static void expectSafety(GroupSafety got, GroupSafety want, const char* what) {
    if (got != want) {
        printf("[FAIL] %s (got %d want %d)\n", what, (int)got, (int)want);
        ++g_fail;
    }
}

static GroupItem file(const std::wstring& p)   { GroupItem g; g.path=p; g.isFolder=false; g.isVirtual=false; return g; }
static GroupItem folder(const std::wstring& p) { GroupItem g; g.path=p; g.isFolder=true;  g.isVirtual=false; return g; }
static GroupItem virt(const std::wstring& p)   { GroupItem g; g.path=p; g.isFolder=false; g.isVirtual=true;  return g; }

int main() {
    // 1. Empty capture -> EmptySelection (capture returned nothing selected).
    expectSafety(ValidateGroup({}, L"C:\\shoot\\Prewedding", true),
                 GroupSafety::EmptySelection, "empty capture");

    // 2. A virtual/non-filesystem item captured among normal items -> VirtualItem.
    {
        std::vector<GroupItem> sel = { file(L"C:\\shoot\\a.jpg"),
                                       folder(L"C:\\shoot\\clips"),
                                       virt(L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}") };
        expectSafety(ValidateGroup(sel, L"C:\\shoot\\Prewedding", true),
                     GroupSafety::VirtualItem, "virtual item captured");
    }

    // 3. All real filesystem files+folders with an unrelated destination -> Ok.
    {
        std::vector<GroupItem> sel = { file(L"C:\\shoot\\a.jpg"),
                                       file(L"C:\\shoot\\b.jpg"),
                                       folder(L"C:\\shoot\\clipsdir") };
        expectSafety(ValidateGroup(sel, L"C:\\shoot\\Prewedding", true),
                     GroupSafety::Ok, "all real items ok");
    }

    // 4. A selected folder equal to the destination -> MoveIntoSelf.
    {
        std::vector<GroupItem> sel = { folder(L"C:\\shoot\\Prewedding") };
        expectSafety(ValidateGroup(sel, L"C:\\shoot\\Prewedding", true),
                     GroupSafety::MoveIntoSelf, "dest equals selected folder");
    }

    // 5. A selected folder that is an ancestor of the destination -> MoveIntoSelf.
    {
        std::vector<GroupItem> sel = { folder(L"C:\\shoot\\Prewedding") };
        expectSafety(ValidateGroup(sel, L"C:\\shoot\\Prewedding\\sub\\x", true),
                     GroupSafety::MoveIntoSelf, "dest nested under selected folder");
    }

    if (g_fail == 0) { printf("capture_selection_test: PASS\n"); return 0; }
    printf("capture_selection_test: FAIL (%d)\n", g_fail);
    return 1;
}
