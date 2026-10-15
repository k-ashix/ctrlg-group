// auto_group_button_logic.h
//
// PURE appearance model for the timed Auto Group button (id 120). Mirrored
// byte-for-byte into ctrlg-group.wh.cpp (namespace agbtn). No Win32.
//
// fix thse isseue.md top note ("green color is not good for auto group - keep
// it same as group"): the Auto Group button must NEVER use a dedicated green
// surface. Once the 5s safety delay elapses (agcd::State::Enabled) the button
// is styled IDENTICALLY to the ordinary Group button -- the same create /
// primary palette tokens. While Waiting / Unavailable it keeps the NEUTRAL
// (Cancel-style) secondary surface the bold M:SS countdown sits on. The
// caption / countdown is ALWAYS drawn with the SEMIBOLD UI font. The decision
// is derived from the SAME agcd::State the monotonic clock resolves, so
// appearance and clickability can never disagree.
//
#pragma once
#include "auto_group_countdown_logic.h"

namespace agbtn {

// GroupEnabled == the exact Group-button surface (create/primary tokens).
enum class Fill { NeutralWaiting, GroupEnabled };

// Enabled (safety delay elapsed) -> Group-button surface; Waiting/Unavailable
// -> neutral Cancel-style surface the countdown sits on.
inline Fill ResolveFill(agcd::State s) {
    return s == agcd::State::Enabled ? Fill::GroupEnabled : Fill::NeutralWaiting;
}

// True once the button is clickable: it is then painted with the SAME tokens as
// the Group button (no green). False while the safety delay counts down.
inline bool UsesGroupFill(agcd::State s) {
    return ResolveFill(s) == Fill::GroupEnabled;
}

// Phase 5A lists Auto Group among the semibold action labels, and the countdown
// was already bold -- so BOTH states draw their text with the semibold UI font.
inline bool CaptionSemibold(agcd::State) { return true; }

}  // namespace agbtn
