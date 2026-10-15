// button_state_logic.h
//
// PURE Create/Cancel button feedback state machine (waht to create.md
// #10/#11/#12), mirrored from ctrlg-group.wh.cpp so it can be
// unit tested with a plain g++ build (no Win32 runtime). The mod keeps a
// byte-for-byte copy and drives the owner-drawn Create/Cancel buttons + the
// duplicate-submission guard from these functions.
//
// Rules:
//   * mouse Create and Enter map to ONE action; mouse Cancel and Esc map to ONE
//     action (a single shared code path);
//   * Create is the accent/primary button and must NOT flash green before a
//     real success -- Processing stays accent;
//   * Cancel is neutral and is NEVER permanently red; red communicates the
//     Cancelled state (and failure is red on Create); cancellation is not a
//     failure;
//   * a duplicate Create while one is in flight is ignored.
//
#pragma once

namespace btn {

enum class Phase { Idle, Processing, Success, Warning, Failure, Cancelled };
enum class Tone { Accent, Neutral, Green, Amber, Red };
enum class Input { MouseCreate, KeyEnter, MouseCancel, KeyEsc };
enum class Action { Create, Cancel, None };

// Mouse and keyboard collapse onto the same action, so both drive one path.
inline Action ActionFor(Input in) {
    switch (in) {
        case Input::MouseCreate:
        case Input::KeyEnter:   return Action::Create;
        case Input::MouseCancel:
        case Input::KeyEsc:     return Action::Cancel;
        default:                return Action::None;
    }
}

// Create button tone. Accent/primary while idle and processing (NEVER green
// until success is actually known), then green/amber/red for the terminal
// result. A cancellation is not a Create failure, so Create returns to accent.
inline Tone CreateTone(Phase p) {
    switch (p) {
        case Phase::Success: return Tone::Green;
        case Phase::Warning: return Tone::Amber;
        case Phase::Failure: return Tone::Red;
        case Phase::Idle:
        case Phase::Processing:
        case Phase::Cancelled:
        default:             return Tone::Accent;
    }
}

// Cancel button tone. Neutral everywhere EXCEPT the Cancelled state, so Cancel
// is never permanently red.
inline Tone CancelTone(Phase p) {
    return p == Phase::Cancelled ? Tone::Red : Tone::Neutral;
}

// Create is disabled only while Processing, preventing a duplicate submission.
inline bool CreateEnabled(Phase p) { return p != Phase::Processing; }

// A Create activation is accepted only from Idle (the duplicate-submission
// guard). Applying it moves Idle -> Processing; any other phase is unchanged.
inline bool AcceptCreate(Phase current) { return current == Phase::Idle; }
inline Phase OnCreate(Phase current) {
    return current == Phase::Idle ? Phase::Processing : current;
}

// Cancellation always lands in Cancelled from any phase and is NOT a failure.
inline Phase OnCancel(Phase /*current*/) { return Phase::Cancelled; }

inline bool IsFailure(Phase p) { return p == Phase::Failure; }
inline bool IsCancelled(Phase p) { return p == Phase::Cancelled; }

// ---- Press feedback (fix thse isseue.md #2/#5) ----
// While a button is physically held down (mouse-down, or the keyboard
// equivalent), it flashes a SEMANTIC colour: the confirm GREEN for the Create
// and the Copy(-path) buttons, the cancel RED for Cancel/Close. This is a
// TRANSIENT press cue only -- independent of CreateTone()/CancelTone() above,
// which describe the result phase (Create is still accent while idle; it only
// flashes green WHILE pressed). Buttons with no special press meaning stay
// neutral.
enum class Button { Create, Copy, Cancel, Close, Other };
inline Tone PressTone(Button b) {
    switch (b) {
        case Button::Create:
        case Button::Copy:   return Tone::Green;
        case Button::Cancel:
        case Button::Close:  return Tone::Red;
        case Button::Other:
        default:             return Tone::Neutral;
    }
}

}  // namespace btn
