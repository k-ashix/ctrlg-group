// auto_group_settings_test.cpp
// Pins fix thse isseue.md ADD-ON Phase B: Auto Group settings parsing is pure,
// isolated, and safely defaulted. The mod's LoadSettings() calls Windhawk APIs
// (not runnable headless), so the DECODING rules it applies are mirrored as the
// pure autogroup::ParseLayout / autogroup::NameOrDefault helpers and pinned
// here. Self-contained: own main(), exit 0 = pass.

#include "auto_group_logic.h"

#include <cstdio>

using namespace autogroup;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ---- layout option decoding (default Flat, only exact id selects Images) ----
    check(ParseLayout(L"flat") == Layout::Flat, "flat -> Flat");
    check(ParseLayout(L"imagesParent") == Layout::ImagesParent,
          "imagesParent -> ImagesParent");
    check(ParseLayout(nullptr) == Layout::Flat, "null option -> Flat default");
    check(ParseLayout(L"") == Layout::Flat, "empty option -> Flat default");
    check(ParseLayout(L"Flat") == Layout::Flat, "unknown case -> Flat default");
    check(ParseLayout(L"images") == Layout::Flat,
          "partial/unknown id -> Flat default (no accidental Images)");

    // ---- configurable names: trim + non-empty fallback ----
    check(NameOrDefault(L"Images", L"Images") == L"Images", "plain name kept");
    check(NameOrDefault(L"  Pics  ", L"Images") == L"Pics",
          "surrounding whitespace trimmed");
    check(NameOrDefault(L"", L"Images") == L"Images", "blank -> default");
    check(NameOrDefault(L"   ", L"All Folder") == L"All Folder",
          "whitespace-only -> default");
    check(NameOrDefault(L"My Photos", L"Images") == L"My Photos",
          "internal spaces preserved");
    check(NameOrDefault(L"\tNo Ext\t", L"No Extension") == L"No Ext",
          "tabs trimmed, inner text kept");

    // ---- the defaults a blank setting falls back to are themselves valid
    // bucket names (so a default config always yields a usable path) ----
    check(SanitizeBucketName(NameOrDefault(L"", L"Images")).ok,
          "default Images is a valid bucket");
    check(SanitizeBucketName(NameOrDefault(L"", L"All Folder")).ok,
          "default All Folder is a valid bucket");
    check(SanitizeBucketName(NameOrDefault(L"", L"No Extension")).ok,
          "default No Extension is a valid bucket");

    // ---- Settings defaults mirror the documented Windhawk defaults ----
    {
        Settings s;  // the struct's in-class defaults
        check(s.layout == Layout::Flat, "default layout is Flat");
        check(s.imagesParentName == L"Images", "default images parent is Images");
        check(!s.groupSelectedFolders, "selected-folder grouping OFF by default");
        check(s.allFolderName == L"All Folder", "default folders name is All Folder");
        check(s.noExtensionEnabled, "no-extension grouping ON by default");
        check(s.noExtensionName == L"No Extension", "default no-ext name");
    }

    if (g_fail) { printf("auto_group_settings_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("auto_group_settings_test: OK\n");
    return 0;
}
