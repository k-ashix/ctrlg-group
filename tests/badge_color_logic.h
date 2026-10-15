// badge_color_logic.h
//
// PURE per-extension badge COLOUR logic for fix thse isseue.md #12: instead of
// one neutral chip for every row, the extension pill gets a SOFT, category-
// based colour (soft background + a darker ink used for both the text and the
// hairline border -- never a saturated full-strength block). Byte-for-byte
// mirror of namespace badgecolor in ctrlg-group.wh.cpp; the GDI
// paint lives in the mod and is pinned by source invariants.
//
// The input is the badge LABEL as produced by sel::ExtLabel (uppercase, no
// leading dot, e.g. "PDF"); Norm() lowercases and tolerates an optional dot.
// Colours are packed 0xRRGGBB; the Neutral category returns inherit==true,
// meaning "keep the themed pal.badgeFill / pal.badgeBorder chip".
//
#pragma once

#include <string>

namespace badgecolor {

enum class Cat { Neutral, Image, Audio, Video, Document, Archive, Code };

// Lowercase an extension label and drop a single leading dot if present.
inline std::wstring Norm(const std::wstring& ext) {
    std::wstring e = ext;
    size_t start = (!e.empty() && e[0] == L'.') ? 1u : 0u;
    std::wstring out;
    for (size_t i = start; i < e.size(); ++i) {
        wchar_t c = e[i];
        if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
        out.push_back(c);
    }
    return out;
}

inline Cat CategoryFor(const std::wstring& extLabel) {
    const std::wstring e = Norm(extLabel);
    auto is = [&](std::initializer_list<const wchar_t*> xs) {
        for (const wchar_t* x : xs) if (e == x) return true;
        return false;
    };
    if (is({L"jpg", L"jpeg", L"png", L"gif", L"bmp", L"webp", L"svg", L"ico",
            L"tif", L"tiff", L"heic"})) return Cat::Image;
    if (is({L"mp3", L"wav", L"flac", L"aac", L"ogg", L"m4a", L"wma"}))
        return Cat::Audio;
    if (is({L"mp4", L"mkv", L"mov", L"avi", L"wmv", L"webm", L"flv", L"m4v"}))
        return Cat::Video;
    if (is({L"pdf", L"doc", L"docx", L"txt", L"rtf", L"odt", L"xls", L"xlsx",
            L"ppt", L"pptx", L"md", L"csv"})) return Cat::Document;
    if (is({L"zip", L"rar", L"7z", L"tar", L"gz", L"bz2", L"xz", L"cab"}))
        return Cat::Archive;
    if (is({L"c", L"cpp", L"cc", L"h", L"hpp", L"cs", L"js", L"ts", L"py",
            L"java", L"rb", L"go", L"rs", L"php", L"json", L"xml", L"html",
            L"css", L"sh", L"ps1"})) return Cat::Code;
    return Cat::Neutral;
}

// Resolved chip: a soft fill + a darker (light theme) / lighter (dark theme)
// "ink" reused for the text and the border. Neutral inherits the themed chip.
struct Chip {
    bool inherit;
    unsigned fill;   // 0xRRGGBB
    unsigned ink;    // 0xRRGGBB (text + border)
};

inline Chip ChipFor(Cat c, bool dark) {
    switch (c) {
        case Cat::Image:
            return dark ? Chip{false, 0x1E3346u, 0x8FC2ECu}
                        : Chip{false, 0xDCEBFBu, 0x1A4E7Au};
        case Cat::Audio:
            return dark ? Chip{false, 0x332844u, 0xC3A6E6u}
                        : Chip{false, 0xE9DEF7u, 0x5B3B86u};
        case Cat::Video:
            return dark ? Chip{false, 0x3E2838u, 0xE6A6CCu}
                        : Chip{false, 0xF7DEEEu, 0x7A2E5Bu};
        case Cat::Document:
            return dark ? Chip{false, 0x44281Eu, 0xE6A48Fu}
                        : Chip{false, 0xFBE2DCu, 0x8A2E1Au};
        case Cat::Archive:
            return dark ? Chip{false, 0x3C3418u, 0xDBC27Eu}
                        : Chip{false, 0xF7EDD2u, 0x7A5A12u};
        case Cat::Code:
            return dark ? Chip{false, 0x1E3A28u, 0x8FD6A6u}
                        : Chip{false, 0xDCF2E3u, 0x1E5F38u};
        case Cat::Neutral:
        default:
            return Chip{true, 0u, 0u};
    }
}

// Convenience: resolve straight from the label.
inline Chip ChipForLabel(const std::wstring& extLabel, bool dark) {
    return ChipFor(CategoryFor(extLabel), dark);
}

// A SOFTER hairline than the text ink (fix thse isseue.md #9/#14: "border less
// dominant", "no hard black outline"). The text keeps the full category ink;
// the border is the ink blended 55% toward the chip fill, so it reads as a
// muted edge of the same hue -- lighter than the ink in light theme, softer
// than the ink in dark theme, and never a saturated/black outline. Only
// meaningful for a tinted (non-inherit) chip; the neutral chip keeps the
// themed pal.badgeBorder.
inline unsigned SoftBorder(const Chip& c) {
    auto chan = [](unsigned v, int shift) { return (v >> shift) & 0xFFu; };
    // 45% ink + 55% fill, rounded -> biased toward the fill (less dominant).
    auto mix = [](unsigned ink, unsigned fill) {
        return (ink * 45u + fill * 55u + 50u) / 100u;
    };
    const unsigned r = mix(chan(c.ink, 16), chan(c.fill, 16));
    const unsigned g = mix(chan(c.ink, 8),  chan(c.fill, 8));
    const unsigned b = mix(chan(c.ink, 0),  chan(c.fill, 0));
    return (r << 16) | (g << 8) | b;
}

}  // namespace badgecolor
