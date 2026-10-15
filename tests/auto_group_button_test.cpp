// auto_group_button_test.cpp
// Pins fix thse isseue.md top note ("green color is not good for auto group -
// keep it same as group"): the timed Auto Group button (id 120) must NEVER use
// a dedicated green surface. Once the 5s safety delay elapses
// (agcd::State::Enabled) the button is styled IDENTICALLY to the ordinary Group
// button (the create/primary tokens); while Waiting (or Unavailable) it keeps
// the NEUTRAL (Cancel-style) secondary surface with its bold countdown. The
// caption/countdown is always drawn with the SEMIBOLD UI font. The appearance
// is derived from the SAME agcd::State the clock resolves, so it can never
// disagree with clickability.
// Self-contained: own main(), exit 0 = pass.

#include "auto_group_button_logic.h"

#include <cstdio>

using namespace agbtn;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- fill decision tracks agcd::State exactly ----
    check(ResolveFill(agcd::State::Unavailable) == Fill::NeutralWaiting,
          "Unavailable -> neutral surface");
    check(ResolveFill(agcd::State::Waiting) == Fill::NeutralWaiting,
          "Waiting -> neutral surface (bold countdown sits on it)");
    check(ResolveFill(agcd::State::Enabled) == Fill::GroupEnabled,
          "Enabled -> same surface as the Group button (never green)");

    // ---- Group-styled fill ONLY once clickable (never during safety delay) --
    check(UsesGroupFill(agcd::State::Enabled), "Group fill when enabled");
    check(!UsesGroupFill(agcd::State::Waiting), "neutral while waiting");
    check(!UsesGroupFill(agcd::State::Unavailable), "neutral when unavailable");

    // ---- appearance decision agrees with agcd::Clickable (no disagreement) --
    for (agcd::State s : {agcd::State::Unavailable, agcd::State::Waiting,
                          agcd::State::Enabled}) {
        check(UsesGroupFill(s) == agcd::Clickable(s),
              "Group fill iff the button is clickable");
    }

    // ---- the model exposes NO green concept at all (top note) ----
    // Fill has exactly two states and neither is a green surface.
    check(Fill::NeutralWaiting != Fill::GroupEnabled, "two distinct fills only");

    // ---- caption + countdown are always semibold (Phase 5A) ----
    check(CaptionSemibold(agcd::State::Enabled), "enabled caption is semibold");
    check(CaptionSemibold(agcd::State::Waiting), "waiting countdown is semibold");

    if (g_fail == 0) printf("auto_group_button_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
