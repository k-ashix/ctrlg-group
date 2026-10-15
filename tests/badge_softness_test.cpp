// badge_softness_test.cpp
// Pins fix thse isseue.md #9/#14 badge polish: the extension pill border must be
// "less dominant" than the text ink and must never be a hard black outline. The
// text keeps the full category ink; the HAIRLINE uses a softer tone biased
// toward the chip fill. Self-contained: own main(), exit 0 = pass.

#include "badge_color_logic.h"

#include <cstdio>
#include <cstdlib>

using namespace badgecolor;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

static int ch(unsigned v, int shift) { return (int)((v >> shift) & 0xFFu); }
static int dist(unsigned a, unsigned b) {
    return abs(ch(a, 16) - ch(b, 16)) + abs(ch(a, 8) - ch(b, 8)) +
           abs(ch(a, 0) - ch(b, 0));
}
static bool between(int lo, int hi, int x) {
    if (lo > hi) { int t = lo; lo = hi; hi = t; }
    return x >= lo && x <= hi;
}

int main() {
    const Cat known[] = {Cat::Image, Cat::Audio, Cat::Video,
                         Cat::Document, Cat::Archive, Cat::Code};
    for (bool dark : {false, true}) {
        for (Cat c : known) {
            Chip chip = ChipFor(c, dark);
            unsigned border = SoftBorder(chip);

            check((border & 0xFF000000u) == 0u, "soft border packs into 24 bits");
            // The whole point of #14: the hairline is softer than the text ink.
            check(border != chip.ink,
                  "soft border is NOT the full-strength text ink (less dominant)");
            // ...but still a real edge, not identical to the fill (invisible).
            check(border != chip.fill,
                  "soft border is still distinct from the fill (visible hairline)");
            // No hard black outline (fix thse isseue.md #9: "no hard black outline").
            check(border != 0x000000u, "no hard black outline");
            // Each channel sits between the fill and the ink.
            check(between(ch(chip.fill, 16), ch(chip.ink, 16), ch(border, 16)),
                  "border red between fill and ink");
            check(between(ch(chip.fill, 8), ch(chip.ink, 8), ch(border, 8)),
                  "border green between fill and ink");
            check(between(ch(chip.fill, 0), ch(chip.ink, 0), ch(border, 0)),
                  "border blue between fill and ink");
            // "less dominant" == biased toward the fill, i.e. closer to fill
            // than to the ink.
            check(dist(border, chip.fill) <= dist(border, chip.ink),
                  "border leans toward the fill (less dominant than ink)");
        }
    }

    if (g_fail == 0) printf("badge_softness_test: OK\n");
    else             printf("badge_softness_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
