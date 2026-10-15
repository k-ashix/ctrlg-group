// chip_nav_test.cpp
// Pins fix thse isseue.md Phase 5 + ADD-ON (Click-to-Find and Cycle Through
// Matching Items). A summary chip is both a FILTER and a find-navigator:
//   - first click on an extension chip reveals the FIRST matching item,
//   - clicking the already-active chip again reveals the NEXT match, wrapping,
//   - the viewport only scrolls as much as needed (adjacent matches do not
//     jump), minimal scroll for widely separated matches,
//   - the current match is a THIRD visual state layered on the Matched rows,
//   - List and grid (Large / Extra large) use the correct row/cell mapping,
//   - empty results never cause an invalid jump,
//   - the current-match cursor is a STABLE ITEM INDEX, so it survives a mode
//     switch (its visual row is recomputed from the new column count).
// Self-contained: own main(), exit 0 = pass. The matching rule is NOT
// duplicated -- it is delegated to chipfilter::RowFor.

#include "chip_nav_logic.h"

#include <cstdio>
#include <vector>
#include <string>

using chipfilter::State;
using chipnav::Item;
using viewmode::Mode;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// Build 500 items where indices 7, 150, 151, 300 and 499 are EXE files, a few
// folders, and the rest a mix of other extensions.
static std::vector<Item> Make500() {
    std::vector<Item> v;
    v.reserve(500);
    for (int i = 0; i < 500; ++i) {
        if (i == 7 || i == 150 || i == 151 || i == 300 || i == 499)
            v.push_back(Item{L"exe", false});
        else if (i % 50 == 0)
            v.push_back(Item{L"", true});          // a folder
        else if (i % 3 == 0)
            v.push_back(Item{L"", false});         // extensionless file
        else
            v.push_back(Item{L"txt", false});
    }
    return v;
}

int main() {
    const std::vector<Item> items = Make500();

    // ---- Matches(): ordered matching indices, order preserved -------------
    State exe;
    chipfilter::ActivateExt(exe, L"exe");
    std::vector<int> m = chipnav::Matches(exe, items);
    check(m.size() == 5, "five EXE among 500 found");
    check(m == (std::vector<int>{7, 150, 151, 300, 499}),
          "match indices preserve original ordering");

    // No active filter / Items chip -> no navigable matches.
    State none;
    check(chipnav::Matches(none, items).empty(), "no filter -> no matches");
    State itemsChip;
    chipfilter::ActivateItems(itemsChip);
    check(chipnav::Matches(itemsChip, items).empty(), "Items chip -> no matches");

    // ---- NextMatch(): first click, cycle, wrap ----------------------------
    // First click (fresh activation, reclick == false) reveals FIRST match.
    int cur = chipnav::NextMatch(m, /*curItem=*/-1, /*reclick=*/false);
    check(cur == 7, "first click reveals EXE match 1 (index 7)");
    // Repeated clicks on the already-active chip cycle through all five.
    cur = chipnav::NextMatch(m, cur, true); check(cur == 150, "click 2 -> match 2");
    cur = chipnav::NextMatch(m, cur, true); check(cur == 151, "click 3 -> match 3");
    cur = chipnav::NextMatch(m, cur, true); check(cur == 300, "click 4 -> match 4");
    cur = chipnav::NextMatch(m, cur, true); check(cur == 499, "click 5 -> match 5");
    cur = chipnav::NextMatch(m, cur, true); check(cur == 7,   "click 6 wraps to match 1");

    // Switching to ANOTHER chip resets the cycle (reclick == false -> first).
    check(chipnav::NextMatch(m, 499, false) == 7, "new chip resets to first match");
    // A stale cursor (not among the matches) also resets to the first match.
    check(chipnav::NextMatch(m, 42, true) == 7, "stale cursor -> first match");

    // Empty results never jump to an arbitrary item.
    std::vector<int> empty;
    check(chipnav::NextMatch(empty, -1, false) == -1, "empty results -> no match (-1)");
    check(chipnav::NextMatch(empty, 7, true) == -1, "empty results stay -1 on reclick");

    // ---- RevealTopRow(): minimal scroll, List mode ------------------------
    const int itemCount = 500;
    // Match 1 (index 7) with a 6-row viewport scrolled near the top:
    // visible [5..10] already contains row 7 -> DO NOT scroll (adjacent).
    check(chipnav::RevealTopRow(Mode::List, 7, itemCount, 1, /*topRow=*/5, 6) == 5,
          "List: already-visible match keeps scroll position");
    // Match far below: index 300, viewport at top -> scroll so 300 is last row.
    check(chipnav::RevealTopRow(Mode::List, 300, itemCount, 1, 0, 6) == 300 - 6 + 1,
          "List: match below viewport scrolls minimally (becomes last row)");
    // Match above the current viewport -> target becomes the top row.
    check(chipnav::RevealTopRow(Mode::List, 7, itemCount, 1, /*topRow=*/100, 6) == 7,
          "List: match above viewport scrolls up to it");
    // itemIndex -1 (empty results) -> unchanged scroll.
    check(chipnav::RevealTopRow(Mode::List, -1, itemCount, 1, 42, 6) == 42,
          "List: no match -> scroll unchanged");

    // ---- RevealTopRow(): grid row/cell mapping ----------------------------
    // Large icons, 4 columns: item 10 is on grid row 2 (10 / 4). Viewport at
    // top showing 3 rows -> row 2 already visible -> no scroll.
    check(viewmode::TopRowForItem(Mode::Large, 10, 4) == 2,
          "grid maps item index -> grid row by actual column count");
    check(chipnav::RevealTopRow(Mode::Large, 10, itemCount, 4, 0, 3) == 0,
          "grid: already-visible cell keeps scroll position");
    // Grid: item 300 at 4 cols is row 75; viewport 3 rows at top -> last row.
    check(chipnav::RevealTopRow(Mode::Large, 300, itemCount, 4, 0, 3) == 75 - 3 + 1,
          "grid: match below viewport scrolls minimally to reveal its cell");

    // ---- Stable cursor survives a mode switch -----------------------------
    // The cursor is item index 300; its visual row differs per mode/columns but
    // the item index is unchanged, so navigation keeps pointing at the file.
    check(viewmode::TopRowForItem(Mode::List, 300, 1) == 300, "list row for item 300");
    check(viewmode::TopRowForItem(Mode::ExtraLarge, 300, 3) == 100, "xl row for item 300 at 3 cols");

    // ---- IsCurrentMatch(): the extra visual state -------------------------
    check(chipnav::IsCurrentMatch(300, 300), "current match flagged");
    check(!chipnav::IsCurrentMatch(151, 300), "other matches not current");
    check(!chipnav::IsCurrentMatch(300, -1), "no cursor -> nothing current");

    if (g_fail) { printf("chip_nav_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("chip_nav_test: OK\n");
    return 0;
}
