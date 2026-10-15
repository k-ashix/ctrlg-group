// capture_logic.h
//
// PURE logic for the "name-capture" forks (inline_rename_test.wh.cpp and
// dailogbox_rename_test.wh.cpp). Both forks capture a base name from the user
// (inline temp-folder rename, or a popup box) and then create the batch. The
// decision "given a captured base + count, which folder paths do we create?"
// is pure and lives here so it can be unit-tested with a plain g++ build.
//
// This header is fork-only: it is NOT mirrored into the main mod
// (ctrlg-group.wh.cpp), which is intentionally left untouched.
// It builds on the existing mirrored core in newfolder_logic.h.
//
#pragma once

#include "newfolder_logic.h"

#include <cwctype>
#include <map>
#include <set>
#include <string>
#include <vector>

// Characters Windows forbids in a file/folder name.
inline bool IsForbiddenNameChar(wchar_t c) {
    return c == L'\\' || c == L'/' || c == L':' || c == L'*' ||
           c == L'?'  || c == L'"' || c == L'<' || c == L'>' || c == L'|';
}

// Case-fold a name for case-INSENSITIVE comparison. Windows directory names
// are case-insensitive, so "raw" and "RAW" name the same folder; dedup and
// reservation must fold before comparing (finding #F3).
inline std::wstring ToLowerFold(const std::wstring& s) {
    std::wstring r;
    r.reserve(s.size());
    for (wchar_t c : s) {
        r.push_back(static_cast<wchar_t>(towlower(c)));
    }
    return r;
}

// Sanitize a user-typed base name into something usable as a folder base:
//   - strip control chars and Windows-forbidden characters (\ / : * ? " < > |)
//   - trim leading/trailing whitespace
//   - strip trailing dots and spaces (Windows silently drops these)
// May return empty -> caller falls back to the default "New folder" flow.
inline std::wstring SanitizeFolderBase(const std::wstring& raw) {
    std::wstring s;
    s.reserve(raw.size());
    for (wchar_t c : raw) {
        // Strip ALL C0 control characters (< 0x20), which covers \r \n \t plus
        // every other control char that would otherwise survive into
        // CreateDirectoryW and make it fail.
        if (c < 0x20) {
            continue;
        }
        if (IsForbiddenNameChar(c)) {
            continue;
        }
        s.push_back(c);
    }

    // Trim leading whitespace.
    size_t b = 0;
    while (b < s.size() && iswspace(s[b])) {
        ++b;
    }
    // Trim trailing whitespace AND dots (Windows silently drops a trailing
    // '.' or ' ' from folder names, so we normalise them away up front).
    size_t e = s.size();
    while (e > b && (iswspace(s[e - 1]) || s[e - 1] == L'.')) {
        --e;
    }
    return s.substr(b, e - b);
}

// One uniquely-named folder for the single-folder case: "<base>", then
// "<base> (2)", "<base> (3)", ... (Explorer-native single-name style).
inline std::wstring MakeUniqueNamedFolder(const std::wstring& dir,
                                          const std::wstring& base,
                                          const NameExists& exists,
                                          bool autoResolve = true) {
    std::wstring first = JoinPath(dir, base);
    // #9: when auto-resolve is off we never rename a clashing custom name to
    // "(2)" -- we return the bare "<base>" and let the create layer report the
    // collision, exactly as autoResolveNames=off means for the default name.
    if (!autoResolve || !exists(first)) {
        return first;
    }
    for (int i = 2; i < 100000; ++i) {
        wchar_t buf[24] = {0};
        std::swprintf(buf, 24, L" (%d)", i);
        std::wstring candidate = JoinPath(dir, base + buf);
        if (!exists(candidate)) {
            return candidate;
        }
    }
    return first;
}

// Resolve the captured base + count into the concrete list of folder paths:
//   - sanitized base empty  -> default "New folder"/"New folder (N)" sequence
//                              (honouring autoResolve, same as the main mod)
//   - base + count == 1      -> single "<base>" (with " (2)" fallback)
//   - base + count > 1       -> "<base> 1", "<base> 2", ... (skips existing)
inline std::vector<std::wstring> ResolveCaptureNames(
        const std::wstring& dir, const std::wstring& rawBase,
        int count, const NameExists& exists, bool autoResolve = true) {
    std::wstring base = SanitizeFolderBase(rawBase);

    if (base.empty()) {
        // No usable name typed: behave exactly like the stock mod.
        return MakeUniqueFolderNames(dir, count, exists, autoResolve);
    }

    if (count <= 1) {
        // A single named folder keeps the bare name ("BIG"), only numbering
        // "(2)"-style if that exact name is taken AND auto-resolve is on.
        return { MakeUniqueNamedFolder(dir, base, exists, autoResolve) };
    }

    // A batch is "<base> 1", "<base> 2", ... skipping any that already exist.
    return MakeNamedFolderNames(dir, base, count, exists);
}

// ---------------------------------------------------------------------------
// #1  Capture + validate the origin Explorer target HWND.
//
// The bug: GetTargetExplorerHWND() was resolved LATE, on the worker thread,
// inside PerformNewFolderAction -- long after the shortcut fired (the multi-
// folder count timer alone is up to 1200ms). By then the foreground window /
// cursor may have moved, so the worker acted on the WRONG window (or none).
//
// The fix carries the HWND captured AT TRIGGER TIME through the queued
// WM_APP+1 message, and the worker only acts when that same target is still a
// valid, live Explorer window. This is the pure decision the worker consults;
// the Win32 layer supplies the observed facts.
//
// A pointer-width integer models an HWND without pulling in <windows.h>, so
// this stays g++-buildable alongside the rest of capture_logic.h.
typedef unsigned long long TargetHandle;   // stand-in for HWND in pure logic

struct TargetCheck {
    TargetHandle captured;   // HWND captured on the hook thread at trigger time
    bool stillValid;         // IsWindow(captured) at process time on the worker
    bool stillExplorer;      // captured is still an Explorer/shell target now
};

// True only when we captured a real target that is still a live Explorer
// window when the worker gets to it. A null capture, a destroyed window, or a
// window that is no longer Explorer all mean "cancel -- do not act".
inline bool ShouldActOnTarget(const TargetCheck& t) {
    if (t.captured == 0) {
        return false;   // nothing was captured at trigger time
    }
    if (!t.stillValid) {
        return false;   // window went away between trigger and processing
    }
    if (!t.stillExplorer) {
        return false;   // still alive but no longer an Explorer target
    }
    return true;
}

// ---------------------------------------------------------------------------
// #2  Stop the popup loop from draining stale actions off the worker queue.
//
// The bug: the popup (PromptForFolderName) runs a NESTED GetMessageW loop on
// the worker thread while PerformNewFolderAction is still in flight. The
// "already posted" latch is cleared BEFORE the handler runs, so a second
// WM_APP+1 posted during the modal popup sits in the queue and fires a stale
// duplicate action the moment the popup closes.
//
// This is the pure guard the worker consults before running a dequeued action:
// run it only when no action is already in progress AND the message actually
// carried a captured origin target (a bare/legacy post with no target is a
// leftover that must be drained, not executed).
struct QueuedAction {
    bool unloading;           // g_unloading set by Worker_Exit (teardown in flight)
    bool actionInProgress;    // g_actionRunning already held by an earlier action
    TargetHandle target;      // origin HWND carried in the message (0 == none)
};

inline bool ShouldRunQueuedAction(const QueuedAction& q) {
    if (q.unloading) {
        return false;   // teardown in flight -> drain, never START a new action
                        // during unload (would seed a temp folder mid-shutdown)
    }
    if (q.actionInProgress) {
        return false;   // drain (discard) -- an action is already running
    }
    if (q.target == 0) {
        return false;   // stale/legacy post with no captured target -> drain
    }
    return true;
}

// ---------------------------------------------------------------------------
// #2 (worker-busy false-cancel): inputs the worker has when deciding whether a
// queued folder action is safe to run. snapshotPosted = the WM_APP+2 snapshot
// POST succeeded; snapshotDirResolved = the snapshot actually resolved a dir;
// currentDirResolved = the active view resolved a dir now; dirsMatch = the
// snapshot dir equals the current dir (no same-tab drift); tabStillValid = the
// captured tab HWND is still the active tab (no tab switch). workerWasBusy is
// informational only: a busy worker (late snapshot) must NOT by itself cancel.
struct SnapshotTrust {
    bool snapshotPosted;
    bool snapshotDirResolved;
    bool currentDirResolved;
    bool dirsMatch;
    bool tabStillValid;
    bool workerWasBusy;   // late snapshot due to a prior in-flight action
};

// True => RUN the action. The drift protection is the DIRECTORY COMPARE plus the
// TAB-HWND guard, both authoritative. A late snapshot (workerWasBusy) is NOT a
// reason to cancel on its own -- that was the false-cancel bug. Cancel only when
// there is no drift protection at all (no post / unresolved dirs) or when a real
// drift is detected (dirs differ or the tab switched).
inline bool ShouldRunAfterSnapshot(const SnapshotTrust& s) {
    if (!s.snapshotPosted)       return false; // no drift protection -> cancel
    if (!s.snapshotDirResolved)  return false; // snapshot empty -> cancel
    if (!s.currentDirResolved)   return false; // can't verify -> cancel
    if (!s.tabStillValid)        return false; // tab switched -> cancel
    if (!s.dirsMatch)            return false; // real same-tab drift -> cancel
    return true;                                // safe: busy-but-no-drift -> RUN
}

// ---------------------------------------------------------------------------
// (regression) Inline rename Esc must CANCEL -- it must never create folders.
//
// The bug: WaitForInlineRenameResult() only returned the last text it saw in
// the rename EDIT and a bare "did an EDIT ever appear" bool. It inferred
// "committed" from "the EDIT disappeared" -- but pressing Esc makes the EDIT
// disappear exactly like Enter does, leaving the typed text ("Projects") in
// `typed`. In multi-folder mode the batch path then ran and turned the temp
// folder into "Projects 1..N", violating the README's "Esc = cancel" contract.
//
// The fix makes the watcher report a terminal OUTCOME, and this pure decision
// says whether the captured name may drive folder creation. Only a real commit
// (Enter, or focus-lost-commit) counts; cancel / timeout / never-started must
// discard the captured text and cancel the action -- never create.
enum class RenameOutcome {
    NeverStarted,        // the rename EDIT never appeared
    Committed,           // user pressed Enter -> use the typed name
    FocusLostCommitted,  // focus left the EDIT with text intact -> commit
    Cancelled,           // user pressed Esc -> discard, cancel
    Timeout,             // watcher timed out -> discard, cancel
};

// True only when the captured name may be used to create folders.
inline bool ShouldUseCapturedName(RenameOutcome outcome) {
    return outcome == RenameOutcome::Committed ||
           outcome == RenameOutcome::FocusLostCommitted;
}

// ---------------------------------------------------------------------------
// #3  Tab/path-safe target: capturing the Explorer WINDOW HWND is not enough.
//
// A single Explorer window can host several tabs. If the user triggers in tab A
// and then switches to tab B while the action is queued/waiting, the worker
// resolves the window's *currently active* tab and can create folders in B.
// So in addition to the window HWND we capture the active TAB identity at
// trigger time and, on the worker, require the active tab to be unchanged.
//
// This is the pure decision. `capturedTab` is the tab that was active when the
// shortcut fired; `currentTab` is the active tab now, on the worker. A zero
// captured tab means the platform could not identify a tab at trigger time
// (e.g. Desktop / single-tab shell) -- there is no tab to drift, so proceed.
struct TabCheck {
    TargetHandle capturedTab;   // active shell tab HWND at trigger time (0 = none)
    TargetHandle currentTab;    // active shell tab HWND now, on the worker
};

inline bool ShouldActOnTab(const TabCheck& t) {
    if (t.capturedTab == 0) {
        return true;    // no tab identity to compare (Desktop / single tab)
    }
    return t.capturedTab == t.currentTab;   // same tab -> safe to act
}

// ---------------------------------------------------------------------------
// #2  Directory-safe target: the tab guard (#3) is still not enough.
//
// A single tab keeps its HWND across navigation, so triggering in C:\A and then
// navigating that SAME tab to C:\B during count entry passes ShouldActOnTab yet
// resolves the directory to B on the worker. The trigger-time dir snapshot is
// itself resolved on the worker (via a posted WM_APP+2), so it is really a
// "worker-processing-time" snapshot -- if the nav landed before the snapshot
// ran it too reads B. In every case, if the directory we captured for the
// trigger differs from the directory the view resolves to NOW, we can neither
// trust "create where you were browsing" nor safely drive the inline rename
// against the live view (which is showing B). The safe decision is to CANCEL.
//
// Pure decision (unified #2 + #3 rule). We ACT only when we have a snapshot we
// can FULLY trust and it still matches the live dir. Everything else CANCELS --
// we never fall back to "create in the current dir with no verified snapshot".
//   snapshotExpected : the WM_APP+2 snapshot POST succeeded (false == the post
//                      failed, so there is no drift protection at all).       [#3]
//   snapshotFresh    : the snapshot was RESOLVED close enough to the keypress
//                      that the user could not have navigated the tab before it
//                      was captured (see SnapshotIsFresh). A snapshot resolved
//                      late (the worker was busy) is a "worker-processing-time"
//                      value, not a keypress-time capture, and can't be trusted. [#2]
//   capturedDir      : the browse-dir snapshot for the trigger. Empty here means
//                      the post succeeded but resolution FAILED -- "post ok" !=
//                      "snapshot ok" -- which is unsafe, NOT "nothing to compare". [#3]
//   currentDir       : the dir the active view resolves to now, on the worker
//                      (empty == could not resolve -> unverifiable -> cancel).
// Comparison is case-insensitive (Windows paths).
struct DirCheck {
    bool snapshotExpected;      // WM_APP+2 snapshot POST succeeded for this action
    bool snapshotFresh;         // #2: snapshot resolved close enough to the keypress
    std::wstring capturedDir;   // browse dir snapshot for the trigger ("" = none)
    std::wstring currentDir;    // dir the active view resolves to now on the worker
};

inline bool ShouldActOnDir(const DirCheck& d) {
    // #3 (post-failure hole): the WM_APP+2 snapshot post FAILED, so this action
    // has no directory-drift protection at all. Rather than run it unprotected
    // (possibly creating in a drifted-to dir), CANCEL.
    if (!d.snapshotExpected) {
        return false;   // could not even request a snapshot -> safe cancel
    }
    // #2 (true trigger-time capture): the snapshot resolved too late after the
    // keypress to be trustworthy (the worker was busy, so the user may have
    // navigated the tab BEFORE the snapshot ran). It is a worker-processing-time
    // value, not a keypress-time capture -> cancel.
    if (!d.snapshotFresh) {
        return false;   // stale snapshot -> safe cancel
    }
    // #3 (resolution-failure hole): "post ok" != "snapshot ok". A successful post
    // with an EMPTY captured dir means ResolveActiveShellView failed, so we have
    // nothing trustworthy to compare -- CANCEL instead of proceeding on the
    // current dir. An empty current dir is likewise unverifiable -> CANCEL.
    if (d.capturedDir.empty() || d.currentDir.empty()) {
        return false;   // no resolved snapshot / current dir -> safe cancel
    }
    // Case-insensitive compare (Windows paths are case-insensitive).
    if (d.capturedDir.size() != d.currentDir.size()) {
        return false;
    }
    for (size_t i = 0; i < d.capturedDir.size(); ++i) {
        if (towlower(d.capturedDir[i]) != towlower(d.currentDir[i])) {
            return false;   // directory changed since trigger -> cancel
        }
    }
    return true;    // fresh, resolved, and unchanged -> safe to act
}

// #2 (true trigger-time capture): decide whether a browse-dir snapshot is fresh
// enough to be treated as a keypress-time capture. `keypressTick` is stamped on
// the hook thread the instant the shortcut fires; `resolveTick` is stamped on the
// worker the instant it actually resolves the snapshot. If the worker was busy,
// resolveTick lands well after keypressTick and the snapshot may reflect a dir
// the user navigated to AFTER the keypress -- so it is not fresh. A zero tick
// (never stamped) or a backwards delta is treated as NOT fresh (cancel). Mirrors
// the freshness computation the worker does before calling PerformNewFolderAction.
inline bool SnapshotIsFresh(unsigned long long keypressTick,
                            unsigned long long resolveTick,
                            unsigned long long maxLatencyMs) {
    if (keypressTick == 0 || resolveTick == 0) {
        return false;   // never stamped -> cannot prove freshness -> cancel
    }
    if (resolveTick < keypressTick) {
        return false;   // clock skew / out of order -> don't trust
    }
    return (resolveTick - keypressTick) <= maxLatencyMs;
}

// ===========================================================================
// Folder Templates (feature) -- pure logic. See SPEC.md "Folder Templates".
//
// A template is a fixed, named set of folders created in one shortcut. Two
// pure decisions live here and are unit-tested without Win32:
//   1. ResolveTemplateKeys   -- load-time: normalise keys, dedup keys
//      (first-in-list wins), drop invalid templates, enforce the 26 cap.
//   2. ResolveTemplateFolders -- fire-time: given a template + which folders
//      already exist + collision mode + batch NamePattern, return the concrete
//      folder paths to create (empty for "stop when any exists").
//
// >>> RED STUBS <<< : the bodies below are intentionally wrong so the tests
// fail first. They are filled in during the GREEN step.
// ===========================================================================

// Caps (SPEC: single ASCII letter keys A-Z; guard runaway settings).
static const int kMaxTemplates       = 26;
static const int kMaxFoldersPerTemplate = 100;

enum class TemplateCollision { Skip = 0, Number = 1, Stop = 2 };

// A template as read from settings (before validation/normalisation).
// The user assigns the key explicitly, independent of the name.
struct FolderTemplate {
    std::wstring key;                    // user-assigned single letter
    std::wstring name;
    std::vector<std::wstring> folders;   // in creation order
};

// One usable template after key resolution.
struct ResolvedTemplate {
    wchar_t key = 0;                     // normalised single lowercase letter
    std::wstring name;
    std::vector<std::wstring> folders;   // validated leaf names, deduped
};

// Result of load-time resolution.
struct TemplateResolution {
    std::vector<ResolvedTemplate> usable;   // in list order, one per key
    std::vector<std::wstring> ignored;      // human-readable reasons (logged)
};

// True if `folder` is a valid leaf folder name for a template entry:
//   - not empty / not whitespace-only
//   - no path separator (\ or /)
//   - not "." / ".." and does not start with ".."
//   - not an absolute path (drive-letter or leading separator)
//   - no C0 control characters (< 0x20)
inline bool IsValidTemplateFolderName(const std::wstring& folder) {
    if (folder.empty()) {
        return false;
    }
    // Whitespace-only?
    bool allSpace = true;
    for (wchar_t c : folder) {
        if (!iswspace(c)) { allSpace = false; break; }
    }
    if (allSpace) {
        return false;
    }
    // #7: reject the SAME character set that a typed folder name strips
    // (IsForbiddenNameChar covers \ / : * ? " < > |), plus C0 control chars.
    // Previously only \ / : were rejected, so a template could smuggle in a
    // name (e.g. "a*b") that the normal typed-name path would never allow.
    for (wchar_t c : folder) {
        if (c < 0x20) {
            return false;
        }
        if (IsForbiddenNameChar(c)) {
            return false;
        }
    }
    // "." / ".." and any name that starts with ".." (relative traversal).
    if (folder == L"." || folder == L".." ) {
        return false;
    }
    if (folder.size() >= 2 && folder[0] == L'.' && folder[1] == L'.') {
        return false;
    }
    return true;
}

// Normalise a raw key string to a single lowercase letter, or 0 if invalid
// (empty, digit, or non-letter first char). Trims surrounding whitespace.
inline wchar_t NormalizeTemplateKey(const std::wstring& rawKey) {
    // Trim surrounding whitespace, take the first surviving character.
    size_t b = 0;
    while (b < rawKey.size() && iswspace(rawKey[b])) {
        ++b;
    }
    if (b >= rawKey.size()) {
        return 0;   // empty / whitespace only
    }
    wchar_t c = rawKey[b];
    if (c >= L'A' && c <= L'Z') {
        c = static_cast<wchar_t>(c - L'A' + L'a');
    }
    if (c >= L'a' && c <= L'z') {
        return c;
    }
    return 0;   // digit or any non a-z letter -> invalid
}

// Load-time resolution (SPEC "Duplicate keys -- first in list wins").
inline TemplateResolution ResolveTemplateKeys(
        const std::vector<FolderTemplate>& raw) {
    TemplateResolution res;
    std::set<wchar_t> claimed;              // keys already owned
    std::map<wchar_t, std::wstring> owner;  // key -> owning template name

    for (const auto& t : raw) {
        // 26-template cap by list position: once 26 usable are collected,
        // every further template is ignored.
        if (static_cast<int>(res.usable.size()) >= kMaxTemplates) {
            res.ignored.push_back(
                L"Template '" + t.name + L"' ignored (over 26-template cap)");
            continue;
        }

        if (t.name.empty()) {
            res.ignored.push_back(L"Template with blank name ignored");
            continue;
        }

        // The key is user-assigned and independent of the name.
        wchar_t key = NormalizeTemplateKey(t.key);
        if (key == 0) {
            res.ignored.push_back(
                L"Template '" + t.name +
                L"' ignored (key must be a single letter A-Z)");
            continue;
        }

        if (claimed.count(key)) {
            res.ignored.push_back(
                L"Template '" + t.name + L"' key '" + std::wstring(1, key) +
                L"' ignored (already used by '" + owner[key] + L"')");
            continue;
        }

        // Validate + dedup folder names (first occurrence wins), enforce the
        // per-template 100-folder cap.
        std::vector<std::wstring> folders;
        std::set<std::wstring> seen;   // folded names (case-insensitive dedup)
        for (const auto& f : t.folders) {
            if (static_cast<int>(folders.size()) >= kMaxFoldersPerTemplate) {
                break;   // cap hit; remaining entries dropped
            }
            if (!IsValidTemplateFolderName(f)) {
                continue;   // invalid leaf name dropped
            }
            std::wstring fold = ToLowerFold(f);
            if (seen.count(fold)) {
                continue;   // duplicate within the template dropped (raw==RAW)
            }
            seen.insert(fold);
            folders.push_back(f);
        }

        if (folders.empty()) {
            res.ignored.push_back(
                L"Template '" + t.name +
                L"' ignored (no valid folder names)");
            continue;
        }

        ResolvedTemplate rt;
        rt.key = key;
        rt.name = t.name;
        rt.folders = std::move(folders);
        res.usable.push_back(std::move(rt));
        claimed.insert(key);
        owner[key] = t.name;
    }

    return res;
}

// Fire-time resolution. `exists(fullPath)` -> folder already present.
// Returns concrete paths to create, in template order.
//   Skip   -> drop existing, keep the rest.
//   Number -> for an existing name, make a numbered sibling via FormatBatchName
//             (never overwrites); non-existing names created as-is.
//   Stop   -> if ANY folder already exists, return empty (create nothing).
// One planned folder: the ORIGINAL template base name plus the concrete path
// to create. #1: carrying `base` lets a create-time race retry number from the
// original name ("raw" -> "raw 3"), never from an already-numbered candidate
// ("raw 2" -> "raw 2 2").
struct TemplateFolderPlan {
    std::wstring base;   // original template leaf name (for race-retry numbering)
    std::wstring path;   // resolved full path to create
};

inline std::vector<TemplateFolderPlan> ResolveTemplateFolders(
        const std::wstring& dir, const ResolvedTemplate& tmpl,
        const NameExists& exists, TemplateCollision collision,
        NamePattern pattern) {
    // Validate + dedup (first wins, CASE-INSENSITIVE per Windows) + cap, so
    // this is safe even when the caller built a ResolvedTemplate directly.
    std::vector<std::wstring> names;
    std::set<std::wstring> seen;   // folded names
    for (const auto& f : tmpl.folders) {
        if (static_cast<int>(names.size()) >= kMaxFoldersPerTemplate) {
            break;
        }
        if (!IsValidTemplateFolderName(f)) {
            continue;
        }
        std::wstring fold = ToLowerFold(f);
        if (seen.count(fold)) {
            continue;   // raw|RAW name the same folder -> keep first only (#F3)
        }
        seen.insert(fold);
        names.push_back(f);
    }

    std::vector<TemplateFolderPlan> out;

    // #2: reserve every path chosen THIS action (case-insensitive: raw==RAW on
    // Windows). Without it, [raw, "raw 2"] with "raw" on disk plans "raw 2"
    // twice. A path is "taken" if it is on disk OR already reserved this run.
    std::set<std::wstring> planned;   // folded paths
    auto taken = [&](const std::wstring& p) {
        return exists(p) || planned.count(ToLowerFold(p)) > 0;
    };
    // #1: each plan carries the ORIGINAL base name for correct race-retry.
    auto reserve = [&](const std::wstring& base, const std::wstring& path) {
        planned.insert(ToLowerFold(path));
        out.push_back(TemplateFolderPlan{ base, path });
    };

    // Stop: preflight. If ANY requested folder already exists on disk, create
    // nothing. (Names are deduped above; races after this point are not
    // transactional -- see SPEC.)
    if (collision == TemplateCollision::Stop) {
        for (const auto& n : names) {
            if (exists(JoinPath(dir, n))) {
                return {};   // cancel whole action
            }
        }
        for (const auto& n : names) {
            reserve(n, JoinPath(dir, n));
        }
        return out;
    }

    for (const auto& n : names) {
        std::wstring path = JoinPath(dir, n);
        if (!taken(path)) {
            reserve(n, path);
            continue;
        }
        // The name is taken (already on disk, or already reserved this run).
        if (collision == TemplateCollision::Skip) {
            continue;   // skip it, keep the rest
        }
        // Number: numbered sibling of the ORIGINAL base `n`, never overwriting
        // AND never colliding with a path already planned this run.
        for (int i = 2; i < 1000000; ++i) {
            std::wstring candidate = JoinPath(dir, FormatBatchName(n, i, pattern));
            if (!taken(candidate)) {
                reserve(n, candidate);
                break;
            }
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// #1  Template-key mask (lock-free hook lookup).
//
// The LL keyboard hook must NOT enumerate the g_templates vector (it has to
// stay non-blocking and thread-safe). Instead the mod publishes a 26-bit mask
// (bit (c-'a') set == letter 'c' fires a template). The hook reads the mask
// with one atomic load and asks IsTemplateKeyBound. An UNBOUND letter is not a
// template -- the caller falls through to the normal single-folder commit,
// instead of posting a template the worker cannot resolve (which created
// nothing: the bug in #1).
inline unsigned long BuildTemplateKeyMask(
        const std::vector<ResolvedTemplate>& usable) {
    unsigned long mask = 0;
    for (const auto& t : usable) {
        wchar_t c = t.key;
        if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
        if (c >= L'a' && c <= L'z') {
            mask |= (1UL << (c - L'a'));
        }
    }
    return mask;
}

inline bool IsTemplateKeyBound(unsigned long mask, wchar_t typed) {
    wchar_t c = typed;
    if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
    if (c < L'a' || c > L'z') return false;
    return (mask & (1UL << (c - L'a'))) != 0;
}

// ---------------------------------------------------------------------------
// #6  Next free numbered sibling (create-time race recovery for Number mode).
//
// ResolveTemplateFolders picks names that are free at PLAN time. Between plan
// and CreateDirectoryW another process can create one (a lost check-then-create
// race -> ERROR_ALREADY_EXISTS). In Number mode the create loop recovers by
// asking for the next free numbered sibling of the leaf, re-scanning the disk
// so it advances past whatever just appeared. Never overwrites.
//
// #8-1: the retry must ALSO avoid every OTHER path this action already reserved
// (the other TemplateFolderPlan.path values). Consulting the disk alone lets a
// retry grab a name a LATER plan is going to create -- e.g. [raw, "raw 3"] with
// "raw" on disk plans ["raw 2", "raw 3"]; if "raw 2" is raced away, a disk-only
// retry sees "raw 3" free (it is only RESERVED, not yet on disk) and collides
// with the second plan. `reserved` holds the folded paths still owned by this
// action; a candidate is taken if it is on disk OR in that set.
inline std::wstring NextFreeTemplateSibling(const std::wstring& dir,
                                            const std::wstring& leaf,
                                            NamePattern pattern,
                                            const NameExists& exists,
                                            const std::set<std::wstring>& reserved = {}) {
    for (int i = 2; i < 1000000; ++i) {
        std::wstring candidate = JoinPath(dir, FormatBatchName(leaf, i, pattern));
        if (!exists(candidate) && reserved.count(ToLowerFold(candidate)) == 0) {
            return candidate;
        }
    }
    return JoinPath(dir, leaf);
}
