// chip_filter_logic.h
//
// PURE state model for chip-click filtering / row highlighting (fix thse
// isseue.md #6). At most ONE extension chip is active at a time. Clicking a
// chip switches the active extension; clicking the already-active chip keeps it
// active; clicking OUTSIDE the strip is the explicit reset. Highlighting is
// non-destructive: matching rows are highlighted, the rest are subdued but keep
// their position and the real selection is untouched. Mirrored byte-for-byte
// into ctrlg-group.wh.cpp.
#pragma once

#include <string>

namespace chipfilter {

enum class Row { Normal, Matched, Subdued };

// Which chip is the single active filter. fix thse isseue.md #2: besides the
// per-extension chips, the Items chip restores the all-items view, the Folders
// chip highlights folder rows, and the No-extension chip highlights
// extensionless files. At most ONE is active at a time.
enum class Kind { None, Items, Folders, NoExt, Ext };

struct State {
    std::wstring activeKey;   // normalized lowercase ext key when kind==Ext; else empty
    Kind kind = Kind::None;
    // "Active" means a highlight is applied: None and the Items (all-items
    // view) chip are NOT highlighting states.
    bool active() const { return kind != Kind::None && kind != Kind::Items; }
};

// The Items chip: restore the all-items view (no row is subdued).
inline void ActivateItems(State& s) { s.kind = Kind::Items; s.activeKey.clear(); }

// The Folders chip: highlight the selected folder rows.
inline void ActivateFolders(State& s) { s.kind = Kind::Folders; s.activeKey.clear(); }

// The No-extension chip: highlight files that have no extension.
inline void ActivateNoExt(State& s) { s.kind = Kind::NoExt; s.activeKey.clear(); }

// Click an extension chip. A non-empty key becomes the single active ext
// filter, replacing any previous one; clicking the active key again leaves it
// active. An empty key is not an extension filter and does nothing.
inline void ActivateExt(State& s, const std::wstring& key) {
    if (key.empty()) return;
    s.kind = Kind::Ext;
    s.activeKey = key;
}

// Backward-compatible shorthand: Activate(key) == ActivateExt(key).
inline void Activate(State& s, const std::wstring& key) { ActivateExt(s, key); }

// The explicit reset: a deliberate click outside the chip strip.
inline void ClickOutside(State& s) {
    s.kind = Kind::None;
    s.activeKey.clear();
}

// Per-row highlight state. Normal for every row when no highlight is active
// (None / Items); otherwise the rows that match the active chip are Matched and
// every other row is Subdued (never reordered or removed).
inline Row RowFor(const State& s, const std::wstring& rowExtKey, bool isFolder) {
    switch (s.kind) {
        case Kind::None:
        case Kind::Items:   return Row::Normal;
        case Kind::Folders: return isFolder ? Row::Matched : Row::Subdued;
        case Kind::NoExt:
            return (!isFolder && rowExtKey.empty()) ? Row::Matched : Row::Subdued;
        case Kind::Ext:
            return (!isFolder && rowExtKey == s.activeKey) ? Row::Matched
                                                           : Row::Subdued;
    }
    return Row::Normal;
}

// fix thse isseue.md Phase 2A: the ACTIVE chip is indicated ONLY by a dotted
// outline (the existing focus/selection styling), never by a second accent fill
// overlay -- the chip keeps its muted/per-ext fill in every state. A chip draws
// the dotted outline when it is the active filter OR the keyboard-focused chip.
// Keyboard focus and active-filter state stay logically distinct (two inputs)
// even though they share the one compatible dotted-outline style.
inline bool ChipShowsDottedOutline(bool isActiveFilter, bool isFocused) {
    return isActiveFilter || isFocused;
}

}  // namespace chipfilter
