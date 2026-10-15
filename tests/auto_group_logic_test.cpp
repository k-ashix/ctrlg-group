// auto_group_logic_test.cpp
// Pins fix thse isseue.md ADD-ON section 9 Phase A: PURE Auto Group
// classification + destination-planning logic. No Win32, no filesystem, no UI.
// Covers: mixed extensions; case variants; JPG/JPEG dedupe; extensionless
// policy; folders excluded by default; configured destination; same-parent
// requirement; invalid/reserved bucket names; deterministic ordering; image
// category; Images-parent layout; All Folder selected-folder grouping and
// self/recursive safety. Self-contained: own main(), exit 0 = pass.

#include "auto_group_logic.h"

#include <cstdio>

using namespace autogroup;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

static SrcItem F(const wchar_t* p) { return SrcItem{p, false}; }
static SrcItem D(const wchar_t* p) { return SrcItem{p, true}; }

int main() {
    // ---- #1 canonical bucket names + case-insensitive normalization ----
    check(CanonicalExtFolder(L"a.jpg", false) == L"JPG", ".jpg -> JPG");
    check(CanonicalExtFolder(L"a.JPG", false) == L"JPG", ".JPG -> JPG");
    check(CanonicalExtFolder(L"a.JpG", false) == L"JPG", "mixed case -> JPG");
    check(CanonicalExtFolder(L"a.jpeg", false) == L"JPG", ".jpeg dedupes -> JPG");
    check(CanonicalExtFolder(L"a.JPEG", false) == L"JPG", ".JPEG dedupes -> JPG");
    check(CanonicalExtFolder(L"a.png", false) == L"PNG", ".png -> PNG");
    check(CanonicalExtFolder(L"a.md", false) == L"MD", ".md -> MD");
    check(CanonicalExtFolder(L"a.cpp", false) == L"CPP", ".cpp -> CPP");
    check(CanonicalExtFolder(L"a.pdf", false) == L"PDF", ".pdf -> PDF");
    check(CanonicalExtFolder(L"a.zip", false) == L"ZIP", ".zip -> ZIP");
    // uncommon/new extension still buckets (no hardcoded whitelist)
    check(CanonicalExtFolder(L"a.weirdext", false) == L"WEIRDEXT",
          "uncommon extension still produces a bucket");
    // folders / extensionless never get a bucket
    check(CanonicalExtFolder(L"folder", true).empty(), "folder -> no bucket");
    check(CanonicalExtFolder(L"README", false).empty(),
          "extensionless file -> no bucket");
    check(CanonicalExtFolder(L".gitignore", false).empty(),
          "leading-dot hidden name -> no bucket (ExtLabel rule)");

    // ---- #2 image category ----
    check(IsImageKey(L"jpg"), "jpg is image");
    check(IsImageKey(L"png"), "png is image");
    check(IsImageKey(L"gif"), "gif is image");
    check(IsImageKey(L"webp"), "webp is image");
    check(!IsImageKey(L"md"), "md is not image");
    check(!IsImageKey(L"cpp"), "cpp is not image");
    check(IsImageBucket(L"JPG"), "bucket JPG is image");
    check(!IsImageBucket(L"PDF"), "bucket PDF is not image");

    // ---- #3 sanitize / reject invalid + reserved names ----
    check(SanitizeBucketName(L"JPG").ok, "JPG is a valid bucket");
    check(SanitizeBucketName(L"No Extension").name == L"No Extension",
          "spaces kept inside a configured name");
    check(!SanitizeBucketName(L"").ok, "empty rejected");
    check(!SanitizeBucketName(L"   ").ok, "whitespace-only rejected");
    check(!SanitizeBucketName(L"a/b").ok, "slash rejected");
    check(!SanitizeBucketName(L"a:b").ok, "colon rejected");
    check(!SanitizeBucketName(L"a*b").ok, "wildcard rejected");
    check(!SanitizeBucketName(L"CON").ok, "reserved CON rejected");
    check(!SanitizeBucketName(L"con").ok, "reserved con (case) rejected");
    check(!SanitizeBucketName(L"NUL").ok, "reserved NUL rejected");
    check(!SanitizeBucketName(L"COM1").ok, "reserved COM1 rejected");
    check(!SanitizeBucketName(L"LPT9").ok, "reserved LPT9 rejected");
    check(SanitizeBucketName(L"COM10").ok, "COM10 is NOT reserved");
    check(SanitizeBucketName(L"trailingdot.").name == L"trailingdot",
          "trailing dot trimmed");
    {
        std::wstring tooLong(80, L'A');
        check(!SanitizeBucketName(tooLong).ok, "over-long name rejected");
    }

    // ---- #4 destination resolution precedence ----
    {
        // Fixed destination wins even when items span different parents.
        std::vector<SrcItem> items = { F(L"C:\\a\\x.jpg"), F(L"D:\\b\\y.png") };
        DestResult d = ResolveAutoDest(items, L"E:\\sorted");
        check(d.status == DestStatus::Fixed && d.root == L"E:\\sorted",
              "fixed destination wins over scattered parents");
    }
    {
        // No fixed dest, all share one parent -> common parent.
        std::vector<SrcItem> items = {
            F(L"C:\\pics\\a.jpg"), F(L"C:\\pics\\b.png"), F(L"C:\\pics\\c.md") };
        DestResult d = ResolveAutoDest(items, L"");
        check(d.status == DestStatus::CommonParent && d.root == L"C:\\pics",
              "common parent used when no fixed dest");
    }
    {
        // No fixed dest, multiple parents -> safe cancel.
        std::vector<SrcItem> items = { F(L"C:\\a\\x.jpg"), F(L"C:\\b\\y.jpg") };
        DestResult d = ResolveAutoDest(items, L"");
        check(d.status == DestStatus::NoCommonParent && !d.resolved(),
              "multiple parents + no fixed dest -> cancel");
    }
    {
        DestResult d = ResolveAutoDest({}, L"");
        check(d.status == DestStatus::EmptySelection && !d.resolved(),
              "empty selection -> cancel");
    }

    // ---- #5 deterministic flat plan: files -> buckets, folders untouched ----
    {
        std::vector<SrcItem> items = {
            F(L"C:\\s\\a.jpg"), F(L"C:\\s\\b.JPEG"), F(L"C:\\s\\c.png"),
            F(L"C:\\s\\d.md"),  F(L"C:\\s\\e.PDF"),  D(L"C:\\s\\sub1"),
            D(L"C:\\s\\sub2"),  F(L"C:\\s\\README"),
        };
        Settings st;  // flat, folders untouched, no-extension ON by default
        Plan p = BuildPlan(items, L"", st);
        check(p.ok && p.dest.root == L"C:\\s", "plan resolves to common parent");
        // a.jpg + b.JPEG share the JPG bucket -> deterministic single dir.
        int jpgCount = 0, dirJpg = 0;
        for (auto& f : p.files) if (f.bucket == L"JPG") ++jpgCount;
        for (auto& d : p.dirsToCreate) if (d == L"C:\\s\\JPG") ++dirJpg;
        check(jpgCount == 2, "jpg + jpeg routed to the one JPG bucket");
        check(dirJpg == 1, "JPG directory listed exactly once (deduped)");
        // README (extensionless) goes into No Extension by default.
        bool sawNoExt = false;
        for (auto& f : p.files)
            if (f.bucket == L"No Extension" && f.src == L"C:\\s\\README")
                sawNoExt = true;
        check(sawNoExt, "extensionless file -> No Extension bucket (default on)");
        // folders are NOT grouped by default.
        check(p.folders.empty(), "folders untouched by default");
        // every file op's destDir sits under the resolved root.
        bool allUnderRoot = true;
        for (auto& f : p.files)
            if (f.destDir.rfind(L"C:\\s\\", 0) != 0) allUnderRoot = false;
        check(allUnderRoot, "all buckets created under destination root");
        // 6 files mapped (5 ext + README), 2 folders ignored.
        check(p.files.size() == 6, "exactly the 6 eligible files are mapped");
    }

    // ---- #5b no-extension disabled -> extensionless files skipped ----
    {
        std::vector<SrcItem> items = { F(L"C:\\s\\a.jpg"), F(L"C:\\s\\README") };
        Settings st; st.noExtensionEnabled = false;
        Plan p = BuildPlan(items, L"", st);
        bool readmeMapped = false;
        for (auto& f : p.files) if (f.src == L"C:\\s\\README") readmeMapped = true;
        check(!readmeMapped, "no-ext OFF: extensionless file not mapped");
        bool readmeSkipped = false;
        for (auto& sk : p.skipped)
            if (sk.path == L"C:\\s\\README" && sk.reason == Skip::NoExtDisabled)
                readmeSkipped = true;
        check(readmeSkipped, "no-ext OFF: extensionless file recorded as skipped");
    }

    // ---- #5c Images-parent layout ----
    {
        std::vector<SrcItem> items = {
            F(L"C:\\s\\a.jpg"), F(L"C:\\s\\b.png"), F(L"C:\\s\\c.md"),
            F(L"C:\\s\\d.cpp"),
        };
        Settings st; st.layout = Layout::ImagesParent;  // Images parent name default
        Plan p = BuildPlan(items, L"", st);
        std::wstring jpgDir, mdDir;
        for (auto& f : p.files) {
            if (f.bucket == L"JPG") jpgDir = f.destDir;
            if (f.bucket == L"MD")  mdDir = f.destDir;
        }
        check(jpgDir == L"C:\\s\\Images\\JPG",
              "image bucket nested under Images parent");
        check(mdDir == L"C:\\s\\MD", "non-image bucket stays at root");
        // Images parent dir itself is scheduled for creation.
        bool imagesDir = false;
        for (auto& d : p.dirsToCreate) if (d == L"C:\\s\\Images") imagesDir = true;
        check(imagesDir, "Images parent directory is created");
        // The worker creates dirsToCreate in order, one leaf at a time, so the
        // Images parent MUST be scheduled BEFORE its child image bucket --
        // otherwise CreateFolderWithShellElevation would fail on the nested
        // C:\s\Images\JPG path. Pin that ordering.
        int idxParent = -1, idxChild = -1;
        for (int i = 0; i < (int)p.dirsToCreate.size(); ++i) {
            if (p.dirsToCreate[i] == L"C:\\s\\Images")      idxParent = i;
            if (p.dirsToCreate[i] == L"C:\\s\\Images\\JPG")  idxChild = i;
        }
        check(idxParent >= 0 && idxChild >= 0 && idxParent < idxChild,
              "Images parent scheduled before its child image bucket");
    }

    // ---- #6 selected-folder grouping into All Folder (opt-in) ----
    {
        std::vector<SrcItem> items = {
            F(L"C:\\s\\a.jpg"), D(L"C:\\s\\sub1"), D(L"C:\\s\\sub2"),
        };
        Settings st; st.groupSelectedFolders = true;  // All Folder default name
        Plan p = BuildPlan(items, L"", st);
        check(p.folders.size() == 2, "both selected folders queued for All Folder");
        bool allDir = false;
        for (auto& f : p.folders) if (f.destDir != L"C:\\s\\All Folder") allDir = true;
        check(!allDir, "selected folders target the All Folder directory");
        // generated output buckets must NOT be moved into All Folder.
        for (auto& f : p.folders)
            check(f.src.rfind(L"C:\\s\\All Folder", 0) != 0,
                  "no generated bucket is routed into All Folder");
        // the JPG bucket still lives at the root, not inside All Folder.
        bool jpgAtRoot = false;
        for (auto& f : p.files) if (f.destDir == L"C:\\s\\JPG") jpgAtRoot = true;
        check(jpgAtRoot, "extension buckets stay at destination root");
    }

    // ---- #6b self/recursive folder move rejected ----
    {
        // A selected folder that IS the All Folder target must be rejected.
        std::vector<SrcItem> items = {
            D(L"C:\\s\\All Folder"), D(L"C:\\s\\keep"),
        };
        Settings st; st.groupSelectedFolders = true;
        Plan p = BuildPlan(items, L"", st);
        bool rejectedSelf = false;
        for (auto& sk : p.skipped)
            if (sk.path == L"C:\\s\\All Folder" && sk.reason == Skip::SelfRecursive)
                rejectedSelf = true;
        check(rejectedSelf, "selecting the All Folder target itself is rejected");
        bool keepQueued = false;
        for (auto& f : p.folders) if (f.src == L"C:\\s\\keep") keepQueued = true;
        check(keepQueued, "an unrelated selected folder is still queued");
    }

    // ---- #5d invalid bucket from a bad extension is skipped, not merged ----
    {
        // An extension that sanitizes to a reserved device name must be skipped
        // rather than silently merged into another bucket.
        std::vector<SrcItem> items = { F(L"C:\\s\\weird.con"), F(L"C:\\s\\ok.txt") };
        Settings st;
        Plan p = BuildPlan(items, L"", st);
        bool conSkipped = false;
        for (auto& sk : p.skipped)
            if (sk.path == L"C:\\s\\weird.con" && sk.reason == Skip::Rejected)
                conSkipped = true;
        check(conSkipped, "reserved-name bucket skipped (not merged)");
        bool txtMapped = false;
        for (auto& f : p.files) if (f.bucket == L"TXT") txtMapped = true;
        check(txtMapped, "the valid TXT file is still mapped");
    }

    if (g_fail) { printf("auto_group_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("auto_group_logic_test: OK\n");
    return 0;
}
