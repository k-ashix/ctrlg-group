// namefield_logic.h
//
// PURE Folder-name field decisions (fix thse isseue.md #7), mirrored from
// ctrlg-group.wh.cpp. The field used to look like a blank region
// with a floating caret. Two decisions make it read as a real input:
//   * Placeholder(): the secondary-tone cue shown when the field is empty and
//     unfocused (a de-themed EDIT ignores EM_SETCUEBANNER, so the mod paints
//     this string itself in a subclass);
//   * BorderIsAccent(focused): the field border is the accent colour while the
//     field has keyboard focus, the neutral card border otherwise.
// Keep byte-for-byte in lockstep with the mod's namespace namefield.
#pragma once

namespace namefield {

// fix thse isseue.md #7 (pass6): the Folder name field is the STRONGEST input,
// so its rounded border uses a 7-8px corner (slightly rounder than the generic
// card). RoundRect takes the ellipse diameter, so the mod passes kRadius * 2.
inline constexpr int kRadius = 8;

inline const wchar_t* Placeholder() { return L"Enter folder name..."; }

inline bool BorderIsAccent(bool focused) { return focused; }

// fix thse isseue.md #5: the placeholder cue is shown ONLY when the field is
// empty AND unfocused. A pure predicate so the subclass's empty+unfocused paint
// path (one BeginPaint/EndPaint, no post-paint GetDC) is unit-testable without
// a real HWND. Keep byte-for-byte in lockstep with the mod's namespace namefield.
inline bool ShowPlaceholder(bool isEmpty, bool focused) { return isEmpty && !focused; }

}  // namespace namefield
