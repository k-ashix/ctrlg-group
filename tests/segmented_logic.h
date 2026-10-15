// segmented_logic.h
//
// PURE Copy/Move segmented-control state machine + keyboard mapping
// (waht to create.md #7 / #8), mirrored from ctrlg-group.wh.cpp
// so it can be unit tested with a plain g++ build (no Win32 runtime). The mod
// keeps a byte-for-byte copy and drives the control from these functions.
//
// The control is a per-operation override of the existing g_groupIsMove
// setting. Copy is the default. d->isMove stays the single source of truth the
// worker reads back -- this module only decides HOW a keystroke or click maps
// to Copy/Move, and (crucially) that Left/Right act only when the control owns
// focus, so the EDIT keeps normal caret behavior.
//
#pragma once

#include <string>

namespace segmented {

// Virtual-key codes for the arrow keys (== Win32 VK_LEFT / VK_RIGHT). Named
// locally so the pure header needs no windows.h.
inline constexpr int kVkLeft  = 0x25;
inline constexpr int kVkRight = 0x27;

enum class Seg { Copy, Move };

// Bridge to the existing bool flag. Copy is the default (isMove == false).
inline Seg FromIsMove(bool isMove) { return isMove ? Seg::Move : Seg::Copy; }
inline bool ToIsMove(Seg s) { return s == Seg::Move; }

inline std::wstring SegmentLabel(Seg s) {
    return s == Seg::Move ? L"Move" : L"Copy";
}

// fix thse isseue.md #5 A: the access-key / mnemonic form of each label. The
// '&' prefix makes a prefix-aware DrawTextW UNDERLINE the following character,
// so the Copy segment shows an underlined "C" and Move an underlined "M" -- the
// standard Windows access-key hint for the C/M keyboard mnemonics the segmented
// control already handles (see Mnemonic() below). The VISIBLE text is identical
// to SegmentLabel (the '&' is consumed, not drawn), so the mod keeps measuring
// the plain SegmentLabel to centre the icon+label group.
inline std::wstring MnemonicLabel(Seg s) {
    return s == Seg::Move ? L"&Move" : L"&Copy";
}
// The underlined mnemonic character for a segment: Copy -> 'C', Move -> 'M'.
inline wchar_t MnemonicChar(Seg s) { return s == Seg::Move ? L'M' : L'C'; }

struct SegKeyResult {
    Seg seg;         // resulting segment
    bool consumed;   // did the segmented control handle this key?
};

// Map a keystroke to a segment. Left -> Copy, Right -> Move, but ONLY when the
// segmented control owns keyboard focus. When it does not (e.g. the EDIT has
// focus), arrows are NOT consumed so the text caret moves normally. Non-arrow
// keys are never consumed here.
inline SegKeyResult OnKey(Seg current, int vk, bool segFocused) {
    if (!segFocused) {
        return SegKeyResult{current, false};
    }
    if (vk == kVkLeft)  return SegKeyResult{Seg::Copy, true};
    if (vk == kVkRight) return SegKeyResult{Seg::Move, true};
    return SegKeyResult{current, false};
}

// Map a C/M mnemonic to a segment (fix thse isseu e.md #4/#6). 'C'/'c' -> Copy,
// 'M'/'m' -> Move, but ONLY when the segmented control owns keyboard focus. When
// the EDIT owns focus the letters are NOT consumed, so typing "My Camera" into
// the folder-name field is never hijacked (the review's explicit #4 warning
// against a global C/M hotkey). Any other character is left for the EDIT. Shares
// the SegKeyResult shape with OnKey so the modal loop handles arrows and letters
// through one identical path.
inline SegKeyResult Mnemonic(Seg current, int ch, bool segFocused) {
    if (!segFocused) {
        return SegKeyResult{current, false};
    }
    if (ch == 'C' || ch == 'c') return SegKeyResult{Seg::Copy, true};
    if (ch == 'M' || ch == 'm') return SegKeyResult{Seg::Move, true};
    return SegKeyResult{current, false};
}

// Clicking a segment selects exactly that segment -- the same state the arrow
// keys produce, so mouse and keyboard share one result.
inline Seg OnClickSegment(Seg clicked) { return clicked; }

// ---- one-pill corner rounding (fix thse isseu e.md #7) -------------------
// The two segments must read as ONE segmented control, not two independent
// pills: only the OUTER left/right corners are rounded and the shared middle
// boundary is straight. This model says, per segment, which of its four
// corners are rounded.
struct SegCorners {
    bool topLeft, topRight, bottomRight, bottomLeft;
};

// Copy is the LEFT segment: round its left corners, keep the right (middle)
// edge square. Move is the RIGHT segment: mirror image.
inline SegCorners CornersFor(Seg s) {
    if (s == Seg::Copy) return SegCorners{true,  false, false, true};   // left rounded
    return                       SegCorners{false, true,  true,  false}; // right rounded
}

// The two segments meet edge-to-edge (no gap) so the straight middle boundary
// is a single shared line. Given the left segment's right edge x, the right
// segment starts at exactly that x (adjacency == one continuous pill).
inline bool SegmentsAreAdjacent(int leftRight, int rightLeft) {
    return leftRight == rightLeft;
}

}  // namespace segmented
