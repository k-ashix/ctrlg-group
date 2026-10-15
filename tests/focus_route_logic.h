// focus_route_logic.h
//
// PURE model for the fix thse isseue.md P1 finding "Keyboard navigation does not
// exclude every interactive control" (ctrlg-group.wh.cpp lines ~12283-12369).
// No Win32. Mirrored byte-for-byte into ctrlg-group.wh.cpp (namespace
// focusroute) and consumed by BOTH keyboard routers in the Group modal loop:
//
//   * chip Left/Right navigation, and
//   * Selected-list Up/Down/Home/End/PageUp/PageDown scrolling.
//
// The single rule: a navigation keystroke drives the chip strip or the Selected
// list ONLY when NO interactive control owns focus. The classifier below lists
// EVERY focusable interactive control -- including destCopyBtn, autoGroupBtn,
// infoBtn, sizeToggle, minBtn and closeBtn, which the previous ad-hoc checks
// omitted, letting those controls' focus leak nav keys into the chips/list.
#pragma once

namespace focusroute {

// True when ANY interactive control owns focus (and must therefore swallow the
// navigation keys instead of letting them reach the chips / list).
inline bool IsInteractiveFocus(bool edit, bool createBtn, bool cancelBtn,
                               bool copySegment, bool moveSegment,
                               bool destCopyBtn, bool autoGroupBtn,
                               bool infoBtn, bool sizeToggle,
                               bool minBtn, bool closeBtn) {
    return edit || createBtn || cancelBtn || copySegment || moveSegment ||
           destCopyBtn || autoGroupBtn || infoBtn || sizeToggle ||
           minBtn || closeBtn;
}

// Navigation keys drive the chip strip / Selected list ONLY when no interactive
// control owns focus.
inline bool NavKeysDriveChipsOrList(bool interactiveFocus) {
    return !interactiveFocus;
}

}  // namespace focusroute
