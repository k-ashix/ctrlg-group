// shortcut_options_test.cpp
// Task B: Ctrl+Q and Ctrl+G are single-press shortcut options on top of the
//   existing set. Each parses to {triggerVk:'Q'/'G', heldVks[0]=Ctrl,
//   heldCount:1}. Verify these pack/unpack round-trip losslessly via the same
//   PackShortcut/UnpackShortcut the LL hook uses for its atomic snapshot.
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <cstdio>

static int g_fail = 0;
static void ok(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

static const int VK_CTRL_ = 0x11;
static const int VK_Q_    = 'Q';
static const int VK_G_    = 'G';

int main() {
    // --- Ctrl+Q: single held modifier (Ctrl), trigger Q ---------------
    {
        int held[4] = { VK_CTRL_, 0, 0, 0 };
        ShortcutSnapshot s = UnpackShortcut(PackShortcut(VK_Q_, held, 1));
        ok(s.triggerVk == VK_Q_,   "Ctrl+Q roundtrip: triggerVk = Q");
        ok(s.heldCount == 1,       "Ctrl+Q roundtrip: heldCount = 1");
        ok(s.heldVks[0] == VK_CTRL_, "Ctrl+Q roundtrip: held Ctrl");
    }
    // --- Ctrl+G: single held modifier (Ctrl), trigger G ---------------
    {
        int held[4] = { VK_CTRL_, 0, 0, 0 };
        ShortcutSnapshot s = UnpackShortcut(PackShortcut(VK_G_, held, 1));
        ok(s.triggerVk == VK_G_,   "Ctrl+G roundtrip: triggerVk = G");
        ok(s.heldCount == 1,       "Ctrl+G roundtrip: heldCount = 1");
        ok(s.heldVks[0] == VK_CTRL_, "Ctrl+G roundtrip: held Ctrl");
    }

    if (g_fail == 0) { printf("shortcut_options_test: PASS\n"); return 0; }
    printf("shortcut_options_test: FAIL (%d)\n", g_fail);
    return 1;
}
