// hover_tint_logic.h
//
// PURE hover/pressed visual-state helper for the owner-drawn Create / Cancel
// buttons and the Copy/Move segments (fix thse isseue.md #14). The caption
// chrome buttons already react to hover/pressed via chromebtn::; this gives the
// MAIN action controls the same affordance. Pressed state and the keyboard
// focus ring are already handled by the mod; this header owns ONLY the hover
// decision + the colour-blend math, so a plain g++ build can unit-test it.
//
// Byte-for-byte mirror of namespace hovertint in ctrlg-group.wh.cpp.
//
#pragma once

namespace hovertint {

enum class Vis { Normal, Hover, Pressed };

// Pressed wins over hover; otherwise hot => Hover. Same shape as
// chromebtn::FillFor, kept separate so the action buttons can tune their own
// blend without disturbing the caption-chrome tints.
inline Vis VisFor(bool hot, bool pressed) {
    if (pressed) return Vis::Pressed;
    if (hot)     return Vis::Hover;
    return Vis::Normal;
}

// Only the Hover state repaints the fill here (pressed already has its own
// fill token; Normal is unchanged).
inline bool PaintsHover(Vis v) { return v == Vis::Hover; }

// Blend one 0-255 channel toward `target` by pct% (clamped). Used to lighten
// the resolved button fill toward white on hover so the control visibly reacts
// on both light and dark themes.
inline int BlendChannel(int c, int target, int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    int r = c + (target - c) * pct / 100;
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    return r;
}

// Hover lightens the fill by this fraction toward white.
inline constexpr int kHoverPct = 12;

// Keyboard-focus-visible nudges the fill toward the accent by this fraction --
// a subtle focus COLOUR (fix thse isseue.md #4) that replaces the old painted
// focus ring. Separate from kHoverPct so focus and hover stay independent.
inline constexpr int kFocusPct = 16;

}  // namespace hovertint
