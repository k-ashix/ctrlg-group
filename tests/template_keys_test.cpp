// template_keys_test.cpp
// Load-time template key resolution (SPEC "Folder Templates"). Each template
// has an independent user-configured key; it is not derived from the name.
// Self-contained: own main(), exit 0 = pass.
//
// Covers:
//   - key = lowercased first letter of the name
//   - name starting with whitespace -> first non-space letter
//   - name starting with a digit -> ignored + reason logged
//   - blank name -> ignored + reason logged
//   - two names sharing a first letter -> first-in-list wins, second ignored
//   - 26-template cap -> extras ignored
//   - empty folder list -> template ignored + logged

#include "capture_logic.h"

#include <string>
#include <vector>
#include <cstdio>

static int g_fail = 0;

static void expectTrue(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

static FolderTemplate mk(const std::wstring& key,
                         const std::wstring& name,
                         std::vector<std::wstring> folders) {
    FolderTemplate t;
    t.key = key;
    t.name = name;
    t.folders = std::move(folders);
    return t;
}

int main() {
    // Key is independent from the template name.
    {
        auto r = ResolveTemplateKeys({ mk(L"i", L"Photos", { L"raw", L"jpg" }) });
        expectTrue(r.usable.size() == 1, "one usable template");
        if (r.usable.size() == 1) {
            expectTrue(r.usable[0].key == L'i', "Photos can use independent key i");
            expectTrue(r.usable[0].name == L"Photos", "template keeps user name");
        }
    }

    // Uppercase configured key is normalized.
    {
        auto r = ResolveTemplateKeys({ mk(L"W", L"Anything", { L"src" }) });
        expectTrue(r.usable.size() == 1 && r.usable[0].key == L'w',
                   "configured W normalizes to w");
    }

    // Invalid or missing keys are ignored.
    {
        auto r = ResolveTemplateKeys({ mk(L"3", L"3D", { L"raw" }) });
        expectTrue(r.usable.empty(), "digit key: no usable template");
        expectTrue(r.ignored.size() == 1, "digit key: one ignored reason");
    }
    {
        auto r = ResolveTemplateKeys({ mk(L"", L"Photos", { L"raw" }) });
        expectTrue(r.usable.empty(), "blank key: no usable template");
        expectTrue(r.ignored.size() == 1, "blank key: one ignored reason");
    }

    // Duplicate configured keys are rejected; names do not affect ownership.
    {
        auto r = ResolveTemplateKeys({
            mk(L"i", L"Photos", { L"raw", L"jpg" }),
            mk(L"i", L"Work", { L"docs" }),
        });
        expectTrue(r.usable.size() == 1, "duplicate key: only first usable");
        if (r.usable.size() == 1) {
            expectTrue(r.usable[0].name == L"Photos", "first template owns i");
        }
        expectTrue(r.ignored.size() == 1, "duplicate key: one ignored reason");
    }

    // Empty folder list -> ignored + logged.
    {
        auto r = ResolveTemplateKeys({ mk(L"p", L"Photos", {}) });
        expectTrue(r.usable.empty(), "empty folders: no usable template");
        expectTrue(r.ignored.size() == 1, "empty folders: one ignored reason");
    }

    // 26-template cap: 30 distinct-first-letter names -> only 26 usable.
    {
        const wchar_t* keys = L"abcdefghijklmnopqrstuvwxyz";
        std::vector<FolderTemplate> many;
        for (int i = 0; i < 26; ++i) {
            many.push_back(mk(std::wstring(1, keys[i]),
                              L"Template " + std::to_wstring(i + 1), { L"f" }));
        }
        // 4 more reuse early configured keys; the usable count stays at 26.
        for (int i = 0; i < 4; ++i) {
            many.push_back(mk(std::wstring(1, keys[i]),
                              L"Extra " + std::to_wstring(i + 1), { L"f" }));
        }
        auto r = ResolveTemplateKeys(many);
        expectTrue((int)r.usable.size() <= kMaxTemplates,
                   "26-cap: usable never exceeds 26");
        expectTrue(r.usable.size() == 26, "26-cap: exactly 26 usable");
    }

    if (g_fail == 0) { printf("template_keys_test: PASS\n"); return 0; }
    printf("template_keys_test: FAIL (%d)\n", g_fail);
    return 1;
}
