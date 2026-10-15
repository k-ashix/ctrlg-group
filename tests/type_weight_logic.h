// type_weight_logic.h
//
// PURE type-weight policy for fix thse isseue.md S14 (v0.12.54): the section
// TITLES ("Selected items", "Destination", "Folder name", "Status", "Action")
// plus the "Group into folder" title-bar heading move from a single regular UI
// weight to a SEMIBOLD weight, matching the heavier headings in the target
// Win11 dialog. Body text, helper descriptions and the live status/state value
// stay regular. The weights are the Win32 FW_* constants so the mod can
// static_assert this policy against <wingdi.h>; byte-for-byte mirror of
// namespace typeweight in ctrlg-group.wh.cpp. The live font
// creation (UiFont / UiFontSemibold) is clang-only and pinned by source
// invariants.
//
#pragma once

namespace typeweight {

// Win32 font weights: regular body text vs. semibold section headings.
inline constexpr int kRegular  = 400;  // FW_NORMAL
inline constexpr int kSemibold = 600;  // FW_SEMIBOLD

// A heading must read HEAVIER than the body, otherwise the title hierarchy the
// target UI shows is lost.
inline constexpr bool SemiboldIsHeavier() { return kSemibold > kRegular; }

}  // namespace typeweight
