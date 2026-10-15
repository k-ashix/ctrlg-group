// focus_visible_test.cpp
// Pins the PURE "focus-visible" decision (fix thse isseue.md P0 #1): the custom
// focus ring on the owner-drawn buttons (Create / Cancel / Copy / Move / title-
// bar) must appear ONLY when focus was moved by the KEYBOARD, never on a mouse
// click. The mod keeps a byte-for-byte copy of focusvis:: and gates the ring
// paint with it; this test owns the decision so a regression can't slip past
// the clang-only owner-draw.
//
// Self-contained: own main(), exit 0 = pass. RED before focus_visible_logic.h
// exists; GREEN once the header mirrors the mod.

#include "focus_visible_logic.h"

#include <cstdio>

using namespace focusvis;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- input source -> focus-visible ----
    check(FromInput(Input::KeyboardNav) == true,  "keyboard nav makes focus visible");
    check(FromInput(Input::PointerPress) == false, "pointer press hides focus ring");

    // ---- the ring gate: focus AND focus-visible ----
    check(ShouldShowRing(true,  true)  == true,  "focused + keyboard -> ring");
    check(ShouldShowRing(true,  false) == false, "focused by mouse -> NO ring (the #1 bug)");
    check(ShouldShowRing(false, true)  == false, "not focused -> no ring");
    check(ShouldShowRing(false, false) == false, "not focused + mouse -> no ring");

    // ---- navigation-key classification (keyboard focus) ----
    check(IsNavKey(0x09), "Tab is a nav key");
    check(IsNavKey(0x25), "Left is a nav key");
    check(IsNavKey(0x26), "Up is a nav key");
    check(IsNavKey(0x27), "Right is a nav key");
    check(IsNavKey(0x28), "Down is a nav key");
    check(IsNavKey(0x0D), "Enter is a nav key");
    check(IsNavKey(0x20), "Space is a nav key");
    check(IsNavKey(0x21), "PageUp is a nav key");
    check(IsNavKey(0x22), "PageDown is a nav key");
    check(IsNavKey(0x24), "Home is a nav key");
    check(IsNavKey(0x23), "End is a nav key");
    check(!IsNavKey('A'), "a letter is NOT a nav key");
    check(!IsNavKey('M'), "'M' is NOT a nav key (segment mnemonic stays caret)");
    check(!IsNavKey(0x00), "null vk is not a nav key");

    // ---- the review's exact scenario: click, then keyboard ----
    // Fresh popup: focus ring starts hidden.
    bool focusVisible = false;
    check(ShouldShowRing(true, focusVisible) == false, "fresh focus shows no ring until keyboard");
    // User clicks a button (pointer press): still hidden even though it has focus.
    focusVisible = FromInput(Input::PointerPress);
    check(ShouldShowRing(true, focusVisible) == false, "after click: focused button draws no ring");
    // User presses Tab/arrow (keyboard nav): now the ring is shown.
    focusVisible = FromInput(Input::KeyboardNav);
    check(ShouldShowRing(true, focusVisible) == true, "after keyboard nav: ring returns");

    if (g_fail) { printf("focus_visible_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("focus_visible_test: OK\n");
    return 0;
}
