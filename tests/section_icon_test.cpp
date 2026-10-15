// section_icon_test.cpp
// Pins fix thse isseue.md #3: the per-section icon lives in a reserved column
// INSIDE the card, with the section TITLE shifted right past it -- the icon is
// no longer hung off the card's left border. The pure decision is the label
// geometry: Destination / Folder name / Action titles start at innerX +
// kSectionIconCol (the icon column sits at the card inner-left, innerX). The
// Win32 paint (DrawSectionIcon drawing at innerX) is pinned by a source
// invariant; this file pins the layout the paint depends on.
//
// Self-contained: own main(), exit 0 = pass. RED before layout_logic.h gains
// kSectionIconCol + the shifted title rects.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}
static bool contains(const Rect& o, const Rect& r) {
    return r.x >= o.x && r.y >= o.y &&
           r.right() <= o.right() && r.bottom() <= o.bottom();
}

int main() {
    PromptLayout g = ComputePromptLayout(true);
    const int innerX = kOuterMargin + kCardInset;  // card inner-left (32)

    // The icon column leaves room for the 14px section glyph + a gap.
    check(kSectionIconCol >= 16, "icon column leaves room for a 14px icon + gap");

    // Each section TITLE is shifted right past the reserved icon column.
    check(g.destLabel.x == innerX + kSectionIconCol,
          "Destination title shifted right past the icon column");
    check(g.nameLabel.x == innerX + kSectionIconCol,
          "Folder name title shifted right past the icon column");
    check(g.actionTitle.x == innerX + kSectionIconCol,
          "Action title shifted right past the icon column");
    // fix thse isseue.md #1: the Status card + its heading were removed, so
    // there is no Status title / icon column to check here.

    // The icon column sits INSIDE the card content (not on the card border).
    check(innerX > g.destCard.x,
          "icon column is inside the card, not on its left border");
    check(innerX + kSectionIconCol <= g.destLabel.x,
          "icon column is to the LEFT of the title text");

    // Titles still fit inside their cards after the shift.
    check(contains(g.destCard, g.destLabel),  "Destination title still inside its card");
    check(contains(g.nameCard, g.nameLabel),  "Folder name title still inside its card");
    check(contains(g.actionCard, g.actionTitle), "Action title still inside its card");

    if (g_fail) { printf("section_icon_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("section_icon_test: OK\n");
    return 0;
}
