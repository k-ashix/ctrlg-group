// quick_action_logic.h
//
// PURE state model for fix thse isseue.md Phase 4: the OPT-IN two-stroke
// "Ctrl+G then C/M" quick Copy/Move shortcut. Mirrored byte-for-byte into
// ctrlg-group.wh.cpp. No Win32 here: the model takes a MONOTONIC tick so every
// boundary is deterministically unit-testable, and it is designed to run inside
// the low-level keyboard hook WITHOUT ever blocking (no Sleep, no wait) -- the
// hook simply feeds it key events and acts on the returned decision.
//
// Safety contract (why this is a state machine, not an inline hack):
//   - Arming happens ONLY when the feature is enabled AND a remembered VALID
//     target exists; otherwise Ctrl+G opens the ordinary popup immediately.
//   - A quick action fires ONLY on an explicit C/M inside a bounded window.
//   - Any other key, or a timeout, opens the ordinary popup and NEVER consumes
//     (loses) that key.
//   - Cancel / modifier-release / unload / a stale delayed event disarm with NO
//     action, so a quick action can never fire by accident or twice.
//
#pragma once

#include <cstdint>

namespace qseq {

// What the hook should do with the current event.
enum class Action {
    None,        // nothing yet (armed and waiting, or not our event)
    OpenPopup,   // open the ordinary Ctrl+G Group popup
    QuickCopy,   // perform Copy to the remembered target (worker validates it)
    QuickMove,   // perform Move to the remembered target (worker validates it)
};

// The classified second keystroke after Ctrl+G.
enum class Key { Other, C, M };

enum class Phase { Idle, Armed };

// Bounded two-stroke window. Short enough that it never feels like a hang, long
// enough for a deliberate second keystroke. A RULE, not a user setting.
inline uint64_t WindowMs() { return 1500; }

struct State {
    Phase phase = Phase::Idle;
    uint64_t armedTick = 0;
};

struct Decision {
    Action action = Action::None;
    State next;
    bool consumeKey = false;   // true => the hook swallows this key (don't pass it on)
};

inline bool Armed(const State& s) { return s.phase == Phase::Armed; }

inline bool Expired(const State& s, uint64_t nowTick) {
    if (s.phase != Phase::Armed) return false;
    const uint64_t elapsed = (nowTick >= s.armedTick) ? (nowTick - s.armedTick) : 0;
    return elapsed >= WindowMs();
}

// Ctrl+G was pressed. With the feature off or no remembered valid target, open
// the popup immediately (unchanged behaviour). Otherwise ARM the window and
// consume the Ctrl+G so the popup does not also open.
inline Decision OnCtrlG(bool enabled, bool hasValidTarget, uint64_t nowTick) {
    Decision d;
    if (!enabled || !hasValidTarget) {
        d.action = Action::OpenPopup;   // next stays Idle
        return d;
    }
    d.next.phase = Phase::Armed;
    d.next.armedTick = nowTick;
    d.action = Action::None;
    d.consumeKey = true;                // Ctrl+G handled; wait for C/M
    return d;
}

// A key arrived. Only meaningful while Armed. C/M inside the window fire the
// quick action (and consume the key); any other key, or an expired window,
// opens the popup and PRESERVES the key (never consumed). Firing always returns
// to Idle so a stale repeat can never fire twice.
inline Decision OnKey(const State& s, Key key, uint64_t nowTick) {
    Decision d;
    if (s.phase != Phase::Armed) {
        return d;                       // not our event: None, Idle, not consumed
    }
    if (Expired(s, nowTick)) {
        d.action = Action::OpenPopup;   // too late -> ordinary popup, key preserved
        return d;
    }
    if (key == Key::C) { d.action = Action::QuickCopy; d.consumeKey = true; return d; }
    if (key == Key::M) { d.action = Action::QuickMove; d.consumeKey = true; return d; }
    d.action = Action::OpenPopup;       // some other key -> popup, key preserved
    return d;
}

// The repaint/arming timer ticked. Once the window elapses with no command key,
// open the ordinary popup; before that, keep waiting.
inline Decision OnTimeout(const State& s, uint64_t nowTick) {
    Decision d;
    if (s.phase == Phase::Armed && Expired(s, nowTick)) {
        d.action = Action::OpenPopup;   // next stays Idle
        return d;
    }
    d.next = s;                         // still armed (or already idle): no change
    return d;
}

// Explicit cancel, modifier release, mod unload, or a stale delayed message:
// disarm with NO action so a quick op can never launch by accident.
inline Decision Reset(const State& /*s*/) {
    return Decision{};                  // None, Idle, not consumed
}

}  // namespace qseq
