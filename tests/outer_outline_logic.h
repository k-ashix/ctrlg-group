// outer_outline_logic.h
//
// PURE "unwanted outer outline" decision (fix thse isseue.md P0 #2), mirrored
// byte-for-byte into ctrlg-group.wh.cpp so it can be unit tested with a plain
// g++ build (no Win32 runtime).
//
// THE DEFECT: the custom borderless Group popup was created with
// WS_EX_DLGMODALFRAME. That extended-style paints a raised 3-D modal frame
// around the WHOLE window -- the stray bright outer perimeter the issue reports
// -- which is redundant on a window that already draws its own surface and
// relies on the DWM rounded corners + DWMWA_COLOR_NONE transparent border.
//
// THE FIX: one shared extended-style calculation, used by BOTH PromptOuterSize()
// (so the non-client size math matches) AND CreateWindowExW() (so the real
// window matches). The Group popup drops WS_EX_DLGMODALFRAME entirely; the
// native-caption New Folder popup keeps it (its behaviour is unchanged).
//
// Keep byte-for-byte in lockstep with the mod's namespace outeroutline.
#pragma once

namespace outeroutline {

// winuser.h extended window styles (values pinned so the mirror never drifts
// from the real bits the window is created with).
inline constexpr unsigned long kExDlgModalFrame = 0x00000001UL; // WS_EX_DLGMODALFRAME
inline constexpr unsigned long kExTopmost       = 0x00000008UL; // WS_EX_TOPMOST

// The ONE shared extended style for the folder prompt. PromptOuterSize() and
// CreateWindowExW() MUST both call this with the same isGroup, so the frame the
// size math assumes can never disagree with the frame the window actually gets.
//
//   * Group popup   -> WS_EX_TOPMOST only (no raised modal perimeter).
//   * New Folder     -> WS_EX_TOPMOST | WS_EX_DLGMODALFRAME (unchanged).
inline unsigned long PromptExStyle(bool isGroup) {
    unsigned long ex = kExTopmost;
    if (!isGroup) {
        ex |= kExDlgModalFrame;
    }
    return ex;
}

// True when the given extended style carries the raised modal frame.
inline bool HasDlgModalFrame(unsigned long exStyle) {
    return (exStyle & kExDlgModalFrame) != 0UL;
}

// The Group popup must NOT draw the extra outer modal frame; New Folder keeps it.
inline bool DrawsOuterModalFrame(bool isGroup) {
    return HasDlgModalFrame(PromptExStyle(isGroup));
}

}  // namespace outeroutline
