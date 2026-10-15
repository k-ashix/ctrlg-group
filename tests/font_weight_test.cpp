// font_weight_test.cpp
// Pins fix thse isseue.md S14 (v0.12.54): section titles render in a SEMIBOLD
// weight, distinct from and heavier than the regular body weight. The mod backs
// this with UiFontSemibold() (a FW_SEMIBOLD Segoe UI face, clang-only, pinned by
// a source invariant); this test locks the pure weight policy both sides share
// and the FW_* values the mod static_asserts against. Self-contained: own
// main(), exit 0 == pass.

#include "type_weight_logic.h"

#include <cstdio>

using namespace typeweight;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    check(kRegular == 400,  "regular body weight is FW_NORMAL (400)");
    check(kSemibold == 600, "section-title weight is FW_SEMIBOLD (600)");
    check(kSemibold != kRegular,
          "the title weight is a DIFFERENT handle/weight from the body weight");
    check(SemiboldIsHeavier(),
          "section titles are heavier than the body text (visible hierarchy)");

    if (g_fail == 0) printf("font_weight_test: OK\n");
    else             printf("font_weight_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
