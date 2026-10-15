// auto_group_logic.h
//
// PURE logic for the Ctrl+G "Auto Group" ADD-ON (fix thse isseue.md, ADD-ON
// section 9 Phase A): automatically organize the selected files into folders
// named after their normalized extensions, WITHOUT a user-typed destination
// name. Mirrored byte-for-byte into ctrlg-group.wh.cpp so it can be unit tested
// with a plain g++ build (no Win32 runtime).
//
// Decisions that live here (pure, testable without the shell):
//   1. CanonicalExtFolder   -- normalized UPPERCASE bucket name for a file
//                              (.jpg/.jpeg/.JPG -> "JPG"); folders excluded.
//   2. IsImageExt           -- explicit, tested image-extension category.
//   3. SanitizeBucketName   -- validate/sanitize a generated folder name,
//                              rejecting invalid Windows names + reserved
//                              device names (CON, NUL, COM1..9, LPT1..9, ...).
//   4. ResolveAutoDest      -- DEDICATED Auto Group destination resolver:
//                              fixed destination -> common parent -> cancel.
//                              NEVER reuses the last typed name / last target.
//   5. BuildPlan            -- deterministic plan: every eligible source file
//                              mapped to its destination bucket directory,
//                              honoring flat vs Images-parent layout, the
//                              optional "All Folder" selected-folder grouping,
//                              and the No-Extension policy.
//
// This phase is PURE PLANNING ONLY: no Win32, no filesystem mutation, no UI.
// Later phases (D..F) wire the plan into the existing worker / IFileOperation.
//
#pragma once

#include <string>
#include <vector>
#include <cstddef>

#include "selection_logic.h"   // sel::Item, sel::ExtLabel

namespace autogroup {

// ---------------------------------------------------------------------------
// Path helpers (pure, separator-normalised). Windows paths use backslash but
// the mod also tolerates forward slashes, so these treat both as separators.
// Kept local so this model is self-contained and testable on its own.
// ---------------------------------------------------------------------------

inline bool IsSep(wchar_t c) { return c == L'\\' || c == L'/'; }

// Strip one trailing separator ("C:\a\" -> "C:\a"); a bare drive root keeps it.
inline std::wstring TrimSep(const std::wstring& p) {
    if (p.size() > 1 && IsSep(p[p.size() - 1])) {
        if (p.size() == 3 && p[1] == L':') return p;  // keep "C:\"
        return p.substr(0, p.size() - 1);
    }
    return p;
}

// Parent directory of a path (everything before the last separator). Empty when
// there is no separator or the path is only a drive root.
inline std::wstring ParentDir(const std::wstring& path) {
    std::wstring p = TrimSep(path);
    if (p.size() <= 3 && p.size() >= 2 && p[1] == L':') return std::wstring();
    std::wstring::size_type pos = std::wstring::npos;
    for (std::wstring::size_type i = p.size(); i > 0; --i) {
        if (IsSep(p[i - 1])) { pos = i - 1; break; }
    }
    if (pos == std::wstring::npos) return std::wstring();
    if (pos == 2 && p[1] == L':') return p.substr(0, 3);  // "C:\a" -> "C:\"
    return p.substr(0, pos);
}

// Case-insensitive equality for ASCII paths (Windows paths are case-insensitive).
inline bool PathEqualCI(const std::wstring& a, const std::wstring& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        wchar_t ca = a[i], cb = b[i];
        if (ca >= L'A' && ca <= L'Z') ca = static_cast<wchar_t>(ca - L'A' + L'a');
        if (cb >= L'A' && cb <= L'Z') cb = static_cast<wchar_t>(cb - L'A' + L'a');
        // Normalise separators so '\\' and '/' compare equal.
        if (IsSep(ca)) ca = L'\\';
        if (IsSep(cb)) cb = L'\\';
        if (ca != cb) return false;
    }
    return true;
}

// Join a directory and a child name with a single backslash.
inline std::wstring Join(const std::wstring& dir, const std::wstring& child) {
    std::wstring d = TrimSep(dir);
    if (d.empty()) return child;
    return d + L"\\" + child;
}

// ---------------------------------------------------------------------------
// #1  Canonical extension-bucket folder name.
// Normalises an item's extension to an UPPERCASE folder name. JPG and JPEG map
// to the SAME canonical bucket "JPG" (no duplicate image folders). Works for
// new / uncommon extensions -- there is NO hardcoded whitelist. Folders and
// extensionless files return an empty string (they are not extension buckets).
// ---------------------------------------------------------------------------

// Lowercase + strip a single leading dot. "MD"/".md"/"Md" -> "md".
inline std::wstring NormKey(const std::wstring& extLabel) {
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
inline std::wstring Upper(const std::wstring& key) {
    std::wstring out;
    for (wchar_t c : key) {
        if (c >= L'a' && c <= L'z') c = static_cast<wchar_t>(c - L'a' + L'A');
        out.push_back(c);
    }
    return out;
}

// Canonical lowercase bucket KEY for an item. ".jpeg" collapses to "jpg" so a
// single image bucket holds both. Returns "" for folders / extensionless files.
inline std::wstring CanonicalKey(const std::wstring& name, bool isFolder) {
    if (isFolder) return std::wstring();
    std::wstring key = NormKey(sel::ExtLabel(name, false));
    if (key.empty()) return std::wstring();
    if (key == L"jpeg" || key == L"jpe") key = L"jpg";  // canonicalise JPEG->JPG
    return key;
}

// Canonical UPPERCASE bucket FOLDER NAME for an item (e.g. "JPG"). Empty for
// folders / extensionless files.
inline std::wstring CanonicalExtFolder(const std::wstring& name, bool isFolder) {
    return Upper(CanonicalKey(name, isFolder));
}

// ---------------------------------------------------------------------------
// #2  Image-extension category (explicit, tested). Only these go under the
// configured Images parent in Mode B. Keys are the CANONICAL keys, so "jpeg"
// is already folded into "jpg" before this is consulted.
// ---------------------------------------------------------------------------

inline bool IsImageKey(const std::wstring& canonicalKey) {
    static const wchar_t* kImageKeys[] = {
        L"jpg",  L"png",  L"gif",  L"webp", L"bmp",  L"tif",  L"tiff",
        L"heic", L"heif", L"avif", L"ico",  L"svg",  L"jfif", L"raw",
    };
    for (const wchar_t* k : kImageKeys) {
        if (canonicalKey == k) return true;
    }
    return false;
}

// Convenience overload taking a bucket FOLDER NAME (e.g. "JPG").
inline bool IsImageBucket(const std::wstring& bucketFolder) {
    return IsImageKey(NormKey(bucketFolder));
}

// ---------------------------------------------------------------------------
// #3  Folder-name validation + sanitization.
// A bucket folder name is derived from an extension (and extensions can be odd:
// long, containing invalid characters, or spelling a reserved device name).
// Reject names we cannot represent safely rather than silently producing a bad
// path. Returns true + the sanitized name when usable; false when the name must
// be rejected (so the caller can safely skip that bucket).
// ---------------------------------------------------------------------------

inline bool IsReservedDeviceName(const std::wstring& nameUpper) {
    static const wchar_t* kReserved[] = {
        L"CON", L"PRN", L"AUX", L"NUL",
        L"COM1", L"COM2", L"COM3", L"COM4", L"COM5",
        L"COM6", L"COM7", L"COM8", L"COM9",
        L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5",
        L"LPT6", L"LPT7", L"LPT8", L"LPT9",
    };
    for (const wchar_t* r : kReserved) {
        if (nameUpper == r) return true;
    }
    return false;
}

inline bool IsInvalidNameChar(wchar_t c) {
    // Windows forbids these in a path component, plus control chars.
    if (c < 0x20) return true;
    switch (c) {
        case L'<': case L'>': case L':': case L'"': case L'/':
        case L'\\': case L'|': case L'?': case L'*':
            return true;
        default:
            return false;
    }
}

// Longest path component we will accept for a generated bucket (well under
// MAX_PATH component limits, leaving room for the destination root).
inline std::size_t MaxBucketNameLen() { return 64u; }

struct SanitizeResult {
    bool ok = false;             // false => reject this bucket entirely
    std::wstring name;           // sanitized name when ok
};

// Validate/sanitize a proposed bucket folder name. The input is normally an
// uppercase extension bucket ("JPG") or a configured name ("No Extension",
// "Images", "All Folder"). Trailing dots/spaces are trimmed (Windows strips
// them anyway). Any invalid character, an empty result, a reserved device name,
// or an over-long name is REJECTED (ok=false) so the caller can skip it rather
// than silently merging unrelated types.
inline SanitizeResult SanitizeBucketName(const std::wstring& proposed) {
    SanitizeResult r;
    std::wstring s = proposed;

    // Trim leading/trailing whitespace.
    std::size_t b = 0, e = s.size();
    while (b < e && (s[b] == L' ' || s[b] == L'\t')) ++b;
    while (e > b && (s[e - 1] == L' ' || s[e - 1] == L'\t')) --e;
    s = s.substr(b, e - b);

    // Windows strips trailing dots and spaces from a component; a name that is
    // ONLY dots/spaces becomes empty and must be rejected.
    while (!s.empty() && (s.back() == L'.' || s.back() == L' ')) s.pop_back();

    if (s.empty()) return r;                 // nothing usable
    if (s.size() > MaxBucketNameLen()) return r;

    for (wchar_t c : s) {
        if (IsInvalidNameChar(c)) return r;  // reject, do not substitute
    }

    // Reserved device names are compared case-insensitively, bare or with any
    // extension (e.g. "CON", "con.txt") -- here the component is a bare bucket.
    std::wstring upper = Upper(NormKey(s.find(L'.') == std::wstring::npos
                                           ? s
                                           : s.substr(0, s.find(L'.'))));
    if (IsReservedDeviceName(upper)) return r;

    r.ok = true;
    r.name = s;
    return r;
}

// ---------------------------------------------------------------------------
// #4  Dedicated Auto Group destination resolver.
// Precedence (NEVER uses g_lastGroupTarget or the last typed name):
//   1. configured Fixed destination folder, if set.
//   2. else the common filesystem parent of the selection (if they share one).
//   3. else CANNOT resolve -> safe cancel (needs a common parent or a
//      configured destination).
// ---------------------------------------------------------------------------

enum class DestStatus {
    Fixed,          // used the configured fixed destination
    CommonParent,   // used the common parent of the selection
    NoCommonParent, // selection spans multiple parents, no fixed dest -> cancel
    EmptySelection, // nothing selected -> cancel
};

struct DestResult {
    DestStatus status = DestStatus::EmptySelection;
    std::wstring root;              // destination root (empty unless resolved)
    bool resolved() const {
        return status == DestStatus::Fixed || status == DestStatus::CommonParent;
    }
};

// One captured selection entry for planning: full path + whether it is a folder.
struct SrcItem {
    std::wstring path;      // full path, e.g. "C:\pics\a.jpg"
    bool isFolder = false;
};

// Common parent of all selected items (the directory each item lives in).
// Returns empty when the items do not all share one parent directory.
inline std::wstring CommonParent(const std::vector<SrcItem>& items) {
    if (items.empty()) return std::wstring();
    std::wstring parent = ParentDir(items[0].path);
    if (parent.empty()) return std::wstring();
    for (std::size_t i = 1; i < items.size(); ++i) {
        std::wstring p = ParentDir(items[i].path);
        if (p.empty() || !PathEqualCI(p, parent)) return std::wstring();
    }
    return parent;
}

inline DestResult ResolveAutoDest(const std::vector<SrcItem>& items,
                                  const std::wstring& fixedDest) {
    DestResult out;
    if (items.empty()) {
        out.status = DestStatus::EmptySelection;
        return out;
    }
    if (!fixedDest.empty()) {
        out.status = DestStatus::Fixed;
        out.root = TrimSep(fixedDest);
        return out;
    }
    std::wstring parent = CommonParent(items);
    if (parent.empty()) {
        out.status = DestStatus::NoCommonParent;
        return out;
    }
    out.status = DestStatus::CommonParent;
    out.root = TrimSep(parent);
    return out;
}

// ---------------------------------------------------------------------------
// #5  Deterministic plan.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Phase 6: Custom Auto Group rules. A rule routes files whose extension matches
// into a NAMED output bucket instead of the default per-extension folder. Each
// rule is one string "FolderName=ext1|ext2|ext3". Pure + unit-tested; no
// filesystem access and NO dependency on the New Folder template settings.
//   - Empty rule list -> behaviour is unchanged (no rule ever matches).
//   - Unmatched exts   -> still go to their canonical per-extension folder.
//   - Matching is case-insensitive; jpeg/jpe collapse to jpg like CanonicalKey.
//   - First valid rule wins when two rules claim the same extension.
//   - Folder names validated with SanitizeBucketName (reserved/invalid/empty
//     rejected); a duplicate rule folder name is rejected (ambiguous target).
//   - Documented caps clamp the rule and per-rule extension counts.
// ---------------------------------------------------------------------------

inline std::size_t MaxCustomRules() { return 64u; }
inline std::size_t MaxExtsPerRule() { return 64u; }

enum class RuleImageClass { AllNonImage, AllImage, Mixed };

struct CustomRule {
    std::wstring folder;                  // sanitized bucket folder name
    std::vector<std::wstring> keys;       // canonical lowercase ext keys (deduped)
    RuleImageClass imageClass = RuleImageClass::AllNonImage;
};

enum class RuleReject {
    None,
    Malformed,        // no '=', empty folder, or empty extension list
    BadFolderName,    // folder failed SanitizeBucketName
    DuplicateFolder,  // a later rule reuses an earlier rule's folder name
    LimitReached,     // rule / extension cap hit (extras dropped)
};

struct RuleRejectLog {
    std::wstring text;                    // offending raw line
    RuleReject reason = RuleReject::None;
};

struct ParsedRules {
    std::vector<CustomRule> rules;              // valid, first-seen order
    std::vector<std::wstring> dupExtDropped;    // ext keys ignored (first rule wins)
    std::vector<RuleRejectLog> rejected;        // rejected lines, for the log
    bool empty() const { return rules.empty(); }
};

// Canonicalize an extension TOKEN exactly as CanonicalKey normalizes a file's
// extension (lowercase, drop a leading dot, jpeg/jpe -> jpg) so a rule token
// matches the key a real file of that type produces.
inline std::wstring CanonicalizeExtToken(const std::wstring& token) {
    std::wstring key = NormKey(token);
    if (key == L"jpeg" || key == L"jpe") key = L"jpg";
    return key;
}

// Split "Folder=ext1|ext2" into folder + raw ext tokens. Returns false when the
// line has no '='. Blanks on the folder are left for the sanitizer; empty ext
// tokens are dropped by the caller.
inline bool SplitRuleLine(const std::wstring& line, std::wstring& folderOut,
                          std::vector<std::wstring>& extTokensOut) {
    folderOut.clear();
    extTokensOut.clear();
    const std::wstring::size_type eq = line.find(L'=');
    if (eq == std::wstring::npos) return false;
    folderOut = line.substr(0, eq);
    const std::wstring rest = line.substr(eq + 1);
    std::wstring cur;
    for (wchar_t c : rest) {
        if (c == L'|') { extTokensOut.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    extTokensOut.push_back(cur);
    return true;
}

// Classify a rule's canonical keys as all-image / all-non-image / mixed.
inline RuleImageClass ClassifyRuleKeys(const std::vector<std::wstring>& keys) {
    bool anyImg = false, anyNon = false;
    for (const auto& k : keys) {
        if (IsImageKey(k)) anyImg = true; else anyNon = true;
    }
    if (anyImg && anyNon) return RuleImageClass::Mixed;
    return anyImg ? RuleImageClass::AllImage : RuleImageClass::AllNonImage;
}

// Parse the raw settings array (one "Folder=ext|ext" per entry) into validated
// rules plus a log of everything ignored. Blank entries are skipped silently
// (the empty default slot). First rule wins on a duplicate extension.
inline ParsedRules ParseCustomRules(const std::vector<std::wstring>& rawLines) {
    ParsedRules out;
    auto folderTaken = [&](const std::wstring& f) {
        for (const auto& r : out.rules) if (PathEqualCI(r.folder, f)) return true;
        return false;
    };
    auto extTaken = [&](const std::wstring& k) {
        for (const auto& r : out.rules)
            for (const auto& e : r.keys) if (e == k) return true;
        return false;
    };

    for (const auto& rawLine : rawLines) {
        bool blank = true;
        for (wchar_t c : rawLine) if (c != L' ' && c != L'\t') { blank = false; break; }
        if (blank) continue;

        if (out.rules.size() >= MaxCustomRules()) {
            out.rejected.push_back({rawLine, RuleReject::LimitReached});
            continue;
        }

        std::wstring folderRaw;
        std::vector<std::wstring> tokens;
        if (!SplitRuleLine(rawLine, folderRaw, tokens)) {
            out.rejected.push_back({rawLine, RuleReject::Malformed});
            continue;
        }

        SanitizeResult fsan = SanitizeBucketName(folderRaw);
        if (!fsan.ok) {
            out.rejected.push_back({rawLine, RuleReject::BadFolderName});
            continue;
        }
        if (folderTaken(fsan.name)) {
            out.rejected.push_back({rawLine, RuleReject::DuplicateFolder});
            continue;
        }

        CustomRule rule;
        rule.folder = fsan.name;
        for (const auto& tok : tokens) {
            const std::wstring key = CanonicalizeExtToken(tok);
            if (key.empty()) continue;                 // drop empty ext token
            if (rule.keys.size() >= MaxExtsPerRule()) {
                out.rejected.push_back({rawLine, RuleReject::LimitReached});
                break;
            }
            bool already = extTaken(key);
            for (const auto& e : rule.keys) if (e == key) already = true;
            if (already) { out.dupExtDropped.push_back(key); continue; }
            rule.keys.push_back(key);
        }

        if (rule.keys.empty()) {
            out.rejected.push_back({rawLine, RuleReject::Malformed});
            continue;
        }
        rule.imageClass = ClassifyRuleKeys(rule.keys);
        out.rules.push_back(rule);
    }
    return out;
}

// Match a file's canonical key against the parsed rules. `imagesParentEnabled`
// gates the one layout-dependent safety rule: a MIXED-image rule is ambiguous
// under the Images-parent layout, so it is NOT honored there (the file falls
// back to its default per-extension bucket). In Flat mode image-ness is
// irrelevant, so a mixed rule routes normally.
struct RuleMatch {
    bool matched = false;
    std::wstring folder;
    bool isImage = false;
};
inline RuleMatch MatchCustomRule(const ParsedRules& pr,
                                 const std::wstring& canonicalKey,
                                 bool imagesParentEnabled) {
    RuleMatch m;
    for (const auto& r : pr.rules) {
        for (const auto& k : r.keys) {
            if (k != canonicalKey) continue;
            if (imagesParentEnabled && r.imageClass == RuleImageClass::Mixed) {
                return m;   // ambiguous under Images-parent -> fall back
            }
            m.matched = true;
            m.folder = r.folder;
            m.isImage = (r.imageClass == RuleImageClass::AllImage);
            return m;
        }
    }
    return m;
}

enum class Layout { Flat, ImagesParent };

// ---------------------------------------------------------------------------
// Settings-parsing helpers (pure, so the mod's LoadSettings and the unit tests
// share ONE implementation). These never touch the Group settings -- they only
// decode the Auto Group $options string and the configurable folder names.
// ---------------------------------------------------------------------------

// Map the autoGroupLayout $option identifier to the layout enum. Only the exact
// identifier "imagesParent" selects the Images-parent layout; anything else
// (including null/blank/unknown) is the safe Flat default.
inline Layout ParseLayout(const wchar_t* option) {
    if (option && std::wstring(option) == L"imagesParent") return Layout::ImagesParent;
    return Layout::Flat;
}

// Trim surrounding ASCII whitespace from a configured name; when the result is
// empty, fall back to the documented default so a blank setting can never
// produce an empty bucket path.
inline std::wstring NameOrDefault(const std::wstring& raw, const std::wstring& fallback) {
    std::size_t b = 0, e = raw.size();
    while (b < e && (raw[b] == L' ' || raw[b] == L'\t')) ++b;
    while (e > b && (raw[e - 1] == L' ' || raw[e - 1] == L'\t')) --e;
    std::wstring s = raw.substr(b, e - b);
    return s.empty() ? fallback : s;
}

struct Settings {
    Layout layout = Layout::Flat;
    std::wstring imagesParentName = L"Images";
    bool groupSelectedFolders = false;        // move selected folders -> AllFolder
    std::wstring allFolderName = L"All Folder";
    bool noExtensionEnabled = true;           // put extensionless files in a bucket
    std::wstring noExtensionName = L"No Extension";
    bool isMove = true;                        // Copy/Move for FILES
    ParsedRules customRules;                   // Phase 6: optional extension->folder rules
};

// One planned file move/copy: a source file routed into a destination directory.
struct FileOp {
    std::wstring src;        // source file path
    std::wstring destDir;    // full destination directory (bucket) path
    std::wstring bucket;     // the bucket folder name (last path component)
};

// One planned selected-folder move (only when groupSelectedFolders is on).
struct FolderOp {
    std::wstring src;        // selected folder path
    std::wstring destDir;    // the "All Folder" directory
};

enum class Skip {
    None,
    Rejected,         // bucket name failed sanitization
    NoExtDisabled,    // extensionless file, No-Extension bucket turned off
    SelfRecursive,    // folder move would be recursive / into itself
};

struct SkippedItem {
    std::wstring path;
    Skip reason = Skip::None;
};

struct Plan {
    DestResult dest;
    std::vector<FileOp> files;
    std::vector<FolderOp> folders;
    std::vector<std::wstring> dirsToCreate;   // deterministic, de-duplicated
    std::vector<SkippedItem> skipped;
    bool ok = false;    // true only when the destination resolved
};

// Resolve the full destination directory for a file bucket under the layout.
// Image buckets go under <root>\<ImagesParent>\<BUCKET> in ImagesParent mode;
// everything else (and all buckets in Flat mode) go to <root>\<BUCKET>.
inline std::wstring BucketDir(const std::wstring& root,
                              const std::wstring& bucketName,
                              bool isImage,
                              const Settings& s) {
    if (s.layout == Layout::ImagesParent && isImage) {
        return Join(Join(root, s.imagesParentName), bucketName);
    }
    return Join(root, bucketName);
}

// Build the deterministic Auto Group plan. Pure: no filesystem access. Order is
// stable -- files keep their captured (first-seen) order and each new
// destination directory is appended to dirsToCreate the first time it appears.
inline Plan BuildPlan(const std::vector<SrcItem>& items,
                      const std::wstring& fixedDest,
                      const Settings& s) {
    Plan plan;
    plan.dest = ResolveAutoDest(items, fixedDest);
    if (!plan.dest.resolved()) {
        plan.ok = false;
        return plan;
    }
    const std::wstring root = plan.dest.root;

    // Validate the configured special folder names up-front so a bad config is
    // reported once rather than per item.
    SanitizeResult imgSan = SanitizeBucketName(s.imagesParentName);
    SanitizeResult allSan = SanitizeBucketName(s.allFolderName);
    SanitizeResult noExtSan = SanitizeBucketName(s.noExtensionName);

    auto addDir = [&](const std::wstring& dir) {
        for (const auto& d : plan.dirsToCreate) {
            if (PathEqualCI(d, dir)) return;
        }
        plan.dirsToCreate.push_back(dir);
    };

    for (const auto& it : items) {
        if (it.isFolder) continue;   // folders handled separately below

        const std::wstring key = CanonicalKey(it.path, false);

        if (key.empty()) {
            // Extensionless file.
            if (!s.noExtensionEnabled || !noExtSan.ok) {
                plan.skipped.push_back({it.path,
                    noExtSan.ok ? Skip::NoExtDisabled : Skip::Rejected});
                continue;
            }
            const std::wstring dir = Join(root, noExtSan.name);
            addDir(dir);
            plan.files.push_back({it.path, dir, noExtSan.name});
            continue;
        }

        // Phase 6: a custom rule may override the default per-extension bucket
        // for this file's extension (first matching rule wins). A mixed-image
        // rule is ignored under Images-parent layout (handled in the matcher),
        // so a match here is uniformly image or non-image.
        const RuleMatch cr = MatchCustomRule(s.customRules, key,
                                             s.layout == Layout::ImagesParent);
        const std::wstring bucket = cr.matched ? cr.folder : Upper(key);
        SanitizeResult san = SanitizeBucketName(bucket);
        if (!san.ok) {
            plan.skipped.push_back({it.path, Skip::Rejected});
            continue;
        }
        const bool img = cr.matched ? cr.isImage : IsImageKey(key);
        // In ImagesParent mode an image bucket also needs the parent name valid.
        if (s.layout == Layout::ImagesParent && img && !imgSan.ok) {
            plan.skipped.push_back({it.path, Skip::Rejected});
            continue;
        }
        const std::wstring dir = BucketDir(root, san.name, img, s);
        // Make sure the Images parent itself is created before its children.
        if (s.layout == Layout::ImagesParent && img) {
            addDir(Join(root, imgSan.name));
        }
        addDir(dir);
        plan.files.push_back({it.path, dir, san.name});
    }

    // Selected folders: untouched by default. Only when grouping is enabled do
    // we move the ORIGINAL captured folders into the All Folder directory. The
    // generated output buckets above are NEVER moved into All Folder (we only
    // ever consider items from the immutable captured selection here).
    if (s.groupSelectedFolders && allSan.ok) {
        const std::wstring allDir = Join(root, allSan.name);
        bool anyFolder = false;
        for (const auto& it : items) {
            if (!it.isFolder) continue;
            // Reject a recursive / self move: source folder is the All Folder
            // target, contains it, or is contained by it.
            const std::wstring src = TrimSep(it.path);
            if (PathEqualCI(src, allDir) ||
                PathEqualCI(src, TrimSep(root)) ||
                // src is an ancestor of allDir (would move a parent into child)
                (allDir.size() > src.size() &&
                 PathEqualCI(allDir.substr(0, src.size()), src) &&
                 IsSep(allDir[src.size()]))) {
                plan.skipped.push_back({it.path, Skip::SelfRecursive});
                continue;
            }
            plan.folders.push_back({it.path, allDir});
            anyFolder = true;
        }
        if (anyFolder) addDir(allDir);
    } else if (s.groupSelectedFolders && !allSan.ok) {
        for (const auto& it : items) {
            if (it.isFolder) plan.skipped.push_back({it.path, Skip::Rejected});
        }
    }

    plan.ok = true;
    return plan;
}

}  // namespace autogroup
