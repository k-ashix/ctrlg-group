// shortcut_flows_test.cpp
//
// Config-driven trigger behavior across ALL shortcut modes, mirroring the mod's
// LowLevelKeybdProc. One spec, four gestures:
//
//   1. Ctrl+Q / Ctrl+G  -- hold Ctrl, tap Q/G -> FIRE (opens capture window;
//      user may then type a batch count or template letter, or leave it -> one
//      folder). Same flow as Ctrl+Shift+N.
//   2. Ctrl+N           -- single press fires AND is swallowed (no new window).
//   3. Ctrl+N+F         -- tap N (swallowed, no new window) then F -> fire.
//   4. Ctrl+F+N         -- tap F (swallowed, no search) then N -> fire.
//
// Exercises SnapshotTriggerDecide (pure, mirrored). Self-contained.

#include "newfolder_logic.h"

#include <cstdio>
#include <set>

static int g_fail = 0;
static void ok(bool cond, const char* what) {
    if (!cond) { std::printf("[FAIL] %s\n", what); ++g_fail; }
}

// Build a snapshot for "hold <mods>, trigger <triggerVk>".
static ShortcutSnapshot Snap(int triggerVk, std::vector<int> held) {
    ShortcutSnapshot s;
    s.triggerVk = triggerVk;
    s.heldCount = (int)held.size();
    for (int i = 0; i < s.heldCount && i < 4; ++i) s.heldVks[i] = held[i];
    return s;
}

// A keyDown predicate over a set of currently-down vks.
static std::function<bool(int)> Down(std::set<int> down) {
    return [down](int vk) { return down.count(vk) != 0; };
}

static KeyEvent KeyDown(int vk, unsigned long long now, bool fg = true) {
    KeyEvent e; e.vk = vk; e.isDown = true; e.explorerForeground = fg; e.nowMs = now;
    return e;
}

int main() {
    const int Q = 'Q', G = 'G', N = 'N', F = 'F', Ctrl = kVkControl;

    // ---- 1. Ctrl+Q single press: hold Ctrl, tap Q -> fire + swallow --------
    {
        ShortcutSnapshot sc = Snap(Q, {Ctrl});
        TriggerState st;
        TriggerResult r = SnapshotTriggerDecide(sc, st, KeyDown(Q, 100), Down({Ctrl, Q}));
        ok(r.action == TriggerAction::CreateFolder, "Ctrl+Q: tap Q -> CreateFolder (opens capture)");
        ok(r.swallow, "Ctrl+Q: Q is swallowed");
    }

    // ---- 1b. Ctrl+G single press -------------------------------------------
    {
        ShortcutSnapshot sc = Snap(G, {Ctrl});
        TriggerState st;
        TriggerResult r = SnapshotTriggerDecide(sc, st, KeyDown(G, 100), Down({Ctrl, G}));
        ok(r.action == TriggerAction::CreateFolder, "Ctrl+G: tap G -> CreateFolder (opens capture)");
        ok(r.swallow, "Ctrl+G: G is swallowed");
    }

    // ---- 1c. Ctrl+Q must NOT fire if Ctrl is not held (normal Q typing) ----
    {
        ShortcutSnapshot sc = Snap(Q, {Ctrl});
        TriggerState st;
        TriggerResult r = SnapshotTriggerDecide(sc, st, KeyDown(Q, 100), Down({Q}));
        ok(r.action == TriggerAction::None, "bare Q (no Ctrl) -> no fire (normal typing safe)");
        ok(!r.swallow, "bare Q is not swallowed");
    }

    // ---- 2. Ctrl+N single press: fires AND swallows (no new window) --------
    {
        ShortcutSnapshot sc = Snap(N, {Ctrl});
        TriggerState st;
        TriggerResult r = SnapshotTriggerDecide(sc, st, KeyDown(N, 100), Down({Ctrl, N}));
        ok(r.action == TriggerAction::CreateFolder, "Ctrl+N: tap N -> CreateFolder");
        ok(r.swallow, "Ctrl+N: N swallowed -> no new Explorer window");
    }

    // ---- 3. Ctrl+N+F chord: tap N (swallow, no window), then F -> fire -----
    {
        ShortcutSnapshot sc = Snap(F, {Ctrl, N});   // hold Ctrl, tap N, tap F
        TriggerState st;
        // First key: N is the middle chord key -> swallowed, goes pending.
        TriggerResult r1 = SnapshotTriggerDecide(sc, st, KeyDown(N, 100), Down({Ctrl, N}));
        ok(r1.swallow, "Ctrl+N+F: N swallowed -> no new window");
        ok(r1.action == TriggerAction::None, "Ctrl+N+F: N does not create yet (pending)");
        ok(r1.next.pending, "Ctrl+N+F: chord pending after N");
        // Second key: F completes the chord -> fire.
        TriggerResult r2 = SnapshotTriggerDecide(sc, r1.next, KeyDown(F, 200), Down({Ctrl, F}));
        ok(r2.action == TriggerAction::CreateFolder, "Ctrl+N+F: F completes -> CreateFolder");
        ok(r2.swallow, "Ctrl+N+F: F swallowed");
        // No Ctrl+F replay for the N-first chord (only F-first owes search).
        ok(r2.action != TriggerAction::ReplayCtrlF, "Ctrl+N+F: no Ctrl+F replay");
    }

    // ---- 4. Ctrl+F+N chord: tap F (swallow search), then N -> fire ---------
    {
        ShortcutSnapshot sc = Snap(N, {Ctrl, F});   // hold Ctrl, tap F, tap N
        TriggerState st;
        TriggerResult r1 = SnapshotTriggerDecide(sc, st, KeyDown(F, 100), Down({Ctrl, F}));
        ok(r1.swallow, "Ctrl+F+N: F swallowed -> no search opens");
        ok(r1.next.pending, "Ctrl+F+N: chord pending after F");
        TriggerResult r2 = SnapshotTriggerDecide(sc, r1.next, KeyDown(N, 200), Down({Ctrl, N}));
        ok(r2.action == TriggerAction::CreateFolder, "Ctrl+F+N: N completes -> CreateFolder");
        ok(r2.swallow, "Ctrl+F+N: N swallowed");
    }

    // ---- 4b. Ctrl+F+N abandoned (F then a non-N key) replays Ctrl+F --------
    {
        ShortcutSnapshot sc = Snap(N, {Ctrl, F});
        TriggerState st;
        TriggerResult r1 = SnapshotTriggerDecide(sc, st, KeyDown(F, 100), Down({Ctrl, F}));
        // Timeout tick after the chord window elapses -> replay Ctrl+F.
        KeyEvent tick; tick.isTimeout = true; tick.nowMs = 100 + kChordTimeoutMs + 1;
        TriggerResult r2 = SnapshotTriggerDecide(sc, r1.next, tick, Down({}));
        ok(r2.action == TriggerAction::ReplayCtrlF, "Ctrl+F+N: expired chord replays Ctrl+F (search preserved)");
        ok(!r2.next.pending, "Ctrl+F+N: pending cleared after expiry");
    }

    if (g_fail == 0) { std::printf("shortcut_flows_test: PASS\n"); return 0; }
    std::printf("shortcut_flows_test: FAIL (%d)\n", g_fail);
    return 1;
}
