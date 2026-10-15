// popup_default_test.cpp
//
// Ctrl+F+N naming offers BOTH modes and ships INLINE by default: the folder
// opens in Explorer's native inline rename box (feels native), and the user can
// switch to the themed popup via the "Name folders with a popup box" toggle.
// This pins that shipped default to OFF (inline) so a future edit can't
// silently force popup-first on the Ctrl+F+N path.
//
// NOTE: this is ONLY about the Ctrl+F+N New-Folder naming. Ctrl+G Group
// Selection has its own dedicated popup that is always used and is unrelated to
// this toggle.
//
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <cstdio>

int main() {
    // The popup toggle ships OFF: Ctrl+F+N uses the native inline rename box by
    // default. Both modes stay available; the user opts into the popup.
    if (kUsePopupDefaultOn != false) {
        std::printf("[FAIL] Ctrl+F+N popup (useDialog) default must be OFF "
                    "(inline); both modes must remain available\n");
        std::printf("popup_default_test: FAIL\n");
        return 1;
    }
    std::printf("popup_default_test: PASS\n");
    return 0;
}
