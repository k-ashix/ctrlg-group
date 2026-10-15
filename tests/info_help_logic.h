// info_help_logic.h
//
// PURE model for the footer INFORMATION / help button (fix thse isseue.md
// Phase C). Mirrored byte-for-byte into ctrlg-group.wh.cpp (namespace
// infohelp). No Win32. This is the single source of truth for:
//   * the accessible name of the footer info button,
//   * the SUMMARIZED help content (top note: "just a summarized info button ...
//     5-6 lines of single line information") -- exactly one short line each,
//     only documenting behaviours that are actually implemented,
//   * the hover + click-to-pin open/close state machine (moving the pointer
//     from the button into the popup must NOT dismiss it -> no flicker),
//   * the footer-fit math so adding the button never pushes the actions
//     ([info] [Auto Group] [Group] [Cancel]) off the popup.
//
#pragma once

#include <string>
#include <vector>

namespace infohelp {

// Assistive-tech / tooltip name for the '?' navigation-help button
// (fix thse isseue.md Issue 1). The spec names it exactly "Navigation help".
inline const wchar_t* AccessibleName() { return L"Navigation help"; }

// Semibold title drawn at the top of the help card. Spec section 2 renames the
// visible card title to exactly "Help" (the '?' button's accessible name stays
// AccessibleName() == "Navigation help").
inline const wchar_t* Heading() { return L"Help"; }

// Navigation-help content ("Overflow and Scrollable Help Card"). The card now
// shows the FULL navigation help -- NAVIGATE ITEMS plus the New Folder, Group
// Creation and Auto Grouping sections -- authored as single short lines with
// **bold** key spans. Each line carries a Kind so the renderer draws section
// headers, dividers and body rows with the right typography/spacing while the
// "Navigation help" heading stays FIXED and the body scrolls when it overflows
// the capped card. Lines stay single-line; the renderer wraps long ones.
// Spec sections 3/5/6: FOUR sections whose headings are exactly the chip
// labels (Navigation / New Folder / Group Creation / Auto Group), authored as
// single short lines. Each line carries a Kind so the renderer applies the size
// hierarchy -- Section (13 px bold), Body (12 px), Bullet (indented 12 px),
// Note (11 px italic secondary) and Divider (section separator). Keyboard
// combinations are **`bold mono`** key tokens; action names are **bold**;
// inline examples are `mono`. Markers are NEVER rendered literally (ParseRuns).
enum class Kind { Section, Body, Bullet, Note, Divider, Footer };
struct HelpLine { const wchar_t* text; Kind kind; };
inline const HelpLine* Lines(int* count) {
    static const HelpLine kLines[] = {
        { L"Navigation", Kind::Section },
        { L"**`Mouse wheel`** - Scroll the selected items", Kind::Body },
        { L"**`Shift + Wheel`** - Scroll the summary chips horizontally", Kind::Body },
        { L"**`Ctrl + Wheel`** - Switch between List, Large and Extra large icon views", Kind::Body },
        { L"**Copy / Move** - Click a segment, or use **Left** / **Right** when focused", Kind::Body },
        { L"**`Tab`** / **`Shift + Tab`** - Move between controls", Kind::Body },
        { L"**`Enter`** - Activate the focused action", Kind::Body },
        { L"**`Esc`** - Cancel the Group popup", Kind::Body },
        { L"Image previews appear only in Large and Extra large views, once the thumbnail path is verified", Kind::Note },
        { L"", Kind::Divider },
        { L"New Folder", Kind::Section },
        { L"**Shortcut**: **`Ctrl + F + N`**", Kind::Body },
        { L"**Single folder**: enter a name and confirm", Kind::Bullet },
        { L"**Batch creation**: when enabled, type a count after the shortcut (e.g. `9` for nine), up to the limit", Kind::Bullet },
        { L"**Naming**: inline naming is the default; popup naming is available in settings", Kind::Bullet },
        { L"**Templates**: press an assigned template key to create its folder set", Kind::Bullet },
        { L"", Kind::Divider },
        { L"Group Creation", Kind::Section },
        { L"**Shortcut**: **`Ctrl + G`**", Kind::Body },
        { L"**Group**: enter a destination name, choose Copy or Move, confirm", Kind::Bullet },
        { L"**Copy**: keep the originals", Kind::Bullet },
        { L"**Move**: relocate the selected items", Kind::Bullet },
        { L"**Blank name**: reuse the last used target name when that applies", Kind::Bullet },
        { L"**Optional quick actions**: if enabled, **`Ctrl + G then C`** copies or **`Ctrl + G then M`** moves to the last destination", Kind::Body },
        { L"Quick Copy/Move is setting-dependent and never active by default", Kind::Note },
        { L"", Kind::Divider },
        { L"Auto Group", Kind::Section },
        { L"**Auto Group**: organize selected files into folders by extension, with no folder name", Kind::Body },
        { L"**Safety delay**: wait for the configured eight-second countdown before clicking Auto Group", Kind::Bullet },
        { L"**Flat layout**: create extension folders directly in the destination", Kind::Bullet },
        { L"**Images parent**: group image-extension folders beneath the configured Images parent", Kind::Bullet },
        { L"**Custom rules**: let configured rules place matching extensions into named buckets", Kind::Bullet },
        { L"The countdown never runs the operation on its own; some features may be disabled", Kind::Note },
        { L"Visit `Ashix` _Ctrl + Left-click to open_", Kind::Footer },
    };
    if (count) *count = static_cast<int>(sizeof(kLines) / sizeof(kLines[0]));
    return kLines;
}
inline int LineCount() { int n = 0; Lines(&n); return n; }
inline const wchar_t* Line(int i) {
    int n = 0; const HelpLine* ls = Lines(&n);
    if (i < 0 || i >= n) return L"";
    return ls[i].text;
}
inline Kind LineKind(int i) {
    int n = 0; const HelpLine* ls = Lines(&n);
    if (i < 0 || i >= n) return Kind::Body;
    return ls[i].kind;
}

// fix thse isseue.md add-on section 4: the single Kind::Footer line's raw text
// (markers intact). The footer is painted in a FIXED band OUTSIDE the scrollable
// body; the renderer runs this through ParseRuns so "Visit" stays regular, the
// `Ashix` backtick span becomes the rounded key token and the _Ctrl + ..._
// underscore span is the smaller italic hint. No literal "GitHub" label.
inline const wchar_t* FooterText() {
    int n = 0; const HelpLine* ls = Lines(&n);
    for (int i = 0; i < n; ++i)
        if (ls[i].kind == Kind::Footer) return ls[i].text;
    return L"";
}

// ---- bold-run parsing for the help-card renderer --------------------------
// A run is a contiguous span of text that is either bold (a key name) or
// regular (explanatory text). ParseRuns() splits a Line() on the `**` markers;
// the markers are NEVER rendered literally. The renderer draws bold runs with
// the semibold font and regular runs with the body font, advancing x between
// them. Easy to extend later with more marker kinds.
// fix thse isseue.md (nav-help typography): a run can be bold (**key**),
// MONOSPACE (`literal`, a backtick span -> a Windows mono face for keystrokes /
// literal tokens) or ITALIC (_hint_, an underscore span -> an italic face for
// secondary guidance). The three toggles are INDEPENDENT and the markers are
// NEVER rendered literally. Unmarked text is regular body.
struct HelpRun { std::wstring text; bool bold = false; bool mono = false; bool italic = false; };
inline std::vector<HelpRun> ParseRuns(const wchar_t* line) {
    std::vector<HelpRun> runs;
    std::wstring cur;
    bool bold = false, mono = false, italic = false;
    auto flush = [&]() {
        if (!cur.empty()) { runs.push_back(HelpRun{cur, bold, mono, italic}); cur.clear(); }
    };
    for (const wchar_t* p = line; *p; ) {
        if (p[0] == L'*' && p[1] == L'*') {
            flush();
            bold = !bold;
            p += 2;
        } else if (p[0] == L'`') {
            flush();
            mono = !mono;
            ++p;
        } else if (p[0] == L'_') {
            flush();
            italic = !italic;
            ++p;
        } else {
            cur.push_back(*p);
            ++p;
        }
    }
    flush();
    return runs;
}

// ---- transient hover help-card state machine (NO pin) ---------------------
// Issue 1: the '?' is NOT a pin/unpin toggle. Hovering OR clicking it opens the
// card; it stays open while the pointer is on the button OR the card; when the
// pointer leaves BOTH, a short grace timer (flicker guard across the gap) hides
// it; Esc / popup close dismiss it.
enum class Vis { Hidden, Shown };

// Hover over '?' opens the card.
inline Vis OnHoverEnter(Vis) { return Vis::Shown; }

// Click over '?' ALSO just opens it -- it never pins and never toggles closed.
inline Vis OnClick(Vis) { return Vis::Shown; }

// Explicit dismiss (Esc / popup closing / click-away) always hides.
inline Vis OnDismiss(Vis) { return Vis::Hidden; }

// The card stays open while shown AND the pointer is on the button or the card.
// Sliding from the button into the card keeps both-false from ever happening in
// one step, so the card never flickers while crossing the gap.
inline bool StaysOpen(Vis v, bool pointerOnTrigger, bool pointerOnCard) {
    return v == Vis::Shown && (pointerOnTrigger || pointerOnCard);
}

// Leaving BOTH surfaces while shown arms the close timer instead of hiding now.
inline bool ShouldArmClose(Vis v, bool pointerOnTrigger, bool pointerOnCard) {
    return v == Vis::Shown && !pointerOnTrigger && !pointerOnCard;
}

// The grace timer firing hides the card ONLY if the pointer is still off both
// surfaces. If the pointer returned to either surface first, the card stays
// shown -- so crossing the button/card gap never dismisses it.
inline Vis OnCloseTimer(Vis v, bool pointerOnTrigger, bool pointerOnCard) {
    if (v == Vis::Shown && !pointerOnTrigger && !pointerOnCard) return Vis::Hidden;
    return v;
}

// Short grace delay (ms) between the pointer leaving both surfaces and the card
// actually hiding. Long enough to cross the button/card gap, short enough to
// feel immediate.
inline int CloseDelayMs() { return 180; }

// ---- footer-fit math: never push the actions off the popup ----------------
// The small square info button (infoW) plus one gap fits in front of the
// actions only when there is room. The caller keeps the actions fully visible;
// it never shrinks them to make space.
inline bool FooterFitsWithInfo(int footerW, int infoW, int gap, int actionsW) {
    return (infoW + gap + actionsW) <= footerW;
}

// A square info button sized to the footer button height, so it matches the
// other controls' sizing rhythm and DPI scaling.
inline int InfoButtonSize(int buttonH) { return buttonH; }

}  // namespace infohelp
