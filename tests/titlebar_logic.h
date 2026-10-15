// titlebar_logic.h
//
// PURE custom-title-bar hit classification (fix thse isseue.md #2), mirrored
// from ctrlg-group.wh.cpp so it can be unit tested with a plain
// g++ build (no Win32 runtime). The Group popup dropped WS_CAPTION/WS_SYSMENU
// for a borderless WS_POPUP, so it paints its OWN title bar (folder icon +
// title + Expand/Minimize/Close) and must stay draggable. WM_NCHITTEST feeds a
// client point + the band height + the three button boxes to Classify():
//   * a point inside a button box -> that button (buttons win over caption);
//   * any other point inside the band (y < bandH) -> Caption (HTCAPTION drag);
//   * below the band -> Client.
// Keep byte-for-byte in lockstep with the mod's namespace titlebar.
#pragma once

namespace titlebar {

enum class Region { Client, Caption, Expand, Minimize, Close };

struct Box {
    int x = 0, y = 0, w = 0, h = 0;
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

inline Region Classify(int px, int py, int bandH,
                       Box expand, Box minimize, Box close) {
    if (close.contains(px, py))    return Region::Close;
    if (minimize.contains(px, py)) return Region::Minimize;
    if (expand.contains(px, py))   return Region::Expand;
    if (py >= 0 && py < bandH)     return Region::Caption;
    return Region::Client;
}

}  // namespace titlebar
