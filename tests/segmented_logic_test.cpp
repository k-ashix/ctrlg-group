// segmented_logic_test.cpp
// Pins the PURE Copy/Move segmented-control state machine + keyboard mapping
// (waht to create.md #7 / #8, plan Phase 4). The control:
//   * initializes from the existing g_groupIsMove setting (Copy is default),
//   * lets the user override it for the current operation only,
//   * maps Left -> Copy and Right -> Move ONLY when the control owns focus,
//   * leaves Left/Right alone when the EDIT owns focus (text caret moves),
//   * and always resolves to the same d->isMove the worker already reads.
//
// Self-contained: own main(), exit 0 = pass. RED before segmented_logic.h
// exists; GREEN once it mirrors the mod.

#include "segmented_logic.h"

#include <cstdio>

using namespace segmented;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- init + round-trip with the existing isMove flag ----
    check(FromIsMove(false) == Seg::Copy, "isMove=false -> Copy (default)");
    check(FromIsMove(true) == Seg::Move, "isMove=true -> Move");
    check(ToIsMove(Seg::Copy) == false, "Copy -> isMove=false");
    check(ToIsMove(Seg::Move) == true, "Move -> isMove=true");
    check(ToIsMove(FromIsMove(true)) == true, "round-trips move");
    check(ToIsMove(FromIsMove(false)) == false, "round-trips copy");

    // ---- labels ----
    check(SegmentLabel(Seg::Copy) == L"Copy", "Copy label");
    check(SegmentLabel(Seg::Move) == L"Move", "Move label");

    // ---- underlined C/M access-key mnemonics (fix thse isseue.md #5 A) ----
    check(MnemonicLabel(Seg::Copy) == L"&Copy", "Copy access-key label underlines C");
    check(MnemonicLabel(Seg::Move) == L"&Move", "Move access-key label underlines M");
    check(MnemonicChar(Seg::Copy) == L'C', "Copy mnemonic char is C");
    check(MnemonicChar(Seg::Move) == L'M', "Move mnemonic char is M");
    // The '&' is a prefix hint only -- the VISIBLE text is exactly the plain
    // label, so measuring SegmentLabel still centres the icon+label group.
    check(MnemonicLabel(Seg::Copy).substr(1) == SegmentLabel(Seg::Copy), "Copy access-key label draws the same visible text");
    check(MnemonicLabel(Seg::Move).substr(1) == SegmentLabel(Seg::Move), "Move access-key label draws the same visible text");

    // ---- keyboard: only acts when the control owns focus ----
    {
        SegKeyResult r = OnKey(Seg::Copy, kVkRight, true);
        check(r.consumed && r.seg == Seg::Move, "Right (focused) -> Move, consumed");
    }
    {
        SegKeyResult r = OnKey(Seg::Move, kVkLeft, true);
        check(r.consumed && r.seg == Seg::Copy, "Left (focused) -> Copy, consumed");
    }
    // Left while already Copy: no change, but still consumed (owns the arrow).
    {
        SegKeyResult r = OnKey(Seg::Copy, kVkLeft, true);
        check(r.consumed && r.seg == Seg::Copy, "Left at Copy stays Copy, consumed");
    }
    {
        SegKeyResult r = OnKey(Seg::Move, kVkRight, true);
        check(r.consumed && r.seg == Seg::Move, "Right at Move stays Move, consumed");
    }
    // A non-arrow key is never consumed by the segmented control.
    {
        SegKeyResult r = OnKey(Seg::Copy, 0x41 /* 'A' */, true);
        check(!r.consumed && r.seg == Seg::Copy, "other key not consumed");
    }

    // ---- keyboard: EDIT owns focus -> arrows are NOT consumed (caret moves) ----
    {
        SegKeyResult r = OnKey(Seg::Copy, kVkRight, false);
        check(!r.consumed && r.seg == Seg::Copy, "Right (edit focused) not consumed");
    }
    {
        SegKeyResult r = OnKey(Seg::Move, kVkLeft, false);
        check(!r.consumed && r.seg == Seg::Move, "Left (edit focused) not consumed");
    }

    // ---- mouse == keyboard: clicking a segment yields the same state ----
    check(OnClickSegment(Seg::Move) == Seg::Move, "click Move -> Move");
    check(OnClickSegment(Seg::Copy) == Seg::Copy, "click Copy -> Copy");
    check(OnClickSegment(Seg::Move) == OnKey(Seg::Copy, kVkRight, true).seg,
          "click Move == Right-from-Copy (mouse == keyboard)");

    // ---- one-pill corner rounding (fix thse isseu e.md #7) ----
    // Copy (left) rounds ONLY its left corners; Move (right) rounds ONLY its
    // right corners; the shared middle boundary is straight on both.
    SegCorners cp = CornersFor(Seg::Copy);
    check(cp.topLeft && cp.bottomLeft, "Copy rounds its LEFT corners");
    check(!cp.topRight && !cp.bottomRight, "Copy keeps its middle (right) edge square");
    SegCorners mv = CornersFor(Seg::Move);
    check(mv.topRight && mv.bottomRight, "Move rounds its RIGHT corners");
    check(!mv.topLeft && !mv.bottomLeft, "Move keeps its middle (left) edge square");
    // Together: exactly the four OUTER corners are rounded, the two middle ones flat.
    int rounded = (cp.topLeft?1:0)+(cp.topRight?1:0)+(cp.bottomRight?1:0)+(cp.bottomLeft?1:0)
                + (mv.topLeft?1:0)+(mv.topRight?1:0)+(mv.bottomRight?1:0)+(mv.bottomLeft?1:0);
    check(rounded == 4, "exactly the four outer corners are rounded (one pill)");

    // ---- segments meet edge-to-edge (shared straight middle boundary) ----
    check(SegmentsAreAdjacent(110, 110), "adjacent when left.right == right.left");
    check(!SegmentsAreAdjacent(110, 112), "a gap breaks the single-pill boundary");

    // ---- C / M mnemonics (fix thse isseu e.md #4/#6) ----
    // C/c selects Copy, M/m selects Move -- but ONLY when a segment owns focus,
    // so the folder-name EDIT keeps C/M as ordinary typed characters (#4: NOT a
    // global hotkey while typing). Any other key, or an unfocused control, is
    // not consumed. Mnemonic shares the SegKeyResult shape OnKey uses so the
    // modal loop treats arrows and letters identically.
    {
        SegKeyResult r = Mnemonic(Seg::Move, 'C', true);
        check(r.consumed && r.seg == Seg::Copy, "C (focused) -> Copy, consumed");
    }
    {
        SegKeyResult r = Mnemonic(Seg::Copy, 'M', true);
        check(r.consumed && r.seg == Seg::Move, "M (focused) -> Move, consumed");
    }
    // Lowercase letters map the same (WM_KEYDOWN delivers the upper VK, but the
    // model accepts either so a direct char path also works).
    {
        SegKeyResult r = Mnemonic(Seg::Move, 'c', true);
        check(r.consumed && r.seg == Seg::Copy, "c (focused) -> Copy, consumed");
    }
    {
        SegKeyResult r = Mnemonic(Seg::Copy, 'm', true);
        check(r.consumed && r.seg == Seg::Move, "m (focused) -> Move, consumed");
    }
    // C while already Copy: no change, but still consumed (the segment owns it).
    {
        SegKeyResult r = Mnemonic(Seg::Copy, 'C', true);
        check(r.consumed && r.seg == Seg::Copy, "C at Copy stays Copy, consumed");
    }
    // A non-C/M key is never consumed by the mnemonic path.
    {
        SegKeyResult r = Mnemonic(Seg::Copy, 'A', true);
        check(!r.consumed && r.seg == Seg::Copy, "other letter not consumed");
    }
    // EDIT owns focus -> C/M are NOT consumed (typed into the name field).
    {
        SegKeyResult r = Mnemonic(Seg::Copy, 'M', false);
        check(!r.consumed && r.seg == Seg::Copy, "M (edit focused) not consumed");
    }
    {
        SegKeyResult r = Mnemonic(Seg::Move, 'C', false);
        check(!r.consumed && r.seg == Seg::Move, "C (edit focused) not consumed");
    }

    if (g_fail) { printf("segmented_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("segmented_logic_test: OK\n");
    return 0;
}
