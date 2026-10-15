// row_format_test.cpp
// Pins the Selected Items row decomposition (waht to create.md #1 / #12):
// filename column, a secondary uppercased extension chip, and an optional
// human-readable size. Mirrored from the mod into selection_logic.h. Pixel
// truncation is the GDI layer's job (DT_END_ELLIPSIS), so it is NOT tested here.
//
// Self-contained: own main(), exit 0 = pass. RED before the header is
// implemented; GREEN once it mirrors the mod.

#include "selection_logic.h"

#include <cstdio>

using namespace sel;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- extension split ----
    check(SplitName(L"song.mp3", false).stem == L"song", "file stem");
    check(SplitName(L"song.mp3", false).ext == L"mp3", "file ext");
    check(SplitName(L"archive.tar.gz", false).ext == L"gz", "ext is last segment");
    check(SplitName(L"README", false).ext == L"", "dotless file has no ext");
    check(SplitName(L".gitignore", false).ext == L"", "leading-dot hidden file has no ext");
    check(SplitName(L".gitignore", false).stem == L".gitignore", "hidden file keeps whole name");
    check(SplitName(L"photo.", false).ext == L"", "trailing-dot has no ext");
    check(SplitName(L"docs", true).ext == L"", "folder has no ext");
    check(SplitName(L"my.folder", true).ext == L"", "dotted folder still has no ext");
    check(SplitName(L"my.folder", true).stem == L"my.folder", "folder keeps whole name");

    // ---- row filename column ----
    check(RowName(L"song.mp3", false) == L"song", "file row name is the stem");
    check(RowName(L"README", false) == L"README", "dotless file row name is whole");
    check(RowName(L"my.folder", true) == L"my.folder", "folder row name is whole name");

    // ---- extension chip (uppercased) ----
    check(ExtLabel(L"song.mp3", false) == L"MP3", "ext chip uppercased");
    check(ExtLabel(L"Photo.JpG", false) == L"JPG", "mixed-case ext uppercased");
    check(ExtLabel(L"README", false) == L"", "no ext -> empty chip");
    check(ExtLabel(L"docs", true) == L"", "folder -> empty chip");

    // ---- human size ----
    check(HumanSize(-1) == L"", "unknown size -> empty");
    check(HumanSize(0) == L"0 B", "zero bytes");
    check(HumanSize(512) == L"512 B", "bytes under 1 KB");
    check(HumanSize(1024) == L"1.0 KB", "exactly 1 KB");
    check(HumanSize(1536) == L"1.5 KB", "1.5 KB");
    check(HumanSize(9 * 1024) == L"9.0 KB", "under 10 in unit -> one decimal");
    check(HumanSize(10 * 1024) == L"10 KB", "exactly 10 in unit -> whole number");
    check(HumanSize(12 * 1024) == L"12 KB", "12 KB (spec example)");
    check(HumanSize(24 * 1024) == L"24 KB", "24 KB (spec example)");
    check(HumanSize(318 * 1024) == L"318 KB", "318 KB (spec example)");
    check(HumanSize(34009088) == L"32 MB", "MB >= 10 -> whole number");
    check(HumanSize(1073741824) == L"1.0 GB", "1 GB");

    // ---- size-on-disk (allocated) vs logical length selection ----
    check(ChooseSizeBytes(1000, 4096) == 4096, "prefer allocated size on disk");
    check(ChooseSizeBytes(1000, -1) == 1000, "fall back to logical length");
    check(ChooseSizeBytes(-1, -1) == -1, "both unknown -> unknown");

    if (g_fail) { printf("row_format_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("row_format_test: OK\n");
    return 0;
}
