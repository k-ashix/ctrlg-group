// mode_logic.h
//
// PURE compact/expanded mode model for the popup (waht to create.md #15),
// mirrored from ctrlg-group.wh.cpp so it can be unit tested with
// a plain g++ build (no Win32 runtime). The mod keeps a byte-for-byte copy: it
// remembers the mode for the current session, picks how many Selected Items
// rows to show, and feeds `expanded` into layout::ComputePromptLayout.
//
// Expanded is the SAME interface with more space (a taller Selected Items
// list), not a second UI. A large selection auto-opens expanded so more of it
// is visible without the user resizing first.
//
#pragma once

#include <cstddef>

namespace mode {

enum class Mode { Compact, Expanded };

inline Mode Toggle(Mode m) {
    return m == Mode::Compact ? Mode::Expanded : Mode::Compact;
}

inline bool IsExpanded(Mode m) { return m == Mode::Expanded; }

// Selected Items rows visible in each mode. Compact keeps a usable viewport;
// expanded shows strictly more.
inline int CompactRows()  { return 4; }
inline int ExpandedRows() { return 9; }

inline int VisibleRows(Mode m) {
    return m == Mode::Expanded ? ExpandedRows() : CompactRows();
}

// Rows actually drawn for a selection of `itemCount` in mode `m`: the per-mode
// cap, but never more rows than there are items (fix thse isseue.md #1 -- the
// Selected Items list is content-sized, so 5 items show 5 rows, not a 9-row
// viewport). The layout uses this to size the list card to its content.
inline int VisibleRowsFor(size_t itemCount, Mode m) {
    const int cap = VisibleRows(m);
    return static_cast<int>(itemCount) < cap ? static_cast<int>(itemCount) : cap;
}

// Auto-pick a mode for a selection: expand as soon as the selection would
// overflow the compact viewport, so a large group opens with more visible.
inline Mode AutoMode(size_t itemCount) {
    return itemCount > static_cast<size_t>(CompactRows())
               ? Mode::Expanded
               : Mode::Compact;
}

// The mode a fresh popup should open in (waht to create.md #15 / #11): once the
// user has picked a size this session, reuse that remembered mode; otherwise
// fall back to auto-expand for a large selection. Auto-expand is only the
// INITIAL default, never the sole mechanism -- the titlebar toggle (and Ctrl+E)
// let the user override it, and the override is what gets remembered.
inline Mode InitialMode(bool hasSessionMode, Mode sessionMode,
                        size_t itemCount) {
    return hasSessionMode ? sessionMode : AutoMode(itemCount);
}

}  // namespace mode
