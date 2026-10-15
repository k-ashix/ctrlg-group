// template_letter_gating_test.cpp
//
// Two guarantees for template letters (requested after the Ctrl+Shift+N fix):
//
//   (a) NORMALIZE TO LOWERCASE even with Shift held. A letter VK is Shift-
//       independent, so typing Shift+I (uppercase) must resolve the template
//       bound to lowercase 'i'.
//
//   (b) FIRE ONLY INSIDE THE CAPTURE WINDOW. When the capture window is NOT
//       open (normal typing anywhere -- e.g. editing code), the same letters
//       must NEVER fire a template. No accidental folder creation.
//
// Exercises the pure ResolveTemplateLetterKey / NormalizeTemplateLetter helpers
// mirrored into the mod. Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <cstdio>

static int g_fail = 0;
static void ok(bool cond, const char* what) {
    if (!cond) { std::printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // Templates bound to 'i' and 'p'. Mask bit (c-'a') set for each.
    const unsigned long mask = (1UL << ('i' - 'a')) | (1UL << ('p' - 'a'));

    // --- (a) LOWERCASE NORMALIZATION, Shift-independent ---------------------
    // The VK for 'i' and Shift+'i' is identical (0x49 == 'I'); folding it must
    // give lowercase 'i' either way.
    ok(NormalizeTemplateLetter('I') == L'i',
       "VK 'I' normalizes to lowercase 'i' (Shift held or not)");
    ok(NormalizeTemplateLetter('P') == L'p',
       "VK 'P' normalizes to lowercase 'p'");
    ok(NormalizeTemplateLetter('3') == 0,
       "digit VK is not a letter -> 0");
    ok(NormalizeTemplateLetter(0x0D) == 0,
       "Enter VK (0x0D) is not a letter -> 0");

    // A keyboard VK code for a letter is ALWAYS the uppercase code (VK 'I' ==
    // 0x49, VK 'P' == 0x50) whether or not Shift is held -- Shift never changes
    // the VK, only the produced character. So the hook always sees 'I'/'P' and
    // must fold to lowercase itself. Passing the uppercase VK models both the
    // Shift-held and Shift-released cases.
    ok(ResolveTemplateLetterKey(/*captureActive=*/true, 'I', mask) == L'i',
       "capture active + VK 'I' (Shift held or not) -> fires lowercase 'i' template");
    ok(ResolveTemplateLetterKey(/*captureActive=*/true, 'P', mask) == L'p',
       "capture active + VK 'P' -> fires 'p' template");

    // --- (b) FIRES ONLY INSIDE THE CAPTURE WINDOW ---------------------------
    // Normal typing: capture window closed. The very same bound letters must
    // NOT fire -- returns 0 (no template action).
    ok(ResolveTemplateLetterKey(/*captureActive=*/false, 'I', mask) == 0,
       "capture INACTIVE + VK 'I' -> no template (normal typing is safe)");
    ok(ResolveTemplateLetterKey(/*captureActive=*/false, 'P', mask) == 0,
       "capture INACTIVE + VK 'P' -> no template");

    // --- unbound letters never fire, even inside the window -----------------
    ok(ResolveTemplateLetterKey(/*captureActive=*/true, 'X', mask) == 0,
       "capture active + unbound VK 'X' -> no template");
    ok(ResolveTemplateLetterKey(/*captureActive=*/true, '5', mask) == 0,
       "capture active + digit '5' -> no template (it is a count digit)");

    if (g_fail == 0) { std::printf("template_letter_gating_test: PASS\n"); return 0; }
    std::printf("template_letter_gating_test: FAIL (%d)\n", g_fail);
    return 1;
}
