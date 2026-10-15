// auto_group_countdown_logic.h
//
// PURE state model for the Auto Group button's EIGHT-SECOND safety delay
// (fix thse isseue.md ADD-ON section 2 + Phase C). Mirrored byte-for-byte into
// ctrlg-group.wh.cpp. No Win32: the model takes a MONOTONIC tick value (the mod
// feeds it GetTickCount64()) and the fixed start tick, so every boundary is
// deterministically unit-testable. The model NEVER executes the operation --
// expiry only flips the button to Enabled; the user must click it.
//
#pragma once

#include <string>
#include <cstdint>

namespace agcd {

// Button lifecycle while a Ctrl+G popup is open.
enum class State {
    Unavailable,  // Auto Group disabled in settings -> button not shown/active
    Waiting,      // within the 5s safety delay -> disabled, shows a countdown
    Enabled,      // 8s elapsed -> clickable "Auto Group"
};

// Fixed safety delay in milliseconds. This is a RULE, not a user setting.
inline uint64_t DelayMs() { return 8000; }

// Total whole seconds in the countdown (ceil of DelayMs / 1000) == 8.
inline int TotalSeconds() { return 8; }

// Resolve the state from the monotonic clock. `enabledSetting` is the
// autoGroupEnabled flag. `startTick` is the tick captured when THIS popup
// opened; `nowTick` is the current monotonic tick. Using (now - start) against
// a monotonic source means a stalled/busy message loop or delayed WM_TIMER can
// never unlock early or late -- the elapsed time is recomputed from the clock,
// not from a count of ticks received.
inline State Resolve(bool enabledSetting, uint64_t startTick, uint64_t nowTick) {
    if (!enabledSetting) return State::Unavailable;
    // Guard against a clock that appears to go backwards (never on a monotonic
    // source, but keep the model total): treat now < start as "just opened".
    const uint64_t elapsed = (nowTick >= startTick) ? (nowTick - startTick) : 0;
    return (elapsed >= DelayMs()) ? State::Enabled : State::Waiting;
}

// Remaining milliseconds until the button unlocks (0 once enabled/unavailable).
inline uint64_t RemainingMs(bool enabledSetting, uint64_t startTick, uint64_t nowTick) {
    if (!enabledSetting) return 0;
    const uint64_t elapsed = (nowTick >= startTick) ? (nowTick - startTick) : 0;
    return (elapsed >= DelayMs()) ? 0 : (DelayMs() - elapsed);
}

// Whole seconds shown in the "Auto Group (N)" countdown: 8,7,6,5,4,3,2,1. Computed as
// ceil(remaining / 1000) so the label reads "8" for the whole first second and
// only reaches "1" during the final second, never showing "0".
inline int RemainingSeconds(bool enabledSetting, uint64_t startTick, uint64_t nowTick) {
    const uint64_t rem = RemainingMs(enabledSetting, startTick, nowTick);
    if (rem == 0) return 0;
    return static_cast<int>((rem + 999) / 1000);
}

// Compose the simple accessible button LABEL. Waiting -> "Auto Group (N)" where
// N is the whole-second countdown; Enabled -> "Auto Group" (no number); the
// disabled/unavailable case is not shown so returns the plain label too.
inline std::wstring Label(State s, int remainingSeconds) {
    if (s == State::Waiting) {
        return L"Auto Group (" + std::to_wstring(remainingSeconds) + L")";
    }
    return L"Auto Group";
}

// Compose the STABLE countdown shown inside the button during the delay: a
// minutes:seconds clock (M:SS, seconds zero-padded) with NO milliseconds, e.g.
// 8s remaining -> "0:08", 1s -> "0:01". Driven by the whole-second
// RemainingSeconds value so the text changes at most once per second (no
// flicker, no millisecond suffix).
inline std::wstring CountdownText(int remainingSeconds) {
    if (remainingSeconds < 0) remainingSeconds = 0;
    const int m = remainingSeconds / 60;
    const int s = remainingSeconds % 60;
    std::wstring ss = std::to_wstring(s);
    if (ss.size() < 2) ss = L"0" + ss;
    return std::to_wstring(m) + L":" + ss;
}

// fix thse isseue.md Phase 4: the repaint timer must invalidate the Auto Group
// button ONLY when the VISIBLE whole-second countdown OR the enabled/disabled
// state actually changes -- never once per tick (that per-tick erase/repaint
// was the flashing cause). The WM_TIMER handler calls this before any
// InvalidateRect, so the anti-flicker contract is unit-tested, not inlined.
inline bool ShouldRepaint(int curSec, int lastSec, bool curEnabled,
                          bool lastEnabled) {
    return curSec != lastSec || curEnabled != lastEnabled;
}

// Whether the button accepts a click right now. ONLY Enabled is clickable --
// Waiting and Unavailable are not, so the countdown can never run the op early
// and a disabled-in-settings button can never run at all.
inline bool Clickable(State s) { return s == State::Enabled; }

// Whether the button is mid-countdown (the stable M:SS timer is shown and the
// button stays disabled). Only true while Waiting.
inline bool ShowOverlay(State s) { return s == State::Waiting; }

// Whether the button participates in the layout / is shown at all. Hidden only
// when disabled in settings.
inline bool Visible(State s) { return s != State::Unavailable; }

}  // namespace agcd
