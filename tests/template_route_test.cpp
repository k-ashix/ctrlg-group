// template_route_test.cpp
// Issue #1: a template letter fires ONLY when it is bound to a template.
// An unbound letter must fall through to normal single-folder behaviour, so
// the hook needs a lock-free way to ask "is this letter a template key?"
// without touching the g_templates vector. That decision lives in
// BuildTemplateKeyMask + IsTemplateKeyBound (pure, mirrored into the mod).
// Self-contained: own main(), exit 0 = pass.

#include "capture_logic.h"

#include <cstdio>
#include <string>
#include <vector>

static int g_fail = 0;
static void ok(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    std::vector<FolderTemplate> raw;
    { FolderTemplate t; t.key = L"p"; t.name = L"Photos"; t.folders = { L"raw", L"jpg" }; raw.push_back(t); }
    { FolderTemplate t; t.key = L"w"; t.name = L"Work";   t.folders = { L"src" };          raw.push_back(t); }

    TemplateResolution res = ResolveTemplateKeys(raw);
    unsigned long mask = BuildTemplateKeyMask(res.usable);

    ok(IsTemplateKeyBound(mask, L'p'), "bound letter 'p' -> template");
    ok(IsTemplateKeyBound(mask, L'P'), "bound letter 'P' -> template (case-insensitive)");
    ok(IsTemplateKeyBound(mask, L'w'), "bound letter 'w' -> template");

    ok(!IsTemplateKeyBound(mask, L'x'), "unbound letter 'x' -> NOT a template (single folder)");
    ok(!IsTemplateKeyBound(mask, L'3'), "digit '3' -> not a template key");
    ok(!IsTemplateKeyBound(mask, L'!'), "symbol '!' -> not a template key");
    ok(!IsTemplateKeyBound(0UL, L'p'),  "empty mask -> nothing is bound");

    if (g_fail == 0) { printf("template_route_test: PASS\n"); return 0; }
    printf("template_route_test: FAIL (%d)\n", g_fail);
    return 1;
}
