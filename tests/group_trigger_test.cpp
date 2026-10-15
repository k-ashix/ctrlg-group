// group_trigger_test.cpp
// Task 5a: ClassifyTrigger -- the pure gate that decides whether a keystroke is
// the dedicated Ctrl+G "Group Selection" gesture. The whole point of the
// feature: Ctrl+G is a hardwired Group key, gated by an on/off toggle, and it
// can NEVER be routed to New Folder. New Folder keeps its own configured
// shortcut, which is a separate decision the mod already owns.
//
// Self-contained: exit 0 = pass. RED before ClassifyTrigger exists.

#include "group_logic.h"

#include <cstdio>

static int g_fail = 0;
static void expectKind(TriggerKind got, TriggerKind want, const char* what) {
    if (got != want) {
        printf("[FAIL] %s (got %d want %d)\n", what, (int)got, (int)want);
        ++g_fail;
    }
}

// 'G' virtual-key code (VK 0x47). Mirrors the mod; kept local so the pure test
// never needs Win32.
static const int kVkG = 0x47;
static const int kVkNKey = 0x4E;

int main() {
    // 1. Ctrl+G alone, toggle ON -> the Group gesture.
    expectKind(ClassifyTrigger(kVkG, /*ctrlDown*/true, /*otherModifierDown*/false,
                               /*groupEnabled*/true),
               TriggerKind::GroupSelection, "Ctrl+G with toggle on groups");

    // 2. Ctrl+G, toggle OFF -> nothing (user disabled the feature).
    expectKind(ClassifyTrigger(kVkG, true, false, /*groupEnabled*/false),
               TriggerKind::None, "Ctrl+G with toggle off does nothing");

    // 3. G without Ctrl -> nothing (bare typing must never trigger grouping).
    expectKind(ClassifyTrigger(kVkG, /*ctrlDown*/false, false, true),
               TriggerKind::None, "bare G does not group");

    // 4. Ctrl+Shift+G (an extra modifier) -> nothing. The gesture is exactly
    //    Ctrl+G; a decorated chord is not the dedicated Group key.
    expectKind(ClassifyTrigger(kVkG, true, /*otherModifierDown*/true, true),
               TriggerKind::None, "Ctrl+Shift+G is not the Group gesture");

    // 5. Some other Ctrl+key (e.g. Ctrl+N) is NOT the Group gesture. New Folder
    //    is decided elsewhere; ClassifyTrigger only owns the Group gate, so a
    //    non-G key returns None here and can never be mistaken for Group.
    expectKind(ClassifyTrigger(kVkNKey, true, false, true),
               TriggerKind::None, "Ctrl+N is not the Group gesture");

    // 6. Invariant: Ctrl+G never yields NewFolder under ANY toggle/modifier mix.
    for (int ge = 0; ge <= 1; ++ge) {
        for (int om = 0; om <= 1; ++om) {
            for (int cd = 0; cd <= 1; ++cd) {
                if (ClassifyTrigger(kVkG, cd != 0, om != 0, ge != 0)
                        == TriggerKind::NewFolder) {
                    printf("[FAIL] Ctrl+G routed to NewFolder (ge=%d om=%d cd=%d)\n",
                           ge, om, cd);
                    ++g_fail;
                }
            }
        }
    }

    if (g_fail == 0) { printf("group_trigger_test: PASS\n"); return 0; }
    printf("group_trigger_test: FAIL (%d)\n", g_fail);
    return 1;
}
