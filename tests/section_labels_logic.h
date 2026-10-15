// section_labels_logic.h
//
// PURE section-wording model for the Ctrl+G group popup (fix thse isseu e.md
// #4/#6/#5, Phase 7/E), mirrored from ctrlg-group.wh.cpp so it can
// be unit tested with a plain g++ build (no Win32 runtime). The mod keeps a
// byte-for-byte copy and renders these strings; the tests pin them here.
//
// This module decides ONLY the wording shown for each form section's LABEL and
// its short supporting description. It adds NO behaviour: the reference UI wants
// a clear title + a one-line helper under it, and the old heavy "Group into
// folder name" phrasing is replaced with a lighter "Folder name" + helper.
// (The Destination section already has dest::SectionLabel /
// dest::SectionDescription; this covers the Name section to match.)
//
#pragma once

#include <string>

namespace seclbl {

// ---- Name section ------------------------------------------------------
// Reworded per fix thse isseu e.md #6: "Group into folder name" was too heavy.
inline const wchar_t* NameLabel()       { return L"Folder name"; }
inline const wchar_t* NameDescription() { return L"Name for the new folder."; }

// The New Folder (non-group) popup keeps its own simpler label; expose it here
// too so the one wording source stays in this module.
inline const wchar_t* NewFolderNameLabel() { return L"Enter your folder name"; }

// ---- Selected Items section -------------------------------------------
inline const wchar_t* SelectedLabel() { return L"Selected items"; }

// ---- Primary (submit) button caption (fix thse isseue.md #4) ----------
// The one primary button says "Group" in the Ctrl+G group popup and "Create"
// in the New Folder popup. Both drive the SAME IDOK command path (validation,
// copy/move execution, submission protection and Enter are unchanged); only
// the caption string differs on d->isGroup.
inline const wchar_t* SubmitLabel(bool isGroup) { return isGroup ? L"Group" : L"Create"; }

// ---- Action section footer hint ---------------------------------------
// fix thse isseu e.md #5: the old hint only mentioned the arrows ("Use <- -> to
// switch"). The popup now also accepts C (Copy) / M (Move) when a segment owns
// focus, so the footer must advertise them. Kept to ONE compact line so the
// Action card geometry is unchanged (no relayout): C/M lead, then the arrows,
// then Enter/Esc.
inline const wchar_t* ActionKeyboardHint() {
    return L"C Copy  \u00b7  M Move  \u00b7  \u2190 \u2192 switch  \u00b7  Enter create  \u00b7  Esc cancel";
}

}  // namespace seclbl
