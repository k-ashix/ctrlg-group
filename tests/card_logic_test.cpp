// card_logic_test.cpp
// Pins the PURE section-card model (Phase 4/B, fix thse isseu e.md #2): WHICH
// Group sections render as their own rounded cards, in what order, and the
// uniform corner radius. The mod's WM_ERASEBKGND iterates card::GroupCards()
// and paints each rect with DrawSectionCard(); if a card is dropped or
// reordered here (and mirrored), this test catches it.
//
// Self-contained: own main(), exit 0 = pass. RED before card_logic.h exists.

#include "card_logic.h"

#include <cstdio>

using namespace card;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    auto cards = GroupCards();

    // ---- four card surfaces, top -> bottom, in the reference's order ----
    check(cards.size() == 4, "four section cards");
    check(kCount == 4, "kCount == 4");
    check(cards[0] == Id::Selected,    "card 0 = Selected Items");
    check(cards[1] == Id::Destination, "card 1 = Destination");
    check(cards[2] == Id::Name,        "card 2 = Folder name");
    check(cards[3] == Id::Status,      "card 3 = Status");

    // ---- a real, positive uniform corner radius (not a square panel) ----
    check(kCardRadius > 0, "card radius is positive");

    if (g_fail) { printf("card_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("card_logic_test: OK\n");
    return 0;
}
