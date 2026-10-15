// paint_mode_logic_test.cpp
// Pins the PURE background-paint-mode decision (fix thse isseue.md P0 #1/#2/#3):
//   * The New Folder popup ALWAYS paints its themed client-area background.
//   * The Group popup may use the DWM title-band backdrop ONLY with a valid
//     custom-frame setup (backdrop accepted AND frame extended).
//   * A failed backdrop OR a failed frame extension always falls back to the
//     complete solid theme.
//   * Windows 10, Transparency effects off, and unavailable DWM functions all
//     select the safe solid fallback.
// Self-contained: own main(), exit 0 = pass. RED before paint_mode_logic.h.

#include "paint_mode_logic.h"

#include <cstdio>

using namespace paintmode;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// Readable wrappers for the 6-arg decision (win11, transparency, requested,
// accepted, extended).
static PaintMode NewFolder(bool win11, bool trans, bool req, bool acc, bool ext) {
    return ChoosePaintMode(false, win11, trans, req, acc, ext);
}
static PaintMode Group(bool win11, bool trans, bool req, bool acc, bool ext) {
    return ChoosePaintMode(true, win11, trans, req, acc, ext);
}

int main() {
    // ---- #1 New Folder ALWAYS paints its full themed client area ----
    // No combination of inputs may drop the New Folder solid fill: it has a
    // native caption and never extends its frame, so there is no backdrop
    // surface. This is the fix for the white body in dark mode.
    check(NewFolder(true,  true,  true,  true,  true)  == PaintMode::SolidTheme,
          "New Folder with everything 'on' still paints solid (no white body)");
    check(NewFolder(false, false, false, false, false) == PaintMode::SolidTheme,
          "New Folder on Win10 paints solid");
    check(PaintsFullClient(NewFolder(true, true, true, true, true)),
          "New Folder fills the WHOLE client area");
    check(!UsesTitleBandBackdrop(NewFolder(true, true, true, true, true)),
          "New Folder never uses the title-band backdrop");
    check(!GroupBandCanUseBackdrop(false, true, true),
          "a non-group popup can never use the band backdrop");

    // ---- #2/#3 Group may use the DWM title band ONLY when FULLY confirmed ----
    check(Group(true, true, true, true, true) == PaintMode::DwmTitleBand,
          "Group + win11 + transparency + backdrop accepted + frame extended -> band");
    check(UsesTitleBandBackdrop(Group(true, true, true, true, true)),
          "a confirmed Group uses the title-band backdrop");
    check(GroupBandCanUseBackdrop(true, true, true),
          "Group band allowed only when BOTH DWM calls succeeded");

    // ---- Failed backdrop OR failed frame extension -> solid fallback ----
    check(Group(true, true, true, false, true)  == PaintMode::SolidTheme,
          "backdrop attribute REJECTED -> solid theme (no white strip)");
    check(Group(true, true, true, true,  false) == PaintMode::SolidTheme,
          "frame extension FAILED -> solid theme (the discarded-HRESULT bug)");
    check(!GroupBandCanUseBackdrop(true, true, false),
          "frame-extension failure forbids the band backdrop");
    check(!GroupBandCanUseBackdrop(true, false, true),
          "backdrop rejection forbids the band backdrop");

    // ---- Win10 / transparency off / no request -> safe fallback even if the
    //      DWM calls reported success ----
    check(Group(false, true, true, true, true) == PaintMode::SolidTheme,
          "Win10 Group paints solid even if DWM calls reported success");
    check(Group(true, false, true, true, true) == PaintMode::SolidTheme,
          "Transparency OFF Group paints solid");
    check(Group(true, true, false, true, true) == PaintMode::SolidTheme,
          "no backdrop requested -> solid");

    if (g_fail) {
        printf("paint_mode_logic_test: %d check(s) FAILED\n", g_fail);
        return 1;
    }
    printf("paint_mode_logic_test: OK\n");
    return 0;
}
