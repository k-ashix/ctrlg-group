// layout_sections_test.cpp
// Phase 3a (waht to create.md #3/#5/#9): the popup layout must give EVERY
// section its own rectangle instead of cramming Selected Items + Destination +
// last-target into one STATIC text blob. This pins the sectioned model added to
// layout_logic.h (mirrored into the mod): a Selected Items card (header + a
// scrollable list viewport), a Destination card (label + read-only field), a
// Folder name card (label + field), a Status card (icon + label + progress
// bar) and a Copy/Move segmented control on the action row.
//
// The point of this file is the FIX described in "fix thse isseu e.md": the
// layout system now HAS somewhere to put each component. Assertions are mostly
// structural (containment / ordering / no-overlap / within-window) so they stay
// faithful without being brittle about every pixel.
//
// Self-contained: own main(), exit 0 = pass. RED before layout_logic.h grows
// the section rects; GREEN once it mirrors the mod.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// r wholly inside outer (allowing touching edges).
static bool contains(const Rect& outer, const Rect& r) {
    return r.x >= outer.x && r.y >= outer.y &&
           r.right() <= outer.right() && r.bottom() <= outer.bottom();
}
// a sits strictly above b (no vertical overlap).
static bool above(const Rect& a, const Rect& b) { return a.bottom() <= b.y; }

int main() {
    // ---- New Folder is the SMALLER subset: no group-only section cards ----
    PromptLayout nf = ComputePromptLayout(false);
    check(nf.selectedCard.empty(),   "nf has no Selected Items card");
    check(nf.selectedHeader.empty(), "nf has no Selected Items header");
    check(nf.selectedList.empty(),   "nf has no Selected Items list");
    check(nf.destCard.empty(),       "nf has no Destination card");
    check(nf.destField.empty(),      "nf has no Destination field");
    // New Folder still keeps its own name label/field + buttons (unchanged).
    check(!nf.nameField.empty() && !nf.createButton.empty(),
          "nf still has a name field + Create button");

    // ---- Group: every section has a real rectangle ----
    PromptLayout g = ComputePromptLayout(true);
    check(!g.selectedCard.empty(),   "g Selected Items card exists");
    check(!g.selectedHeader.empty(), "g Selected Items header exists");
    check(!g.selectedList.empty(),   "g Selected Items list viewport exists");
    check(!g.destCard.empty(),       "g Destination card exists");
    check(!g.destLabel.empty(),      "g Destination label exists");
    check(!g.destField.empty(),      "g Destination field exists");
    check(!g.nameCard.empty(),       "g Folder name card exists");
    // fix thse isseue.md #1: the Status card / progress bar were removed from
    // the PromptLayout model entirely -- there are no statusCard/progressBar
    // fields left to probe.
    check(!g.segCopy.empty() && !g.segMove.empty(), "g Copy/Move segments exist");

    // groupHeader stays an alias of the Selected Items card (mod wiring reuses it).
    check(g.groupHeader.x == g.selectedCard.x && g.groupHeader.y == g.selectedCard.y &&
          g.groupHeader.w == g.selectedCard.w && g.groupHeader.h == g.selectedCard.h,
          "groupHeader aliases the Selected Items card");

    // ---- containment: controls live INSIDE their card ----
    check(contains(g.selectedCard, g.selectedHeader), "header inside Selected card");
    check(contains(g.selectedCard, g.selectedList),   "list inside Selected card");
    check(above(g.selectedHeader, g.selectedList),    "header above the list");
    check(contains(g.destCard, g.destLabel), "dest label inside dest card");
    check(contains(g.destCard, g.destField), "dest field inside dest card");
    // fix thse isseu e.md #1: each info card stacks title -> description -> field.
    // The description row is its OWN rect BETWEEN the label and the field (it was
    // previously painted UNDER the field, which cramped/clipped it).
    check(!g.destDesc.empty(), "dest description row exists");
    check(contains(g.destCard, g.destDesc), "dest description inside dest card");
    check(above(g.destLabel, g.destDesc), "dest label above description");
    check(above(g.destDesc, g.destField), "dest description above field");
    check(!g.nameDesc.empty(), "name description row exists");
    check(contains(g.nameCard, g.nameDesc), "name description inside name card");
    check(above(g.nameLabel, g.nameDesc), "name label above description");
    check(above(g.nameDesc, g.nameField), "name description above field");
    check(above(g.destLabel, g.destField),   "dest label above dest field");
    check(contains(g.nameCard, g.nameLabel), "name label inside name card");
    check(contains(g.nameCard, g.nameField), "name field inside name card");
    check(above(g.nameLabel, g.nameField),   "name label above name field");
    // fix thse isseue.md #1: the Status card (icon/label/progress/heading) was
    // removed, so there is nothing to contain here.

    // ---- cards stack top -> bottom with no overlap ----
    check(above(g.selectedCard, g.destCard), "Selected card above Destination card");
    check(above(g.destCard, g.nameCard),     "Destination card above Folder name card");
    // fix thse isseue.md #1: the Status card was removed, so the Folder name
    // card is directly above the Action card.
    check(above(g.nameCard, g.actionCard),   "Folder name card above the Action card");
    check(above(g.actionCard, g.createButton),
          "Action card above the Create/Cancel bottom row");

    // ---- Action card: title -> description -> segments -> keyboard hint ----
    check(!g.actionCard.empty(),  "Action card exists");
    check(!g.actionTitle.empty(), "Action title exists");
    check(!g.actionDesc.empty(),  "Action description exists");
    check(!g.keyboardHint.empty(),"keyboard hint exists");
    check(contains(g.actionCard, g.actionTitle), "action title inside Action card");
    check(contains(g.actionCard, g.actionDesc),  "action description inside Action card");
    check(contains(g.actionCard, g.segCopy),     "Copy segment inside Action card");
    check(contains(g.actionCard, g.segMove),     "Move segment inside Action card");
    check(contains(g.actionCard, g.keyboardHint),"keyboard hint inside Action card");
    check(above(g.actionTitle, g.actionDesc), "action title above description");
    check(above(g.actionDesc, g.segCopy),     "action description above the segments");
    check(above(g.segCopy, g.keyboardHint),   "segments above the keyboard hint");

    // ---- segmented Copy/Move share one baseline, inside the card ----
    check(g.segCopy.right() <= g.segMove.x, "Copy segment left of Move segment");
    check(g.segCopy.y == g.segMove.y, "Copy/Move segments share one baseline");
    // ---- Create/Cancel are on their OWN bottom row, BELOW the segments ----
    check(g.segMove.bottom() <= g.createButton.y, "segments sit above Create/Cancel");
    check(g.createButton.y == g.cancelButton.y, "Create/Cancel share the bottom row");
    check(g.createButton.right() <= g.cancelButton.x, "Create left of Cancel");
    check(g.cancelButton.right() == g.windowW - kOuterMargin,
          "Cancel right-aligned to the margin");

    // ---- everything is inside the window box ----
    const Rect window{0, 0, g.windowW, g.windowH};
    check(contains(window, g.selectedCard), "Selected card within window");
    check(contains(window, g.destCard),     "Destination card within window");
    check(contains(window, g.nameCard),     "Folder name card within window");
    check(contains(window, g.actionCard),   "Action card within window");
    check(g.cancelButton.bottom() <= g.windowH, "action row within window");

    // ---- expanded gives the LIST more room; everything below shifts down ----
    PromptLayout gx = ComputePromptLayout(true, true);
    // fix thse isseue.md #1: the Selected Items list is content-sized, so the
    // expanded gain is (expanded rows - compact rows) * row height for a full
    // selection -- not a fixed band.
    const int extra = (kListExpandedRows - kListCompactRows) * kSelRowH;
    check(gx.selectedList.h == g.selectedList.h + extra, "expanded list grows by extra");
    check(gx.selectedHeader.y == g.selectedHeader.y, "header stays put when expanded");
    check(gx.destCard.y == g.destCard.y + extra,   "Destination card shifts down");
    check(gx.nameCard.y == g.nameCard.y + extra,   "Folder name card shifts down");
    check(gx.actionCard.y == g.actionCard.y + extra, "Action card shifts down");
    check(gx.createButton.y == g.createButton.y + extra, "action row shifts down");
    check(gx.windowH == g.windowH + extra, "expanded window grows by extra");
    // still ordered + contained after expansion
    check(above(gx.selectedCard, gx.destCard), "expanded: Selected above Destination");
    check(above(gx.actionCard, gx.createButton), "expanded: Action above action row");
    check(contains(gx.selectedCard, gx.selectedList), "expanded: list still inside card");

    // ---- DPI scaling preserves the section relationships ----
    Rect sc = ScaleRect(g.selectedCard, 192);
    Rect dc = ScaleRect(g.destCard, 192);
    check(sc.bottom() <= dc.y, "scaled Selected card still above Destination");

    // ---- fix thse isseu e.md #5: size control lives in the window CHROME ----
    // strip ABOVE the first card (not inside the Selected Items card), and is
    // right-aligned to the window edge.
    check(!g.chromeSizeBtn.empty(), "group has a chrome size button");
    check(nf.chromeSizeBtn.empty(), "New Folder has no chrome size button");
    check(above(g.chromeSizeBtn, g.selectedCard),
          "size button sits ABOVE the Selected Items card (in the chrome)");
    check(!contains(g.selectedCard, g.chromeSizeBtn),
          "size button is NOT inside the Selected Items card");
    // fix thse isseue.md #5: three title-bar controls, right-aligned in the
    // Windows caption order Minimize | Expand/Restore | Close (Close rightmost).
    check(!g.chromeMinBtn.empty() && !g.chromeCloseBtn.empty(),
          "group has Minimize + Close title-bar controls");
    check(nf.chromeMinBtn.empty() && nf.chromeCloseBtn.empty(),
          "New Folder has no title-bar controls");
    check(g.chromeMinBtn.right() <= g.chromeSizeBtn.x &&
          g.chromeSizeBtn.right() <= g.chromeCloseBtn.x,
          "title-bar order is Minimize | Expand/Restore | Close (left to right)");
    // fix thse isseue.md #6: caption controls are a comfortable hit target
    // (>=28px square) in a 32-36px band; the drawn glyph stays small (paint).
    check(g.chromeCloseBtn.w >= 28 && g.chromeCloseBtn.h >= 28 &&
          g.chromeMinBtn.w >= 28 && g.chromeSizeBtn.w >= 28,
          "title-bar controls are a >=28px hit target (#6)");
    check(TitleBandH() >= 32 && TitleBandH() <= 36, "title band is 32-36px (#6)");
    // fix thse isseue.md: caption controls span the FULL header height (top y==0,
    // bottom == band) so there is no dead strip above/below that swallows a
    // click, and the click area is WIDER than the old 28px square (native ~45px).
    check(g.chromeMinBtn.y == 0 && g.chromeSizeBtn.y == 0 && g.chromeCloseBtn.y == 0,
          "caption controls start at the band top (y==0, no dead strip)");
    check(g.chromeMinBtn.h == TitleBandH() && g.chromeSizeBtn.h == TitleBandH() &&
          g.chromeCloseBtn.h == TitleBandH(),
          "caption controls span the FULL header height (bottom == band)");
    check(g.chromeMinBtn.w >= 40 && g.chromeSizeBtn.w >= 40 && g.chromeCloseBtn.w >= 40,
          "caption click area is WIDER than the old 28px square (native ~45px)");
    check(g.chromeCloseBtn.right() == g.windowW - kOuterMargin,
          "Close button right-aligned to the window edge");
    check(above(g.chromeMinBtn, g.selectedCard) &&
          above(g.chromeCloseBtn, g.selectedCard),
          "Minimize + Close sit in the title bar above the first card");
    check(contains(window, g.chromeSizeBtn) && contains(window, g.chromeMinBtn) &&
          contains(window, g.chromeCloseBtn),
          "title-bar controls within the window");

    if (g_fail) { printf("layout_sections_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("layout_sections_test: OK\n");
    return 0;
}
