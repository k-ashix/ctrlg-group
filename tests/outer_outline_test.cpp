// outer_outline_test.cpp
// Pins the PURE "unwanted outer outline" decision (fix thse isseue.md P0 #2):
//   * the custom borderless Group popup drops WS_EX_DLGMODALFRAME, so no raised
//     3-D modal frame is painted around the whole window (the stray bright
//     outer perimeter the issue reports);
//   * the native-caption New Folder popup keeps WS_EX_DLGMODALFRAME (unchanged);
//   * both popups always keep WS_EX_TOPMOST;
//   * there is ONE shared ex-style helper so PromptOuterSize() and
//     CreateWindowExW() can never assume different frames.
//
// Self-contained: own main(), exit 0 = pass. RED before outer_outline_logic.h
// exists; GREEN once it mirrors the mod.

#include "outer_outline_logic.h"

#include <cstdio>

using namespace outeroutline;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- the Group popup drops the raised modal frame ----
    check(!HasDlgModalFrame(PromptExStyle(/*isGroup=*/true)),
          "Group popup has NO WS_EX_DLGMODALFRAME (no outer outline)");
    check(!DrawsOuterModalFrame(/*isGroup=*/true),
          "Group popup draws no outer modal frame");

    // ---- the New Folder popup is unchanged (keeps its native modal frame) ----
    check(HasDlgModalFrame(PromptExStyle(/*isGroup=*/false)),
          "New Folder popup keeps WS_EX_DLGMODALFRAME (unchanged)");
    check(DrawsOuterModalFrame(/*isGroup=*/false),
          "New Folder popup still draws its modal frame");

    // ---- both popups stay topmost ----
    check((PromptExStyle(true)  & kExTopmost) != 0UL, "Group popup is WS_EX_TOPMOST");
    check((PromptExStyle(false) & kExTopmost) != 0UL, "New Folder popup is WS_EX_TOPMOST");

    // ---- the shared helper differs ONLY by the modal-frame bit ----
    check((PromptExStyle(false) ^ PromptExStyle(true)) == kExDlgModalFrame,
          "Group vs New Folder ex-style differ only by WS_EX_DLGMODALFRAME");

    // ---- the pinned bit values match the real winuser.h constants ----
    check(kExDlgModalFrame == 0x00000001UL, "WS_EX_DLGMODALFRAME value");
    check(kExTopmost       == 0x00000008UL, "WS_EX_TOPMOST value");

    if (g_fail) { printf("outer_outline_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("outer_outline_test: OK\n");
    return 0;
}
