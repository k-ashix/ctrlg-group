// field_radius_test.cpp
// Pins fix thse isseue.md #2/#6/#7 radius polish (UI polish pass 6, PASS 3):
//   #2 the EXT badge pill corner grows 4 -> 5-6 so it reads as a soft chip;
//   #6 the read-only Destination field gets an 8px rounded border;
//   #7 the Folder name field (the strongest input) gets a 7-8px rounded border.
// These are the SAME radius tokens the mod feeds to RoundRect; the GDI paint is
// pinned by source invariants. Self-contained: own main().
// RED while listbadge::kRadius is still 4 and the field kRadius tokens are absent.

#include "list_badge_logic.h"
#include "destfield_logic.h"
#include "namefield_logic.h"

#include <cstdio>

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- #2: the EXT badge pill is a soft 5-6px chip (was 4) ----
    check(listbadge::kRadius >= 5 && listbadge::kRadius <= 6,
          "EXT badge corner radius is 5-6 (fix #2)");
    // padding already satisfies #2 (8 / 3) -- re-pin so a regression is caught
    check(listbadge::kPadX == 8, "badge horizontal padding stays 8 (fix #2)");
    check(listbadge::kPadY == 3, "badge vertical padding stays 3 (fix #2)");

    // ---- #6: the Destination field has an 8px rounded border ----
    check(destfield::kRadius == 8, "Destination field corner radius is 8 (fix #6)");

    // ---- #7: the Folder name field has a 7-8px rounded border ----
    check(namefield::kRadius >= 7 && namefield::kRadius <= 8,
          "Folder name field corner radius is 7-8 (fix #7)");

    // ---- the input fields are MORE rounded than the small metadata pill ----
    check(destfield::kRadius > listbadge::kRadius,
          "fields are more rounded than the EXT pill");
    check(namefield::kRadius >= destfield::kRadius - 1,
          "name + destination fields share a consistent field radius");

    if (g_fail) { printf("field_radius_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("field_radius_test: OK\n");
    return 0;
}
