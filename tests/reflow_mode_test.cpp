// reflow_mode_test.cpp
// Pins fix thse isseue.md Phase 2 (mode-aware geometry on manual resize /
// maximize). layout::ReflowToClient must NOT assume List-mode 24px rows when
// the Selected Items view is a GRID (Large / Extra large):
//   - the shrink FLOOR reserves one WHOLE cell (CellH), never a 24px list row,
//     so a grid cell / filename is never clipped by the resize path;
//   - grid "row-capped" (overflow) is derived from the grid cell height, so a
//     grown window pours the extra height into the card ONLY when cells are
//     actually hidden;
//   - a grid selection whose cells ALL fit stays content-sized (no dead space);
//   - List mode (the default args) is completely unchanged.
// Self-contained: own main(), exit 0 = pass.

#include "layout_logic.h"

#include <cstdio>

using namespace layout;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const int MINW = 360, MINH = 320;

    // ======== 1. Shrink FLOOR is one WHOLE cell in grid mode ========
    PromptLayout g = ComputePromptLayout(true, false, 6);
    const int headerBlockH = g.selectedList.y - g.selectedContent.y;
    const int cellH = 86;                 // a Large-ish whole cell (icon+pad+label band)

    // Aggressive shrink forces the Selected card down to its floor.
    PromptLayout rGridFloor =
        ReflowToClient(g, g.windowW, MINH, MINW, MINH, cellH, /*selCapRows=*/100);
    check(rGridFloor.selectedContent.h == headerBlockH + cellH,
          "grid reflow floor reserves exactly one WHOLE cell (never 24px)");

    PromptLayout rListFloor = ReflowToClient(g, g.windowW, MINH, MINW, MINH);
    check(rListFloor.selectedContent.h == headerBlockH + kSelRowH,
          "list reflow floor reserves one 24px list row (default unchanged)");

    check(rGridFloor.selectedContent.h > rListFloor.selectedContent.h,
          "grid floor is taller than the list floor -> cells are not clipped to 24px");

    // ======== 2. Grid overflow absorbs extra height; all-fit does not ========
    // Hand-built base: a grid card showing exactly 2 whole cells of height cellH.
    PromptLayout base{};
    base.isGroup = true;
    base.expanded = false;
    const int gHeaderH = 60;
    base.selectedContent = Rect{20, 50, 300, gHeaderH + 2 * cellH};
    base.selectedList    = Rect{20, 50 + gHeaderH, 300, 2 * cellH};
    base.windowW = 360;
    base.windowH = base.selectedList.bottom() + 200;   // footer below the list
    const int GROW = 240;

    // selCapRows == 2 and 2 visible cells -> row-capped (overflow): grow the card.
    PromptLayout rCap =
        ReflowToClient(base, base.windowW, base.windowH + GROW, MINW, MINH, cellH, 2);
    check(rCap.selectedList.h == base.selectedList.h + GROW,
          "grid overflow: Selected card absorbs the extra height (more cells + scroll)");

    // selCapRows == 5 and only 2 visible cells -> NOT capped: keep content height.
    PromptLayout rFit =
        ReflowToClient(base, base.windowW, base.windowH + GROW, MINW, MINH, cellH, 5);
    check(rFit.selectedList.h == base.selectedList.h,
          "grid all-fit: no dead space, the card stays content-sized");

    // ======== 3. Grid shrink floor from the hand base reserves a whole cell ====
    PromptLayout rCapFloor =
        ReflowToClient(base, base.windowW, MINH, MINW, MINH, cellH, 2);
    check(rCapFloor.selectedContent.h >= gHeaderH + cellH,
          "grid shrink never clips below one whole cell");

    if (g_fail) { printf("reflow_mode_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("reflow_mode_test: OK\n");
    return 0;
}
