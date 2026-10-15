// name_validation_logic.h
//
// PURE folder-name validation for the popup's contextual, pre-Create
// validation (waht to create.md #6 / #14), mirrored from
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build (no Win32 runtime). The mod keeps a byte-for-byte copy and shows the
// message beside the name field; the tests pin the decisions here.
//
// This does NOT change folder-creation semantics or the Ctrl+G destination
// resolver. It only decides, up front, whether the CURRENT form state would be
// rejected -- so the popup can show a concise inline message instead of failing
// silently later. Blank names are deliberately allowed where existing behavior
// makes them valid (New Folder default; Group fixed-dest / last-target reuse).
//
#pragma once

#include <string>
#include <cwctype>

namespace namevalid {

enum class NameCheck {
    Ok = 0,
    EmptyNoDestination,   // blank name and nothing else can produce a destination
    IllegalCharacter,     // contains \ / : * ? " < > | or a control character
    ReservedName,         // CON, PRN, AUX, NUL, COM1-9, LPT1-9 (any case/ext)
    DotsOnly,             // only dots ("." / ".." / ...)
    TrailingDotOrSpace,   // ends with a space or a period (Windows strips these)
};

inline std::wstring TrimWs(const std::wstring& s) {
    size_t b = 0, e = s.size();
    while (b < e && iswspace(s[b])) ++b;
    while (e > b && iswspace(s[e - 1])) --e;
    return s.substr(b, e - b);
}

inline bool HasIllegalChar(const std::wstring& name) {
    for (wchar_t c : name) {
        if (c < 0x20) return true;                 // control characters
        if (c == L'<' || c == L'>' || c == L':' || c == L'"' || c == L'/' ||
            c == L'\\' || c == L'|' || c == L'?' || c == L'*') {
            return true;
        }
    }
    return false;
}

inline bool IsDotsOnly(const std::wstring& name) {
    if (name.empty()) return false;
    for (wchar_t c : name) if (c != L'.') return false;
    return true;
}

// Reserved DOS device names. The stem (text before the first '.') is compared
// case-insensitively; an extension does NOT make the name usable ("CON.txt" is
// still reserved).
inline bool IsReservedName(const std::wstring& name) {
    std::wstring::size_type dot = name.find(L'.');
    std::wstring stem = (dot == std::wstring::npos) ? name : name.substr(0, dot);
    if (stem.empty()) return false;
    std::wstring up;
    up.reserve(stem.size());
    for (wchar_t c : stem) up.push_back(static_cast<wchar_t>(towupper(c)));
    if (up == L"CON" || up == L"PRN" || up == L"AUX" || up == L"NUL") return true;
    // COM1-9 / LPT1-9 (exactly one digit 1..9).
    if (up.size() == 4) {
        std::wstring prefix = up.substr(0, 3);
        wchar_t d = up[3];
        if ((prefix == L"COM" || prefix == L"LPT") && d >= L'1' && d <= L'9') {
            return true;
        }
    }
    return false;
}

// Forbidden filename characters (mirror of the mod's IsForbiddenNameChar).
inline bool IsForbiddenNameChar(wchar_t c) {
    return c == L'<' || c == L'>' || c == L':' || c == L'"' || c == L'/' ||
           c == L'\\' || c == L'|' || c == L'?' || c == L'*';
}

// Byte-for-byte mirror of the mod's SanitizeFolderBase(): strip C0 control
// chars and forbidden characters, then trim leading whitespace and trailing
// whitespace/dots. This is what the existing creation pipeline does BEFORE
// creating the folder, so validation must reason about the sanitized base --
// never reject a name the pipeline would have quietly repaired (#1).
inline std::wstring SanitizeFolderBase(const std::wstring& raw) {
    std::wstring s;
    s.reserve(raw.size());
    for (wchar_t c : raw) {
        if (c < 0x20) continue;
        if (IsForbiddenNameChar(c)) continue;
        s.push_back(c);
    }
    size_t b = 0;
    while (b < s.size() && iswspace(s[b])) ++b;
    size_t e = s.size();
    while (e > b && (iswspace(s[e - 1]) || s[e - 1] == L'.')) --e;
    return s.substr(b, e - b);
}

// Validate a NON-EMPTY sanitized base. After sanitizing, the only thing that
// still makes a base un-creatable is a reserved DOS device name (CON, NUL,
// COM1..9, LPT1..9) -- those survive sanitizing but CreateDirectoryW rejects
// them. Illegal chars / dots-only / trailing dot+space are already gone.
inline NameCheck ValidateName(const std::wstring& sanitizedBase) {
    if (IsReservedName(sanitizedBase)) return NameCheck::ReservedName;
    return NameCheck::Ok;
}

// New Folder: a blank (or sanitizes-to-blank) name defaults to "New folder", so
// blank is Ok; a non-empty sanitized base is Ok unless it is reserved.
inline NameCheck ValidateNewFolderName(const std::wstring& typedName) {
    std::wstring base = SanitizeFolderBase(typedName);
    if (base.empty()) return NameCheck::Ok;
    return ValidateName(base);
}

// Group: mirror ResolveGroupDestination precedence (fixed > typed > last).
//   * A fixed destination wins outright -> the typed name is irrelevant, so we
//     never reject it (the configured path is assumed valid).
//   * Otherwise the typed name is sanitized (like the creation pipeline). A
//     non-empty sanitized base is Ok unless it is a reserved device name.
//   * A blank (or sanitizes-to-blank) typed name is Ok when a last target can
//     be reused; otherwise it needs a name.
inline NameCheck ValidateGroupName(const std::wstring& typedName,
                                   bool hasFixedDest, bool hasLastTarget) {
    if (hasFixedDest) return NameCheck::Ok;
    std::wstring base = SanitizeFolderBase(typedName);
    if (base.empty()) {
        return hasLastTarget ? NameCheck::Ok : NameCheck::EmptyNoDestination;
    }
    return ValidateName(base);
}

// Concise, human-readable message for the inline validation label. Ok -> empty.
inline std::wstring ValidationMessage(NameCheck c) {
    switch (c) {
        case NameCheck::EmptyNoDestination:
            return L"Enter a folder name";
        case NameCheck::IllegalCharacter:
            return L"A folder name can't contain \\ / : * ? \" < > |";
        case NameCheck::ReservedName:
            return L"That name is reserved by Windows";
        case NameCheck::DotsOnly:
            return L"Enter a valid folder name";
        case NameCheck::TrailingDotOrSpace:
            return L"A folder name can't end with a space or period";
        case NameCheck::Ok:
        default:
            return std::wstring();
    }
}

}  // namespace namevalid
