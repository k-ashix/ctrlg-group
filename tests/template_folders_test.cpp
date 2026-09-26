// template_folders_test.cpp
// Fire-time template folder resolution (SPEC "Folder Templates" collision
// modes + validation + caps). Self-contained: own main(), exit 0 = pass.
//
// Covers:
//   - Skip: existing dropped, rest created, order preserved
//   - Number: existing name -> numbered sibling via FormatBatchName
//   - Stop: any existing -> empty (create nothing)
//   - Stop: none existing -> all created
//   - duplicate folder name inside a template -> first kept, dup dropped
//   - invalid folder names rejected (separator, .., absolute, empty, control)
//   - 100-folder cap
//   - IsValidTemplateFolderName / NormalizeTemplateKey unit checks

#include "capture_logic.h"

#include <set>
#include <string>
#include <vector>
#include <cstdio>

static int g_fail = 0;

static void expectVec(const std::vector<std::wstring>& got,
                      const std::vector<std::wstring>& want,
                      const char* what) {
    bool ok = got.size() == want.size();
    for (size_t i = 0; ok && i < want.size(); ++i) {
        if (got[i] != want[i]) ok = false;
    }
    if (!ok) {
        printf("[FAIL] %s\n  got (%zu):\n", what, got.size());
        for (const auto& s : got) printf("    %ls\n", s.c_str());
        printf("  want (%zu):\n", want.size());
        for (const auto& s : want) printf("    %ls\n", s.c_str());
        ++g_fail;
    }
}

// Overload: #1 fix makes ResolveTemplateFolders return {base, path} plans.
// Compare by PATH so the existing path-based expectations keep working.
static void expectVec(const std::vector<TemplateFolderPlan>& got,
                      const std::vector<std::wstring>& want,
                      const char* what) {
    std::vector<std::wstring> p;
    for (const auto& e : got) p.push_back(e.path);
    expectVec(p, want, what);
}

static void expectTrue(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// Build a resolved template directly (bypasses key resolution for folder tests).
static ResolvedTemplate tmpl(std::vector<std::wstring> folders) {
    ResolvedTemplate t;
    t.key = L'p';
    t.name = L"Photos";
    t.folders = std::move(folders);
    return t;
}

int main() {
    const std::wstring dir = L"C:\\x";

    // --- IsValidTemplateFolderName --------------------------------------
    expectTrue(IsValidTemplateFolderName(L"raw"), "valid: 'raw'");
    expectTrue(!IsValidTemplateFolderName(L"raw\\jpg"), "invalid: backslash");
    expectTrue(!IsValidTemplateFolderName(L"raw/jpg"), "invalid: forward slash");
    expectTrue(!IsValidTemplateFolderName(L"../jpg"), "invalid: dot-dot");
    expectTrue(!IsValidTemplateFolderName(L".."), "invalid: '..'");
    expectTrue(!IsValidTemplateFolderName(L"C:\\jpg"), "invalid: absolute");
    expectTrue(!IsValidTemplateFolderName(L""), "invalid: empty");
    expectTrue(!IsValidTemplateFolderName(L"   "), "invalid: whitespace only");
    expectTrue(!IsValidTemplateFolderName(std::wstring(L"a\tb")),
               "invalid: control char");

    // --- NormalizeTemplateKey -------------------------------------------
    expectTrue(NormalizeTemplateKey(L"P") == L'p', "key 'P' -> 'p'");
    expectTrue(NormalizeTemplateKey(L"  q ") == L'q', "key '  q ' -> 'q'");
    expectTrue(NormalizeTemplateKey(L"") == 0, "key '' -> 0");
    expectTrue(NormalizeTemplateKey(L"3") == 0, "key '3' -> 0 (digit)");

    // --- Skip: existing dropped, order preserved ------------------------
    {
        std::set<std::wstring> existing = { L"C:\\x\\jpg" };
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(ResolveTemplateFolders(dir, tmpl({ L"raw", L"jpg", L"png" }),
                                         exists, TemplateCollision::Skip,
                                         NamePattern::Space),
                  { L"C:\\x\\raw", L"C:\\x\\png" },
                  "skip: existing 'jpg' dropped, order preserved");
    }

    // --- Skip: nothing existing -> all created --------------------------
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(ResolveTemplateFolders(dir, tmpl({ L"raw", L"jpg" }),
                                         exists, TemplateCollision::Skip,
                                         NamePattern::Space),
                  { L"C:\\x\\raw", L"C:\\x\\jpg" },
                  "skip: none existing -> all created");
    }

    // --- Number: existing name -> numbered sibling via FormatBatchName --
    {
        std::set<std::wstring> existing = { L"C:\\x\\raw" };
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        // "raw" exists -> Space pattern first free is "raw 2"; "jpg" is free.
        expectVec(ResolveTemplateFolders(dir, tmpl({ L"raw", L"jpg" }),
                                         exists, TemplateCollision::Number,
                                         NamePattern::Space),
                  { L"C:\\x\\raw 2", L"C:\\x\\jpg" },
                  "number: existing 'raw' -> 'raw 2', 'jpg' as-is");
    }

    // --- Stop: any existing -> empty ------------------------------------
    {
        std::set<std::wstring> existing = { L"C:\\x\\png" };
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(ResolveTemplateFolders(dir, tmpl({ L"raw", L"jpg", L"png" }),
                                         exists, TemplateCollision::Stop,
                                         NamePattern::Space),
                  {},
                  "stop: one exists -> create nothing");
    }

    // --- Stop: none existing -> all created -----------------------------
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(ResolveTemplateFolders(dir, tmpl({ L"raw", L"jpg" }),
                                         exists, TemplateCollision::Stop,
                                         NamePattern::Space),
                  { L"C:\\x\\raw", L"C:\\x\\jpg" },
                  "stop: none exist -> all created");
    }

    // --- Duplicate folder inside template: first kept, dup dropped ------
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(ResolveTemplateFolders(dir, tmpl({ L"raw", L"jpg", L"raw" }),
                                         exists, TemplateCollision::Skip,
                                         NamePattern::Space),
                  { L"C:\\x\\raw", L"C:\\x\\jpg" },
                  "dup folder: [raw,jpg,raw] -> [raw,jpg]");
    }

    // --- Invalid folder names dropped -----------------------------------
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(ResolveTemplateFolders(dir,
                    tmpl({ L"raw", L"a\\b", L"..", L"C:\\z", L"", L"png" }),
                    exists, TemplateCollision::Skip, NamePattern::Space),
                  { L"C:\\x\\raw", L"C:\\x\\png" },
                  "invalid names dropped, valid kept in order");
    }

    // --- 100-folder cap -------------------------------------------------
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        std::vector<std::wstring> folders;
        for (int i = 0; i < 150; ++i) folders.push_back(L"f" + std::to_wstring(i));
        auto got = ResolveTemplateFolders(dir, tmpl(folders), exists,
                                          TemplateCollision::Skip,
                                          NamePattern::Space);
        expectTrue((int)got.size() <= kMaxFoldersPerTemplate,
                   "100-cap: never more than 100 created");
        expectTrue(got.size() == 100, "100-cap: exactly 100 kept");
    }

    // --- #7 template validation == typed-name validation ----------------
    // A typed folder name strips \ / : * ? " < > | (SanitizeFolderBase). A
    // template entry must REJECT the same set -- not just \ / : -- so a
    // template can never smuggle a character Windows forbids in a folder name.
    {
        expectTrue(!IsValidTemplateFolderName(L"a*b"), "invalid: asterisk");
        expectTrue(!IsValidTemplateFolderName(L"a?b"), "invalid: question mark");
        expectTrue(!IsValidTemplateFolderName(L"a\"b"), "invalid: double quote");
        expectTrue(!IsValidTemplateFolderName(L"a<b"), "invalid: less-than");
        expectTrue(!IsValidTemplateFolderName(L"a>b"), "invalid: greater-than");
        expectTrue(!IsValidTemplateFolderName(L"a|b"), "invalid: pipe");
        // still accepts a legit name with a comma / space / ampersand
        expectTrue(IsValidTemplateFolderName(L"01 Brief & Notes, v2"),
                   "valid: comma/space/ampersand kept");
    }
    {
        // The full forbidden set is dropped by the resolver too, valid kept.
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(ResolveTemplateFolders(dir,
                    tmpl({ L"raw", L"a*b", L"a?b", L"a|b", L"png" }),
                    exists, TemplateCollision::Skip, NamePattern::Space),
                  { L"C:\\x\\raw", L"C:\\x\\png" },
                  "resolver drops * ? | names, keeps valid ones");
    }

    // --- #2 Number must not PLAN the same path twice --------------------
    // Template [raw, "raw 2"] with "raw" already on disk. Numbering "raw"
    // yields "raw 2"; the literal "raw 2" entry must then be treated as taken
    // (already reserved this run) and get its own free sibling -- never a
    // duplicate path that would collide at create time.
    {
        std::set<std::wstring> existing = { L"C:\\x\\raw" };
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        auto got = ResolveTemplateFolders(dir, tmpl({ L"raw", L"raw 2" }),
                                          exists, TemplateCollision::Number,
                                          NamePattern::Space);
        expectVec(got, { L"C:\\x\\raw 2", L"C:\\x\\raw 2 2" },
                  "number: reserves planned path (raw->raw 2, 'raw 2'->'raw 2 2')");
        std::set<std::wstring> uniq;
        for (const auto& e : got) uniq.insert(e.path);
        expectTrue(uniq.size() == got.size(), "number: no duplicate planned paths");
    }

    // --- #6 NextFreeTemplateSibling skips existing numbered siblings ----
    // Recovery name used when a create loses the check-then-create race: it
    // must return the FIRST free numbered sibling (never an existing one).
    {
        std::set<std::wstring> existing = { L"C:\\x\\raw 2" };
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectTrue(NextFreeTemplateSibling(dir, L"raw", NamePattern::Space, exists)
                       == L"C:\\x\\raw 3",
                   "next sibling: 'raw 2' taken -> 'raw 3'");
    }
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectTrue(NextFreeTemplateSibling(dir, L"raw", NamePattern::Space, exists)
                       == L"C:\\x\\raw 2",
                   "next sibling: none taken -> 'raw 2'");
    }

    // --- #F1 Number plan carries the ORIGINAL base for race retry -------
    // A create-time race on "raw 2" must retry as "raw 3", never "raw 2 2".
    // So the plan must expose base="raw" (the template name), not the already-
    // numbered candidate, and NextFreeTemplateSibling(base) skips "raw 2".
    {
        std::set<std::wstring> existing = { L"C:\\x\\raw" };
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        auto plans = ResolveTemplateFolders(dir, tmpl({ L"raw" }), exists,
                                            TemplateCollision::Number,
                                            NamePattern::Space);
        expectTrue(plans.size() == 1, "F1: one plan for [raw] when 'raw' exists");
        expectTrue(plans[0].base == L"raw", "F1: base is original 'raw' (not 'raw 2')");
        expectTrue(plans[0].path == L"C:\\x\\raw 2", "F1: path is 'raw 2'");
        // Simulate the race: 'raw 2' now also exists. Retrying from the base
        // 'raw' must yield 'raw 3' -- not 'raw 2 2'.
        std::set<std::wstring> raced = { L"C:\\x\\raw", L"C:\\x\\raw 2" };
        auto rexists = [&](const std::wstring& p) { return raced.count(p) > 0; };
        expectTrue(NextFreeTemplateSibling(dir, plans[0].base, NamePattern::Space, rexists)
                       == L"C:\\x\\raw 3",
                   "F1: race retry from base -> 'raw 3' (not 'raw 2 2')");
    }

    // --- #F3 dedup is case-insensitive (Windows paths) ------------------
    // raw|RAW refer to the SAME Windows directory, so a template must plan
    // only ONE of them -- not two paths that collide on disk.
    {
        std::set<std::wstring> existing;
        auto exists = [&](const std::wstring& p) { return existing.count(p) > 0; };
        expectVec(ResolveTemplateFolders(dir, tmpl({ L"raw", L"RAW" }),
                                         exists, TemplateCollision::Skip,
                                         NamePattern::Space),
                  { L"C:\\x\\raw" },
                  "F3: raw|RAW dedup case-insensitively -> one 'raw'");
    }

    if (g_fail == 0) { printf("template_folders_test: PASS\n"); return 0; }
    printf("template_folders_test: FAIL (%d)\n", g_fail);
    return 1;
}
