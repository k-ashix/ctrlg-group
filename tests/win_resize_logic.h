// win_resize_logic.h
//
// PURE window maximize/restore + resize-frame model for fix thse isseue.md
// Phase 3: the Group popup's title-bar square button is a real Maximize/Restore
// (driven by the ACTUAL maximized state, not the compact/expanded content
// preference), the borderless popup grows a resize frame, maximizing occupies
// the monitor WORK AREA (never exclusive full screen), and Ctrl+E (density)
// stays independent of maximize. Mirrored byte-for-byte into namespace
// winresize in ctrlg-group.wh.cpp; the Win32 wiring (WS_THICKFRAME,
// WM_GETMINMAXINFO, WM_NCHITTEST, WM_SIZE relayout, ShowWindow) is asserted by
// scripts/regression_check.ps1 source invariants.
//
// The Ht enum values ARE the Win32 HT* codes (HTCLIENT=1, HTLEFT=10 ..
// HTBOTTOMRIGHT=17), so the live WM_NCHITTEST handler can return the result
// directly.
#pragma once

namespace winresize {

// Minimum CLIENT size (96-DPI base px); the live code DPI-scales these so a
// user can never shrink the popup into an unusable sliver.
inline int MinClientW() { return 360; }
inline int MinClientH() { return 320; }

inline int ClampW(int w, int minW) { return w < minW ? minW : w; }
inline int ClampH(int h, int minH) { return h < minH ? minH : h; }

// Win32 hit-test codes (mirrored so the pure test needs no windows.h).
enum Ht {
    Client = 1, Left = 10, Right = 11, Top = 12, TopLeft = 13,
    TopRight = 14, Bottom = 15, BottomLeft = 16, BottomRight = 17
};

// Resize-border hit test for the borderless popup. (x,y) are CLIENT coords;
// returns the edge/corner HT code within borderPx of an edge, else Client.
// Caption drag (HTCAPTION) is owned elsewhere, so this is only the frame.
inline Ht HitTest(int x, int y, int clientW, int clientH, int borderPx) {
    const bool l = x >= 0 && x < borderPx;
    const bool r = x < clientW && x >= clientW - borderPx;
    const bool t = y >= 0 && y < borderPx;
    const bool b = y < clientH && y >= clientH - borderPx;
    if (t && l) return TopLeft;
    if (t && r) return TopRight;
    if (b && l) return BottomLeft;
    if (b && r) return BottomRight;
    if (l) return Left;
    if (r) return Right;
    if (t) return Top;
    if (b) return Bottom;
    return Client;
}

// The caption square button is Maximize/Restore. Its glyph follows the ACTUAL
// window state: true -> show the RESTORE glyph (window is maximized).
inline bool ShowRestoreGlyph(bool isZoomed) { return isZoomed; }

struct Rect { int left, top, right, bottom; };
inline int Width(const Rect& r)  { return r.right - r.left; }
inline int Height(const Rect& r) { return r.bottom - r.top; }
inline bool Valid(const Rect& r) { return r.right > r.left && r.bottom > r.top; }

// Maximizing occupies the monitor WORK AREA (taskbar respected), never an
// exclusive full-screen/kiosk frame -- so the maximized frame IS the work area.
inline Rect MaximizeTo(const Rect& workArea) { return workArea; }

// Restore returns to the user's saved previous bounds when they are valid,
// otherwise a sensible default size centred nowhere in particular (caller
// supplies the fallback). Never a hard-coded compact/expanded width.
inline Rect RestoreTo(const Rect& prev, const Rect& fallback) {
    return Valid(prev) ? prev : fallback;
}

// WM_GETMINMAXINFO geometry. A borderless WS_POPUP maximized with the DEFAULT
// MINMAXINFO covers the ENTIRE monitor (over the taskbar) and can be drag-sized
// into an unusable sliver. ComputeMinMax derives the fields the live handler
// writes into MINMAXINFO so that maximize fills the monitor WORK AREA (taskbar
// respected, never exclusive full screen) and a resize can never shrink below
// the minimum. ptMaxPosition is RELATIVE to the monitor origin (so a secondary
// monitor at negative virtual coords maximizes correctly). minTrack{W,H} are
// the minimum OUTER (device-px) window dimensions supplied by the caller.
struct SizeI  { int cx; int cy; };
struct MinMax { SizeI maxPos; SizeI maxSize; SizeI minTrack; };

inline MinMax ComputeMinMax(const Rect& monitor, const Rect& work,
                            int minTrackW, int minTrackH) {
    MinMax m;
    m.maxPos.cx   = work.left - monitor.left;   // -> MINMAXINFO.ptMaxPosition.x
    m.maxPos.cy   = work.top  - monitor.top;    // -> MINMAXINFO.ptMaxPosition.y
    m.maxSize.cx  = Width(work);                 // -> MINMAXINFO.ptMaxSize.x
    m.maxSize.cy  = Height(work);                // -> MINMAXINFO.ptMaxSize.y
    m.minTrack.cx = minTrackW;                   // -> MINMAXINFO.ptMinTrackSize.x
    m.minTrack.cy = minTrackH;                   // -> MINMAXINFO.ptMinTrackSize.y
    return m;
}

// Ctrl+E (compact/expanded CONTENT density) is independent of maximize: while
// maximized, a density toggle must NOT unmaximize or resize the top-level
// window (it may only reflow the content). True -> a density toggle is allowed
// to resize the top-level window (i.e. only when NOT maximized).
inline bool DensityMayResizeWindow(bool isZoomed) { return !isZoomed; }

}  // namespace winresize
