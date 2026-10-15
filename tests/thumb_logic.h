// thumb_logic.h
//
// PURE model for fix thse isseue.md Issue 2 -- image-only thumbnail previews in
// the Large / Extra large Selected Items views. No Win32. Mirrored byte-for-byte
// into ctrlg-group.wh.cpp (namespace thumbs). This is the single source of truth
// for:
//   * which file extensions are eligible for a thumbnail (raster images ONLY --
//     never PDF/video/audio/archive/executable/etc.),
//   * the gating rule (thumbnails ONLY in a grid view, only for non-folder image
//     files; List view and every non-image type keep their Shell icon),
//   * the aspect-ratio-preserving fit of a thumbnail into a fixed grid cell,
//   * the square pixel size to request a thumbnail at for a cell.
// The live Win32 glue (IShellItemImageFactory async extraction + cache + paint)
// is wired to THESE functions and verified on-device.
#pragma once

#include <string>

namespace thumbs {

// ASCII lower-case of a single wide char.
inline wchar_t Lower(wchar_t c) {
    return (c >= L'A' && c <= L'Z') ? static_cast<wchar_t>(c - L'A' + L'a') : c;
}

// Is `ext` (with or without a leading dot, any case) a raster image format we
// offer a thumbnail for? Deliberately conservative: only true image types.
inline bool IsImageExtension(const wchar_t* ext) {
    if (!ext) return false;
    std::wstring e;
    for (const wchar_t* p = ext; *p; ++p) e.push_back(Lower(*p));
    if (!e.empty() && e[0] == L'.') e.erase(0, 1);
    if (e.empty()) return false;
    static const wchar_t* kImg[] = {
        L"jpg", L"jpeg", L"jpe", L"jfif", L"png", L"gif", L"bmp",
        L"dib", L"tif", L"tiff", L"webp", L"ico", L"heic", L"heif", nullptr};
    for (int i = 0; kImg[i]; ++i) if (e == kImg[i]) return true;
    return false;
}

// Extract the extension (no dot) from a filename or path. Empty if there is no
// extension (a trailing dot, or the last dot belongs to a directory segment).
inline std::wstring ExtOf(const std::wstring& name) {
    const size_t dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos) return L"";
    const size_t slash = name.find_last_of(L"\\/");
    if (slash != std::wstring::npos && dot < slash) return L"";
    if (dot + 1 >= name.size()) return L"";
    return name.substr(dot + 1);
}

// A row shows a real image thumbnail ONLY in a grid (Large / Extra large) view,
// for a NON-folder whose extension is a supported image type. List view and all
// non-image types fall back to the crisp Shell icon.
inline bool WantsThumbnail(bool isGridView, bool isFolder, bool isImageExt) {
    return isGridView && !isFolder && isImageExt;
}

// Convenience: decide straight from a filename / path.
inline bool WantsThumbnailFor(bool isGridView, bool isFolder,
                              const std::wstring& name) {
    return WantsThumbnail(isGridView, isFolder,
                          IsImageExtension(ExtOf(name).c_str()));
}

// Aspect-fit an srcW x srcH image INSIDE a cellW x cellH cell, centered,
// preserving aspect ratio (never stretched). Returns the destination rect
// (offsets relative to the cell top-left, plus size).
struct FitBox { int x = 0, y = 0, w = 0, h = 0; };
inline FitBox FitRect(int srcW, int srcH, int cellW, int cellH) {
    if (cellW < 0) cellW = 0;
    if (cellH < 0) cellH = 0;
    if (srcW <= 0 || srcH <= 0) return FitBox{0, 0, cellW, cellH};
    // Scale so the whole image fits inside the cell (the smaller ratio wins).
    // 64-bit intermediates avoid overflow for large images.
    long long w = static_cast<long long>(cellH) * srcW / srcH;  // height-limited
    long long h;
    if (w <= cellW) {
        h = cellH;                                              // height is the limit
    } else {
        w = cellW;                                              // width is the limit
        h = static_cast<long long>(cellW) * srcH / srcW;
    }
    int iw = static_cast<int>(w);
    int ih = static_cast<int>(h);
    if (iw < 1) iw = 1;
    if (ih < 1) ih = 1;
    return FitBox{(cellW - iw) / 2, (cellH - ih) / 2, iw, ih};
}

// Square pixel size to request a thumbnail at for a given cell (the shorter
// side, so the image is never requested larger than the cell can show).
inline int RequestPx(int cellW, int cellH) {
    return cellW < cellH ? cellW : cellH;
}

}  // namespace thumbs
