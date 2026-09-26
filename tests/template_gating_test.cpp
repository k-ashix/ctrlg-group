// template_gating_test.cpp
//
// Regression test for the bug: "templates are not working even after being
// applied". The key-capture window (the ONLY place a template letter is caught
// after a trigger) used to open only when "Multiple folders in one go" was ON.
// So with the default settings (multiFolder OFF) a configured template key
// never fired -- no folders were created for ANY letter the user chose.
//
// ShouldOpenCaptureWindow must open the window when EITHER multi-folder is on
// OR at least one template is configured.

#include "newfolder_logic.h"

#include <cstdio>

static int g_failures = 0;

static void expectBool(bool actual, bool expected, const char* what) {
    if (actual != expected) {
        std::printf("  [FAIL] %s: expected %s, got %s\n", what,
                    expected ? "true" : "false", actual ? "true" : "false");
        ++g_failures;
    }
}

int main() {
    // The bug scenario: multi-folder OFF but the user HAS templates.
    // Old behavior returned false here -> template keys never captured.
    expectBool(ShouldOpenCaptureWindow(/*multi=*/false, /*hasTemplates=*/true),
               true, "templates configured, multiFolder OFF -> window opens");

    // Multi-folder ON with no templates still opens (count entry).
    expectBool(ShouldOpenCaptureWindow(/*multi=*/true, /*hasTemplates=*/false),
               true, "multiFolder ON, no templates -> window opens");

    // Both on -> opens.
    expectBool(ShouldOpenCaptureWindow(/*multi=*/true, /*hasTemplates=*/true),
               true, "both on -> window opens");

    // Neither: nothing to capture, go straight to a single folder.
    expectBool(ShouldOpenCaptureWindow(/*multi=*/false, /*hasTemplates=*/false),
               false, "nothing configured -> straight to single folder");

    if (g_failures == 0) {
        std::printf("template_gating_test: all assertions passed\n");
        return 0;
    }
    std::printf("template_gating_test: %d assertion(s) failed\n", g_failures);
    return 1;
}
