// group_popup_test.cpp
// Task 8: pure popup-summary formatting for the Ctrl+G Group Selection popup.
// The popup reuses the existing WhNewFolderHybridPrompt window; these pure
// functions decide the TEXT it shows so the Win32 side can mirror them without
// any shell/Win32 dependency. Self-contained: exit 0 = pass. RED before the
// popup-summary functions exist in group_logic.h.

#include "group_logic.h"

#include <string>
#include <vector>
#include <cstdio>

static int g_fail = 0;
static void expectEqW(const std::wstring& got, const std::wstring& want,
                      const char* what) {
    if (got != want) {
        printf("[FAIL] %s\n  got : %ls\n  want: %ls\n",
               what, got.c_str(), want.c_str());
        ++g_fail;
    }
}
static void expectTrue(bool c, const char* what) {
    if (!c) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- 1. Count line: 0 / 1 / many (plural handling) ------------------
    expectEqW(FormatGroupCountLine(0), L"No items selected",
              "count line: zero");
    expectEqW(FormatGroupCountLine(1), L"1 item selected",
              "count line: one (singular)");
    expectEqW(FormatGroupCountLine(5), L"5 items selected",
              "count line: many (plural)");

    // ---- 2. Item-name list with truncation / overflow -------------------
    {
        // Under the cap: every name shown, newline-joined, no overflow line.
        std::vector<std::wstring> names = { L"a.jpg", L"b.jpg", L"sub" };
        expectEqW(FormatGroupItemList(names, 5),
                  L"a.jpg\nb.jpg\nsub",
                  "item list: all shown under cap");
    }
    {
        // Over the cap: first `maxShown` names, then a "+N more" overflow line.
        std::vector<std::wstring> names = {
            L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8",
        };
        expectEqW(FormatGroupItemList(names, 5),
                  L"1\n2\n3\n4\n5\n+3 more",
                  "item list: truncated with +N more");
    }
    {
        // Exactly at the cap: no overflow line.
        std::vector<std::wstring> names = { L"1", L"2", L"3" };
        expectEqW(FormatGroupItemList(names, 3),
                  L"1\n2\n3",
                  "item list: exactly at cap, no overflow");
    }
    {
        // Empty selection -> empty list text.
        std::vector<std::wstring> names;
        expectTrue(FormatGroupItemList(names, 5).empty(),
                   "item list: empty selection -> empty");
    }

    // ---- 3. Move/Copy label (Copy is the DEFAULT) -----------------------
    expectEqW(FormatGroupOperationLabel(/*isMove*/false), L"Copy",
              "operation label: copy is default");
    expectEqW(FormatGroupOperationLabel(/*isMove*/true), L"Move",
              "operation label: move");

    // ---- 4. Destination display -----------------------------------------
    expectEqW(FormatGroupDestinationLine(L"C:\\shoot\\Prewedding"),
              L"Destination: C:\\shoot\\Prewedding",
              "destination line: shows resolved path");
    expectEqW(FormatGroupDestinationLine(L""),
              L"Destination: (choose a folder name)",
              "destination line: blank dest -> prompt to type a name");

    // ---- 5. Blank-name -> last-target display ---------------------------
    // When the user leaves the name blank, the popup shows the last target it
    // will reuse. When there is no last target, it says none is remembered.
    expectEqW(FormatGroupLastTargetLine(L"C:\\shoot\\Prewedding"),
              L"Last target: C:\\shoot\\Prewedding",
              "last-target line: shows remembered target");
    expectEqW(FormatGroupLastTargetLine(L""),
              L"Last target: none",
              "last-target line: none remembered");

    // ---- 6. Effective destination preview (typed vs blank->last) --------
    // Mirrors ResolveGroupTarget: a typed name joins under the common parent;
    // a blank name reuses the last target verbatim.
    expectEqW(FormatGroupEffectiveDestination(L"Prewedding", L"C:\\shoot",
                                              L"C:\\shoot\\Old"),
              L"C:\\shoot\\Prewedding",
              "effective dest: typed name under parent");
    expectEqW(FormatGroupEffectiveDestination(L"   ", L"C:\\shoot",
                                              L"C:\\shoot\\Old"),
              L"C:\\shoot\\Old",
              "effective dest: blank name reuses last target");
    expectTrue(FormatGroupEffectiveDestination(L"", L"C:\\shoot", L"").empty(),
               "effective dest: blank name + no last target -> empty");

    if (g_fail == 0) { printf("group_popup_test: PASS\n"); return 0; }
    printf("group_popup_test: FAIL (%d)\n", g_fail);
    return 1;
}
