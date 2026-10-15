// section_labels_test.cpp
// Pins the PURE section-wording model for the Ctrl+G group popup
// (fix thse isseu e.md #4/#6, Phase 7/E). This module decides ONLY the wording
// shown for each form section's LABEL and its short supporting description.
// The reference UI wants a light "Folder name" + a one-line helper instead of
// the old heavy "Group into folder name"; the tests pin that exact wording so
// it cannot silently drift.
//
// Self-contained: own main(), exit 0 = pass. RED before section_labels_logic.h
// exists; GREEN once it mirrors the mod.

#include "section_labels_logic.h"

#include <cstdio>

using namespace seclbl;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // Name section: reworded per #6 -- lighter title + a clear one-line helper.
    check(std::wstring(NameLabel()) == L"Folder name",
          "name label is 'Folder name' (not 'Group into folder name')");
    check(std::wstring(NameLabel()) != L"Group into folder name",
          "old heavy name wording is gone");
    check(std::wstring(NameDescription()) == L"Name for the new folder.",
          "name supporting description");

    // The New Folder (non-group) popup keeps its own simpler label.
    check(std::wstring(NewFolderNameLabel()) == L"Enter your folder name",
          "new folder label unchanged");

    // Selected Items section title.
    check(std::wstring(SelectedLabel()) == L"Selected items",
          "selected items section label");

    // fix thse isseue.md #4: the primary button caption is "Group" in the
    // Ctrl+G group popup and "Create" in the New Folder popup. Same IDOK path;
    // only the wording differs. Pin both so neither can silently flip.
    check(std::wstring(SubmitLabel(true)) == L"Group",
          "group popup primary button says 'Group'");
    check(std::wstring(SubmitLabel(false)) == L"Create",
          "new folder popup primary button says 'Create'");
    check(std::wstring(SubmitLabel(true)) != std::wstring(SubmitLabel(false)),
          "the two popup modes use distinct captions");

    // fix thse isseue.md #1: the Status feature was removed entirely, so there
    // is no longer a StatusLabel() heading to pin.

    // Action keyboard hint (fix thse isseu e.md #5): the footer must advertise
    // the C/M mnemonics alongside the arrows, not just "<- -> to switch". Pin
    // the exact wording so it cannot silently drop C/M again.
    {
        std::wstring hint = ActionKeyboardHint();
        check(hint.find(L"C Copy") != std::wstring::npos,
              "hint advertises 'C Copy'");
        check(hint.find(L"M Move") != std::wstring::npos,
              "hint advertises 'M Move'");
        check(hint.find(L"Enter") != std::wstring::npos,
              "hint still mentions Enter to create");
        check(hint.find(L"Esc") != std::wstring::npos,
              "hint still mentions Esc to cancel");
        check(hint.find(L"\u2190") != std::wstring::npos &&
              hint.find(L"\u2192") != std::wstring::npos,
              "hint still shows the left/right arrows");
    }

    if (g_fail) { printf("section_labels_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("section_labels_test: OK\n");
    return 0;
}
