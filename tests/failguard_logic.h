// failguard_logic.h
//
// PURE failure-handling decisions for fix thse isseue.md item 6, mirrored
// byte-for-byte into ctrlg-group.wh.cpp. The Win32 call sites (DeferWindowPos,
// RegisterClassExW, CreateFontIndirectW/SelectObject) cannot run on the
// un-compilable Windhawk translation unit, so the DECISIONS around them live
// here and are exercised by tests\failguard_test.cpp; the structural wiring is
// pinned by scripts\regression_check.ps1.
#pragma once

// ---------------------------------------------------------------------------
// layoutdefer:: -- ApplyPromptLayout() deferred-move commit (P1 6417-6445).
// The relayout batches every child move into a BeginDeferWindowPos() /
// DeferWindowPos() / EndDeferWindowPos() transaction. DeferWindowPos only
// APPLIES the moves when EndDeferWindowPos() succeeds, so if the batch cannot
// be started or committed, ZERO children move and they are left at stale
// coordinates. In that case the mod must fall back to individual
// SetWindowPos() calls.
// ---------------------------------------------------------------------------
namespace layoutdefer {

// The deferred transaction truly succeeded only when it both started
// (BeginDeferWindowPos returned a handle) AND committed (EndDeferWindowPos
// returned non-zero after every DeferWindowPos chained successfully).
inline bool DeferredCommitOk(bool beginOk, bool endOk) {
    return beginOk && endOk;
}

// When the atomic transaction did not fully succeed, the children never moved,
// so every move must be replayed individually with SetWindowPos().
inline bool NeedsIndividualFallback(bool beginOk, bool endOk) {
    return !DeferredCommitOk(beginOk, endOk);
}

}  // namespace layoutdefer

// ---------------------------------------------------------------------------
// helpclass:: -- EnsureHelpCardClass() registration flag (P2 8929-8941).
// The static `reg` flag must be set ONLY when the window class is actually
// usable: RegisterClassExW() succeeded, OR it failed solely because the class
// was already registered (a benign race / re-entry). Any other failure must
// leave reg=false so a later call retries instead of creating windows for a
// class that does not exist.
// ---------------------------------------------------------------------------
namespace helpclass {

// GetLastError() value Windows returns when RegisterClassExW() fails because a
// class with that name is already registered.
inline unsigned long ClassAlreadyExistsCode() { return 1410UL; }  // ERROR_CLASS_ALREADY_EXISTS

inline bool RegistrationUsable(bool registered, unsigned long lastErr) {
    return registered || lastErr == ClassAlreadyExistsCode();
}

}  // namespace helpclass

// ---------------------------------------------------------------------------
// gdiguard:: -- DrawSemanticSectionIconDev() GDI defensiveness (P2 8366-8378).
// Under resource pressure CreateFontIndirectW() can return null and
// SelectObject() can fail (null / HGDI_ERROR). The renderer must never select a
// null font onto the DC, never draw with an unknown font, never restore an
// invalid previous object, and must always delete a font it did create.
// ---------------------------------------------------------------------------
namespace gdiguard {

// Draw the glyph only when the font was created AND successfully selected.
inline bool ShouldDrawGlyph(bool fontCreated, bool selectOk) {
    return fontCreated && selectOk;
}

// Restore the previous font only when SelectObject() actually returned a valid
// previous object (i.e. the select succeeded). Restoring an invalid handle
// would corrupt the DC's font state.
inline bool ShouldRestoreOldFont(bool selectOk) {
    return selectOk;
}

// A font that WAS created must always be deleted, even if it could not be
// selected or drawn with -- otherwise it leaks.
inline bool MustDeleteCreatedFont(bool fontCreated) {
    return fontCreated;
}

}  // namespace gdiguard
