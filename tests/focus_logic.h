// focus_logic.h
//
// PURE focus-ring + tab-order model (waht to create.md #13), mirrored from
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build (no Win32 runtime). The mod keeps a byte-for-byte copy: it draws the
// focus ring on the owner-drawn controls and relies on the same logical order
// the Win32 dialog manager walks with Tab/Shift+Tab.
//
// The ring is a REAL outline (DPI-scaled thickness + inset), so keyboard focus
// is visible without relying on color alone, and it survives Tab/Shift+Tab/
// mouse/activation/resize/DPI because it is recomputed from these values.
//
#pragma once

namespace focus {

enum class Ctl { Name, OpToggle, Create, Cancel };

// Initial focus is always the name field.
inline Ctl First() { return Ctl::Name; }

// Tab order. Group includes the Copy/Move toggle; New Folder skips it. Wraps.
inline Ctl Next(Ctl c, bool isGroup) {
    switch (c) {
        case Ctl::Name:     return isGroup ? Ctl::OpToggle : Ctl::Create;
        case Ctl::OpToggle: return Ctl::Create;
        case Ctl::Create:   return Ctl::Cancel;
        case Ctl::Cancel:   return Ctl::Name;
    }
    return Ctl::Name;
}

// Shift+Tab: the reverse walk, wrapping.
inline Ctl Prev(Ctl c, bool isGroup) {
    switch (c) {
        case Ctl::Name:     return Ctl::Cancel;
        case Ctl::OpToggle: return Ctl::Name;
        case Ctl::Create:   return isGroup ? Ctl::OpToggle : Ctl::Name;
        case Ctl::Cancel:   return Ctl::Create;
    }
    return Ctl::Name;
}

// Every interactive control still carries a focus STATE. fix thse isseue.md #4
// removed the painted focus OUTLINE in favour of a focus-COLOUR change on the
// control, so the ring geometry helpers (thickness/inset/scale) are gone --
// see focusvis:: for the keyboard-focus-visible gate that drives the colour.
inline bool ShowsFocusRing(Ctl) { return true; }

}  // namespace focus
