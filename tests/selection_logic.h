// selection_logic.h
//
// PURE logic for the Ctrl+G "Selected Items" panel (waht to create.md #1),
// mirrored from ctrlg-group.wh.cpp so it can be unit tested with a
// plain g++ build (no Win32 runtime). The mod keeps a byte-for-byte copy and
// renders these strings; the tests pin the strings here.
//
// This module owns the *summary* of a selection ("Selected items (12)" and
// "12 items - 8 files - 4 folders"). Row formatting (icon -> name -> extension
// -> size) and the scroll model live in their own mirrored modules.
//
#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include <cwctype>

namespace sel {

// One entry in the current Explorer selection. sizeBytes < 0 means "unknown"
// (the shell did not report a size, e.g. for a folder).
struct Item {
    std::wstring name;
    bool isFolder = false;
    long long sizeBytes = -1;
};

// Count of files (non-folders) in the selection.
inline size_t FileCount(const std::vector<Item>& items) {
    size_t n = 0;
    for (const auto& it : items) if (!it.isFolder) ++n;
    return n;
}

// Count of folders in the selection.
inline size_t FolderCount(const std::vector<Item>& items) {
    size_t n = 0;
    for (const auto& it : items) if (it.isFolder) ++n;
    return n;
}

// "<n> <singular|plural>", picking singular only when n == 1.
inline std::wstring CountNoun(size_t n, const wchar_t* singular,
                              const wchar_t* plural) {
    return std::to_wstring(n) + L" " + (n == 1 ? singular : plural);
}

// Space + U+00B7 middle dot + space, the separator between count nouns.
inline const wchar_t* CountSep() { return L" \u00b7 "; }

// Title line shown at the top of the Selected Items panel, e.g.
// "Selected items (12)".
inline std::wstring SummaryTitle(size_t count) {
    return L"Selected items -- " + std::to_wstring(count);
}

// Detail line under the title, e.g. "12 items - 8 files - 4 folders", using a
// middle-dot separator. Each noun is singular only when its count is exactly 1.
inline std::wstring SummaryCounts(const std::vector<Item>& items) {
    const size_t files = FileCount(items);
    const size_t folders = FolderCount(items);
    const std::wstring sep = CountSep();
    return CountNoun(items.size(), L"item", L"items") + sep +
           CountNoun(files, L"file", L"files") + sep +
           CountNoun(folders, L"folder", L"folders");
}

// ---- Actionable type breakdown (fix thse isseu e.md #11) ----
// A summary that leads with the item count and then lists per-TYPE counts in
// first-seen order (uppercased extension; extensionless files as FILE/FILES;
// folders as folder/folders), finishing with the total KNOWN size. Example:
// "4 items - 3 MP3 - 1 JPG - 7.0 MB". More actionable than files/folders only.
// Forward declarations of the row-formatting pieces this reuses (defined below).
inline std::wstring ExtLabel(const std::wstring& name, bool isFolder);
inline std::wstring HumanSize(long long bytes);

inline std::wstring TypeBreakdown(const std::vector<Item>& items) {
    const std::wstring sep = CountSep();
    std::wstring out = CountNoun(items.size(), L"item", L"items");

    // Per-type groups keyed by an uppercase label, preserving first-seen order.
    // key: "" sentinel is unused; folder group uses a dedicated flag so it can
    // keep folder/folders pluralisation separate from file-type labels.
    std::vector<std::wstring> order;   // labels in first-seen order
    std::vector<size_t>       counts;  // parallel counts
    std::vector<bool>         isFolderGroup;
    auto bump = [&](const std::wstring& label, bool folderGroup) {
        for (size_t i = 0; i < order.size(); ++i) {
            if (order[i] == label && isFolderGroup[i] == folderGroup) {
                ++counts[i];
                return;
            }
        }
        order.push_back(label);
        counts.push_back(1);
        isFolderGroup.push_back(folderGroup);
    };

    long long totalBytes = 0;
    bool haveSize = false;
    for (const auto& it : items) {
        if (it.isFolder) {
            bump(L"folder", true);
        } else {
            std::wstring ext = ExtLabel(it.name, false);  // uppercased, no dot
            bump(ext.empty() ? L"FILE" : ext, false);
        }
        if (!it.isFolder && it.sizeBytes >= 0) {
            totalBytes += it.sizeBytes;
            haveSize = true;
        }
    }

    for (size_t i = 0; i < order.size(); ++i) {
        out += sep;
        if (isFolderGroup[i]) {
            out += CountNoun(counts[i], L"folder", L"folders");
        } else if (order[i] == L"FILE") {
            out += CountNoun(counts[i], L"FILE", L"FILES");
        } else {
            // File-type groups keep the type label fixed (e.g. "3 MP3"), not
            // pluralised, so it reads as a type tally rather than a noun.
            out += std::to_wstring(counts[i]) + L" " + order[i];
        }
    }

    if (haveSize && totalBytes > 0) {
        out += sep + HumanSize(totalBytes);
    }
    return out;
}

// ---- Row formatting (waht to create.md #1 / #12) ----
// icon -> filename -> extension -> optional size. The extension is a secondary,
// uppercased chip; the filename column shows the stem (without extension) for
// files and the whole name for folders. Pixel truncation of the filename is done
// by the GDI layer (DT_END_ELLIPSIS) -- this module only decomposes the pieces.

// stem + extension (extension WITHOUT the dot; empty for folders, dotless names,
// leading-dot "hidden" names, and trailing-dot names).
struct NameParts {
    std::wstring stem;
    std::wstring ext;
};
inline NameParts SplitName(const std::wstring& name, bool isFolder) {
    NameParts p;
    p.stem = name;
    if (isFolder) return p;   // folders never have an extension
    const std::wstring::size_type dot = name.find_last_of(L'.');
    // No dot, a leading dot ("hidden" file), or a trailing dot -> no extension.
    if (dot == std::wstring::npos || dot == 0 || dot + 1 >= name.size()) {
        return p;
    }
    p.stem = name.substr(0, dot);
    p.ext  = name.substr(dot + 1);
    return p;
}

// The filename column: stem for files, whole name for folders.
inline std::wstring RowName(const std::wstring& name, bool isFolder) {
    return SplitName(name, isFolder).stem;
}

// The filename TEXT drawn in a Selected Items row, honoring the Show-extensions
// toggle (fix thse isseue.md Phase 1B): extensions ON renders the COMPLETE
// filename inline for files (e.g. "README.md"); OFF renders the stem. Folders
// always render their whole name regardless of the toggle. A separate extension
// pill is never drawn in either state.
inline std::wstring RowDisplayName(const std::wstring& name, bool isFolder,
                                   bool extInline) {
    if (extInline && !isFolder) return name;
    return RowName(name, isFolder);
}

// The secondary extension chip, uppercased (e.g. "MP3"); empty when none.
inline std::wstring ExtLabel(const std::wstring& name, bool isFolder) {
    std::wstring ext = SplitName(name, isFolder).ext;
    for (auto& c : ext) c = static_cast<wchar_t>(towupper(c));
    return ext;
}

// Human-readable size ("512 B", "1.5 KB", "24 KB", "32 MB", "1.0 GB"); empty
// when the size is unknown (bytes < 0). Uses 1024-based units with one decimal
// place only while the scaled value is < 10, otherwise a whole number, to keep
// the badge short.
inline std::wstring HumanSize(long long bytes) {
    if (bytes < 0) return std::wstring();
    if (bytes < 1024) return std::to_wstring(bytes) + L" B";
    const wchar_t* units[] = { L"KB", L"MB", L"GB", L"TB" };
    long long unit = 1024;
    int idx = 0;
    while (idx < 3 && bytes >= unit * 1024) { unit *= 1024; ++idx; }
    const long long tenths = (bytes * 10 + unit / 2) / unit;
    if (tenths >= 100) {
        // Value is >= 10 in this unit: a whole number reads cleaner than a
        // trailing ".0" and keeps the badge short (e.g. "24 KB", "318 KB").
        const long long whole = (bytes + unit / 2) / unit;
        return std::to_wstring(whole) + L" " + units[idx];
    }
    return std::to_wstring(tenths / 10) + L"." + std::to_wstring(tenths % 10) +
           L" " + units[idx];
}

// Pick which byte count to display: prefer the filesystem's size-ON-DISK
// (allocated) metadata when available (allocated >= 0), else the logical file
// length, else unknown (-1). The actual metadata query lives in the Win32
// capture layer; this is only the selection rule so it can be unit-tested.
inline long long ChooseSizeBytes(long long logicalBytes, long long allocatedBytes) {
    if (allocatedBytes >= 0) return allocatedBytes;
    return logicalBytes;
}

}  // namespace sel
