// show_ext_logic.h
//
// PURE state model for the "Show extensions" toolbar toggle (fix thse
// isseue.md Phase 1B). Default OFF: file rows show the stem only, plus the size
// badge. When ON, a file row renders its COMPLETE filename INLINE (e.g.
// "README.md") instead of a separate extension badge -- there is NO extension
// pill in either state. Folders never show a size badge and are unaffected by
// the toggle. The summary chips above the list are available in BOTH states.
// Mirrored byte-for-byte into ctrlg-group.wh.cpp.
#pragma once

#include <string>

namespace showext {

// fix thse isseue.md Issue 3: the toggle starts ON, so a new Group popup shows
// complete filenames (with extensions) by default. Clicking it turns the
// behavior OFF; subsequent clicks toggle it back ON.
inline bool DefaultOn() { return true; }

// The toolbar action label.
inline const wchar_t* ToggleLabel() { return L"Show extensions"; }

// A size badge is shown for files with a known size; never for folders.
inline bool RowShowsSize(bool isFolder, bool sizeKnown) {
    return !isFolder && sizeKnown;
}

// When the toggle is ON, a file row renders its COMPLETE filename inline
// (e.g. "README.md") instead of the stem; a separate extension pill is NEVER
// drawn. Folders are never affected (always their whole name, no suffix).
inline bool RowShowsExtInline(bool toggleOn, bool isFolder) {
    return toggleOn && !isFolder;
}

// ---- Compact ON/OFF switch (fix thse isseue.md Phase 3) ----
// The toolbar control is a small theme-aware switch with NO inline text label.
// This is the accessible name used for the tooltip + assistive technology (the
// visible text label is gone, so the control still announces its purpose).
inline const wchar_t* AccessibleName() { return L"Show file extensions"; }

// State wording for assistive tech / the live tooltip. The switch has no
// visible text, so screen readers rely on this to know whether extensions are
// currently shown.
inline const wchar_t* StateWord(bool on) { return on ? L"on" : L"off"; }

// Full accessible label for the custom-drawn switch: its purpose plus the
// current state, e.g. "Show file extensions (on)". fix thse isseue.md P2
// (10249-10250): the live build wires THIS string to a real tooltip
// (TTM_UPDATETIPTEXT) instead of computing an accessible name and discarding it.
inline std::wstring AccessibleLabel(bool on) {
    std::wstring s = AccessibleName();
    s += L" (";
    s += StateWord(on);
    s += L")";
    return s;
}

// Switch geometry (unit-agnostic; the renderer passes device px). The switch is
// a 2:1 rounded "pill" track; the knob is a circle inset by `inset` on all
// sides and sits at the LEFT when OFF and the RIGHT when ON. No space is ever
// reserved for a text label.
inline int SwitchTrackW(int trackH) { return trackH * 2; }
inline int SwitchKnobDiameter(int trackH, int inset) {
    const int d = trackH - inset * 2;
    return d < 1 ? 1 : d;
}
inline int SwitchKnobLeft(int trackLeft, int trackW, int trackH, int inset,
                          bool on) {
    const int d = SwitchKnobDiameter(trackH, inset);
    return on ? (trackLeft + trackW - inset - d) : (trackLeft + inset);
}

}  // namespace showext
