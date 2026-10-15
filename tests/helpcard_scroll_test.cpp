// helpcard_scroll_test.cpp
// Pins fix thse isseue.md "Overflow and Scrollable Help Card": the PURE scroll
// model for the Navigation Help card. Overflow detection (a scrollbar appears
// ONLY when the content overflows), scroll clamping, mouse-wheel scrolling,
// work-area capping (the card never enlarges past the monitor), the fixed-
// heading viewport split (heading stays put, only the content area scrolls),
// theme-scrollbar thumb geometry, and the wheel-isolation routing predicate (a
// wheel over the card scrolls the card and NEVER the Selected Items list or the
// summary chips). Self-contained: own main(), exit 0 = pass.

#include "helpcard_scroll_logic.h"

#include <cstdio>

using namespace helpscroll;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- NeedsScroll: a scrollbar appears ONLY when content overflows ----
    check(!NeedsScroll(100, 200), "content shorter than the viewport does not scroll");
    check(!NeedsScroll(200, 200), "content exactly the viewport height does not scroll");
    check(NeedsScroll(201, 200), "content taller than the viewport scrolls");
    check(!NeedsScroll(500, 0), "an unmeasured (zero-height) viewport never claims to scroll");

    // ---- MaxScroll: the last line can reach the bottom, never past it ----
    check(MaxScroll(100, 200) == 0, "no overflow => no scroll range");
    check(MaxScroll(500, 200) == 300, "scroll range is content minus viewport");
    check(MaxScroll(200, 200) == 0, "exact fit => no scroll range");

    // ---- ClampScroll: offset is pinned into [0, maxScroll] ----
    check(ClampScroll(-50, 300) == 0, "negative scroll clamps to the top");
    check(ClampScroll(1000, 300) == 300, "over-scroll clamps to the bottom");
    check(ClampScroll(120, 300) == 120, "an in-range offset is preserved");

    // ---- OnWheel: forward notch reveals earlier content (smaller offset) ----
    const int maxS = MaxScroll(500, 200);  // 300
    check(OnWheel(150, -120, 40, maxS) == 190,
          "one notch DOWN scrolls toward the bottom by one step");
    check(OnWheel(150, 120, 40, maxS) == 110,
          "one notch UP scrolls toward the top by one step");
    check(OnWheel(20, 120, 40, maxS) == 0,
          "scrolling up past the top clamps at 0");
    check(OnWheel(290, -120, 40, maxS) == maxS,
          "scrolling down past the end clamps at maxScroll");
    check(OnWheel(100, -240, 40, maxS) == 180,
          "a two-notch delta moves two steps");

    // ---- CapToWorkArea: card never enlarges past the monitor work area ----
    check(CapToWorkArea(300, 1000, 24) == 300,
          "a card that fits keeps its desired size");
    check(CapToWorkArea(1200, 1000, 24) == 952,
          "an oversized card is capped to the work area minus margins");
    check(CapToWorkArea(1200, 1000, 24) < 1000,
          "the capped card stays strictly inside the work area");
    check(CapToWorkArea(300, 40, 24) == 40,
          "a degenerate work area falls back to its raw extent (never zero)");

    // ---- CapToMax: the card never exceeds its FIXED configured maximum ----
    check(CapToMax(250, 300) == 250, "a card within the fixed maximum keeps its desired size");
    check(CapToMax(300, 300) == 300, "a card exactly at the fixed maximum is unchanged");
    check(CapToMax(900, 300) == 300, "a card taller than the fixed maximum is capped to it (never grows with content)");
    check(CapToMax(900, 0) == 900, "a non-positive maximum means no fixed cap");
    // Fixed maximum + work area compose: the SMALLER bound always wins.
    check(CapToWorkArea(CapToMax(900, 300), 2000, 24) == 300,
          "on a tall monitor the FIXED maximum (300) still bounds the card, not the work area");
    check(CapToWorkArea(CapToMax(900, 300), 320, 24) == 272,
          "on a tiny monitor the work area bounds the card below the fixed maximum");
    // A card bounded to its 300px maximum with 500px of content must scroll.
    check(NeedsScroll(500, ViewportHeight(300, 44, 12)) == true,
          "content taller than the capped viewport scrolls internally instead of enlarging the card");

    // ---- CapToMin: a card with little content still keeps a practical floor ----
    check(CapToMin(120, 180) == 180, "a card shorter than the minimum is raised to the floor");
    check(CapToMin(180, 180) == 180, "a card exactly at the minimum is unchanged");
    check(CapToMin(260, 180) == 260, "a card above the minimum keeps its size");
    check(CapToMin(120, 0) == 120, "a non-positive minimum means no floor");
    // "Subject to available space": the caller passes a floor already capped to
    // the work area, so a tiny monitor caps the floor and CapToMin never pushes
    // the card off-screen.
    check(CapToMin(100, CapToWorkArea(180, 160, 24)) == CapToWorkArea(180, 160, 24),
          "the min floor is itself clamped to the work area (never off-screen)");
    // Compose fixed max + min: a height between the two bounds is unchanged.
    check(CapToMin(CapToMax(250, 320), 180) == 250,
          "a height between the min floor and the fixed max is unchanged by both");

    // ---- ViewportHeight: the heading band is reserved, content fills the rest ----
    check(ViewportHeight(300, 44, 12) == 244,
          "viewport is card height minus the fixed heading band and bottom pad");
    check(ViewportHeight(40, 44, 12) == 0,
          "a card smaller than its chrome yields a non-negative viewport");

    // ---- ThumbRect: proportional thumb, clamped, reaches the track ends ----
    {
        Thumb full = ThumbRect(0, 100, 200, 180, 20);
        check(full.h == 180, "no overflow => the thumb fills the whole track");

        Thumb top = ThumbRect(0, 400, 200, 180, 20);  // content 2x viewport
        check(top.y == 0, "at the top the thumb sits at the track top");
        check(top.h == 90, "thumb height is proportional (viewport / content)");

        const int maxScroll = 400 - 200;  // 200
        Thumb bottom = ThumbRect(maxScroll, 400, 200, 180, 20);
        check(bottom.y + bottom.h == 180,
              "at maxScroll the thumb bottom reaches the track bottom");

        Thumb tiny = ThumbRect(0, 100000, 200, 180, 20);
        check(tiny.h == 20, "a huge content clamps the thumb to the minimum height");
    }

    // ---- WheelTargetsHelpCard: wheel isolation (never the list / chips) ----
    check(WheelTargetsHelpCard(true, true),
          "a wheel over the SHOWN card scrolls the card (isolated from the list)");
    check(!WheelTargetsHelpCard(true, false),
          "a wheel away from the card does not scroll the card");
    check(!WheelTargetsHelpCard(false, true),
          "a hidden card never captures the wheel");

    if (g_fail == 0) printf("helpcard_scroll_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
