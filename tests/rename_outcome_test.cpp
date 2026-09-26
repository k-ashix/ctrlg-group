// rename_outcome_test.cpp
// Pins the inline-rename cancellation contract (ShouldUseCapturedName, mirrored
// from the inline flow in explorer-ctrlfn-newfolder.wh.cpp).
//
// Regression: WaitForInlineRenameResult() returned only the last EDIT text plus
// a "did an EDIT appear" bool, inferring commit from "the EDIT disappeared".
// Esc dismisses the EDIT exactly like Enter, so the typed name survived and, in
// multi-folder mode, the batch path created "<name> 1..N" -- breaking the
// README's "Esc = cancel" contract. The watcher now reports a terminal
// RenameOutcome and this decision gates whether the captured name may create
// folders: only a real commit counts; Esc/timeout/never-started must cancel.
//
// Self-contained: own main(), exit 0 = pass.

#include "capture_logic.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

int main() {
    // Commit paths -> the captured name may drive folder creation.
    check(ShouldUseCapturedName(RenameOutcome::Committed),
          "Enter (Committed) -> use captured name");
    check(ShouldUseCapturedName(RenameOutcome::FocusLostCommitted),
          "focus-lost with text intact (FocusLostCommitted) -> use name");

    // The regression itself: Esc must NEVER create folders.
    check(!ShouldUseCapturedName(RenameOutcome::Cancelled),
          "Esc (Cancelled) -> discard, do NOT create");

    // Non-commit terminal states also cancel.
    check(!ShouldUseCapturedName(RenameOutcome::Timeout),
          "watcher Timeout -> discard, do NOT create");
    check(!ShouldUseCapturedName(RenameOutcome::NeverStarted),
          "rename EDIT never appeared (NeverStarted) -> do NOT create");

    if (g_fail == 0) {
        printf("rename_outcome_test: PASS\n");
        return 0;
    }
    printf("rename_outcome_test: FAIL (%d)\n", g_fail);
    return 1;
}
