// shell_icon_logic.h
//
// PURE decision logic for fix thse isseue.md #9: the Selected Items ROWS
// (files AND folders) must use the REAL Windows Shell icon -- the same icon
// File Explorer shows for a .pdf / .jpg / .mp3 / .docx / folder -- instead of
// the mod's generic custom vector glyph. The mod draws those via
// SHGetFileInfoW(..., SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)
// + DrawIconEx + DestroyIcon (user32/shell32, already linked -- no new
// comctl32 dependency). The custom vector layer stays for the UI-SEMANTIC
// slots only (Selected header, Destination, Name, Action, Status, Expand).
//
// This header is the byte-for-byte mirror of namespace shellicon in the mod,
// so the flag/attribute/geometry decisions are unit-tested with a plain g++
// build (no Win32 runtime). The actual SHGetFileInfoW / DrawIconEx CALLS are
// Win32 and are asserted by scripts/regression_check.ps1 source invariants.
//
// The SHGFI_* / FILE_ATTRIBUTE_* numeric values are mirrored here (same
// pattern as the ScaleDpi/MulDiv and RGB mirrors elsewhere in tests/):
//   SHGFI_ICON              0x000000100
//   SHGFI_SMALLICON         0x000000001
//   SHGFI_USEFILEATTRIBUTES 0x000000010
//   FILE_ATTRIBUTE_DIRECTORY 0x00000010
//   FILE_ATTRIBUTE_NORMAL    0x00000080
//
#pragma once

#include <string>

namespace shellicon {

inline constexpr unsigned kSHGFI_ICON               = 0x000000100u;
inline constexpr unsigned kSHGFI_SMALLICON          = 0x000000001u;
inline constexpr unsigned kSHGFI_USEFILEATTRIBUTES  = 0x000000010u;
inline constexpr unsigned kFILE_ATTRIBUTE_DIRECTORY = 0x00000010u;
inline constexpr unsigned kFILE_ATTRIBUTE_NORMAL    = 0x00000080u;

// Every Selected-Items ROW -- file OR folder -- is rendered from the Shell
// image list. (The custom vector layer is reserved for UI semantics.)
inline bool RowUsesShellIcon(bool /*isFolder*/) { return true; }

// Flags for a TYPE/extension lookup that never touches disk: USEFILEATTRIBUTES
// means SHGetFileInfo reads only the attribute + the path's extension, so an
// item whose real path is gone (or expensive to stat) still gets its icon.
inline unsigned IconFlags() {
    return kSHGFI_ICON | kSHGFI_SMALLICON | kSHGFI_USEFILEATTRIBUTES;
}

// The file-attributes value handed to SHGetFileInfo: directory bit for folders
// (-> Windows folder icon), normal otherwise (-> extension-associated icon).
inline unsigned AttrFor(bool isFolder) {
    return isFolder ? kFILE_ATTRIBUTE_DIRECTORY : kFILE_ATTRIBUTE_NORMAL;
}

// 16px "small icon" baseline, DPI-scaled with MulDiv rounding (matches the
// mod's ScaleDpi(16, dpi)): 96->16, 120->20, 144->24, 168->28, 192->32.
inline int IconPx(unsigned dpi) { return static_cast<int>((16u * dpi + 48u) / 96u); }

// A stable per-type cache key so repeated extensions reuse ONE Shell lookup:
// folders collapse to "<dir>", files to their lowercased extension incl dot,
// and extensionless files to "<noext>".
inline std::wstring IconKey(const std::wstring& name, bool isFolder) {
    if (isFolder) return L"<dir>";
    const size_t dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos || dot + 1 >= name.size()) return L"<noext>";
    std::wstring ext = name.substr(dot);
    for (wchar_t& c : ext)
        if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
    return ext;
}

}  // namespace shellicon
