// template_after_shift_release_test.cpp
//
// Prove-It regression test for the reported bug:
//   "Ctrl+Shift+N then a template letter: first attempt creates nothing (or
//    one default folder); a second identical attempt fires the template and
//    creates all its folders."
//
// ROOT CAUSE: to type a LOWERCASE template letter (e.g. 'i') the user releases
// Shift. The capture window was gated on EVERY configured hold-modifier staying
// down (Ctrl AND Shift for Ctrl+Shift+N). Releasing Shift closed the window and
// committed a single default folder BEFORE the template letter arrived, so the
// template never fired on that attempt.
//
// The fix: once the capture window is open, a letter BOUND to a template fires
// the template REGARDLESS of which hold-modifiers are still down. Modifier
// release only commits an accumulated count -- it must never pre-empt a bound
// template letter.
//
// This exercises the pure decision CaptureLetterDecide (mirrored into the mod's
// count/template capture handler). Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <cstdio>

static int g_fail = 0;
static void ok(bool cond, const char* what) {
    if (!cond) { std::printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // --- THE BUG SCENARIO ---------------------------------------------------
    // Ctrl+Shift+N fired -> capture window open. No digits typed. The user
    // releases Shift (modifierHeld=false) to type the lowercase template letter
    // 'i'. That letter IS bound to a template. It MUST fire the template, not
    // have already committed a folder.
    ok(CaptureLetterDecide(CaptureKey::BoundLetter, /*sawDigit=*/false,
                           /*modifierHeld=*/false) == CaptureRoute::FireTemplate,
       "Ctrl+Shift+N: bound template letter after Shift release -> FireTemplate");

    // Same bound letter while BOTH modifiers are still held (the accidental
    // 'second attempt worked' path) also fires the template -- consistent
    // behaviour, so the outcome no longer depends on how keys were held.
    ok(CaptureLetterDecide(CaptureKey::BoundLetter, /*sawDigit=*/false,
                           /*modifierHeld=*/true) == CaptureRoute::FireTemplate,
       "bound template letter with modifiers still held -> FireTemplate");

    // --- COUNT PATHS STILL WORK (no regression) -----------------------------
    // A digit while the modifier is held keeps capturing.
    ok(CaptureLetterDecide(CaptureKey::Digit, /*sawDigit=*/false,
                           /*modifierHeld=*/true) == CaptureRoute::Continue,
       "digit while modifier held -> Continue (keep capturing count)");

    // Releasing the modifier commits the accumulated count.
    ok(CaptureLetterDecide(CaptureKey::ModifierRelease, /*sawDigit=*/true,
                           /*modifierHeld=*/false) == CaptureRoute::CommitCount,
       "modifier release with digits typed -> CommitCount");

    // Releasing the modifier with no digits commits the default single folder.
    ok(CaptureLetterDecide(CaptureKey::ModifierRelease, /*sawDigit=*/false,
                           /*modifierHeld=*/false) == CaptureRoute::CommitCount,
       "modifier release, no digits -> CommitCount (single default folder)");

    // Escape cancels: nothing is created.
    ok(CaptureLetterDecide(CaptureKey::Escape, /*sawDigit=*/false,
                           /*modifierHeld=*/true) == CaptureRoute::Cancel,
       "Escape -> Cancel (create nothing)");

    // An UNBOUND letter is not a template -> commit the single default folder.
    ok(CaptureLetterDecide(CaptureKey::UnboundLetter, /*sawDigit=*/false,
                           /*modifierHeld=*/false) == CaptureRoute::CommitCount,
       "unbound letter -> CommitCount (single default folder)");

    if (g_fail == 0) { std::printf("template_after_shift_release_test: PASS\n"); return 0; }
    std::printf("template_after_shift_release_test: FAIL (%d)\n", g_fail);
    return 1;
}
