// group_filecollision_test.cpp
//
// Issue 1: when Ctrl+G copies/moves items into a destination that already
// contains a file of the same name, the Windows shell shows its native
// "Replace or Skip Files" dialog. The user wants a setting to control this up
// front -- Ask (native prompt, default), Replace (overwrite silently), or Skip
// (keep the existing file) -- mirroring the choices Windows itself offers.
//
// These tests pin the PURE mapping: settings string -> policy, and policy ->
// the IFileOperation flag bits that PerformGroupOperation must apply. No Win32
// runtime needed; the flag constants are mirrored in group_logic.h.
//
// TDD RED: written before the implementation exists.

#include "group_logic.h"

#include <cassert>
#include <cstdio>

int main() {
    // --- Parsing the settings string -------------------------------------
    // Default / unknown / empty -> Ask (show the native prompt). This keeps
    // today's behavior for anyone who never touches the new setting.
    assert(ParseGroupFileCollision(L"")        == GroupFileCollision::Ask);
    assert(ParseGroupFileCollision(L"ask")     == GroupFileCollision::Ask);
    assert(ParseGroupFileCollision(L"garbage") == GroupFileCollision::Ask);
    assert(ParseGroupFileCollision(L"replace") == GroupFileCollision::Replace);
    assert(ParseGroupFileCollision(L"skip")    == GroupFileCollision::Skip);
    // Compare mirrors Windows' "Compare info for both files": show the native
    // conflict box so the user can compare and decide per file.
    assert(ParseGroupFileCollision(L"compare") == GroupFileCollision::Compare);

    // --- Policy -> IFileOperation flags ----------------------------------
    // Ask: no extra confirmation-suppressing flags. The shell decides and
    // shows its native Replace/Skip dialog. This is today's behavior.
    assert(FileCollisionFlags(GroupFileCollision::Ask) == 0);

    // Replace: silently overwrite. FOF_NOCONFIRMATION suppresses the "Replace
    // or Skip" prompt so the copied file wins with no dialog.
    assert((FileCollisionFlags(GroupFileCollision::Replace) & FOF_NOCONFIRMATION)
           == FOF_NOCONFIRMATION);
    // Replace must NOT rename-on-collision (that would keep both, not replace).
    assert((FileCollisionFlags(GroupFileCollision::Replace) & FOF_RENAMEONCOLLISION)
           == 0);

    // Skip: keep the existing file, never prompt and never overwrite. The
    // worker enforces the actual skip per-item; the flag set still suppresses
    // the native dialog (FOF_NOCONFIRMATION) and carries the dedicated skip
    // sentinel so the worker knows to drop colliding items.
    assert((FileCollisionFlags(GroupFileCollision::Skip) & FOF_NOCONFIRMATION)
           == FOF_NOCONFIRMATION);
    assert((FileCollisionFlags(GroupFileCollision::Skip) & kGroupSkipExistingBit)
           == kGroupSkipExistingBit);
    // Replace must not carry the skip sentinel.
    assert((FileCollisionFlags(GroupFileCollision::Replace) & kGroupSkipExistingBit)
           == 0);

    // Compare: show the native conflict box (so Windows' "Compare info for
    // both files" is available). Like Ask, it does NOT suppress confirmation,
    // and it never silently overwrites or skips.
    assert((FileCollisionFlags(GroupFileCollision::Compare) & FOF_NOCONFIRMATION)
           == 0);
    assert((FileCollisionFlags(GroupFileCollision::Compare) & kGroupSkipExistingBit)
           == 0);
    // Compare carries its own sentinel so the worker/log can tell it apart from
    // plain Ask even though both let the shell prompt.
    assert((FileCollisionFlags(GroupFileCollision::Compare) & kGroupCompareBit)
           == kGroupCompareBit);
    assert((FileCollisionFlags(GroupFileCollision::Ask) & kGroupCompareBit) == 0);

    // All four policies must differ (guards against a stub returning one
    // constant for everything).
    assert(FileCollisionFlags(GroupFileCollision::Ask)
           != FileCollisionFlags(GroupFileCollision::Replace));
    assert(FileCollisionFlags(GroupFileCollision::Replace)
           != FileCollisionFlags(GroupFileCollision::Skip));
    assert(FileCollisionFlags(GroupFileCollision::Ask)
           != FileCollisionFlags(GroupFileCollision::Skip));
    assert(FileCollisionFlags(GroupFileCollision::Ask)
           != FileCollisionFlags(GroupFileCollision::Compare));
    assert(FileCollisionFlags(GroupFileCollision::Compare)
           != FileCollisionFlags(GroupFileCollision::Skip));

    std::puts("group_filecollision_test: PASS");
    return 0;
}
