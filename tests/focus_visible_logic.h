// focus_visible_logic.h
//
// PURE "focus-visible" decision (fix thse isseue.md P0 #1), mirrored into
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build (no Win32 runtime). The mod keeps a byte-for-byte copy and gates the
// owner-drawn focus ring with it.
//
// The problem: the owner-draw drew a focus ring whenever a button held
// ODS_FOCUS, so clicking Create / Cancel / Copy / Move (which gives the control
// focus) painted an extra outline. The platform convention is "focus-visible":
// keyboard navigation shows the ring; a pointer press hides it. This module
// owns that classification; keyboard focus feedback is NOT removed.
//
#pragma once

namespace focusvis {

// Virtual-key codes (== Win32 VK_*). Named locally so the pure header needs no
// windows.h, exactly like segmented::kVkLeft / listscroll.
inline constexpr int kVkTab    = 0x09;
inline constexpr int kVkReturn = 0x0D;
inline constexpr int kVkSpace  = 0x20;
inline constexpr int kVkPrior  = 0x21;  // PageUp
inline constexpr int kVkNext   = 0x22;  // PageDown
inline constexpr int kVkEnd    = 0x23;
inline constexpr int kVkHome   = 0x24;
inline constexpr int kVkLeft   = 0x25;
inline constexpr int kVkUp     = 0x26;
inline constexpr int kVkRight  = 0x27;
inline constexpr int kVkDown   = 0x28;

// What last moved focus. KeyboardNav makes the ring visible; a PointerPress
// hides it.
enum class Input { KeyboardNav, PointerPress };

inline bool FromInput(Input in) {
    return in == Input::KeyboardNav;
}

// The ring is drawn ONLY when the control holds focus AND focus became visible
// via the keyboard. (focused-by-mouse -> false -> no ring: the #1 bug.)
inline bool ShouldShowRing(bool hasFocus, bool focusVisible) {
    return hasFocus && focusVisible;
}

// Does this virtual key move focus (i.e. should it make the ring visible)?
// Tab + arrows + Enter/Space (activation) + Page/Home/End navigation. A typed
// letter is NOT a nav key, so the Copy/Move 'C'/'M' mnemonic and text typing
// never flip focus-visible on their own.
inline bool IsNavKey(int vk) {
    return vk == kVkTab    || vk == kVkReturn || vk == kVkSpace ||
           vk == kVkPrior  || vk == kVkNext   || vk == kVkEnd   ||
           vk == kVkHome   || vk == kVkLeft   || vk == kVkUp    ||
           vk == kVkRight  || vk == kVkDown;
}

}  // namespace focusvis
