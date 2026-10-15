// thumb_test.cpp
// Pins fix thse isseue.md Issue 2: the PURE thumbnail model -- image-only
// eligibility, grid-only gating, aspect-ratio-preserving fit, request size.
// Self-contained: own main(), exit 0 = pass.

#include "thumb_logic.h"

#include <cstdio>

using namespace thumbs;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- image eligibility: common raster formats (any case, with/without dot)
    const wchar_t* yes[] = {L"jpg", L"JPG", L"jpeg", L".png", L"PNG", L"gif",
                            L"bmp", L"tif", L"tiff", L"webp", L"ico", L"heic", nullptr};
    for (int i = 0; yes[i]; ++i) check(IsImageExtension(yes[i]), "image extension is eligible");

    // ---- NOT image types: documents / media / archives / executables / etc.
    const wchar_t* no[] = {L"pdf", L"mp4", L"mov", L"mkv", L"mp3", L"wav",
                           L"zip", L"7z", L"rar", L"exe", L"dll", L"txt",
                           L"docx", L"", nullptr};
    for (int i = 0; no[i]; ++i) check(!IsImageExtension(no[i]), "non-image extension is NOT eligible");
    check(!IsImageExtension(nullptr), "null extension is not eligible");

    // ---- ExtOf ----
    check(ExtOf(L"photo.JPG") == L"JPG", "ExtOf keeps original case");
    check(ExtOf(L"a.b.png") == L"png", "ExtOf takes the last extension");
    check(ExtOf(L"noext") == L"", "ExtOf of an extensionless name is empty");
    check(ExtOf(L"trailingdot.") == L"", "ExtOf of a trailing dot is empty");
    check(ExtOf(L"C:/dir.v2/file") == L"", "ExtOf ignores a dot in a directory segment");
    check(ExtOf(L"C:/dir.v2/pic.jpeg") == L"jpeg", "ExtOf resolves a full path's extension");

    // ---- gating: grid + non-folder + image only ----
    check(WantsThumbnail(true, false, true),  "grid + image file -> thumbnail");
    check(!WantsThumbnail(false, false, true), "List view -> never a thumbnail (icons only)");
    check(!WantsThumbnail(true, true, true),   "folder -> never a thumbnail");
    check(!WantsThumbnail(true, false, false), "grid + non-image -> no thumbnail");

    check(WantsThumbnailFor(true, false, L"C:/pics/sunset.jpg"), "grid image path -> thumbnail");
    check(!WantsThumbnailFor(true, false, L"C:/docs/report.pdf"), "grid PDF -> icon (no thumbnail)");
    check(!WantsThumbnailFor(false, false, L"C:/pics/sunset.jpg"), "List image -> icon (no thumbnail)");
    check(!WantsThumbnailFor(true, true, L"C:/pics"), "grid folder -> icon (no thumbnail)");

    // fix thse isseue.md v0.12.67 (Issue #2): the EXACT files from the report --
    // uppercase .JPEG / .JPG and a spaced .png name -- are eligible in grid
    // views. The previous build still drew their generic Shell icon because the
    // GRID renderer never requested a thumbnail (the eligibility itself was
    // always correct); these pin that eligibility so a future regression shows.
    check(WantsThumbnailFor(true, false, L"C:/pics/pic.JPEG"),
          "grid pic.JPEG -> thumbnail (uppercase .JPEG)");
    check(WantsThumbnailFor(true, false, L"C:/pics/sign.JPG"),
          "grid sign.JPG -> thumbnail (uppercase .JPG)");
    check(WantsThumbnailFor(true, false, L"C:/pics/Frame 1.png"),
          "grid 'Frame 1.png' -> thumbnail (spaced filename)");
    check(!WantsThumbnailFor(false, false, L"C:/pics/pic.JPEG"),
          "List pic.JPEG -> icon only (no thumbnail in List)");

    // ---- aspect-fit: never stretched, centered, fits inside the cell ----
    {
        FitBox sq = FitRect(100, 100, 200, 200);   // square into square: fills, centered
        check(sq.w == 200 && sq.h == 200 && sq.x == 0 && sq.y == 0, "square image fills a square cell");

        FitBox wide = FitRect(200, 100, 200, 200);  // 2:1 into square: width-limited
        check(wide.w == 200 && wide.h == 100, "wide image is width-limited (aspect kept)");
        check(wide.x == 0 && wide.y == 50, "wide image is vertically centered");

        FitBox tall = FitRect(100, 200, 200, 200);  // 1:2 into square: height-limited
        check(tall.h == 200 && tall.w == 100, "tall image is height-limited (aspect kept)");
        check(tall.y == 0 && tall.x == 50, "tall image is horizontally centered");

        FitBox fit = FitRect(400, 300, 200, 200);   // 4:3 into square
        check(fit.w == 200 && fit.h == 150, "4:3 image fits inside the square, aspect kept");
        check(fit.x == 0 && fit.y == 25, "4:3 image centered vertically");
        // fit always stays within the cell
        check(fit.x >= 0 && fit.y >= 0 && fit.x + fit.w <= 200 && fit.y + fit.h <= 200,
              "fit box stays inside the cell");

        FitBox degenerate = FitRect(0, 0, 120, 120);  // unknown source dims
        check(degenerate.w == 120 && degenerate.h == 120, "unknown source dims -> fill the cell");
    }

    // ---- request px is the shorter cell side ----
    check(RequestPx(96, 128) == 96, "request px is the shorter cell side");
    check(RequestPx(160, 160) == 160, "square cell request px");

    if (g_fail) { printf("thumb_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("thumb_test: OK\n");
    return 0;
}
