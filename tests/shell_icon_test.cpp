// shell_icon_test.cpp
// Pins the Shell-icon decision logic (fix thse isseue.md #9): file/folder ROWS
// use the real Windows Shell icon via a disk-free, extension-based lookup.
// Self-contained: own main(), exit 0 = pass. RED before shell_icon_logic.h /
// namespace shellicon exist; GREEN once the mod mirrors them.

#include "shell_icon_logic.h"

#include <cstdio>

using namespace shellicon;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- rows always use the Shell icon (files AND folders) ----
    check(RowUsesShellIcon(false), "file rows use the Shell icon");
    check(RowUsesShellIcon(true),  "folder rows use the Shell icon (Windows folder icon)");

    // ---- flags: icon handle + small size + no-disk extension lookup ----
    check(IconFlags() == (kSHGFI_ICON | kSHGFI_SMALLICON | kSHGFI_USEFILEATTRIBUTES),
          "flags = ICON | SMALLICON | USEFILEATTRIBUTES");
    check((IconFlags() & kSHGFI_USEFILEATTRIBUTES) != 0,
          "USEFILEATTRIBUTES set -> lookup never touches disk");
    check((IconFlags() & kSHGFI_SMALLICON) != 0, "SMALLICON set -> 16px baseline");
    check((IconFlags() & kSHGFI_ICON) != 0, "ICON set -> returns an HICON to DrawIconEx");
    check(IconFlags() == 0x111u, "flag numeric mirror is exact (0x100|0x1|0x10)");

    // ---- attributes drive folder vs file ----
    check(AttrFor(true)  == kFILE_ATTRIBUTE_DIRECTORY, "folder -> DIRECTORY attribute");
    check(AttrFor(false) == kFILE_ATTRIBUTE_NORMAL,    "file -> NORMAL attribute");
    check(AttrFor(true) != AttrFor(false), "folder and file attributes differ");

    // ---- DPI-scaled small-icon size (matches ScaleDpi(16)) ----
    check(IconPx(96)  == 16, "96dpi  -> 16px");
    check(IconPx(120) == 20, "125%   -> 20px");
    check(IconPx(144) == 24, "150%   -> 24px");
    check(IconPx(168) == 28, "175%   -> 28px");
    check(IconPx(192) == 32, "200%   -> 32px");
    check(IconPx(96) < IconPx(192), "icon grows with DPI");

    // ---- cache key normalises by type/extension ----
    check(IconKey(L"song.mp3", false) == L".mp3", "mp3 key");
    check(IconKey(L"Photo.JPG", false) == L".jpg", "extension is lowercased");
    check(IconKey(L"report.PDF", false) == L".pdf", "pdf key");
    check(IconKey(L"archive.tar.gz", false) == L".gz", "multi-dot -> last extension");
    check(IconKey(L"README", false) == L"<noext>", "no extension -> <noext>");
    check(IconKey(L"trailingdot.", false) == L"<noext>", "trailing dot -> <noext>");
    check(IconKey(L"anything", true) == L"<dir>", "folders collapse to <dir>");
    check(IconKey(L"My.Folder.Name", true) == L"<dir>", "dotted folder still <dir>");
    // same extension -> same key -> one shared Shell lookup
    check(IconKey(L"a.mp3", false) == IconKey(L"b.MP3", false),
          "same extension shares one cache entry");

    if (g_fail == 0) printf("shell_icon_test: OK\n");
    else              printf("shell_icon_test: %d check(s) FAILED\n", g_fail);
    return g_fail ? 1 : 0;
}
