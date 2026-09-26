// naming_pattern_test.cpp
// Selectable batch naming pattern. The user chooses HOW the batch number is
// formatted; every pattern still skips existing folders (collision-resolution)
// so a batch never clobbers and always yields the requested count.
//
// Patterns (mirror of the namingPattern setting in the mod):
//   Space  : "<base> 1", "<base> 2", ...      (default; unchanged behaviour)
//   Dash2  : "<base>-01", "<base>-02", ...    (dash + 2-digit zero-pad)
//   Dash3  : "<base>-001", "<base>-002", ...  (dash + 3-digit zero-pad)
//   Paren2 : "<base> (01)", "<base> (02)", ... (space + parenthesised 2-digit)
//
// Self-contained: own main(), exit 0 = pass.

#include "newfolder_logic.h"

#include <cstdio>
#include <set>
#include <string>

static int g_fail = 0;

static void check(bool cond, const char* what) {
    if (!cond) {
        printf("[FAIL] %s\n", what);
        ++g_fail;
    }
}

int main() {
    const std::wstring dir = L"C:\\tmp";
    std::set<std::wstring> present;
    auto exists = [&](const std::wstring& p) { return present.count(p) != 0; };

    // ---- FormatBatchName renders each pattern for a given index ----
    check(FormatBatchName(L"Project", 1, NamePattern::Space)  == L"Project 1",
          "Space  -> 'Project 1'");
    check(FormatBatchName(L"Project", 2, NamePattern::Dash2)  == L"Project-02",
          "Dash2  -> 'Project-02'");
    check(FormatBatchName(L"Project", 3, NamePattern::Dash3)  == L"Project-003",
          "Dash3  -> 'Project-003'");
    check(FormatBatchName(L"Project", 1, NamePattern::Paren2) == L"Project (01)",
          "Paren2 -> 'Project (01)'");
    check(FormatBatchName(L"New folder", 2, NamePattern::Under2) == L"New folder_02",
          "Under2 -> 'New folder_02'");
    check(FormatBatchName(L"New folder", 3, NamePattern::Under3) == L"New folder_003",
          "Under3 -> 'New folder_003'");
    // Zero-pad widths only pad; they never truncate a larger number.
    check(FormatBatchName(L"P", 100, NamePattern::Dash2) == L"P-100",
          "Dash2 does not truncate 100 -> 'P-100'");
    check(FormatBatchName(L"P", 100, NamePattern::Under2) == L"P_100",
          "Under2 does not truncate 100 -> 'P_100'");

    // ---- Dash3 clean batch of 3 ----
    {
        present.clear();
        auto names = MakeNamedFolderNames(dir, L"Project", 3, exists,
                                          NamePattern::Dash3);
        check(names.size() == 3, "Dash3: 3 requested -> 3 names");
        check(names.size() == 3 && names[0] == L"C:\\tmp\\Project-001", "Dash3[0]");
        check(names.size() == 3 && names[1] == L"C:\\tmp\\Project-002", "Dash3[1]");
        check(names.size() == 3 && names[2] == L"C:\\tmp\\Project-003", "Dash3[2]");
    }

    // ---- Smart numbering continuation: Project-001..003 already exist, ----
    // ---- creating 3 more skips them and yields 004..006 (never clobbers). ----
    {
        present = {
            L"C:\\tmp\\Project-001",
            L"C:\\tmp\\Project-002",
            L"C:\\tmp\\Project-003",
        };
        auto names = MakeNamedFolderNames(dir, L"Project", 3, exists,
                                          NamePattern::Dash3);
        check(names.size() == 3, "Dash3 continuation: still 3 names");
        check(names.size() == 3 && names[0] == L"C:\\tmp\\Project-004",
              "Dash3 continuation skips existing -> 004");
        check(names.size() == 3 && names[1] == L"C:\\tmp\\Project-005", "-> 005");
        check(names.size() == 3 && names[2] == L"C:\\tmp\\Project-006", "-> 006");
    }

    // ---- Skip a hole in the middle (Paren2) ----
    {
        present = { L"C:\\tmp\\Report (02)" };
        auto names = MakeNamedFolderNames(dir, L"Report", 3, exists,
                                          NamePattern::Paren2);
        check(names.size() == 3 && names[0] == L"C:\\tmp\\Report (01)", "Paren2[0]");
        check(names.size() == 3 && names[1] == L"C:\\tmp\\Report (03)",
              "Paren2 skips existing (02) -> (03)");
        check(names.size() == 3 && names[2] == L"C:\\tmp\\Report (04)", "Paren2[2]");
    }

    // ---- Default arg keeps Space behaviour (back-compat with existing tests) ----
    {
        present.clear();
        auto names = MakeNamedFolderNames(dir, L"BIG", 2, exists);
        check(names.size() == 2 && names[0] == L"C:\\tmp\\BIG 1",
              "default pattern is Space -> 'BIG 1'");
        check(names.size() == 2 && names[1] == L"C:\\tmp\\BIG 2", "'BIG 2'");
    }

    // ---- README doc-contract: base "New folder" renders exactly as the ----
    // ---- Settings dropdown + README table promise, for all 4 patterns. ----
    {
        present.clear();
        auto sp = MakeNamedFolderNames(dir, L"New folder", 3, exists,
                                       NamePattern::Space);
        check(sp.size() == 3 && sp[0] == L"C:\\tmp\\New folder 1" &&
              sp[1] == L"C:\\tmp\\New folder 2" && sp[2] == L"C:\\tmp\\New folder 3",
              "doc: Space -> 'New folder 1/2/3'");

        auto d2 = MakeNamedFolderNames(dir, L"New folder", 3, exists,
                                       NamePattern::Dash2);
        check(d2.size() == 3 && d2[0] == L"C:\\tmp\\New folder-01" &&
              d2[1] == L"C:\\tmp\\New folder-02" && d2[2] == L"C:\\tmp\\New folder-03",
              "doc: Dash2 -> 'New folder-01/02/03'");

        auto d3 = MakeNamedFolderNames(dir, L"New folder", 3, exists,
                                       NamePattern::Dash3);
        check(d3.size() == 3 && d3[0] == L"C:\\tmp\\New folder-001" &&
              d3[1] == L"C:\\tmp\\New folder-002" && d3[2] == L"C:\\tmp\\New folder-003",
              "doc: Dash3 -> 'New folder-001/002/003'");

        auto pr = MakeNamedFolderNames(dir, L"New folder", 3, exists,
                                       NamePattern::Paren2);
        check(pr.size() == 3 && pr[0] == L"C:\\tmp\\New folder (01)" &&
              pr[1] == L"C:\\tmp\\New folder (02)" && pr[2] == L"C:\\tmp\\New folder (03)",
              "doc: Paren2 -> 'New folder (01)/(02)/(03)'");
    }

    if (g_fail == 0) {
        printf("[PASS] naming_pattern_test\n");
        return 0;
    }
    printf("[FAIL] naming_pattern_test: %d failure(s)\n", g_fail);
    return 1;
}
