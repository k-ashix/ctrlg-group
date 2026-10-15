// chip_filter_test.cpp
// Pins fix thse isseue.md #6: at most one extension chip is active; clicking a
// chip switches the highlight, clicking the active chip keeps it, and clicking
// outside clears it. Matching rows are Matched, the rest Subdued, nothing is
// reordered or removed. Self-contained: own main(), exit 0 = pass.

#include "chip_filter_logic.h"

#include <cstdio>

using namespace chipfilter;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    State s;
    check(!s.active(), "no filter active initially");
    check(RowFor(s, L"md", false) == Row::Normal, "no filter -> every row normal");
    check(RowFor(s, L"", true) == Row::Normal, "no filter -> folder normal");

    // Activate MD
    Activate(s, L"md");
    check(s.active() && s.activeKey == L"md", "md active");
    check(RowFor(s, L"md", false) == Row::Matched, "md file matched");
    check(RowFor(s, L"cpp", false) == Row::Subdued, "non-md file subdued");
    check(RowFor(s, L"", true) == Row::Subdued, "folder subdued while filtering");

    // Switch to CPP -> only one active
    Activate(s, L"cpp");
    check(s.activeKey == L"cpp", "switched to cpp (single active)");
    check(RowFor(s, L"md", false) == Row::Subdued, "md now subdued");
    check(RowFor(s, L"cpp", false) == Row::Matched, "cpp now matched");

    // Clicking the active chip again keeps it active
    Activate(s, L"cpp");
    check(s.activeKey == L"cpp", "re-click keeps filter active");

    // items/folders chip (empty key) does not change the filter
    Activate(s, L"");
    check(s.activeKey == L"cpp", "empty key is not an ext filter");

    // Click outside clears
    ClickOutside(s);
    check(!s.active(), "click outside clears filter");
    check(RowFor(s, L"cpp", false) == Row::Normal, "all rows normal after clear");

    // ---- fix thse isseue.md #2: Items / Folders / No-extension chips ----
    ActivateItems(s);
    check(!s.active(), "Items chip = all-items view, no highlight");
    check(RowFor(s, L"cpp", false) == Row::Normal, "Items: file normal");
    check(RowFor(s, L"", true) == Row::Normal, "Items: folder normal");

    ActivateFolders(s);
    check(s.active(), "Folders chip is an active highlight");
    check(RowFor(s, L"", true) == Row::Matched, "Folders: folder matched");
    check(RowFor(s, L"cpp", false) == Row::Subdued, "Folders: file subdued");

    ActivateNoExt(s);
    check(s.active(), "No-extension chip is an active highlight");
    check(RowFor(s, L"", false) == Row::Matched, "NoExt: extensionless file matched");
    check(RowFor(s, L"cpp", false) == Row::Subdued, "NoExt: file WITH ext subdued");
    check(RowFor(s, L"", true) == Row::Subdued, "NoExt: folder subdued");

    // Switching to an ext filter still works and stays single-active.
    ActivateExt(s, L"png");
    check(s.kind == Kind::Ext && s.activeKey == L"png", "ext filter via ActivateExt");
    check(RowFor(s, L"png", false) == Row::Matched, "png matched");
    check(RowFor(s, L"", false) == Row::Subdued, "extensionless subdued under ext filter");
    // Clicking Items again restores the all-items view.
    ActivateItems(s);
    check(!s.active() && RowFor(s, L"png", false) == Row::Normal, "Items restores all view");
    ClickOutside(s);
    check(s.kind == Kind::None, "click outside -> None");

    if (g_fail) { printf("chip_filter_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("chip_filter_test: OK\n");
    return 0;
}
