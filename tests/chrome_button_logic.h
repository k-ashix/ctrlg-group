// chrome_button_logic.h
//
// PURE visual-state model for the custom title-bar chrome buttons (Expand,
// Minimize, Close) -- fix thse isseue.md #7. Decides ONLY the hover/pressed
// background state and whether the Close button shows its danger (red) tint,
// so the owner-draw paint has a SINGLE tested source of truth. No Win32 here;
// the mod keeps a byte-for-byte copy and maps the states to themed brushes.
//
#pragma once

namespace chromebtn {

// Flat (idle), a subtle hover background, or a stronger pressed background.
enum class Fill { None, Hover, Pressed };

// Pressed wins over hover; neither hovered nor pressed -> flat (None).
inline Fill FillFor(bool hot, bool pressed) {
    if (pressed) return Fill::Pressed;
    if (hot)     return Fill::Hover;
    return Fill::None;
}

// A background is painted for any non-flat state (hover or pressed).
inline bool PaintsBackground(Fill f) { return f != Fill::None; }

// The Close button uses a danger (red) tint when hovered/pressed, matching the
// Windows 11 caption Close control. Expand/Minimize always use a neutral tint.
inline bool IsDanger(bool isClose, Fill f) {
    return isClose && f != Fill::None;
}

// fix thse isseue.md A/D: the hover/press colour feedback now fills the FULL
// caption-button rectangle (native caption-button backplate). The old 4px
// left/right inset left blank strips between the hit area and the visible fill,
// so it is removed -- the guard is now 0 (edge-to-edge, full header height).
inline constexpr int kFillGuard = 0;

// Geometry of that colour feedback inside a caption button cell: it spans the
// FULL cell height (top & bottom == header height) and insets `guard` px on the
// LEFT and RIGHT only. Cell-local coordinates (the cell origin is 0,0).
struct FillBox { int x = 0, y = 0, w = 0, h = 0; };
inline FillBox FillArea(int cellW, int cellH, int guard) {
    return FillBox{ guard, 0, cellW - 2 * guard, cellH };
}

}  // namespace chromebtn
