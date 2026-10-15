// destination_logic.h
//
// PURE Destination-section display model for the Ctrl+G popup
// (waht to create.md #5 / plan Phase 3), mirrored from
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build (no Win32 runtime). The mod keeps a byte-for-byte copy and renders
// these strings; the tests pin them here.
//
// This module decides ONLY how the destination context is PRESENTED. The
// resolution precedence (fixed > typed > last) stays in group_logic.h
// (ResolveGroupDestination) and is NOT changed here -- this is a read-only,
// form-style presentation of what the resolver already knows.
//
#pragma once

#include <string>
#include <cwctype>

namespace dest {

// ---- section chrome ----------------------------------------------------
// The form-section label and its short supporting description. The
// description reflects the current Move/Copy choice so the user can see
// where the selection is going without changing any behavior.
inline const wchar_t* SectionLabel() { return L"Destination"; }

inline std::wstring SectionDescription(bool isMove) {
    return isMove ? L"Selected items will be moved into the folder you name"
                  : L"Selected items will be copied into the folder you name";
}

// Shown in the read-only field when there is no fixed destination and no
// effective context yet (a brand-new group with a blank name).
inline std::wstring FieldPlaceholder() { return L"Enter a folder name below"; }

// ---- small pure path helpers (standalone; no newfolder_logic dependency) ----
inline bool IsSep(wchar_t c) { return c == L'\\' || c == L'/'; }

inline std::wstring TrimWs(const std::wstring& s) {
    size_t b = 0, e = s.size();
    while (b < e && iswspace(s[b])) ++b;
    while (e > b && iswspace(s[e - 1])) --e;
    return s.substr(b, e - b);
}

// Trim whitespace, then one trailing separator, so "D:\\x\\" and " D:\\x "
// compare equal when deciding whether the recent line is redundant.
inline std::wstring NormPath(const std::wstring& s) {
    std::wstring t = TrimWs(s);
    if (!t.empty() && IsSep(t[t.size() - 1])) t = t.substr(0, t.size() - 1);
    return t;
}

// Where the field text came from, so the UI can style it (a real path vs a
// dimmed placeholder).
enum class DestSource { Fixed, Effective, None };

struct DestModel {
    DestSource source = DestSource::None;
    std::wstring fieldText;        // read-only field content
    bool showLastTarget = false;   // render the secondary recent line?
    std::wstring lastTargetText;   // "Recent: <path>" when showLastTarget
};

// Build the presentation model. Precedence for the FIELD text:
//   1. fixed destination (configured)      -> DestSource::Fixed
//   2. effective destination context       -> DestSource::Effective
//   3. neither                             -> DestSource::None (placeholder)
// The recent/last target is SECONDARY: shown only when it is non-empty AND
// not merely a duplicate of the field text (path-normalised comparison).
inline DestModel ComputeDestination(const std::wstring& fixedDest,
                                    const std::wstring& effectiveDest,
                                    const std::wstring& lastTarget) {
    DestModel m;
    const std::wstring fixed = TrimWs(fixedDest);
    const std::wstring eff = TrimWs(effectiveDest);
    if (!fixed.empty()) {
        m.source = DestSource::Fixed;
        m.fieldText = fixed;
    } else if (!eff.empty()) {
        m.source = DestSource::Effective;
        m.fieldText = eff;
    } else {
        m.source = DestSource::None;
        m.fieldText = FieldPlaceholder();
    }

    const std::wstring last = TrimWs(lastTarget);
    if (!last.empty() && NormPath(last) != NormPath(m.fieldText)) {
        m.showLastTarget = true;
        m.lastTargetText = L"Recent: " + last;
    }
    return m;
}

}  // namespace dest
