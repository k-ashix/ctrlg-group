// copy_move_keynav_test.cpp
// Pins fix thse isseue.md Phase D: Copy/Move segmented keyboard navigation.
//   * Left/Right change the selection ONLY while the segment owns focus.
//   * Tab / Enter / Space are NOT consumed by the segment key handler, so Tab
//     navigates normally (no focus trap, no accidental op) and Enter/Space fall
//     through to the focused segment's normal button activation (which selects
//     the segment and returns 0 -- no popup submit; pinned in the regression
//     gate against the id 107/108 WM_COMMAND handler).
//   * Mouse click and keyboard produce the SAME segment (one result).
//   * d->isMove (FromIsMove/ToIsMove) stays the single source of truth; the
//     decision never depends on any animation progress.
// Self-contained: own main(), exit 0 = pass.

#include "segmented_logic.h"

#include <cstdio>

using namespace segmented;

static const int kVkTab   = 0x09;
static const int kVkEnter = 0x0D;
static const int kVkSpace = 0x20;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- Left/Right change the selection ONLY while focused ----
    { SegKeyResult r = OnKey(Seg::Move, kVkLeft, true);
      check(r.consumed && r.seg == Seg::Copy, "focused Left -> Copy (consumed)"); }
    { SegKeyResult r = OnKey(Seg::Copy, kVkRight, true);
      check(r.consumed && r.seg == Seg::Move, "focused Right -> Move (consumed)"); }
    { SegKeyResult r = OnKey(Seg::Copy, kVkLeft, false);
      check(!r.consumed && r.seg == Seg::Copy, "unfocused arrows NOT consumed (EDIT keeps caret)"); }
    { SegKeyResult r = OnKey(Seg::Move, kVkRight, false);
      check(!r.consumed && r.seg == Seg::Move, "unfocused Right NOT consumed"); }

    // ---- Tab / Enter / Space are never consumed by the segment handler ----
    for (int vk : {kVkTab, kVkEnter, kVkSpace}) {
        SegKeyResult rf = OnKey(Seg::Copy, vk, true);
        check(!rf.consumed, "Tab/Enter/Space not consumed even when focused (normal activation)");
        SegKeyResult ru = OnKey(Seg::Copy, vk, false);
        check(!ru.consumed, "Tab/Enter/Space not consumed when unfocused");
    }

    // ---- repeated arrows keep toggling predictably ----
    { Seg s = Seg::Copy;
      s = OnKey(s, kVkRight, true).seg;  check(s == Seg::Move, "Copy + Right -> Move");
      s = OnKey(s, kVkLeft,  true).seg;  check(s == Seg::Copy, "Move + Left  -> Copy");
      s = OnKey(s, kVkLeft,  true).seg;  check(s == Seg::Copy, "Copy + Left stays Copy"); }

    // ---- mouse click and keyboard agree (one result) ----
    check(OnClickSegment(Seg::Copy) == OnKey(Seg::Move, kVkLeft, true).seg,
          "click Copy == focused Left");
    check(OnClickSegment(Seg::Move) == OnKey(Seg::Copy, kVkRight, true).seg,
          "click Move == focused Right");

    // ---- d->isMove is the single source of truth (round-trip) ----
    check(FromIsMove(false) == Seg::Copy && FromIsMove(true) == Seg::Move,
          "isMove maps to the segment");
    check(ToIsMove(Seg::Copy) == false && ToIsMove(Seg::Move) == true,
          "segment maps back to isMove");
    check(ToIsMove(OnClickSegment(Seg::Move)) == true, "selecting Move sets isMove=true");

    // ---- C/M mnemonics mirror the arrows, gated by focus ----
    check(Mnemonic(Seg::Move, 'C', true).consumed &&
          Mnemonic(Seg::Move, 'C', true).seg == Seg::Copy, "focused 'C' -> Copy");
    check(!Mnemonic(Seg::Copy, 'C', false).consumed,
          "unfocused 'C' NOT consumed (typed into the folder-name EDIT)");

    if (g_fail == 0) printf("copy_move_keynav: ALL PASS\n");
    return g_fail ? 1 : 0;
}
