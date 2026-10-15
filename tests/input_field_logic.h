// input_field_logic.h
//
// PURE reusable field-frame decisions (fix thse isseue.md #4), mirrored
// byte-for-byte into ctrlg-group.wh.cpp (namespace inputfield) so they can be
// unit tested with a plain g++ build (no Win32 runtime). ONE helper serves all
// three prompt inputs: the Group Folder-name field, the Group Destination path
// field, and the New Folder name field.
//
// THE DESIGN: the parent paints ONE rounded frame (fill + border); the native
// EDIT is an OPAQUE rectangle INSET inside that frame by kEditInset px on every
// side (InnerEditRect), so the control's square corners sit in the frame's flat
// interior and can never paint over the rounded border or its anti-aliased edge.
// An opaque surface erase also means deleting text never leaves a ghost. There
// is a single frame geometry (ParentPaintsFrame), not two competing surfaces,
// and NO hard-edged SetWindowRgn clip (UsesRegionClip == false). The editable
// fields keep a focus-accent border; the Destination field keeps a neutral
// border and read-only behaviour.
//
// Keep byte-for-byte in lockstep with the mod's namespace inputfield.
#pragma once

namespace inputfield {

// Shared rounded corner (96-dpi design px) for ALL THREE prompt input frames
// (Group Folder name, Group Destination, New Folder name). Matches
// namefield::kRadius / destfield::kRadius / card::kCardRadius -- one 8px
// language. RoundRect takes the ellipse diameter, so a caller passes kRadius*2.
inline constexpr int kRadius = 8;

// The inner EDIT is inset from the outer frame by this many design px on every
// side, so its square corners stay off the rounded border/edge.
inline constexpr int kEditInset = 4;

enum class Field { Name, Destination, NewFolder };

struct FrameRect {
    int x = 0, y = 0, w = 0, h = 0;
    int right()  const { return x + w; }
    int bottom() const { return y + h; }
};

// fix thse isseue.md #2: the parent-painted rounded frame is EXACTLY the field's
// intended bounds -- it is NOT inflated outward. An earlier build drew the frame
// 1px LARGER on every side (a -1 left/top, +1 right/bottom inflation) to keep the
// EDIT's square corners off the rounded perimeter, but the EDIT is already inset
// by kEditInset (>> 1px), so that inflation only let the border SPILL past the
// field bounds and read as a thick / doubled edge. The frame now aligns to the
// field rect: ONE surface, ONE border, no spill.
inline FrameRect OuterFrameRect(int fx, int fy, int fw, int fh) {
    return FrameRect{fx, fy, fw, fh};
}
// Design-space spill of the painted frame beyond the field's intended bounds.
// MUST be 0: the border never extends past the field rect (fix thse isseue.md #2).
inline int FrameSpillPx() { return 0; }

// The inner EDIT rectangle for a given OUTER frame rect: inset by kEditInset on
// all four sides (never negative). Pure design space; the mod scales by DPI.
inline FrameRect InnerEditRect(int fx, int fy, int fw, int fh) {
    int iw = fw - 2 * kEditInset;
    int ih = fh - 2 * kEditInset;
    return FrameRect{fx + kEditInset, fy + kEditInset, iw < 0 ? 0 : iw, ih < 0 ? 0 : ih};
}

// The inner EDIT is OPAQUE-filled with the field surface colour (NOT a hollow
// transparent brush), so deleting text can never leave a ghost.
inline bool EditBackgroundIsOpaque() { return true; }

// The ONE rounded frame (fill + border) is painted by the PARENT, not erased by
// the EDIT itself -- a single frame geometry, not two competing ones.
inline bool ParentPaintsFrame() { return true; }

// A hard-edged region clip is forbidden (it reintroduces stair-stepped corners
// at high DPI); the rounded look comes from the parent-painted frame.
inline bool UsesRegionClip() { return false; }

// Border policy: the editable fields (Name / NewFolder) show the accent colour
// while focused; the read-only Destination field always keeps a neutral border.
inline bool BorderIsAccent(Field f, bool focused) {
    return (f == Field::Name || f == Field::NewFolder) && focused;
}

// Only the Destination field is read-only (informational path preview, no caret).
inline bool IsReadOnly(Field f) { return f == Field::Destination; }

// RoundRect takes the ellipse diameter, so a caller passes kRadius * 2.
inline int CornerDiameter() { return kRadius * 2; }

}  // namespace inputfield
