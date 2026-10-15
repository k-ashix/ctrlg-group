// group_prompt_foreground_test.cpp
//
// Bug: when Ctrl+G groups files and a file with the same name already exists,
// the "Ask" (and "Compare") policy lets the Windows shell show its native
// "Replace or Skip Files" box. That box is modal to the owner (Explorer)
// window; if Explorer is not in the foreground when it appears, the box is
// drawn BEHIND the window the user is actually looking at, so it goes
// unnoticed.
//
// Fix contract (pure part, testable without Win32): PerformGroupOperation must
// raise the owner window to the foreground right before PerformOperations() in
// exactly the cases where the shell will show a native prompt -- i.e. Ask and
// Compare. Replace and Skip suppress the box (FOF_NOCONFIRMATION / skip
// sentinel), so no foreground raise is needed for them.
//
// FileCollisionShowsNativePrompt() encodes that decision. This test pins it.
//
// TDD RED: FileCollisionShowsNativePrompt does not exist yet -> this fails to
// compile until the GREEN step adds it to group_logic.h.

#include "group_logic.h"

#include <cassert>
#include <cstdio>

int main() {
    // Ask and Compare BOTH let the shell draw its native conflict box, so the
    // owner window must be raised to the foreground first (else the box hides
    // behind the active window). These are the two "raise" cases.
    assert(FileCollisionShowsNativePrompt(GroupFileCollision::Ask)     == true);
    assert(FileCollisionShowsNativePrompt(GroupFileCollision::Compare) == true);

    // Replace and Skip suppress the native box entirely, so there is nothing to
    // surface -- no foreground raise.
    assert(FileCollisionShowsNativePrompt(GroupFileCollision::Replace) == false);
    assert(FileCollisionShowsNativePrompt(GroupFileCollision::Skip)    == false);

    // The predicate must line up with the flag mapping: a policy shows a native
    // prompt exactly when it does NOT suppress confirmation
    // (FOF_NOCONFIRMATION). This keeps the two decisions from drifting apart.
    const GroupFileCollision all[] = {
        GroupFileCollision::Ask, GroupFileCollision::Replace,
        GroupFileCollision::Skip, GroupFileCollision::Compare,
    };
    for (GroupFileCollision p : all) {
        bool suppressesConfirm =
            (FileCollisionFlags(p) & FOF_NOCONFIRMATION) == FOF_NOCONFIRMATION;
        assert(FileCollisionShowsNativePrompt(p) == !suppressesConfirm);
    }

    printf("group_prompt_foreground_test: OK\n");
    return 0;
}
