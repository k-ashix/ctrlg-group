// template_retry_reserved_test.cpp
// REPRODUCTION TEST for wh(8) issue #1:
//   "Template retry can collide with another planned template folder."
//
// Claim (verified against code):
//   ResolveTemplateFolders() picks each plan's path with a `taken()` closure
//   that consults BOTH the disk AND the set of paths already reserved by THIS
//   action (the `planned` set). Good.
//
//   BUT the create-time race retry, NextFreeTemplateSibling(), consults ONLY
//   the filesystem (`exists`). It has no knowledge of the OTHER
//   TemplateFolderPlan.path values already reserved by the same action. So when
//   a plan loses the check-then-create race and retries, the retry can hand
//   back a name that a LATER planned item is going to create -- forcing that
//   second item to renumber and destroying the intended template set.
//
// Scenario from the report:
//   Template: [raw, "raw 3"], "raw" already exists on disk.
//   Plan:  raw   -> "raw 2"
//          "raw 3" -> "raw 3"
//   Another process creates "raw 2" before creation.
//   Retry for the first plan (base "raw") scans the DISK, sees "raw 3" is free
//   (it isn't yet -- it's reserved by the second plan) and takes it.
//   -> collides with the reserved "raw 3" plan.
//
// The retry MUST avoid every other TemplateFolderPlan.path already reserved by
// this action. This test encodes that requirement and therefore FAILS against
// the current single-arg NextFreeTemplateSibling(dir, base, pattern, exists).
//
// Self-contained: own main(), exit 0 == pass.

#include "capture_logic.h"

#include <set>
#include <string>
#include <vector>
#include <cstdio>

static int g_fail = 0;

static void expectTrue(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const std::wstring dir = L"C:\\x";

    // --- Build the plan exactly as the report describes -------------------
    // Template folders: "raw" and "raw 3". "raw" already exists on disk, so
    // Number mode resolves "raw" -> "raw 2" and keeps "raw 3" -> "raw 3".
    std::set<std::wstring> disk = { L"C:\\x\\raw" };
    auto exists = [&](const std::wstring& p) { return disk.count(p) > 0; };

    ResolvedTemplate tmpl;
    tmpl.key = L'r';
    tmpl.name = L"Raw";
    tmpl.folders = { L"raw", L"raw 3" };

    auto plans = ResolveTemplateFolders(
        dir, tmpl, exists, TemplateCollision::Number, NamePattern::Space);

    expectTrue(plans.size() == 2, "plan has two entries");
    expectTrue(plans.size() == 2 && plans[0].base == L"raw",
               "plan[0] base is 'raw'");
    expectTrue(plans.size() == 2 && plans[0].path == L"C:\\x\\raw 2",
               "plan[0] path is 'raw 2' (numbered past existing 'raw')");
    expectTrue(plans.size() == 2 && plans[1].path == L"C:\\x\\raw 3",
               "plan[1] path is the reserved 'raw 3'");

    // --- Simulate the create-time race ------------------------------------
    // Another process creates "raw 2" after the plan but before creation.
    // The first plan (base "raw") now loses the race and must retry.
    disk.insert(L"C:\\x\\raw 2");

    // The mod's create loop passes the OTHER reserved plan paths to the retry
    // (every TemplateFolderPlan.path this action still owns, except the one
    // being retried). Build that set the same way.
    std::set<std::wstring> otherReserved;
    for (const auto& pl : plans) {
        if (ToLowerFold(pl.path) != ToLowerFold(plans[0].path)) {
            otherReserved.insert(ToLowerFold(pl.path));
        }
    }

    // FIXED call site: the retry now sees the disk AND the reserved plans.
    std::wstring retry =
        NextFreeTemplateSibling(dir, plans[0].base, NamePattern::Space, exists,
                                otherReserved);

    // The retry must NOT return "raw 3": that path is reserved by plans[1].
    // It numbers past both "raw 2" (disk) and the reserved "raw 3" -> "raw 4",
    // preserving the intended template set instead of collapsing to
    // "raw 3" / "raw 3 2".
    expectTrue(retry != L"C:\\x\\raw 3",
               "retry must not collide with the reserved 'raw 3' plan");
    expectTrue(retry == L"C:\\x\\raw 4",
               "retry skips disk 'raw 2' AND reserved 'raw 3' -> 'raw 4'");

    if (g_fail) {
        printf("template_retry_reserved_test: %d FAIL\n", g_fail);
        return 1;
    }
    printf("template_retry_reserved_test: PASS\n");
    return 0;
}
