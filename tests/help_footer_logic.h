// help_footer_logic.h
//
// PURE model for the Navigation Help card's GitHub author footer (fix thse
// isseue.md section 6 "GitHub author footer"). Mirrored byte-for-byte into
// ctrlg-group.wh.cpp (namespace helpfooter). No Win32. Single source of truth
// for the author URL and the exact open gesture: the link opens ONLY on a
// Ctrl + left-click INSIDE the footer rect -- a plain click never navigates.
#pragma once

namespace helpfooter {

// Author profile opened by the footer link.
inline const wchar_t* Url() { return L"https://github.com/k-ashix"; }

// Point-in-rect test for the footer's hit area ([left,right) x [top,bottom)).
inline bool InsideFooter(int px, int py, int left, int right, int top,
                         int bottom) {
    return px >= left && px < right && py >= top && py < bottom;
}

// The GitHub link opens ONLY on the exact Ctrl + left-click gesture inside the
// footer. Missing Ctrl, a non-left click, or a click outside the footer rect
// must NOT navigate.
inline bool ShouldOpen(bool ctrlDown, bool leftClick, bool insideFooter) {
    return ctrlDown && leftClick && insideFooter;
}

// fix thse isseue.md add-on section 4: the Ashix token ALSO opens on a plain
// DOUBLE left-click inside the footer -- no Ctrl required for the double-click
// path. A single plain click still never navigates (that is ShouldOpen's job).
inline bool ShouldOpenDouble(bool leftDoubleClick, bool insideFooter) {
    return leftDoubleClick && insideFooter;
}

}  // namespace helpfooter
