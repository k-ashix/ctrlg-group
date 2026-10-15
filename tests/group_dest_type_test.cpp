// group_dest_type_test.cpp
// Pins the fix thse isseue.md P1 findings:
//   * "Normal Group does not verify that the destination is a directory"
//     (lines 13448-13475/13477-13519): an existing path may be reused/created
//     into ONLY when it is a directory; a plain file occupying the name is a
//     hard error.
//   * "Self/descendant validation happens after destination creation"
//     (lines 13485-13511): the destination-type check must be reported BEFORE
//     the self/descendant check (and, in the mod, before any filesystem side
//     effect), so a rejected op never leaves an empty directory behind.
// Pure: mirrors tests/group_logic.h (GroupDestinationUsable + the type-aware
// ValidateGroup overload). Self-contained: own main(), exit 0 = pass.

#include "group_logic.h"

#include <string>
#include <vector>
#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- GroupDestinationUsable: file at the destination is NOT usable ----
    check(GroupDestinationUsable(/*exists*/false, /*isDir*/false),
          "nothing at the path -> usable (will be created)");
    check(GroupDestinationUsable(/*exists*/true, /*isDir*/true),
          "existing directory -> usable (reuse)");
    check(!GroupDestinationUsable(/*exists*/true, /*isDir*/false),
          "existing FILE at the path -> NOT usable (hard error)");

    // ---- type-aware ValidateGroup: file dest reported before anything else ----
    std::vector<GroupItem> sel = {
        {L"C:/shoot/a", true, false},
        {L"C:/shoot/b", true, false},
    };
    check(ValidateGroup(sel, L"C:/shoot/Prewedding", /*moving*/true,
                        /*destExistsAsFile*/true) == GroupSafety::DestNotDirectory,
          "a file occupying the destination -> DestNotDirectory");
    check(ValidateGroup(sel, L"C:/shoot/Prewedding", /*moving*/true,
                        /*destExistsAsFile*/false) == GroupSafety::Ok,
          "a free/directory destination with a safe selection -> Ok");

    // Ordering: when the destination is BOTH a file AND a descendant target,
    // the type error wins (it is checked first, before any side effect).
    std::vector<GroupItem> selSelf = { {L"C:/shoot/Prewedding", true, false} };
    check(ValidateGroup(selSelf, L"C:/shoot/Prewedding/inner", /*moving*/true,
                        /*destExistsAsFile*/true) == GroupSafety::DestNotDirectory,
          "file-at-dest is reported before the self/descendant check");
    // Same selection, destination is a real directory -> the self/descendant
    // guard still fires (unchanged behavior).
    check(ValidateGroup(selSelf, L"C:/shoot/Prewedding/inner", /*moving*/true,
                        /*destExistsAsFile*/false) == GroupSafety::MoveIntoSelf,
          "descendant destination still refused when it is a directory");

    // The original 3-arg ValidateGroup overload is unchanged.
    check(ValidateGroup(sel, L"C:/shoot/Prewedding", /*moving*/true) == GroupSafety::Ok,
          "legacy 3-arg ValidateGroup still returns Ok for a safe selection");

    if (g_fail) { printf("group_dest_type: %d check(s) FAILED\n", g_fail); return 1; }
    printf("group_dest_type: all checks passed\n");
    return 0;
}
