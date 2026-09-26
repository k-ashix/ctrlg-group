// shortcut_snapshot_test.cpp
// Issue #3: the configurable shortcut is read on the LL keyboard-hook thread
//   while LoadSettings writes it on another thread. A multi-field struct write
//   is not atomic, so the hook can read a torn ShortcutConfig. The fix packs
//   the config into ONE 64-bit word the hook reads with a single atomic load;
//   PackShortcut / UnpackShortcut must round-trip losslessly.
// Issue #4: the F2-rename fallback waited only for Ctrl/Alt/Shift/N/F to be
//   released before injecting F2. For a configured shortcut like Alt+F2 the
//   trigger key (F2) itself was never waited for. ShortcutReleaseWaitVks must
//   return the standard modifiers PLUS every key in the configured shortcut.
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <cstdio>
#include <vector>

static int g_fail = 0;
static void ok(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}
static bool contains(const std::vector<int>& v, int x) {
    for (int e : v) if (e == x) return true;
    return false;
}

// Win32 VK values (kept literal so the pure test needs no <windows.h>).
static const int VK_CTRL_  = 0x11;
static const int VK_ALT_   = 0x12;
static const int VK_SHIFT_ = 0x10;
static const int VK_F_     = 0x46;
static const int VK_N_     = 0x4E;
static const int VK_F2_    = 0x71;
static const int VK_LWIN_  = 0x5B;

int main() {
    // --- #3 round-trip: default Ctrl+F+N -------------------------------
    {
        int held[4] = { VK_CTRL_, VK_F_, 0, 0 };
        ShortcutSnapshot s = UnpackShortcut(PackShortcut(VK_N_, held, 2));
        ok(s.triggerVk == VK_N_,  "roundtrip: triggerVk = N");
        ok(s.heldCount == 2,      "roundtrip: heldCount = 2");
        ok(s.heldVks[0] == VK_CTRL_ && s.heldVks[1] == VK_F_, "roundtrip: held Ctrl,F");
    }
    // --- #3 round-trip: Alt+F2 -----------------------------------------
    {
        int held[4] = { VK_ALT_, 0, 0, 0 };
        ShortcutSnapshot s = UnpackShortcut(PackShortcut(VK_F2_, held, 1));
        ok(s.triggerVk == VK_F2_, "roundtrip: triggerVk = F2");
        ok(s.heldCount == 1,      "roundtrip: heldCount = 1");
        ok(s.heldVks[0] == VK_ALT_, "roundtrip: held Alt");
    }
    // --- #3 round-trip: four held keys ---------------------------------
    {
        int held[4] = { VK_CTRL_, VK_SHIFT_, VK_ALT_, VK_LWIN_ };
        ShortcutSnapshot s = UnpackShortcut(PackShortcut(VK_N_, held, 4));
        ok(s.heldCount == 4, "roundtrip: heldCount = 4");
        bool allOk = true;
        for (int i = 0; i < 4; ++i) if (s.heldVks[i] != held[i]) allOk = false;
        ok(allOk, "roundtrip: all four held keys preserved");
    }

    // --- #4 release-wait set: Ctrl+F+N ---------------------------------
    {
        int held[4] = { VK_CTRL_, VK_F_, 0, 0 };
        std::vector<int> v = ShortcutReleaseWaitVks(VK_N_, held, 2);
        ok(contains(v, VK_CTRL_),  "wait set includes Ctrl");
        ok(contains(v, VK_ALT_),   "wait set includes Alt (standard modifier)");
        ok(contains(v, VK_SHIFT_), "wait set includes Shift (standard modifier)");
        ok(contains(v, VK_F_),     "wait set includes F (held)");
        ok(contains(v, VK_N_),     "wait set includes N (trigger)");
    }
    // --- #4: Alt+F2 must wait for F2 (the original bug) ----------------
    {
        int held[4] = { VK_ALT_, 0, 0, 0 };
        std::vector<int> v = ShortcutReleaseWaitVks(VK_F2_, held, 1);
        ok(contains(v, VK_F2_), "Alt+F2: wait set includes the F2 trigger");
        ok(contains(v, VK_ALT_), "Alt+F2: wait set includes Alt");
    }
    // --- #4: no duplicate when a shortcut key IS a standard modifier ----
    {
        int held[4] = { VK_CTRL_, 0, 0, 0 };
        std::vector<int> v = ShortcutReleaseWaitVks(VK_N_, held, 1);
        int ctrlCount = 0;
        for (int e : v) if (e == VK_CTRL_) ++ctrlCount;
        ok(ctrlCount == 1, "Ctrl appears exactly once (deduped)");
    }

    if (g_fail == 0) { printf("shortcut_snapshot_test: PASS\n"); return 0; }
    printf("shortcut_snapshot_test: FAIL (%d)\n", g_fail);
    return 1;
}
