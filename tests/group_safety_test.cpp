// group_safety_test.cpp
// Task 4: ValidateGroup -- empty selection, virtual items, move-into-self/child.
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

static GroupItem file(const std::wstring& p) { GroupItem g; g.path=p; g.isFolder=false; return g; }
static GroupItem folder(const std::wstring& p){ GroupItem g; g.path=p; g.isFolder=true;  return g; }
static GroupItem virt(const std::wstring& p) { GroupItem g; g.path=p; g.isVirtual=true; return g; }

int main() {
    // 1. Empty selection -> EmptySelection.
    expectSafety(ValidateGroup({}, L"C:\\shoot\\Prewedding", false),
                 GroupSafety::EmptySelection, "empty selection");

    // 2. Virtual item present -> VirtualItem (both move and copy).
    {
        std::vector<GroupItem> sel = { file(L"C:\\shoot\\a.jpg"),
                                       virt(L"::{GUID}\\ThisPC") };
        expectSafety(ValidateGroup(sel, L"C:\\shoot\\Prewedding", true),
                     GroupSafety::VirtualItem, "virtual item on move");
        expectSafety(ValidateGroup(sel, L"C:\\shoot\\Prewedding", false),
                     GroupSafety::VirtualItem, "virtual item on copy");
    }

    // 3. Move a folder into itself -> MoveIntoSelf.
    {
        std::vector<GroupItem> sel = { folder(L"C:\\shoot\\Prewedding") };
        expectSafety(ValidateGroup(sel, L"C:\\shoot\\Prewedding", true),
                     GroupSafety::MoveIntoSelf, "move folder into itself");
    }

    // 4. Move a folder into one of its own descendants -> MoveIntoSelf.
    {
        std::vector<GroupItem> sel = { folder(L"C:\\shoot\\Prewedding") };
        expectSafety(ValidateGroup(sel, L"C:\\shoot\\Prewedding\\sub\\x", true),
                     GroupSafety::MoveIntoSelf, "move folder into descendant");
    }

    // 5. Ordinary files+folders into a fresh destination -> Ok.
    {
        std::vector<GroupItem> sel = { file(L"C:\\shoot\\a.jpg"),
                                       folder(L"C:\\shoot\\clipsdir") };
        expectSafety(ValidateGroup(sel, L"C:\\shoot\\Prewedding", true),
                     GroupSafety::Ok, "normal move is ok");
    }

    if (g_fail == 0) { printf("group_safety_test: PASS\n"); return 0; }
    printf("group_safety_test: FAIL (%d)\n", g_fail);
    return 1;
}
