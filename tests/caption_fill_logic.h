// caption_fill_logic.h
//
// PURE idle caption-button BACKGROUND model (fix thse isseue.md #1/#3, v0.12.60).
// The three custom title-bar buttons (Expand id103, Minimize id104, Close id105)
// must NOT paint a permanent grey/black backplate. Their IDLE surface MATCHES
// the title band beneath them so they blend into the composed backdrop:
//   * confirmed DWM backdrop -> transparent BLACK (the Win11 material composites
//     through, exactly like the band fill in bandfill::),
//   * otherwise               -> the solid themed panel (solid-header fallback).
// Hover / pressed / red-Close are TRANSIENT feedback painted OVER the idle
// surface (chromebtn::), never a permanent fill. The mod keeps a byte-for-byte
// copy in the WM_DRAWITEM caption paint; this header pins the DECISION.
//
#pragma once

namespace captionfill {

// The brush that fills a caption button's IDLE background. Deliberately the
// SAME two options bandfill:: uses for the band, because the button must match
// the band it sits on. There is no "Grey" / opaque-backplate option any more --
// that permanent block is exactly what the issue removed.
enum class Brush {
    Panel,             // solid themed surface (#202020 dark / #F3F3F3 light)
    TransparentBlack,  // GDI black -> transparent pixels for the extended frame
};

// Idle caption background == the band beneath it.
// backdropConfirmed == groupBandBackdrop (d->isGroup && d->acrylicActive).
inline Brush IdleBrush(bool backdropConfirmed) {
    return backdropConfirmed ? Brush::TransparentBlack : Brush::Panel;
}

// The idle button NEVER paints a permanent opaque grey backplate -- under a
// confirmed backdrop it is transparent (black); otherwise it only matches the
// solid header. Either way it blends into the band, it is not a grey block.
inline bool PaintsPermanentGreyBackplate() {
    return false;
}

// Idle paints NO highlight tint. A highlight is painted ONLY while hovered or
// pressed -- that decision lives in chromebtn::FillFor / PaintsBackground, so
// the feedback is a transient interaction state, not a permanent fill.
inline bool PaintsHighlightWhenIdle() {
    return false;
}

// The idle caption fill ALWAYS matches the band fill for the same backdrop
// state -- the invariant that makes the button blend instead of showing a
// block. (Both are "transparent black when confirmed, else solid panel".)
inline bool MatchesBand(bool backdropConfirmed) {
    const bool captionTransparent =
        IdleBrush(backdropConfirmed) == Brush::TransparentBlack;
    const bool bandTransparent = backdropConfirmed;  // bandfill:: TransparentBlack
    return captionTransparent == bandTransparent;
}

}  // namespace captionfill
