// selected_padding_test.cpp
//
// fix thse isseue.md #3: the Selected Items card gets a separate CONTENT
// rectangle (selectedContent) inset by kSelectedPad (16px) on ALL FOUR sides.
// The owner-drawn list (groupInfo in the mod) lives inside selectedContent, so
// the parent-painted rounded card background + border stay visible and the
// content has consistent padding on every side. The card height is DERIVED from
// the inset content + the bottom padding, so a small selection leaves no empty
// space.
//
// Mirrors layout_logic.h (copied byte-for-byte into the mod). Self-contained:
// own main(), exit 0 == pass. RED before selectedContent exists; GREEN once the
// layout model adds it.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}
static bool contains(const Rect& outer, const Rect& r) {
    return r.x >= outer.x && r.y >= outer.y &&
           r.right() <= outer.right() && r.bottom() <= outer.bottom();
}

static void checkCard(const Rect& card, const Rect& content, const char* tag) {
    char buf[128];
    auto msg = [&](const char* s){ snprintf(buf, sizeof(buf), "%s: %s", tag, s); return buf; };
    // consistent kSelectedPad (20px, fix thse isseue.md #3) padding on ALL four sides
    check(content.x == card.x + kSelectedPad,            msg("content inset by kSelectedPad on the left"));
    check(content.y == card.y + kSelectedPad,            msg("content inset by kSelectedPad on the top"));
    check(content.right() == card.right() - kSelectedPad, msg("content inset by kSelectedPad on the right"));
    check(content.bottom() == card.bottom() - kSelectedPad, msg("content inset by kSelectedPad on the bottom"));
    check(contains(card, content),                        msg("content inside card"));
    // card height is derived from inset content + bottom padding (no dead space)
    check(card.h == content.h + kSelectedPad * 2,         msg("card height = content + 2*pad"));
}

int main() {
    check(kSelectedPad == 20, "Selected card padding is 20px @96dpi (fix thse isseue.md #3, was 16)");

    // Group (full default selection).
    PromptLayout g = ComputePromptLayout(true);
    check(!g.selectedContent.empty(), "g Selected content rect exists");
    checkCard(g.selectedCard, g.selectedContent, "group");
    // header + rows live INSIDE the padded content rect
    check(contains(g.selectedContent, g.selectedHeader), "header inside selectedContent");
    check(contains(g.selectedContent, g.selectedList),   "list inside selectedContent");
    // the rows reach the bottom of the content -- no unnecessary empty space
    check(g.selectedList.bottom() == g.selectedContent.bottom(),
          "rows fill the content to the bottom (no empty space)");

    // Small selection (1 item): still 16px padding, card tightly fits one row.
    PromptLayout g1 = ComputePromptLayout(true, false, 1);
    check(!g1.selectedContent.empty(), "1-item Selected content exists");
    checkCard(g1.selectedCard, g1.selectedContent, "group-1item");
    check(g1.selectedCard.h < g.selectedCard.h,
          "a 1-item card is shorter than a full card (content-sized)");

    // Expanded keeps the same 16px padding model.
    PromptLayout gx = ComputePromptLayout(true, true);
    checkCard(gx.selectedCard, gx.selectedContent, "group-expanded");

    // New Folder has no Selected card / content.
    PromptLayout nf = ComputePromptLayout(false);
    check(nf.selectedContent.empty(), "New Folder has no Selected content");

    if (g_fail) { printf("selected_padding_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("selected_padding_test: OK\n");
    return 0;
}
