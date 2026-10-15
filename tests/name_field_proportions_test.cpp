// name_field_proportions_test.cpp
// Pins fix thse isseue.md #7 (UI polish pass 6): the Folder name field should be
// the STRONGEST input and visually dominate the form -- its height moves 30 ->
// 32 (group popup only). New Folder keeps its original 30px field. The accent-
// on-focus border + placeholder already exist (namefield_logic); this slice is
// the proportions bump. Self-contained: own main(). RED while kNameFieldH is
// absent (compile error).

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- #7: a dedicated, taller name-field height (group popup) ----
    check(kNameFieldH == 32, "group name field height is 32 (fix #7)");
    check(kNameFieldH > kFieldH, "name field is taller than the standard 30px field (fix #7 dominance)");
    check(kNameFieldH % 4 == 0, "name field height stays on the 4px grid");

    PromptLayout g = ComputePromptLayout(true);
    check(g.nameField.h == kNameFieldH, "group name field uses the taller height");
    check(g.nameField.h > g.destField.h, "name field dominates the read-only Destination field");

    // ---- New Folder keeps its original 30px field (unchanged) ----
    PromptLayout nf = ComputePromptLayout(false);
    check(nf.nameField.h == kFieldH, "New Folder field height unchanged (30)");

    if (g_fail) { printf("name_field_proportions_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("name_field_proportions_test: OK\n");
    return 0;
}
