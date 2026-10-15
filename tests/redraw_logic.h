#pragma once
// redraw_logic.h
// Pure mirror of the popup relayout REDRAW policy (fix thse isseue.md "Next
// exact fix order" #1-#4). A compact/expanded +/- resize must be ATOMIC:
//
//   disable intermediate redraw -> resize the window -> move ALL children
//   atomically (no per-child repaint) -> enable redraw -> ONE final repaint.
//
// The per-child MoveWindow(..., TRUE) sequence is what let each child repaint
// mid-transition (the wiggly/staggered redraw). These are the pure, unit-
// testable facts of the policy; the Win32 wiring that realises them
// (WM_SETREDRAW / BeginDeferWindowPos / EndDeferWindowPos / RedrawWindow) is a
// Windhawk translation unit a unit test cannot compile, so it is pinned by
// source invariants, and the mod ties the flag values below to the real Win32
// RDW_* constants with static_asserts.
namespace redraw {

// A child moved during the batch must NOT repaint itself; the single final
// repaint does all painting instead. (BeginDeferWindowPos has no per-move
// bRepaint, unlike MoveWindow(..., TRUE).)
inline constexpr bool kRepaintChildOnMove = false;

// Flag bits mirror the Win32 RDW_* values 1:1 (tied by static_assert in the mod).
inline constexpr unsigned kRdwInvalidate  = 0x0001u; // RDW_INVALIDATE
inline constexpr unsigned kRdwErase       = 0x0004u; // RDW_ERASE
inline constexpr unsigned kRdwAllChildren = 0x0080u; // RDW_ALLCHILDREN
inline constexpr unsigned kRdwUpdateNow   = 0x0100u; // RDW_UPDATENOW

// The one final repaint invalidates + erases + updates NOW + reaches ALL
// children, so the popup resolves in a single clean frame -- no intermediate
// frames, like a normal Windows resize/toggle.
inline constexpr unsigned FinalRepaintFlags() {
    return kRdwInvalidate | kRdwErase | kRdwUpdateNow | kRdwAllChildren;
}

// Because the final repaint already reaches every child, a separate
// pre-relayout InvalidateRect of the size toggle is redundant and must go.
inline constexpr bool FinalRepaintCoversChildren() {
    return (FinalRepaintFlags() & kRdwAllChildren) != 0;
}

} // namespace redraw
