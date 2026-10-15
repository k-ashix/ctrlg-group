// chip_model_logic.h
//
// PURE model for the Selected Items summary CHIP STRIP (fix thse isseue.md
// #2/#3/#6): turns the current selection into an ordered, dynamic list of
// metadata chips -- [N items] [N folders] [count EXT]... with a single
// [count No extension] chip for extensionless files. Extension grouping is
// case-insensitive (.md and .MD share one chip). Mirrored byte-for-byte into
// ctrlg-group.wh.cpp.
//
// Self-contained: own main() lives in chip_model_test.cpp, exit 0 = pass.
#pragma once

#include <string>
#include <vector>
#include <cstddef>

#include "selection_logic.h"

namespace chipmodel {

enum class Kind { Items, Folders, Ext, NoExt };

struct Chip {
    Kind kind = Kind::Items;
    std::wstring label;    // fully composed display text, e.g. "9 items", "3 MD"
    std::size_t count = 0;
    std::wstring extKey;   // normalized lowercase ext key ("md"); empty otherwise
};

// Lowercase + strip a single leading dot. "MD"/".md"/"Md" -> "md".
inline std::wstring NormExtKey(const std::wstring& extLabel) {
    std::wstring e = extLabel;
    std::size_t start = (!e.empty() && e[0] == L'.') ? 1u : 0u;
    std::wstring out;
    for (std::size_t i = start; i < e.size(); ++i) {
        wchar_t c = e[i];
        if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
        out.push_back(c);
    }
    return out;
}

// Uppercase a normalized key for display. "md" -> "MD".
inline std::wstring UpperExt(const std::wstring& key) {
    std::wstring out;
    for (wchar_t c : key) {
        if (c >= L'a' && c <= L'z') c = static_cast<wchar_t>(c - L'a' + L'A');
        out.push_back(c);
    }
    return out;
}

// Build the ordered chip list from the current selection. Order: items chip,
// a folders chip (only when folders exist), then one chip per unique file
// extension in first-seen order, with extensionless files collapsed into a
// single "No extension" chip in their first-seen position.
inline std::vector<Chip> Build(const std::vector<sel::Item>& items) {
    const std::size_t total = items.size();
    std::size_t folders = 0;
    for (const auto& it : items) if (it.isFolder) ++folders;

    std::vector<Chip> out;
    {
        Chip c;
        c.kind = Kind::Items;
        c.count = total;
        c.label = std::to_wstring(total) + L" " + (total == 1 ? L"item" : L"items");
        out.push_back(c);
    }
    if (folders > 0) {
        Chip c;
        c.kind = Kind::Folders;
        c.count = folders;
        c.label = std::to_wstring(folders) + L" " +
                  (folders == 1 ? L"folder" : L"folders");
        out.push_back(c);
    }

    std::vector<std::wstring> keys;   // first-seen order; "" == no extension
    std::vector<std::size_t>  counts;
    for (const auto& it : items) {
        if (it.isFolder) continue;
        const std::wstring key = NormExtKey(sel::ExtLabel(it.name, false));
        std::size_t idx = keys.size();
        for (std::size_t i = 0; i < keys.size(); ++i) {
            if (keys[i] == key) { idx = i; break; }
        }
        if (idx == keys.size()) { keys.push_back(key); counts.push_back(1); }
        else ++counts[idx];
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
        Chip c;
        c.count = counts[i];
        if (keys[i].empty()) {
            c.kind = Kind::NoExt;
            c.label = std::to_wstring(counts[i]) + L" No extension";
        } else {
            c.kind = Kind::Ext;
            c.extKey = keys[i];
            c.label = std::to_wstring(counts[i]) + L" " + UpperExt(keys[i]);
        }
        out.push_back(c);
    }
    return out;
}

}  // namespace chipmodel
