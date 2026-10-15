// destfield_logic.h
//
// PURE read-only "information field" model for the Group popup's Destination
// field (fix thse isseu e.md #2), mirrored from ctrlg-group.wh.cpp
// so it can be unit tested with a plain g++ build (no Win32 runtime). The mod
// keeps a byte-for-byte copy and uses these rects/policies when it creates and
// paints the field; the tests pin them here.
//
// fix thse isseu e.md #2: the Destination field must read as a proper read-only
// INFO field, not a generic edit box:
//   - the path text shown PLAINLY -- NO folder icon inside the field
//     (fix thse isseue.md path pass: "no icon path"); a "copy path" button is
//     a SEPARATE control placed by the layout, outside the field
//   - a subtle field background
//   - NO caret / NO keyboard focus (informational only)
//   - middle/path ellipsis for long paths
// This module owns ONLY the internal geometry + those policy booleans. It adds
// no destination-selection behaviour (there is no Browse in scope).
//
#pragma once

namespace destfield {

// A DPI-independent design rectangle (96-dpi pixels). Same shape as layout::Rect
// but kept local so this header is self-contained for the unit test.
struct Cell {
    int x = 0, y = 0, w = 0, h = 0;
    int right()  const { return x + w; }
    int bottom() const { return y + h; }
    bool empty() const { return w <= 0 || h <= 0; }
};

// ---- field metrics (96-dpi design pixels) ----
// fix thse isseue.md #2 (path pass): "no icon path". The field no longer
// reserves a folder-glyph column; the path text begins at the edge pad and the
// copy-path affordance lives OUTSIDE the field. kEdgePad is the SAME value the
// mod feeds to EM_SETMARGINS on BOTH sides.
inline constexpr int kEdgePad  = 10;  // padding from the field's left/right edge
inline constexpr int kRadius   = 8;   // fix #6 (pass6): read-only field rounded-border corner radius

// Left inset (from the field's left edge) at which the path text begins. With
// no icon column this is just the edge pad, so the path reads plainly.
inline int TextLeftInset() { return kEdgePad; }

// The path-text cell inside a `fieldW` x `fieldH` field: inset by the edge pad
// on both sides so long paths ellipsize before the rounded border.
inline Cell TextCell(int fieldW, int fieldH) {
    const int x = TextLeftInset();
    const int w = fieldW - x - kEdgePad;
    return Cell{x, 0, w < 0 ? 0 : w, fieldH};
}

// ---- read-only info-field policy ----
// The field is informational: it must not take keyboard focus and must not show
// a caret. Long paths are shown with a path/middle ellipsis rather than being
// clipped or horizontally scrolled by the user.
inline bool NoKeyboardFocus() { return true; }
inline bool ShowCaret()       { return false; }
inline bool UsePathEllipsis() { return true; }

// fix thse isseue.md #6: the field has a steady NEUTRAL border -- it is read-only
// INFO, so (unlike the editable Folder name field) it never shows a focus accent.
inline bool BorderIsAccent(bool /*focused*/) { return false; }

}  // namespace destfield
