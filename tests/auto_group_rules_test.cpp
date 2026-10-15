// auto_group_rules_test.cpp
// Pins fix thse isseue.md Phase 6: Custom Auto Group rules. A rule routes files
// whose extension matches into a NAMED output bucket ("FolderName=ext1|ext2")
// instead of the default per-extension folder. Covers parsing/validation,
// case-insensitive + jpeg->jpg canonical matching, first-rule-wins on duplicate
// extensions, duplicate/invalid/reserved folder rejection, documented limits,
// image classification, the Images-parent mixed-rule safety fallback, and the
// BuildPlan routing (empty rules preserve the existing behaviour exactly).
// Self-contained: own main(), exit 0 = pass.

#include "auto_group_logic.h"

#include <cstdio>

using namespace autogroup;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

// Find the planned bucket folder for a given source path (empty if skipped).
static std::wstring bucketOf(const Plan& p, const std::wstring& src) {
    for (const auto& f : p.files) if (f.src == src) return f.bucket;
    return std::wstring();
}
static std::wstring destOf(const Plan& p, const std::wstring& src) {
    for (const auto& f : p.files) if (f.src == src) return f.destDir;
    return std::wstring();
}

int main() {
    // ---- parsing: a valid rule yields one bucket + its canonical ext keys ----
    {
        ParsedRules pr = ParseCustomRules({L"Photos=jpg|jpeg|png|gif"});
        check(pr.rules.size() == 1, "one valid rule parsed");
        check(pr.rules[0].folder == L"Photos", "rule folder is Photos");
        // jpeg collapses to jpg (CanonicalKey parity) so there is ONE jpg key.
        bool hasJpg = false, hasJpeg = false, hasPng = false;
        for (const auto& k : pr.rules[0].keys) {
            if (k == L"jpg") hasJpg = true;
            if (k == L"jpeg") hasJpeg = true;
            if (k == L"png") hasPng = true;
        }
        check(hasJpg && hasPng, "jpg + png keys present");
        check(!hasJpeg, "jpeg canonicalized to jpg (no separate jpeg key)");
        check(pr.rules[0].imageClass == RuleImageClass::AllImage,
              "all-image rule classified AllImage");
    }

    // ---- empty / blank list preserves current behaviour (no rules) ----
    check(ParseCustomRules({}).empty(), "no entries -> no rules");
    check(ParseCustomRules({L"", L"   ", L"\t"}).empty(),
          "blank entries are skipped silently");

    // ---- case-insensitive matching ----
    {
        ParsedRules pr = ParseCustomRules({L"Docs=PDF|.TXT|Md"});
        check(MatchCustomRule(pr, L"pdf", false).matched, "PDF matches pdf");
        check(MatchCustomRule(pr, L"txt", false).folder == L"Docs",
              ".TXT token matches txt -> Docs");
        check(MatchCustomRule(pr, L"md", false).matched, "Md matches md");
        check(!MatchCustomRule(pr, L"cpp", false).matched, "unlisted ext no match");
    }

    // ---- malformed / invalid folder / empty exts are rejected ----
    check(ParseCustomRules({L"NoEquals"}).empty(), "line without '=' rejected");
    check(ParseCustomRules({L"=jpg|png"}).empty(), "empty folder rejected");
    check(ParseCustomRules({L"Photos="}).empty(), "empty extension list rejected");
    check(ParseCustomRules({L"Bad<Name>=jpg"}).empty(),
          "invalid folder characters rejected");
    check(ParseCustomRules({L"CON=jpg"}).empty(),
          "reserved device name folder rejected");

    // ---- duplicate rule folder name: later rule rejected (ambiguous target) --
    {
        ParsedRules pr = ParseCustomRules({L"Media=mp3", L"Media=wav"});
        check(pr.rules.size() == 1, "duplicate folder name -> only first rule");
        bool loggedDup = false;
        for (const auto& r : pr.rejected)
            if (r.reason == RuleReject::DuplicateFolder) loggedDup = true;
        check(loggedDup, "duplicate folder rejection is logged");
    }

    // ---- duplicate extension across rules: FIRST rule wins ----
    {
        ParsedRules pr = ParseCustomRules({L"A=zip|7z", L"B=7z|rar"});
        check(MatchCustomRule(pr, L"7z", false).folder == L"A",
              "duplicate ext 7z stays with the first rule");
        check(MatchCustomRule(pr, L"rar", false).folder == L"B",
              "rar still routes to the second rule");
        bool droppedLogged = false;
        for (const auto& k : pr.dupExtDropped) if (k == L"7z") droppedLogged = true;
        check(droppedLogged, "the ignored duplicate ext is logged");
    }

    // ---- image classification ----
    check(ParseCustomRules({L"Pix=jpg|png"}).rules[0].imageClass ==
          RuleImageClass::AllImage, "all-image rule");
    check(ParseCustomRules({L"Src=cpp|h"}).rules[0].imageClass ==
          RuleImageClass::AllNonImage, "all-non-image rule");
    check(ParseCustomRules({L"Mix=jpg|txt"}).rules[0].imageClass ==
          RuleImageClass::Mixed, "mixed rule");

    // ---- Images-parent mixed-rule safety: NOT honored under imagesParent ----
    {
        ParsedRules pr = ParseCustomRules({L"Mix=jpg|txt"});
        check(MatchCustomRule(pr, L"jpg", /*imagesParent=*/false).matched,
              "mixed rule routes normally in Flat mode");
        check(!MatchCustomRule(pr, L"jpg", /*imagesParent=*/true).matched,
              "mixed rule falls back to defaults under Images-parent");
    }

    // ---- documented caps clamp extensions-per-rule and rule count ----
    {
        std::wstring many = L"Big=";
        for (std::size_t i = 0; i < MaxExtsPerRule() + 10; ++i) {
            if (i) many += L"|";
            many += L"e" + std::to_wstring(i);
        }
        ParsedRules pr = ParseCustomRules({many});
        check(pr.rules.size() == 1, "over-long rule still parses");
        check(pr.rules[0].keys.size() <= MaxExtsPerRule(),
              "extensions-per-rule clamped to the cap");
    }
    {
        std::vector<std::wstring> lines;
        for (std::size_t i = 0; i < MaxCustomRules() + 5; ++i)
            lines.push_back(L"R" + std::to_wstring(i) + L"=x" + std::to_wstring(i));
        ParsedRules pr = ParseCustomRules(lines);
        check(pr.rules.size() == MaxCustomRules(), "rule count clamped to the cap");
    }

    // ---- BuildPlan: a rule overrides the default per-extension bucket ----
    {
        Settings s;                       // Flat, defaults
        s.customRules = ParseCustomRules({L"Photos=jpg|png", L"Source Code=cpp|h"});
        std::vector<SrcItem> items = {
            {L"C:\\x\\a.jpg", false},
            {L"C:\\x\\b.PNG", false},
            {L"C:\\x\\c.cpp", false},
            {L"C:\\x\\d.txt", false},      // unmatched -> default TXT bucket
        };
        Plan p = BuildPlan(items, L"C:\\dest", s);
        check(p.ok, "plan resolves with a fixed dest");
        check(bucketOf(p, L"C:\\x\\a.jpg") == L"Photos", "jpg -> Photos bucket");
        check(bucketOf(p, L"C:\\x\\b.PNG") == L"Photos", "PNG -> Photos bucket");
        check(bucketOf(p, L"C:\\x\\c.cpp") == L"Source Code", "cpp -> Source Code");
        check(bucketOf(p, L"C:\\x\\d.txt") == L"TXT",
              "unmatched txt still uses the canonical TXT bucket");
        check(destOf(p, L"C:\\x\\a.jpg") == L"C:\\dest\\Photos",
              "custom bucket sits directly under the destination root (Flat)");
    }

    // ---- BuildPlan: empty rules reproduce the exact default behaviour ----
    {
        Settings s;                       // no custom rules
        std::vector<SrcItem> items = {
            {L"C:\\x\\a.jpg", false},
            {L"C:\\x\\b.cpp", false},
        };
        Plan p = BuildPlan(items, L"C:\\dest", s);
        check(bucketOf(p, L"C:\\x\\a.jpg") == L"JPG", "default jpg -> JPG");
        check(bucketOf(p, L"C:\\x\\b.cpp") == L"CPP", "default cpp -> CPP");
    }

    // ---- BuildPlan: Images-parent + an all-image rule nests under Images ----
    {
        Settings s;
        s.layout = Layout::ImagesParent;
        s.imagesParentName = L"Images";
        s.customRules = ParseCustomRules({L"Photos=jpg|png"});
        std::vector<SrcItem> items = { {L"C:\\x\\a.jpg", false} };
        Plan p = BuildPlan(items, L"C:\\dest", s);
        check(bucketOf(p, L"C:\\x\\a.jpg") == L"Photos",
              "all-image custom rule keeps its folder name");
        check(destOf(p, L"C:\\x\\a.jpg") == L"C:\\dest\\Images\\Photos",
              "all-image custom bucket nests under the Images parent");
    }

    // ---- BuildPlan: Images-parent + a MIXED rule falls back to defaults ----
    {
        Settings s;
        s.layout = Layout::ImagesParent;
        s.customRules = ParseCustomRules({L"Mix=jpg|txt"});
        std::vector<SrcItem> items = {
            {L"C:\\x\\a.jpg", false},
            {L"C:\\x\\b.txt", false},
        };
        Plan p = BuildPlan(items, L"C:\\dest", s);
        check(bucketOf(p, L"C:\\x\\a.jpg") == L"JPG",
              "mixed rule ignored under Images-parent: jpg -> default JPG");
        check(bucketOf(p, L"C:\\x\\b.txt") == L"TXT",
              "mixed rule ignored under Images-parent: txt -> default TXT");
    }

    if (g_fail) { printf("auto_group_rules_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("auto_group_rules_test: OK\n");
    return 0;
}
