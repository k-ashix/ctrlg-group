// failguard_test.cpp
// Pins the fix thse isseue.md item 6 failure-handling findings as PURE
// predicates the mod mirrors (the Win32 call sites themselves cannot run on the
// un-compilable TU, so the DECISIONS are mirrored and asserted here, and the
// structural wiring is pinned by scripts/regression_check.ps1):
//
//   * P1 6417-6445  ApplyPromptLayout(): a failed BeginDeferWindowPos() /
//     EndDeferWindowPos() must fall back to individual SetWindowPos() calls so
//     no child is left at stale coordinates.
//   * P2 8929-8941  EnsureHelpCardClass(): the static `reg` flag may be set
//     ONLY when the class is actually usable -- RegisterClassExW() succeeded, or
//     it failed solely because the class already exists.
//   * P2 8366-8378  DrawSemanticSectionIconDev(): draw the glyph ONLY when the
//     font was created AND selected; never SelectObject(nullptr); restore the
//     old font only when the select succeeded; always delete a created font.
//
// Self-contained: own main(), exit 0 = pass.

#include "failguard_logic.h"

#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- layoutdefer: deferred-batch success vs. individual fallback --------
    using layoutdefer::DeferredCommitOk;
    using layoutdefer::NeedsIndividualFallback;
    check(DeferredCommitOk(/*beginOk*/true, /*endOk*/true),
          "begin + commit both ok -> deferred transaction succeeded");
    check(!DeferredCommitOk(true, false), "commit failed -> not a success");
    check(!DeferredCommitOk(false, false), "begin failed -> not a success");
    check(!NeedsIndividualFallback(true, true),
          "a clean deferred commit needs NO fallback");
    check(NeedsIndividualFallback(false, false),
          "BeginDeferWindowPos failed -> individual SetWindowPos fallback");
    check(NeedsIndividualFallback(true, false),
          "EndDeferWindowPos failed -> individual SetWindowPos fallback");

    // ---- helpclass: when may the static `reg` flag be set? ------------------
    using helpclass::RegistrationUsable;
    using helpclass::ClassAlreadyExistsCode;
    check(RegistrationUsable(/*registered*/true, /*lastErr*/0),
          "RegisterClassExW succeeded -> usable (mark reg)");
    check(RegistrationUsable(false, ClassAlreadyExistsCode()),
          "failed with ERROR_CLASS_ALREADY_EXISTS -> benign, usable");
    check(!RegistrationUsable(false, 8 /*ERROR_NOT_ENOUGH_MEMORY*/),
          "failed for a real reason -> NOT usable (leave reg=false, retry)");
    check(!RegistrationUsable(false, 0),
          "failed with no sensible error -> NOT usable");
    check(ClassAlreadyExistsCode() == 1410UL,
          "ERROR_CLASS_ALREADY_EXISTS numeric code is 1410");

    // ---- gdiguard: defensive font create/select/draw/restore ----------------
    using gdiguard::ShouldDrawGlyph;
    using gdiguard::ShouldRestoreOldFont;
    using gdiguard::MustDeleteCreatedFont;
    // font not created at all -> never select, never draw, nothing to delete
    check(!ShouldDrawGlyph(/*fontCreated*/false, /*selectOk*/false),
          "no font created -> do not draw");
    check(!MustDeleteCreatedFont(false), "no font created -> nothing to delete");
    // font created but SelectObject failed -> do not draw, do not restore an
    // invalid old object, but DO delete the created font
    check(!ShouldDrawGlyph(true, false), "select failed -> do not draw");
    check(!ShouldRestoreOldFont(false),
          "select failed -> do not restore an invalid previous object");
    check(MustDeleteCreatedFont(true),
          "a created font is always deleted, even when select failed");
    // happy path: created + selected -> draw, restore, delete
    check(ShouldDrawGlyph(true, true), "created + selected -> draw the glyph");
    check(ShouldRestoreOldFont(true), "select ok -> restore the old font");
    check(MustDeleteCreatedFont(true), "created font deleted on the happy path");

    if (g_fail) { printf("failguard: %d check(s) FAILED\n", g_fail); return 1; }
    printf("failguard: all checks passed\n");
    return 0;
}
