// template_log_summary_test.cpp
//
// Regression for the confusing log: pressing a template key a SECOND time
// logged "created 0 of 0 folder(s)" with no hint the folders were skipped
// because they already exist. SummarizeTemplateRun must expose the
// skipped-as-existing count (and failed count) so the log can say so clearly.

#include "newfolder_logic.h"

#include <cstdio>

static int g_fail = 0;

static void expectInt(int got, int want, const char* what) {
    if (got != want) {
        std::printf("  [FAIL] %s: expected %d, got %d\n", what, want, got);
        ++g_fail;
    }
}

int main() {
    // First press: template has 3 names, none exist -> all planned, all created.
    {
        auto s = SummarizeTemplateRun(/*requested=*/3, /*planned=*/3, /*created=*/3);
        expectInt(s.created, 3, "fresh run: created");
        expectInt(s.skippedExisting, 0, "fresh run: none skipped");
        expectInt(s.failed, 0, "fresh run: none failed");
    }

    // Second press (the bug): all 3 already exist, Skip drops them all.
    // Old log said "0 of 0"; now we can report skippedExisting=3.
    {
        auto s = SummarizeTemplateRun(/*requested=*/3, /*planned=*/0, /*created=*/0);
        expectInt(s.created, 0, "all-exist run: created");
        expectInt(s.skippedExisting, 3, "all-exist run: 3 skipped as existing");
        expectInt(s.failed, 0, "all-exist run: none failed");
    }

    // Mixed: 3 requested, 1 already exists (planned 2), 1 of those fails to create.
    {
        auto s = SummarizeTemplateRun(/*requested=*/3, /*planned=*/2, /*created=*/1);
        expectInt(s.skippedExisting, 1, "mixed run: 1 skipped as existing");
        expectInt(s.created, 1, "mixed run: 1 created");
        expectInt(s.failed, 1, "mixed run: 1 failed");
    }

    if (g_fail == 0) {
        std::printf("template_log_summary_test: all assertions passed\n");
        return 0;
    }
    std::printf("template_log_summary_test: %d assertion(s) failed\n", g_fail);
    return 1;
}
