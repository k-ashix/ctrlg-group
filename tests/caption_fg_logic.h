// caption_fg_logic.h
//
// PURE theme-aware caption FOREGROUND model (fix thse isseue.md #2, v0.12.60).
// The custom Ctrl+G title bar paints its "Group into folder" title text AND all
// three caption glyphs (Minimize / Expand-Restore / Close) with ONE shared
// foreground colour -- near-black in light mode, near-white in dark mode, and
// NEVER a hard-coded black in dark mode. The mod keeps a byte-for-byte copy
// (CaptionForeground() / kColCaptionFgLight / kColCaptionFgDark) so the
// owner-draw caption paint has a SINGLE tested source of truth; this header
// pins the DECISION without the un-compilable Win32 translation unit.
//
#pragma once

namespace captionfg {

// A plain RGB triple (0..255 each), so the model stays Win32-free / testable.
struct Rgb {
    int r, g, b;
};

inline bool operator==(const Rgb& a, const Rgb& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

// The shared caption foreground. Light theme -> near-black RGB(24,24,24);
// dark theme -> near-white RGB(245,245,245). Mirrors CaptionForeground().
inline Rgb Foreground(bool dark) {
    return dark ? Rgb{245, 245, 245} : Rgb{24, 24, 24};
}

// The foreground MUST adapt to the theme (not the same colour in both modes).
inline bool AdaptsToTheme() {
    return !(Foreground(true) == Foreground(false));
}

// Dark-mode foreground must be LIGHT (near-white), never a hard-coded black --
// a black glyph was invisible over the dark title-bar material (the bug).
inline bool IsLegibleInDark() {
    const Rgb d = Foreground(true);
    return d.r >= 200 && d.g >= 200 && d.b >= 200;
}

// Light-mode foreground must be DARK (near-black) so it reads on light material.
inline bool IsLegibleInLight() {
    const Rgb l = Foreground(false);
    return l.r <= 48 && l.g <= 48 && l.b <= 48;
}

}  // namespace captionfg
