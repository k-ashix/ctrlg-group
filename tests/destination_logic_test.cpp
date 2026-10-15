// destination_logic_test.cpp
// Pins the PURE Destination-section display model for the Ctrl+G popup
// (waht to create.md #5 / plan Phase 3). This module decides ONLY how the
// already-resolved destination context is PRESENTED:
//   * a fixed destination, when configured, wins the field text;
//   * otherwise the effective destination context is shown;
//   * otherwise a neutral placeholder;
//   * the last/recent target is a SECONDARY line, shown only when it is
//     non-empty AND not just a duplicate of the field text.
// It must NOT change ResolveGroupDestination precedence (that stays in
// group_logic.h) -- this is presentation only.
//
// Self-contained: own main(), exit 0 = pass. RED before destination_logic.h
// exists; GREEN once it mirrors the mod.

#include "destination_logic.h"

#include <cstdio>

using namespace dest;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- section chrome ----
    check(std::wstring(SectionLabel()) == L"Destination", "section label is 'Destination'");
    check(SectionDescription(false) == L"Selected items will be copied into the folder you name",
          "copy description");
    check(SectionDescription(true) == L"Selected items will be moved into the folder you name",
          "move description");

    // ---- fixed destination wins the field ----
    {
        DestModel m = ComputeDestination(L"C:\\Sorted", L"C:\\Photos\\Trip", L"C:\\Old");
        check(m.source == DestSource::Fixed, "fixed dest -> Fixed source");
        check(m.fieldText == L"C:\\Sorted", "fixed dest field text");
    }
    // fixed dest is trimmed of surrounding whitespace
    {
        DestModel m = ComputeDestination(L"   C:\\Sorted   ", L"", L"");
        check(m.source == DestSource::Fixed, "padded fixed dest -> Fixed");
        check(m.fieldText == L"C:\\Sorted", "fixed dest is trimmed");
    }

    // ---- no fixed dest -> effective context ----
    {
        DestModel m = ComputeDestination(L"", L"C:\\Photos\\Trip", L"");
        check(m.source == DestSource::Effective, "effective dest -> Effective source");
        check(m.fieldText == L"C:\\Photos\\Trip", "effective dest field text");
    }
    // fix thse isseue.md #2: with no fixed destination, the CAPTURED PARENT the
    // worker already knows must be shown as the effective destination -- not the
    // empty placeholder. This is the exact review scenario.
    {
        DestModel m = ComputeDestination(
            L"", L"E:\\3_Downloads\\PREVIOUS YEAR QUESTIONS", L"");
        check(m.source == DestSource::Effective, "#2 captured parent -> Effective source");
        check(m.fieldText == L"E:\\3_Downloads\\PREVIOUS YEAR QUESTIONS",
              "#2 captured parent fills the field (not the placeholder)");
        check(m.fieldText != FieldPlaceholder(), "#2 no placeholder when a parent is known");
    }

    // ---- nothing known -> placeholder ----
    {
        DestModel m = ComputeDestination(L"", L"", L"");
        check(m.source == DestSource::None, "no context -> None source");
        check(m.fieldText == FieldPlaceholder(), "placeholder field text");
        check(m.fieldText == L"Enter a folder name below", "placeholder wording");
        check(!m.showLastTarget, "no last target -> secondary hidden");
    }

    // ---- last/recent target is secondary; hidden when empty ----
    {
        DestModel m = ComputeDestination(L"", L"C:\\Photos\\Trip", L"");
        check(!m.showLastTarget, "empty last target hidden");
        check(m.lastTargetText.empty(), "empty last target has no text");
    }
    // shown when present and distinct from the field
    {
        DestModel m = ComputeDestination(L"", L"C:\\Photos\\Trip", L"D:\\Archive\\2024");
        check(m.showLastTarget, "distinct last target shown");
        check(m.lastTargetText == L"Recent: D:\\Archive\\2024", "last target text");
    }
    // hidden when it merely duplicates the field text (not relevant)
    {
        DestModel m = ComputeDestination(L"", L"D:\\Archive\\2024", L"D:\\Archive\\2024");
        check(!m.showLastTarget, "redundant last target hidden");
    }
    // trailing separator / whitespace differences still count as duplicate
    {
        DestModel m = ComputeDestination(L"", L"D:\\Archive\\2024", L"  D:\\Archive\\2024\\ ");
        check(!m.showLastTarget, "last target duplicate modulo trailing sep/space is hidden");
    }

    if (g_fail) { printf("destination_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("destination_logic_test: OK\n");
    return 0;
}
