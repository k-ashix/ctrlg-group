// list_badge_logic_test.cpp
// Pins fix thse isseue.md #5: the Selected Items extension column is a visual
// BADGE (a rounded pill behind the uppercased extension) rather than plain
// text, so each row reads as [icon] name [EXT] size. The pure pill geometry +
// the show/hide decision live in list_badge_logic.h, mirrored from the mod; the
// owner-draw paint itself is pinned by source invariants.
//
// Self-contained: own main(), exit 0 = pass. RED before list_badge_logic.h
// exists; GREEN once it mirrors the mod.

#include "list_badge_logic.h"

#include <cstdio>
#include <string>

using namespace listbadge;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- show/hide: only files WITH an extension get a pill ----
    // (sel::ExtLabel is "" for folders, extensionless, hidden, trailing-dot.)
    check(ShowBadge(L"PDF"), "a file with an extension shows a badge");
    check(ShowBadge(L"JS"), "a short extension still shows a badge");
    check(!ShowBadge(L""), "folders / extensionless rows show no badge");

    // ---- pill width: text + 2*padX, clamped up to a minimum ----
    check(BadgeWidth(4, kPadX, kMinW) == kMinW,
          "a tiny extension clamps to the minimum pill width");
    check(BadgeWidth(40, kPadX, kMinW) == 40 + 2 * kPadX,
          "a wide extension is text width + 2*padX");
    check(BadgeWidth(40, kPadX, kMinW) > kMinW,
          "a wide extension exceeds the minimum");

    // ---- a right-aligned pill, vertically inset inside the row ----
    const int cellRight = 200, rowTop = 10, rowH = 20;
    const int bw = BadgeWidth(30, kPadX, kMinW);
    Box b = BadgeRect(cellRight, rowTop, rowH, bw, kPadY);
    check(!b.empty(), "badge box exists");
    check(b.right == cellRight, "badge is right-aligned to the cell edge");
    check(b.w() == bw, "badge width matches BadgeWidth");
    check(b.left == cellRight - bw, "badge left = right - width");
    check(b.top == rowTop + kPadY, "badge is inset from the row top");
    check(b.bottom == rowTop + rowH - kPadY, "badge is inset from the row bottom");
    check(b.h() == rowH - 2 * kPadY, "badge height = row minus top+bottom inset");
    check(b.bottom <= rowTop + rowH, "badge fits within the row height");

    // ---- metrics are sane positive design-space values ----
    check(kPadX > 0 && kPadY >= 0 && kRadius > 0 && kMinW > 0,
          "pill metrics are positive");

    if (g_fail) { printf("list_badge_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("list_badge_logic_test: OK\n");
    return 0;
}
