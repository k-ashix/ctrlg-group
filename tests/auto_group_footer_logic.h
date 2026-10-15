// auto_group_footer_logic.h
//
// PURE geometry for the Ctrl+G footer button ROW once the Auto Group button is
// added (fix thse isseue.md ADD-ON Phase D). The mod lays the row out in
// layout::ComputePromptLayout; this mirrors ONLY the ordering arithmetic so the
// invariant "[Auto Group] [Group] [Cancel]", right-aligned and non-overlapping,
// is unit-testable without the Win32 layout engine.
//
#pragma once

namespace agfooter {

struct Box { int x = 0; int w = 0; };

struct Row {
    Box autoGroup;
    Box group;   // "Group"/"Create" primary button
    Box cancel;
};

// Reproduces the mod's footer math: Cancel is right-aligned at the content
// edge, Group sits one gap to its left, and the Auto Group button sits one gap
// to the LEFT of Group with its own (wider) width. windowW/outerMargin/btnW/gap
// and autoW match the mod constants.
inline Row Compute(int windowW, int outerMargin, int btnW, int gap, int autoW) {
    Row r;
    const int cancelX = windowW - outerMargin - btnW;
    const int createX = cancelX - btnW - gap;
    const int autoX = createX - gap - autoW;
    r.cancel = {cancelX, btnW};
    r.group = {createX, btnW};
    r.autoGroup = {autoX, autoW};
    return r;
}

// Order invariant: autoGroup is left of group, group is left of cancel.
inline bool OrderOk(const Row& r) {
    return r.autoGroup.x < r.group.x && r.group.x < r.cancel.x;
}

// No two adjacent buttons overlap (each left edge + width ends before the next).
inline bool NoOverlap(const Row& r) {
    return (r.autoGroup.x + r.autoGroup.w) <= r.group.x &&
           (r.group.x + r.group.w) <= r.cancel.x;
}

// Everything fits inside the content area [outerMargin, windowW - outerMargin].
inline bool FitsInside(const Row& r, int windowW, int outerMargin) {
    return r.autoGroup.x >= outerMargin &&
           (r.cancel.x + r.cancel.w) <= (windowW - outerMargin);
}

}  // namespace agfooter
