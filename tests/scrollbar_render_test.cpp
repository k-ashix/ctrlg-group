// scrollbar_render_test.cpp
// Slice 2 / fix thse isseu e.md #4: the Selected Items list must show a VISIBLE
// scrollbar when it overflows. The scroll math (ThumbRect/NeedsScrollbar) is
// already tested; this pins the NEW geometry that lets the mod paint a track +
// thumb without rows underlapping it: a reserved right-edge gutter (TrackRect),
// the reduced content width (ContentWidth) and the gutter constants.
//
// Self-contained: own main(), exit 0 = pass. RED before TrackRect/ContentWidth
// exist; GREEN once list_scroll_logic.h (and the mod) mirror them.

#include "list_scroll_logic.h"

#include <cstdio>

using namespace listscroll;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- gutter constants are sane ----
    check(kScrollbarW > 0, "scrollbar width is positive");
    check(kScrollbarPad >= 0 && kScrollbarPad * 2 < kScrollbarW,
          "thumb padding fits inside the gutter");

    // ---- TrackRect occupies the right edge, full height ----
    Box t = TrackRect(/*left*/0, /*top*/10, /*right*/100, /*bottom*/90, kScrollbarW);
    check(t.right == 100, "track hugs the right edge");
    check(t.left == 100 - kScrollbarW, "track width equals the gutter");
    check(t.top == 10 && t.bottom == 90, "track spans the full viewport height");

    // ---- ContentWidth reserves the gutter ONLY when overflowing ----
    const int viewportW = 280;
    // 10 items, 4 visible -> overflow -> gutter reserved.
    check(NeedsScrollbar(10, 4) == true, "10/4 overflows");
    check(ContentWidth(viewportW, kScrollbarW, NeedsScrollbar(10, 4))
              == viewportW - kScrollbarW,
          "overflow reserves the gutter from content width");
    // 3 items, 4 visible -> fits -> full width, no gutter.
    check(NeedsScrollbar(3, 4) == false, "3/4 fits");
    check(ContentWidth(viewportW, kScrollbarW, NeedsScrollbar(3, 4)) == viewportW,
          "no overflow -> rows use the full width");

    // ---- thumb sits inside the track and moves with topRow ----
    const int trackH = t.bottom - t.top;   // 80
    Thumb top = ThumbRect(/*topRow*/0, /*items*/12, /*visible*/4, trackH, /*min*/12);
    Thumb bot = ThumbRect(/*topRow*/8, /*items*/12, /*visible*/4, trackH, /*min*/12);
    check(top.h > 0 && top.h <= trackH, "thumb height within the track");
    check(top.y == 0, "thumb starts at the top when topRow==0");
    check(bot.y + bot.h <= trackH, "thumb stays within the track at the bottom");
    check(bot.y > top.y, "thumb moves down as topRow increases");

    if (g_fail) { printf("scrollbar_render_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("scrollbar_render_test: OK\n");
    return 0;
}
