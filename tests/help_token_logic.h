// help_token_logic.h
//
// PURE model for the Navigation Help card's inline KEYBOARD TOKEN colours
// (fix thse isseue.md add-on section 6). Mirrored byte-for-byte into
// ctrlg-group.wh.cpp (namespace helptoken). No Win32. Single source of truth
// for the theme-aware token surface so a `mono` key token NEVER switches to a
// bright/white background in dark mode just because it is drawn separately from
// the surrounding text.
//
// Colours are packed in the Win32 COLORREF byte order (0x00BBGGRR) via Rgb() so
// the mirrored values equal RGB(r,g,b) on the device.
#pragma once

namespace helptoken {

struct Colors {
    unsigned long bg;
    unsigned long text;
    unsigned long border;
    unsigned long hover;   // add-on section 2: restrained hover variant of the fill
};

inline unsigned long Rgb(int r, int g, int b) {
    return (unsigned long)(r & 0xFF)
         | ((unsigned long)(g & 0xFF) << 8)
         | ((unsigned long)(b & 0xFF) << 16);
}

// Theme-aware token surface (fix thse isseue.md add-on section 2). The key
// token is a HIGH-CONTRAST INVERSE chip against the card:
//   * DARK theme  -> a MUTED MID-GREY surface (#D1CFC0, easy on the eyes) with
//                    DARK-GREY (#1F1F1F) text -- bright but NEVER pure white.
//   * LIGHT theme -> a DEEP CHARCOAL (#171717) surface with WHITE (#FFFFFF)
//                    text.
// The border is a near-identical neutral (a hair off the fill) so the 0.5 px
// edge stays integrated and never reads as a doubled outline. `hover` is a
// restrained few-step variant of the fill for the interactive footer token.
inline Colors ForTheme(bool dark) {
    if (dark) {
        return Colors{ Rgb(209, 207, 192), Rgb(31, 31, 31),
                       Rgb(188, 186, 172), Rgb(196, 194, 180) };
    }
    return Colors{ Rgb(23, 23, 23), Rgb(255, 255, 255),
                   Rgb(44, 44, 44), Rgb(48, 48, 48) };
}

// Channel + luma helpers (for sanity checks / tests).
inline int R(unsigned long c)    { return (int)(c & 0xFF); }
inline int G(unsigned long c)    { return (int)((c >> 8) & 0xFF); }
inline int B(unsigned long c)    { return (int)((c >> 16) & 0xFF); }
inline int Luma(unsigned long c) { return (R(c) + G(c) + B(c)) / 3; }

}  // namespace helptoken
