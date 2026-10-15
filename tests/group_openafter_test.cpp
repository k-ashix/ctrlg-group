// group_openafter_test.cpp
// Task 9: "Open destination after grouping". Unlike New Folder's
// ShouldOpenAfterCreate (which only opens on a SINGLE create), a Ctrl+G group
// always lands in exactly one destination, so it opens whenever the setting is
// on AND the group plan actually ran (valid). This pins that decision so the
// live handler cannot silently ignore the setting again.
//
// TDD RED: ShouldOpenGroupDestination does not exist in group_logic.h yet ->
// this fails to compile until the GREEN step adds it.

#include "group_logic.h"

#include <cassert>
#include <cstdio>

int main() {
    // Setting ON + a plan that ran -> open the destination.
    assert(ShouldOpenGroupDestination(/*openAfter*/true,  /*planValid*/true)  == true);
    // Setting ON but the plan did not run (cancel / invalid) -> nothing opens.
    assert(ShouldOpenGroupDestination(/*openAfter*/true,  /*planValid*/false) == false);
    // Setting OFF -> never open, even on a valid plan (the default, off).
    assert(ShouldOpenGroupDestination(/*openAfter*/false, /*planValid*/true)  == false);
    assert(ShouldOpenGroupDestination(/*openAfter*/false, /*planValid*/false) == false);

    printf("group_openafter_test: OK\n");
    return 0;
}
