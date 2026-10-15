// info_help_test.cpp
// Pins fix thse isseue.md Phase C: the footer information/help button model.
// Accessible name, SUMMARIZED single-line content (5 lines), the hover +
// click-to-pin open/close state machine (pointer trigger->popup never
// dismisses), and the footer-fit math that keeps the actions on-screen.
// Self-contained: own main(), exit 0 = pass.

#include "info_help_logic.h"

#include <cwchar>
#include <cstdio>

using namespace infohelp;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// Concatenate parsed run text back into a plain string (markers removed).
static std::wstring Flatten(const wchar_t* line) {
    std::wstring out;
    for (const auto& r : ParseRuns(line)) out += r.text;
    return out;
}

int main() {
    // ---- accessible name / heading (Issue 1: exactly "Navigation help") ----
    check(wcscmp(AccessibleName(), L"Navigation help") == 0,
          "accessible name is the Issue-1 spec string 'Navigation help'");
    check(wcscmp(Heading(), L"Help") == 0, "card title is 'Help' (spec section 2)");

    // ---- content: the FULL navigation help, kind-tagged, overflow-ready ----
    check(LineCount() >= 15, "navigation help carries the full multi-section content");
    check(Line(LineCount())[0] == L'\0', "out-of-range line is empty");
    int sections = 0, dividers = 0, bodies = 0;
    for (int i = 0; i < LineCount(); ++i) {
        const wchar_t* s = Line(i);
        const Kind k = LineKind(i);
        if (k == Kind::Section)      ++sections;
        else if (k == Kind::Divider) ++dividers;
        else                         ++bodies;
        // Only dividers may be empty; every text line must have content.
        if (k != Kind::Divider)
            check(s[0] != L'\0', "a section/body help line is non-empty");
        bool singleLine = true;
        for (const wchar_t* p = s; *p; ++p)
            if (*p == L'\n' || *p == L'\r') singleLine = false;
        check(singleLine, "help line is a SINGLE line (renderer wraps long ones)");
    }
    check(sections == 4, "exactly four sections (spec section 3/5)");
    check(dividers >= 3, "a divider separates each adjacent section");
    check(bodies   >= 10, "the body carries the detailed help lines");

    // ---- the four section headings are EXACTLY the chip labels (spec 3/5) ----
    {
        const wchar_t* kChips[4] = { L"Navigation", L"New Folder",
                                     L"Group Creation", L"Auto Group" };
        int seen = 0;
        for (int i = 0; i < LineCount(); ++i) {
            if (LineKind(i) != Kind::Section) continue;
            check(seen < 4 && wcscmp(Line(i), kChips[seen]) == 0,
                  "section heading matches the chip label, in order");
            ++seen;
        }
        check(seen == 4, "all four chip-label sections are present and ordered");
    }

    // ---- bullets and italic secondary notes are their own kinds (spec 6) ----
    {
        int bullets = 0, notes = 0;
        for (int i = 0; i < LineCount(); ++i) {
            if (LineKind(i) == Kind::Bullet) ++bullets;
            if (LineKind(i) == Kind::Note)   ++notes;
        }
        check(bullets >= 8, "bulleted items are tagged Kind::Bullet");
        check(notes   >= 2, "secondary notes are tagged Kind::Note");
    }

    // ---- content accuracy spot-checks (spec section 5) ----
    {
        bool ctrlWheelViews = false, thumbNote = false,
             quickDefault = false, countdownNote = false;
        for (int i = 0; i < LineCount(); ++i) {
            std::wstring f = Flatten(Line(i));
            if (f.find(L"Extra large icon views") != std::wstring::npos)
                ctrlWheelViews = true;
            if (LineKind(i) == Kind::Note &&
                f.find(L"thumbnail") != std::wstring::npos)
                thumbNote = true;
            if (LineKind(i) == Kind::Note &&
                f.find(L"never active by default") != std::wstring::npos)
                quickDefault = true;
            if (LineKind(i) == Kind::Note &&
                f.find(L"never runs the operation on its own") != std::wstring::npos)
                countdownNote = true;
        }
        check(ctrlWheelViews, "Ctrl+Wheel documents the List/Large/Extra large view switch");
        check(thumbNote, "a note flags image previews as verified-thumbnail-only");
        check(quickDefault, "a note marks the quick Copy/Move shortcuts as never default");
        check(countdownNote, "a note states the countdown never auto-runs the operation");
    }

    // ---- author footer (fix thse isseue.md add-on section 4) ----
    {
        int footers = 0, footerIdx = -1;
        bool github = false;
        for (int i = 0; i < LineCount(); ++i) {
            if (LineKind(i) == Kind::Footer) {
                ++footers;
                footerIdx = i;
                if (Flatten(Line(i)).find(L"GitHub") != std::wstring::npos)
                    github = true;
            }
        }
        check(footers == 1, "exactly one Kind::Footer line at the bottom");
        check(!github, "the footer no longer prints a literal 'GitHub' label");
        check(footerIdx >= 0 && wcscmp(FooterText(), Line(footerIdx)) == 0,
              "FooterText() returns the single Kind::Footer line's text");
        // Runs: 'Visit' regular + 'Ashix' token (mono) + 'Ctrl ..' small italic.
        auto fr = ParseRuns(FooterText());
        bool visitPlain = false, ashixMono = false, ctrlItalic = false;
        for (const auto& r : fr) {
            if (!r.bold && !r.mono && !r.italic &&
                r.text.find(L"Visit") != std::wstring::npos) visitPlain = true;
            if (r.mono && r.text == L"Ashix") ashixMono = true;
            if (r.italic && r.text.find(L"Ctrl") != std::wstring::npos) ctrlItalic = true;
        }
        check(visitPlain, "'Visit' is regular-weight text");
        check(ashixMono, "'Ashix' is the rounded mono key token");
        check(ctrlItalic, "'Ctrl + Left-click to open' is a smaller italic hint");
        check(Flatten(FooterText()).find(L"Ashix") != std::wstring::npos,
              "the footer names Ashix (author token)");
    }

    // ---- bold-run parsing: markers stripped, key spans flagged bold ----
    for (int i = 0; i < LineCount(); ++i) {
        std::wstring flat = Flatten(Line(i));
        check(flat.find(L'*') == std::wstring::npos,
              "parsed help text never contains a literal '*' marker");
    }
    {
        // The "Mouse wheel" body line keeps its bold key span. It is no longer
        // line 0: a "NAVIGATE ITEMS" section header now leads the content, so
        // find the body line by its bold key span rather than by index.
        int mw = -1;
        for (int i = 0; i < LineCount(); ++i) {
            auto r = ParseRuns(Line(i));
            if (!r.empty() && r[0].bold && r[0].text == L"Mouse wheel") { mw = i; break; }
        }
        check(mw >= 0, "the 'Mouse wheel' body line is present with a bold key span");
        if (mw >= 0) {
            auto r0 = ParseRuns(Line(mw));
            check(r0.size() >= 2 && !r0[1].bold,
                  "the explanatory remainder is a regular (non-bold) run");
            bool anyBold = false, anyRegular = false;
            for (const auto& run : r0) { anyBold |= run.bold; anyRegular |= !run.bold; }
            check(anyBold && anyRegular, "the line mixes bold key spans and regular text");
        }
    }

    // ---- mono (`backtick`) and italic (_underscore_) run parsing ----
    {
        auto r = ParseRuns(L"press `9` then _wait_");
        std::wstring flat;
        bool anyMono = false, anyItalic = false;
        for (const auto& x : r) { flat += x.text; anyMono |= x.mono; anyItalic |= x.italic; }
        check(flat == L"press 9 then wait", "backtick / underscore markers are stripped");
        check(anyMono, "a `backtick` span is flagged mono");
        check(anyItalic, "an _underscore_ span is flagged italic");
        bool mono9 = false, italWait = false;
        for (const auto& x : r) {
            if (x.mono && x.text == L"9") mono9 = true;
            if (x.italic && x.text == L"wait") italWait = true;
        }
        check(mono9, "the `9` span is the mono run");
        check(italWait, "the _wait_ span is the italic run");
    }
    {
        // the three toggles are independent: bold, mono and italic coexist.
        auto r = ParseRuns(L"**Key**: `v` _soon_");
        bool boldKey = false, monoV = false, italSoon = false;
        for (const auto& x : r) {
            if (x.bold && x.text == L"Key") boldKey = true;
            if (x.mono && x.text == L"v") monoV = true;
            if (x.italic && x.text == L"soon") italSoon = true;
        }
        check(boldKey && monoV && italSoon, "bold, mono and italic runs coexist independently");
    }
    // No literal mono/italic markers survive parsing of ANY help line.
    for (int i = 0; i < LineCount(); ++i) {
        std::wstring flat = Flatten(Line(i));
        check(flat.find(L'`') == std::wstring::npos, "no literal backtick marker remains");
        check(flat.find(L'_') == std::wstring::npos, "no literal underscore marker remains");
    }

    // ---- transient open: hover OR click opens; click NEVER pins/toggles ----
    check(OnHoverEnter(Vis::Hidden) == Vis::Shown, "hover opens a hidden card");
    check(OnHoverEnter(Vis::Shown)  == Vis::Shown, "hover keeps a shown card");
    check(OnClick(Vis::Hidden) == Vis::Shown, "click opens a hidden card");
    check(OnClick(Vis::Shown)  == Vis::Shown, "click on an open card keeps it open (NOT a toggle)");

    // ---- explicit dismiss always hides ----
    check(OnDismiss(Vis::Shown)  == Vis::Hidden, "Esc / popup-close hides a shown card");
    check(OnDismiss(Vis::Hidden) == Vis::Hidden, "dismiss on an already-hidden card is a no-op");

    // ---- stays open: pointer trigger->card must not dismiss (no flicker) ----
    check(!StaysOpen(Vis::Hidden, true, true), "a hidden card is not open");
    check(StaysOpen(Vis::Shown, true, false), "pointer on the button keeps it open");
    check(StaysOpen(Vis::Shown, false, true), "pointer on the card keeps it open (no flicker)");
    check(!StaysOpen(Vis::Shown, false, false), "pointer off both no longer counts as open");

    // ---- grace-timer close instead of instant hide ----
    check(ShouldArmClose(Vis::Shown, false, false),
          "leaving both surfaces arms the close grace timer");
    check(!ShouldArmClose(Vis::Shown, true, false),
          "pointer still on the button does not arm a close");
    check(!ShouldArmClose(Vis::Shown, false, true),
          "pointer still on the card does not arm a close");
    check(!ShouldArmClose(Vis::Hidden, false, false),
          "a hidden card never arms a close");

    // ---- grace timer firing resolves against the CURRENT pointer ----
    check(OnCloseTimer(Vis::Shown, false, false) == Vis::Hidden,
          "timer hides the card when the pointer is still off both surfaces");
    check(OnCloseTimer(Vis::Shown, true, false) == Vis::Shown,
          "timer keeps the card if the pointer returned to the button");
    check(OnCloseTimer(Vis::Shown, false, true) == Vis::Shown,
          "timer keeps the card if the pointer returned to the card");

    // ---- grace delay is short but non-zero ----
    check(CloseDelayMs() > 0 && CloseDelayMs() <= 1000,
          "close grace delay is a short, non-zero flicker guard");

    // ---- footer-fit math (unchanged) ----
    check(FooterFitsWithInfo(340, 28, 6, 300), "info button fits when there is room");
    check(!FooterFitsWithInfo(320, 28, 6, 300), "info button does not push actions off a narrow footer");
    check(FooterFitsWithInfo(334, 28, 6, 300), "exact fit boundary");
    check(InfoButtonSize(28) == 28, "square info button matches the button height");

    if (g_fail == 0) printf("info_help_logic: ALL PASS\n");
    return g_fail ? 1 : 0;
}
