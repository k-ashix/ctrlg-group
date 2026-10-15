// chip_nav_logic.h
//
// PURE result-navigation model for the Ctrl+G summary chips (fix thse isseue.md
// Phase 5 + ADD-ON "Click-to-Find and Cycle Through Matching Items"). A summary
// chip is BOTH a filter and a find-navigator: the first click on an extension
// chip reveals the first matching item, and clicking the already-active chip
// again reveals the next match, wrapping at the end.
//
// This model does NOT duplicate the filtering rules. It delegates the per-row
// match decision to chipfilter::RowFor (the single source of truth) and the
// item<->row mapping to viewmode::TopRowForItem, so List and the two grid modes
// stay consistent with the live renderer. The navigation CURSOR is a STABLE
// ITEM INDEX kept SEPARATELY from the active filter, so it survives a view-mode
// change (only the derived visual row changes with the column count). Nothing
// here reorders items, touches the real Explorer selection, or changes the
// Copy/Move plan. Mirrored byte-for-byte into ctrlg-group.wh.cpp
// (namespace chipnav).
//
#pragma once

#include <string>
#include <vector>

#include "chip_filter_logic.h"
#include "view_mode_logic.h"

namespace chipnav {

// One selected row as the navigator sees it: just enough to run the shared
// chipfilter match rule. `extKey` is the normalized lowercase extension key
// ("" for folders and extensionless files); `isFolder` marks a directory.
struct Item {
    std::wstring extKey;
    bool isFolder;
};

// The ordered list of item indices that MATCH the active chip filter, in the
// ORIGINAL item order (never reordered). Delegates to chipfilter::RowFor so the
// find-navigator and the row highlighter always agree on what "matches". When
// no highlight filter is active (None / Items) nothing matches, so the
// navigator stays idle for those chips.
inline std::vector<int> Matches(const chipfilter::State& s,
                                const std::vector<Item>& items) {
    std::vector<int> out;
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        if (chipfilter::RowFor(s, items[i].extKey, items[i].isFolder)
            == chipfilter::Row::Matched) {
            out.push_back(i);
        }
    }
    return out;
}

// The next current-match item index.
//   matches : ordered matching item indices (from Matches()).
//   curItem : the current match's item index, or -1 when there is none.
//   reclick : true ONLY when the SAME already-active chip was clicked again.
// Rules:
//   - no matches                      -> -1 (empty results never jump).
//   - fresh activation (reclick=false)-> the FIRST match.
//   - reclick with a live cursor      -> the NEXT match, wrapping at the end.
//   - reclick with a stale cursor     -> the FIRST match.
inline int NextMatch(const std::vector<int>& matches, int curItem, bool reclick) {
    if (matches.empty()) return -1;
    if (!reclick) return matches.front();
    int pos = -1;
    for (int i = 0; i < static_cast<int>(matches.size()); ++i) {
        if (matches[i] == curItem) { pos = i; break; }
    }
    if (pos < 0) return matches.front();
    return matches[(pos + 1) % static_cast<int>(matches.size())];
}

// The MINIMAL new top row that reveals the item at `itemIndex`, given the
// current scroll (`topRow`) and how many whole rows are visible (`visibleRows`).
// The item index is mapped to its VISUAL row with viewmode::TopRowForItem using
// the ACTUAL column count, so List (cols==1) and the grid modes are handled by
// one path. If the target row is already visible the scroll is unchanged (so
// adjacent matches do not make the viewport jump); otherwise it scrolls just
// far enough -- up to the target, or down so the target becomes the last
// visible row -- then clamps to the valid range. itemIndex < 0 (empty results)
// leaves the scroll untouched.
inline int RevealTopRow(viewmode::Mode mode, int itemIndex, int itemCount,
                        int cols, int topRow, int visibleRows) {
    if (itemIndex < 0) return topRow;
    if (visibleRows < 1) visibleRows = 1;
    if (cols < 1) cols = 1;

    const int target = viewmode::TopRowForItem(mode, itemIndex, cols);
    const int totalRows = (mode == viewmode::Mode::List)
                              ? itemCount
                              : viewmode::GridRows(itemCount, cols);
    int maxTop = totalRows - visibleRows;
    if (maxTop < 0) maxTop = 0;

    int newTop = topRow;
    if (target < topRow) {
        newTop = target;                          // scroll up to the match
    } else if (target > topRow + visibleRows - 1) {
        newTop = target - visibleRows + 1;        // scroll down just enough
    }                                             // else: already visible

    if (newTop < 0) newTop = 0;
    if (newTop > maxTop) newTop = maxTop;
    return newTop;
}

// Is the item at `itemIndex` the CURRENT match? This is the extra visual state
// layered on top of chipfilter::Row::Matched: every matching row stays Matched
// (normal strength), and additionally the one current-match row gets a subtle
// accent so the user can see where they are without losing the other matches.
// A cursor of -1 means nothing is current.
inline bool IsCurrentMatch(int itemIndex, int curItem) {
    return curItem >= 0 && itemIndex == curItem;
}

}  // namespace chipnav
