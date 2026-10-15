// card_logic.h
//
// PURE section-card model for the Group popup (Phase 4/B, fix thse isseu e.md
// #2), mirrored byte-for-byte into ctrlg-group.wh.cpp. The mod
// paints each of these layout rects as its OWN rounded card instead of one flat
// panel; this header owns the DECISION of which sections are cards, in what
// order, and the uniform corner radius. WM_ERASEBKGND iterates card::GroupCards()
// and calls DrawSectionCard() per id. Self-contained so a plain g++ build (no
// Win32 runtime) can unit-test it via tests\card_logic_test.cpp.
//
#pragma once

#include <array>

namespace card {

// Section surfaces the Group popup renders as cards, top -> bottom. The Action
// row (Copy/Move + Create/Cancel) is NOT a card surface in the reference, so it
// is intentionally absent here.
enum class Id { Selected, Destination, Name, Status };

inline constexpr int kCount = 4;

inline std::array<Id, kCount> GroupCards() {
    return {Id::Selected, Id::Destination, Id::Name, Id::Status};
}

// Uniform corner radius (96-dpi design px) for every section card.
inline constexpr int kCardRadius = 8;

}  // namespace card
