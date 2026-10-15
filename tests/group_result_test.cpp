// group_result_test.cpp
// Pins the fix thse isseue.md P1 finding "PerformGroupOperation over-reports
// success" (lines 13565-13620, 14299-14320):
//
//   * PerformGroupOperation returned a bare bool, and the Ctrl+G caller logged
//     the ORIGINAL selection size (sel.size()) on any non-false result -- even
//     when items were dropped by the skip-existing collision policy, could not
//     be turned into shell items, failed to queue, or the batch was aborted
//     mid-flight. The reported count was therefore dishonest on every partial
//     run.
//
// This test pins the pure accounting that the mod must mirror: an outcome is
// summarised into an HONEST grouped count + a Complete / Partial / Failed /
// NothingToDo classification, and the grouped count NEVER exceeds what was
// actually handed to (and completed by) the shell.
//
// Pure: mirrors tests/group_logic.h (groupresult:: model). Self-contained: own
// main(), exit 0 = pass.

#include "group_logic.h"

#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

using groupresult::GroupOpOutcome;
using groupresult::GroupResultKind;
using groupresult::GroupResultSummary;
using groupresult::ClassifyGroupResult;
using groupresult::GroupOutcomeConsistent;

int main() {
    // ---- NothingToDo: nothing survived upstream filtering --------------------
    {
        GroupOpOutcome o;  // all zero
        GroupResultSummary s = ClassifyGroupResult(o);
        check(s.kind == GroupResultKind::NothingToDo, "requested 0 -> NothingToDo");
        check(s.grouped == 0, "NothingToDo grouped == 0");
        check(s.groupedExact, "NothingToDo count is exact");
    }

    // ---- Complete: every requested item grouped, no skip/fail/abort ----------
    {
        GroupOpOutcome o;
        o.requested = 5; o.queued = 5; o.skipped = 0; o.failedToQueue = 0;
        o.performed = true; o.aborted = false;
        GroupResultSummary s = ClassifyGroupResult(o);
        check(s.kind == GroupResultKind::Complete, "all 5 grouped -> Complete");
        check(s.grouped == 5, "Complete grouped == 5 (not sel.size inflation)");
        check(s.groupedExact, "Complete count is exact");
        check(GroupOutcomeConsistent(o), "5 == 5 + 0 + 0 is a consistent outcome");
    }

    // ---- Partial by collision skip: honest grouped < requested ---------------
    {
        GroupOpOutcome o;
        o.requested = 5; o.queued = 3; o.skipped = 2; o.failedToQueue = 0;
        o.performed = true; o.aborted = false;
        GroupResultSummary s = ClassifyGroupResult(o);
        check(s.kind == GroupResultKind::Partial, "2 skipped -> Partial");
        check(s.grouped == 3, "grouped reports 3, NOT the 5 selected");
        check(s.skipped == 2, "summary carries skipped=2 for the log");
        check(s.groupedExact, "non-aborted partial count is still exact");
        check(GroupOutcomeConsistent(o), "5 == 3 + 2 + 0 is consistent");
    }

    // ---- Partial by failed-to-queue: shell-item/Move|Copy call failures ------
    {
        GroupOpOutcome o;
        o.requested = 5; o.queued = 4; o.skipped = 0; o.failedToQueue = 1;
        o.performed = true; o.aborted = false;
        GroupResultSummary s = ClassifyGroupResult(o);
        check(s.kind == GroupResultKind::Partial, "1 failed to queue -> Partial");
        check(s.grouped == 4, "grouped reports 4, NOT 5");
        check(s.failed == 1, "summary carries failed=1 for the log");
        check(GroupOutcomeConsistent(o), "5 == 4 + 0 + 1 is consistent");
    }

    // ---- Failed: the shell batch call itself failed (performed == false) -----
    {
        GroupOpOutcome o;
        o.requested = 5; o.queued = 5; o.skipped = 0; o.failedToQueue = 0;
        o.performed = false; o.aborted = false;
        GroupResultSummary s = ClassifyGroupResult(o);
        check(s.kind == GroupResultKind::Failed, "PerformOperations failed -> Failed");
        check(s.grouped == 0, "nothing landed -> grouped == 0 (never 5)");
        check(s.groupedExact, "a hard failure reports an exact 0");
    }

    // ---- Failed: nothing was queued at all (every item skipped/failed) -------
    {
        GroupOpOutcome o;
        o.requested = 3; o.queued = 0; o.skipped = 3; o.failedToQueue = 0;
        o.performed = false; o.aborted = false;
        GroupResultSummary s = ClassifyGroupResult(o);
        check(s.kind == GroupResultKind::Failed, "0 queued -> Failed (nothing grouped)");
        check(s.grouped == 0, "0 queued -> grouped == 0");
    }

    // ---- Aborted mid-batch: grouped is an UPPER bound, flagged inexact -------
    {
        GroupOpOutcome o;
        o.requested = 5; o.queued = 5; o.skipped = 0; o.failedToQueue = 0;
        o.performed = true; o.aborted = true;
        GroupResultSummary s = ClassifyGroupResult(o);
        check(s.kind == GroupResultKind::Partial, "aborted -> Partial (never Complete)");
        check(s.grouped <= o.queued, "aborted grouped never exceeds queued");
        check(!s.groupedExact, "aborted count is flagged inexact (phrase as 'up to N')");
    }

    // ---- Core honesty invariant: grouped is never more than requested --------
    {
        GroupOpOutcome o;
        o.requested = 2; o.queued = 2; o.skipped = 0; o.failedToQueue = 0;
        o.performed = true; o.aborted = false;
        GroupResultSummary s = ClassifyGroupResult(o);
        check(s.grouped <= s.requested, "grouped never exceeds requested");
    }

    if (g_fail) { printf("group_result: %d check(s) FAILED\n", g_fail); return 1; }
    printf("group_result: all checks passed\n");
    return 0;
}
