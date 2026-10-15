// field_round_test.cpp
// Add-on (fix thse isseue.md "Fix rounded corners and clipping in all input
// fields"): a programmatic re-verification of the Destination + Folder-name
// field GEOMETRY so the rounded corners stay smooth and the inner edit never
// protrudes at 100/125/150/175/200% DPI. We cannot screenshot in CI, so this
// pins the pure geometry the mod feeds to the AA rounded-surface renderer; the
// GDI/Direct2D paint itself is pinned by source invariants (regression_check).
//
// Acceptance criteria covered here (the testable ones):
//   1/2. field corner radius rides the ONE MulDiv DPI rule and the rounded
//        ellipse never exceeds the field height -> corners stay fully round,
//        never degrade to a square/clipped edge, at every DPI.
//   3.   the Destination path text is inset clear of the folder glyph and keeps
//        a right pad so long paths ellipsize cleanly INSIDE the rounded border.
//   5.   the read-only vs editable field policies (caret / focus / ellipsis)
//        are unchanged, so clipping + keyboard behaviour don't regress.
// Self-contained: own main(), exit 0 = pass.

#include "layout_logic.h"
#include "corner_logic.h"
#include "destfield_logic.h"
#include "namefield_logic.h"

#include <cstdio>

using layout::ScaleForDpi;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const int ladder[] = {96, 120, 144, 168, 192};  // 100/125/150/175/200%

    // The ellipse DIAMETER the mod passes to the rounded renderer is kRadius*2.
    const int nameDia = namefield::kRadius * 2;
    const int destDia = destfield::kRadius * 2;

    // ---- 1: the field radius rides the ONE MulDiv rule (no bespoke scaler) ----
    for (int d : ladder) {
        check(corner::ScaledRadius(nameDia, d) == ScaleForDpi(nameDia, d),
              "name field radius scales via the one MulDiv rule");
        check(corner::ScaledRadius(destDia, d) == ScaleForDpi(destDia, d),
              "dest field radius scales via the one MulDiv rule");
    }
    check(ScaleForDpi(nameDia, 96) == nameDia, "name radius @96 is identity (crisp)");
    check(ScaleForDpi(nameDia, 192) == 2 * nameDia, "name radius @200% is an exact double");
    check(ScaleForDpi(destDia, 192) == 2 * destDia, "dest radius @200% is an exact double");
    int prevN = -1, prevD = -1;
    for (int d : ladder) {
        int sn = ScaleForDpi(nameDia, d), sd = ScaleForDpi(destDia, d);
        check(sn >= prevN, "name radius is monotonic non-decreasing across DPI");
        check(sd >= prevD, "dest radius is monotonic non-decreasing across DPI");
        prevN = sn; prevD = sd;
    }

    // ---- 2: the rounded ellipse never exceeds the field HEIGHT, so the corner
    // is always a true arc (never clamped flat into a square edge) ----
    layout::PromptLayout g = layout::ComputePromptLayout(true);
    check(g.nameField.h > 0 && g.destField.h > 0, "group layout has real field heights");
    check(nameDia <= g.nameField.h, "name ellipse fits the field height @96 (round, not square)");
    check(destDia <= g.destField.h, "dest ellipse fits the field height @96 (round, not square)");
    for (int d : ladder) {
        check(ScaleForDpi(nameDia, d) <= ScaleForDpi(g.nameField.h, d),
              "name corner stays fully rounded at every DPI");
        check(ScaleForDpi(destDia, d) <= ScaleForDpi(g.destField.h, d),
              "dest corner stays fully rounded at every DPI");
        // the "surface 1px larger each side" outset must stay >= 1px device,
        // so the rectangular inner EDIT can never protrude past the rounded edge.
        check(ScaleForDpi(1, d) >= 1, "field surface outset stays >= 1px at every DPI");
    }

    // ---- 3: Destination path clips cleanly inside the rounded border ----
    const int fieldH = g.destField.h > 0 ? g.destField.h : 34;
    const int fieldW = g.destField.w > 0 ? g.destField.w : 300;
    destfield::Cell text = destfield::TextCell(fieldW, fieldH);
    // fix thse isseue.md #2 (path pass): no folder glyph -- the path text now
    // begins right at the edge pad (no icon column to clear).
    check(text.x == destfield::TextLeftInset(), "path text begins at the tested left inset");
    check(text.x == destfield::kEdgePad, "path text starts at the edge pad (no icon column)");
    check(text.right() <= fieldW - destfield::kEdgePad,
          "path text keeps a right pad so long paths ellipsize before the border");
    check(destfield::UsePathEllipsis(), "long paths use a path/middle ellipsis (clean clip)");

    // ---- 5: field policies unchanged (clipping + keyboard behaviour safe) ----
    check(destfield::NoKeyboardFocus(), "Destination field is info-only (no keyboard focus)");
    check(!destfield::ShowCaret(), "Destination field shows no caret");
    check(!destfield::BorderIsAccent(true), "Destination (read-only) never shows a focus accent");
    check(namefield::BorderIsAccent(true), "focused Folder-name field uses the accent border");
    check(!namefield::BorderIsAccent(false), "unfocused Folder-name field uses the neutral border");
    check(namefield::Placeholder() != nullptr && namefield::Placeholder()[0] != L'\0',
          "Folder-name field keeps a placeholder cue");

    if (g_fail) { printf("field_round_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("field_round_test: OK\n");
    return 0;
}
