// button_state_logic_test.cpp
// Pins the PURE Create/Cancel button feedback state machine (waht to create.md
// #10/#11/#12, plan Phase 6):
//   * mouse Create and Enter resolve to the SAME action; mouse Cancel and Esc
//     resolve to the SAME action (one shared path);
//   * Create is accent/primary and must NOT flash green before success is
//     actually known (Processing stays accent);
//   * Cancel is neutral and is NEVER permanently red -- red appears only in the
//     Cancelled state; failure is red on Create, cancellation is not a failure;
//   * a duplicate Create while one is already processing is ignored.
//
// Self-contained: own main(), exit 0 = pass. RED before button_state_logic.h
// exists; GREEN once it mirrors the mod.

#include "button_state_logic.h"

#include <cstdio>

using namespace btn;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- mouse == keyboard: one shared action path ----
    check(ActionFor(Input::MouseCreate) == Action::Create, "MouseCreate -> Create");
    check(ActionFor(Input::KeyEnter) == Action::Create, "Enter -> Create");
    check(ActionFor(Input::MouseCreate) == ActionFor(Input::KeyEnter),
          "mouse Create == Enter");
    check(ActionFor(Input::MouseCancel) == Action::Cancel, "MouseCancel -> Cancel");
    check(ActionFor(Input::KeyEsc) == Action::Cancel, "Esc -> Cancel");
    check(ActionFor(Input::MouseCancel) == ActionFor(Input::KeyEsc),
          "mouse Cancel == Esc");

    // ---- Create tone: accent primary, green ONLY on real success ----
    check(CreateTone(Phase::Idle) == Tone::Accent, "Create idle accent");
    check(CreateTone(Phase::Processing) == Tone::Accent, "Create processing stays accent");
    check(CreateTone(Phase::Processing) != Tone::Green, "Create does NOT flash green early");
    check(CreateTone(Phase::Success) == Tone::Green, "Create success green");
    check(CreateTone(Phase::Warning) == Tone::Amber, "Create warning amber");
    check(CreateTone(Phase::Failure) == Tone::Red, "Create failure red");
    check(CreateTone(Phase::Cancelled) == Tone::Accent, "Create returns to accent on cancel");

    // ---- Cancel tone: neutral, NEVER permanently red ----
    check(CancelTone(Phase::Idle) == Tone::Neutral, "Cancel idle neutral");
    check(CancelTone(Phase::Processing) == Tone::Neutral, "Cancel processing neutral");
    check(CancelTone(Phase::Success) == Tone::Neutral, "Cancel success neutral");
    check(CancelTone(Phase::Failure) == Tone::Neutral, "Cancel failure neutral");
    check(CancelTone(Phase::Cancelled) == Tone::Red, "Cancel red ONLY when cancelled");

    // ---- PRESS feedback tone (fix thse isseue.md #2/#5) ----
    // A TRANSIENT colour flashed while a button is physically pressed, separate
    // from the result-phase tone above: Create + the Copy(-path) button flash
    // the confirm GREEN (the counterpart to the Cancel/Close red); buttons with
    // no special press meaning stay neutral.
    check(PressTone(Button::Create) == Tone::Green, "Create press feedback is green");
    check(PressTone(Button::Copy)   == Tone::Green, "Copy press feedback is green");
    check(PressTone(Button::Cancel) == Tone::Red,   "Cancel press feedback is red");
    check(PressTone(Button::Close)  == Tone::Red,   "Close press feedback is red");
    check(PressTone(Button::Other)  == Tone::Neutral, "other buttons: no special press tone");
    // Press-green is NOT the phase tone: an idle Create is still accent and only
    // flashes green WHILE pressed.
    check(CreateTone(Phase::Idle) == Tone::Accent && PressTone(Button::Create) == Tone::Green,
          "press-green is separate from the idle accent phase tone");

    // ---- enable/disable: no duplicate submission while processing ----
    check(CreateEnabled(Phase::Idle), "Create enabled when idle");
    check(!CreateEnabled(Phase::Processing), "Create disabled while processing");

    // ---- duplicate-submission guard ----
    check(AcceptCreate(Phase::Idle), "first Create accepted from Idle");
    check(!AcceptCreate(Phase::Processing), "second Create ignored while processing");
    check(OnCreate(Phase::Idle) == Phase::Processing, "Idle -> Processing on Create");
    check(OnCreate(Phase::Processing) == Phase::Processing, "duplicate leaves Processing");

    // ---- cancel is a cancellation, not a failure ----
    check(OnCancel(Phase::Idle) == Phase::Cancelled, "cancel from idle -> Cancelled");
    check(OnCancel(Phase::Processing) == Phase::Cancelled, "cancel while processing -> Cancelled");
    check(IsCancelled(Phase::Cancelled) && !IsFailure(Phase::Cancelled),
          "cancelled is not a failure");
    check(IsFailure(Phase::Failure) && !IsCancelled(Phase::Failure),
          "failure is not a cancellation");

    if (g_fail) { printf("button_state_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("button_state_logic_test: OK\n");
    return 0;
}
