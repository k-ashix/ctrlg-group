// input_field_test.cpp
// Pins the PURE reusable field-frame decisions (fix thse isseue.md #4) shared by
// all three prompt inputs (Group Folder name, Group Destination, New Folder):
//   * the native EDIT is an OPAQUE rectangle INSET inside a parent-painted
//     rounded frame -- InnerEditRect insets kEditInset px on every side so the
//     control's square corners stay off the rounded border;
//   * one frame geometry (ParentPaintsFrame), opaque fill (no ghost on delete),
//     and NO hard-edged region clip (SetWindowRgn);
//   * the editable fields (Name / NewFolder) show the accent border while
//     focused; the Destination field keeps a neutral border and is read-only;
//   * the corner radius is 8px (one 8px language), passed to RoundRect doubled.
//
// Self-contained: own main(), exit 0 = pass. RED before input_field_logic.h.

#include "input_field_logic.h"

#include <cstdio>

using namespace inputfield;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- opaque, one-geometry, not region-clipped ----
    check(EditBackgroundIsOpaque(), "EDIT background is opaque (no ghost on delete)");
    check(ParentPaintsFrame(), "the one rounded frame is painted by the parent");
    check(!UsesRegionClip(), "no SetWindowRgn hard-edged clip");

    // ---- radius is the shared 8px language, doubled for RoundRect ----
    check(kRadius == 8, "field corner radius is 8 design px");
    check(CornerDiameter() == 16, "RoundRect gets the diameter (radius * 2)");

    // ---- the inner EDIT is inset inside the outer frame on every side ----
    check(kEditInset > 0, "the inner EDIT is inset from the frame");
    {
        FrameRect e = InnerEditRect(100, 200, 300, 40);
        check(e.x == 100 + kEditInset, "inner EDIT x is inset from the frame left");
        check(e.y == 200 + kEditInset, "inner EDIT y is inset from the frame top");
        check(e.w == 300 - 2 * kEditInset, "inner EDIT width is inset on both sides");
        check(e.h == 40 - 2 * kEditInset, "inner EDIT height is inset top and bottom");
        // the inset rectangle's corners sit strictly inside the frame, so the
        // EDIT's square corners can never paint over the rounded border.
        check(e.x > 100 && e.y > 200 && e.right() < 400 && e.bottom() < 240,
              "inner EDIT corners stay inside the frame, off the rounded border");
    }
    {
        // a tiny frame must clamp the inner EDIT to zero, never go negative
        FrameRect e = InnerEditRect(0, 0, 2, 2);
        check(e.w == 0 && e.h == 0, "degenerate frame clamps inner EDIT to zero");
    }

    // ---- fix thse isseue.md #2: the painted frame is EXACTLY the field bounds
    //      (no outward -1/+1 inflation), so the border never spills past the
    //      field and cannot read as a thick / doubled edge. The inner EDIT still
    //      stays strictly inside the aligned frame.
    check(FrameSpillPx() == 0, "frame does not spill beyond the field's intended bounds (#2)");
    {
        FrameRect f = OuterFrameRect(100, 200, 300, 40);
        check(f.x == 100 && f.y == 200 && f.w == 300 && f.h == 40,
              "outer frame equals the field rect (no -1/+1 inflation, #2)");
        FrameRect e = InnerEditRect(f.x, f.y, f.w, f.h);
        check(e.x > f.x && e.y > f.y && e.right() < f.right() && e.bottom() < f.bottom(),
              "inner EDIT stays strictly inside the aligned frame, off the border (#2)");
    }

    // ---- editable fields: accent border only while focused ----
    check(BorderIsAccent(Field::Name, true), "Name field border is accent when focused");
    check(!BorderIsAccent(Field::Name, false), "Name field border is neutral when unfocused");
    check(BorderIsAccent(Field::NewFolder, true), "New Folder border is accent when focused");
    check(!BorderIsAccent(Field::NewFolder, false), "New Folder border is neutral when unfocused");

    // ---- Destination: neutral border always, read-only ----
    check(!BorderIsAccent(Field::Destination, true), "Destination border is neutral even when focused");
    check(!BorderIsAccent(Field::Destination, false), "Destination border is neutral when unfocused");
    check(IsReadOnly(Field::Destination), "Destination field is read-only");
    check(!IsReadOnly(Field::Name), "Name field is editable");
    check(!IsReadOnly(Field::NewFolder), "New Folder field is editable");

    if (g_fail) { printf("input_field_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("input_field_test: OK\n");
    return 0;
}
