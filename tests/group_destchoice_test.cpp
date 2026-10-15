// group_destchoice_test.cpp
// Task 9: Ctrl+G destination precedence -- fixed destination > typed name under
// the selection's common parent > last-used target. This is the decision the
// live WM_APP_GROUP handler must mirror instead of hard-coding a "New folder"
// destination. Self-contained: exit 0 = pass.
//
// TDD RED: ResolveGroupDestination / GroupDestChoice do not exist in
// group_logic.h yet -> this fails to compile until the GREEN step adds them.

#include "group_logic.h"

#include <string>
#include <cstdio>

static int g_fail = 0;
static void eqW(const std::wstring& got, const std::wstring& want,
                const char* what) {
    if (got != want) {
        printf("[FAIL] %s\n  got : %ls\n  want: %ls\n",
               what, got.c_str(), want.c_str());
        ++g_fail;
    }
}
static void tru(bool c, const char* what) {
    if (!c) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // 1. A fixed destination wins over a typed name and a last target, and is
    //    used wholesale (no collision policy on a name -- it IS the folder).
    {
        GroupDestChoice c =
            ResolveGroupDestination(L"C:\\Sorted", L"C:\\shoot", L"Trip",
                                    L"C:\\old\\Prev");
        tru(c.valid, "fixed: valid");
        tru(!c.useNamedInParent, "fixed: used wholesale");
        eqW(c.wholePath, L"C:\\Sorted", "fixed: path");
    }

    // 2. Fixed destination is whitespace/trailing-separator trimmed.
    {
        GroupDestChoice c =
            ResolveGroupDestination(L"  C:\\Sorted\\  ", L"C:\\shoot", L"", L"");
        eqW(c.wholePath, L"C:\\Sorted", "fixed: trimmed");
    }

    // 3. No fixed destination + a typed name -> <commonParent>\<name>, flagged
    //    so the caller still runs the folder-collision policy on it.
    {
        GroupDestChoice c =
            ResolveGroupDestination(L"", L"C:\\shoot", L"  Prewedding  ",
                                    L"C:\\old\\Prev");
        tru(c.valid && c.useNamedInParent, "typed: named-in-parent");
        eqW(c.parentDir, L"C:\\shoot", "typed: parent");
        eqW(c.folderName, L"Prewedding", "typed: trimmed name");
    }

    // 4. No fixed destination + a blank name -> reuse the last target wholesale.
    {
        GroupDestChoice c =
            ResolveGroupDestination(L"", L"C:\\shoot", L"   ", L"C:\\old\\Trip");
        tru(c.valid && !c.useNamedInParent, "last: used wholesale");
        eqW(c.wholePath, L"C:\\old\\Trip", "last: path");
    }

    // 5. No fixed destination, blank name, no last target -> nothing to do.
    {
        GroupDestChoice c =
            ResolveGroupDestination(L"", L"C:\\shoot", L"", L"");
        tru(!c.valid, "blank + no last target: invalid");
    }

    if (g_fail) {
        printf("group_destchoice_test: %d FAIL\n", g_fail);
        return 1;
    }
    printf("group_destchoice_test: OK\n");
    return 0;
}
