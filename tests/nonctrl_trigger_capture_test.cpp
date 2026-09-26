// nonctrl_trigger_capture_test.cpp
// REPRODUCTION + FIX TEST for wh(8) "existing issue still present":
//   The trigger supports user shortcuts Ctrl+Shift+N, Alt+F2, Win+N, Ctrl+F+N,
//   but count/template CAPTURE was hard-coded around CTRL being held.
//
// Root cause (verified against code): after a trigger fired, the capture loop
// decided whether the gesture was still alive by sampling ONLY
// GetAsyncKeyState(VK_CONTROL) and feeding that to CountDecide as `ctrlHeld`.
// With shortcut = Alt+F2, Ctrl is never down, so every follow-up key looked
// like "modifier released" -> the loop committed the default single folder
// before a batch count or a template letter could be captured.
//
// The FIX generalises the hold-modifier from "always Ctrl" to "whatever
// modifier the configured shortcut holds" (IsGestureModifierHeld over the
// shortcut's held keys). CountDecide is then driven with the RESULT of that,
// so an Alt+F2 gesture with Alt still down keeps capturing.
//
// Self-contained: own main(), exit 0 == pass.

#include "newfolder_logic.h"

#include <cstdio>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    const bool multi = true;      // multiFolder = ON
    const int  maxSetting = 100;

    // ---- Alt+F2 shortcut: held = [Alt], trigger = F2 ---------------------
    const int altHeld[1] = { kVkAlt };
    const int altHeldCount = 1;

    // --- 1) The hold-modifier for an Alt+F2 gesture is ALT, not CTRL ------
    // The fix picks the modifier from the shortcut. When Alt is physically
    // down (and Ctrl is not), the capture window must be considered ALIVE.
    auto onlyAltDown = [](int vk) { return vk == kVkAlt; };
    check(IsGestureModifierHeld(altHeld, altHeldCount, onlyAltDown),
          "Alt+F2: window stays alive while Alt is held (not gated on Ctrl)");

    // Releasing Alt (nothing down) ends the window, same as releasing Ctrl did.
    auto nothingDown = [](int) { return false; };
    check(!IsGestureModifierHeld(altHeld, altHeldCount, nothingDown),
          "Alt+F2: releasing Alt ends the capture window");

    // Old-world bug reproduction: if we only ever sampled Ctrl, Alt+F2 would
    // report "released" even though Alt is down. Confirm the modifier set does
    // NOT include Ctrl for this shortcut, so a Ctrl-only sample can't stand in.
    auto onlyCtrlDown = [](int vk) { return vk == kVkControl; };
    check(!IsGestureModifierHeld(altHeld, altHeldCount, onlyCtrlDown),
          "Alt+F2: a Ctrl-only sample must NOT keep the Alt gesture alive");

    // --- 2) With the correct held state, a batch count accumulates --------
    // Drive CountDecide the way the FIXED loop does: modifierHeld = result of
    // IsGestureModifierHeld (true while Alt is down).
    const bool modHeld = IsGestureModifierHeld(altHeld, altHeldCount, onlyAltDown);

    CountState st;
    st.active = true;

    auto r1 = CountDecide(st, CountKey::Digit, 1, multi, maxSetting, modHeld);
    check(r1.outcome == CountOutcome::Continue,
          "Alt+F2 batch: first digit CONTINUES capture (Alt still held)");
    check(r1.next.active && r1.next.value == 1,
          "Alt+F2 batch: first digit accumulates (value=1), window stays open");

    auto r2 = CountDecide(r1.next, CountKey::Digit, 2, multi, maxSetting, modHeld);
    check(r2.next.value == 12,
          "Alt+F2 batch: second digit accumulates to 12");

    // Releasing Alt now commits the typed count of 12 (not the default 1).
    const bool released =
        IsGestureModifierHeld(altHeld, altHeldCount, nothingDown);
    auto rCommit =
        CountDecide(r2.next, CountKey::Other, 0, multi, maxSetting, released);
    check(rCommit.outcome == CountOutcome::Commit && rCommit.count == 12,
          "Alt+F2 batch: releasing Alt commits the typed count 12");

    // --- 3) Ctrl+F+N still behaves exactly as before (no regression) ------
    const int ctrlHeldKeys[1] = { kVkControl };
    auto ctrlDown = [](int vk) { return vk == kVkControl; };
    check(IsGestureModifierHeld(ctrlHeldKeys, 1, ctrlDown),
          "Ctrl+F+N: window alive while Ctrl held (unchanged)");
    check(!IsGestureModifierHeld(ctrlHeldKeys, 1, nothingDown),
          "Ctrl+F+N: releasing Ctrl ends the window (unchanged)");

    // --- 4) Multi-modifier shortcut: ALL held keys must stay down ---------
    // Ctrl+Shift+N holds BOTH Ctrl AND Shift. The gesture spec is "every
    // preceding key remains held", so the capture window must survive ONLY
    // while ALL configured hold-modifiers are down -- releasing EITHER ends it.
    // (Bug being fixed: an OR check kept the window alive when just one of the
    // two modifiers was still down.)
    const int ctrlShift[2] = { kVkControl, kVkShift };
    const int ctrlShiftCount = 2;

    auto bothDown  = [](int vk) { return vk == kVkControl || vk == kVkShift; };
    auto onlyCtrl  = [](int vk) { return vk == kVkControl; };  // Shift released
    auto onlyShift = [](int vk) { return vk == kVkShift; };    // Ctrl released

    check(IsGestureModifierHeld(ctrlShift, ctrlShiftCount, bothDown),
          "Ctrl+Shift+N: window alive while BOTH Ctrl and Shift are held");
    check(!IsGestureModifierHeld(ctrlShift, ctrlShiftCount, onlyCtrl),
          "Ctrl+Shift+N: releasing Shift (Ctrl still down) ENDS the window");
    check(!IsGestureModifierHeld(ctrlShift, ctrlShiftCount, onlyShift),
          "Ctrl+Shift+N: releasing Ctrl (Shift still down) ENDS the window");
    check(!IsGestureModifierHeld(ctrlShift, ctrlShiftCount, nothingDown),
          "Ctrl+Shift+N: releasing both ENDS the window");

    if (g_fail) {
        printf("nonctrl_trigger_capture_test: %d FAIL\n", g_fail);
        return 1;
    }
    printf("nonctrl_trigger_capture_test: PASS\n");
    return 0;
}
