// group_logic.h
//
// PURE logic for the Ctrl+G "Group Selection" feature, mirrored from
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build (no Win32 runtime). The mod is a Windhawk translation unit with no
// standalone main(), so each pure decision is mirrored here and exercised by
// tests\group_*_test.cpp -- the same contract the rest of the suite uses.
//
// Feature (waht to create.md): select files/folders in Explorer, press Ctrl+G,
// group them into ONE destination folder (Move or Copy). Reuses the mod's
// existing numbering patterns and popup/action pipeline. NOT a second popup.
//
// Decisions that live here (pure, testable without the shell):
//   1. CommonParent          -- shared parent of a selection (empty if mixed)
//   2. GroupCollisionResolve  -- Use-existing (default) / Numbered / Cancel
//   3. ResolveGroupTarget     -- typed name vs blank->last-target resolution
//   4. Group safety           -- empty selection, self/child move, virtual items
//
// >>> RED STUBS <<< : bodies are intentionally unimplemented so the tests fail
// first. They are filled in during the GREEN step.

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cwctype>
#include <cstddef>

// Reuse the shared pure path/numbering helpers already mirrored for the mod.
#include "newfolder_logic.h"  // JoinPath, MakeUniqueFolderName, NameExists,
                              // FormatBatchName, NamePattern, PadNumber

// ---------------------------------------------------------------------------
// Path helpers (pure, separator-normalised). Windows paths use backslash but
// the mod also tolerates forward slashes, so these treat both as separators.
// ---------------------------------------------------------------------------

inline bool IsSepChar(wchar_t c) { return c == L'\\' || c == L'/'; }

// Strip one trailing separator (so "C:\a\" and "C:\a" compare equal). A bare
// root like "C:\" keeps its separator.
inline std::wstring TrimTrailingSep(const std::wstring& p) {
    if (p.size() > 1 && IsSepChar(p[p.size() - 1])) {
        // Keep a drive root's separator ("C:\").
        if (p.size() == 3 && p[1] == L':') {
            return p;
        }
        return p.substr(0, p.size() - 1);
    }
    return p;
}

// Parent directory of a path (everything before the last separator). Returns
// empty when there is no separator or only a drive root.
inline std::wstring ParentDirOf(const std::wstring& path) {
    std::wstring p = TrimTrailingSep(path);
    // A bare drive root has no parent.
    if (p.size() <= 3 && p.size() >= 2 && p[1] == L':') {
        return std::wstring();
    }
    std::wstring::size_type pos = std::wstring::npos;
    for (std::wstring::size_type i = p.size(); i > 0; --i) {
        if (IsSepChar(p[i - 1])) { pos = i - 1; break; }
    }
    if (pos == std::wstring::npos) {
        return std::wstring();
    }
    // Top-level: "C:\a" -> parent is the drive root "C:\".
    if (pos == 2 && p[1] == L':') {
        return p.substr(0, 3);
    }
    return p.substr(0, pos);
}

// True if `child` is `ancestor` itself or nested anywhere beneath it. Used to
// forbid moving a folder into itself or one of its own descendants.
inline bool IsSameOrDescendant(const std::wstring& ancestor,
                               const std::wstring& child) {
    std::wstring a = TrimTrailingSep(ancestor);
    std::wstring c = TrimTrailingSep(child);
    if (a.empty()) return false;
    if (c.size() < a.size()) return false;
    // Case-sensitive compare of the prefix (paths mirrored in tests match case;
    // the mod passes already-normalised paths).
    if (c.compare(0, a.size(), a) != 0) return false;
    if (c.size() == a.size()) return true;            // same path
    return IsSepChar(c[a.size()]);                    // real path boundary
}

// A resolved/desired Group destination path may be used ONLY when nothing
// occupies it (it will be created) OR what occupies it is a directory. A plain
// file at the path is a hard error: the Group operation must be refused BEFORE
// any filesystem side effect, rather than reused or copied into (fix thse
// isseue.md P1 "Normal Group does not verify that the destination is a
// directory"). Mirrored into ctrlg-group.wh.cpp.
inline bool GroupDestinationUsable(bool destExists, bool destIsDirectory) {
    return !destExists || destIsDirectory;
}

// ---------------------------------------------------------------------------
// #1  Common parent of the current selection.
// Returns the shared parent directory when every selected path has the SAME
// parent; returns empty when the selection is empty or spans multiple parents
// (caller must then require an explicit destination).
// ---------------------------------------------------------------------------
inline std::wstring CommonParent(const std::vector<std::wstring>& paths) {
    if (paths.empty()) return std::wstring();
    std::wstring parent = ParentDirOf(paths[0]);
    if (parent.empty()) return std::wstring();
    for (size_t i = 1; i < paths.size(); ++i) {
        if (TrimTrailingSep(ParentDirOf(paths[i])) != TrimTrailingSep(parent)) {
            return std::wstring();
        }
    }
    return parent;
}

// ---------------------------------------------------------------------------
// #2  Existing-folder collision policy.
// ---------------------------------------------------------------------------
enum class GroupCollision {
    UseExisting = 0,  // default: group into the folder that already exists
    Numbered    = 1,  // make a numbered sibling ("Prewedding 2", ...)
    Cancel      = 2,  // do nothing
};

// ---------------------------------------------------------------------------
// Settings string -> decision mapping (Task 9). These map the mod's
// groupOperation / groupCollision $options string values to the pure types.
// Unknown / empty values fall back to the documented DEFAULTS (Copy,
// UseExisting) so a malformed setting can never do something surprising.
// ---------------------------------------------------------------------------

// groupOperation: "move" -> Move (true); anything else (incl. "copy"/empty)
// -> Copy (false), the default.
inline bool ParseGroupOperationIsMove(const std::wstring& value) {
    return value == L"move";
}

// groupCollision: "numbered" -> Numbered, "cancel" -> Cancel; anything else
// (incl. "useExisting"/empty) -> UseExisting, the default.
inline GroupCollision ParseGroupCollision(const std::wstring& value) {
    if (value == L"numbered") return GroupCollision::Numbered;
    if (value == L"cancel")   return GroupCollision::Cancel;
    return GroupCollision::UseExisting;
}

// ---------------------------------------------------------------------------
// Issue 1: FILE-level collision policy for Ctrl+G Move/Copy.
//
// The policies above (GroupCollision) decide what to do when the DESTINATION
// FOLDER already exists. This one decides what happens when, DURING the
// move/copy, a FILE of the same name already lives in the destination -- the
// case where Windows normally pops its "Replace or Skip Files" dialog.
//
//   Ask     (default) -- let the shell show its native Replace/Skip dialog.
//   Replace           -- overwrite the existing file, no prompt.
//   Skip              -- keep the existing file, no prompt, drop the incoming.
//
// The mapping to IFileOperation flag bits is mirrored here so it can be unit
// tested without Win32. FOF_NOCONFIRMATION / FOF_RENAMEONCOLLISION are the real
// shell flag values; guarded with #ifndef so this header stays valid both in
// the pure test build (no windows.h) and inside the mod (windows.h first).
// ---------------------------------------------------------------------------
#ifndef FOF_NOCONFIRMATION
#define FOF_NOCONFIRMATION 0x0010
#endif
#ifndef FOF_RENAMEONCOLLISION
#define FOF_RENAMEONCOLLISION 0x0008
#endif

// A private sentinel bit (NOT a real shell flag; sits well above the defined
// FOF_* range) that tells the worker "on a name clash, skip the incoming item
// and keep the existing one." The worker masks this bit off before handing the
// remaining real flags to IFileOperation::SetOperationFlags.
static const unsigned int kGroupSkipExistingBit = 0x40000000u;
// Second private sentinel: "Compare" -- show the native conflict box (which
// carries Windows' own "Compare info for both files" choice). Lets the worker
// and logs tell Compare apart from plain Ask. Also masked off before the flags
// reach IFileOperation. Distinct bit from the skip sentinel.
static const unsigned int kGroupCompareBit = 0x20000000u;

enum class GroupFileCollision {
    Ask     = 0,  // default: native shell "Replace or Skip Files" dialog
    Replace = 1,  // overwrite the existing file, no prompt
    Skip    = 2,  // keep the existing file, no prompt
    Compare = 3,  // native box, incl. "Compare info for both files"
};

// groupFileCollision: "replace" -> Replace, "skip" -> Skip, "compare" ->
// Compare; anything else (incl. "ask"/empty/unknown) -> Ask, the default.
inline GroupFileCollision ParseGroupFileCollision(const std::wstring& value) {
    if (value == L"replace") return GroupFileCollision::Replace;
    if (value == L"skip")    return GroupFileCollision::Skip;
    if (value == L"compare") return GroupFileCollision::Compare;
    return GroupFileCollision::Ask;
}

// Map the policy to the flag bits PerformGroupOperation applies. Ask adds
// nothing (shell prompts). Replace suppresses confirmation so the copy wins.
// Skip suppresses confirmation AND sets the private skip sentinel the worker
// uses to drop colliding items. Compare shows the native box (no
// confirmation-suppression, like Ask) but carries its own sentinel so it is
// distinguishable from Ask.
inline unsigned int FileCollisionFlags(GroupFileCollision policy) {
    switch (policy) {
        case GroupFileCollision::Replace:
            return FOF_NOCONFIRMATION;
        case GroupFileCollision::Skip:
            return FOF_NOCONFIRMATION | kGroupSkipExistingBit;
        case GroupFileCollision::Compare:
            return kGroupCompareBit;
        case GroupFileCollision::Ask:
        default:
            return 0u;
    }
}

// True when the policy lets the Windows shell draw its native "Replace or Skip
// Files" box: Ask and Compare (neither suppresses confirmation). Replace and
// Skip suppress the box (FOF_NOCONFIRMATION), so nothing is shown.
//
// PerformGroupOperation raises the owner (Explorer) window to the foreground
// right before PerformOperations() exactly when this is true, so the modal box
// surfaces on top instead of hiding behind the window the user is looking at.
inline bool FileCollisionShowsNativePrompt(GroupFileCollision policy) {
    return (FileCollisionFlags(policy) & FOF_NOCONFIRMATION) == 0;
}

enum class GroupResolveKind {
    Create,     // destination folder does not exist -> create it
    UseExisting,// reuse the existing destination folder
    Cancel,     // abort (Cancel policy on an existing folder)
};

struct GroupTargetResolution {
    GroupResolveKind kind = GroupResolveKind::Cancel;
    std::wstring path;  // full destination folder path (empty when Cancel)
};

// Resolve where a group of items should go, given the destination parent
// directory `dir`, the desired folder `name`, the collision `policy`, an
// `exists` probe and the batch `pattern` used for numbered siblings.
//  - name does not exist              -> Create at <dir>\<name>
//  - name exists, UseExisting         -> UseExisting <dir>\<name>
//  - name exists, Numbered            -> Create next free numbered sibling
//  - name exists, Cancel              -> Cancel
inline GroupTargetResolution GroupCollisionResolve(
        const std::wstring& dir, const std::wstring& name,
        GroupCollision policy, const NameExists& exists,
        NamePattern pattern = NamePattern::Space) {
    GroupTargetResolution out;
    std::wstring full = JoinPath(dir, name);

    if (!exists(full)) {
        out.kind = GroupResolveKind::Create;
        out.path = full;
        return out;
    }

    switch (policy) {
        case GroupCollision::UseExisting:
            out.kind = GroupResolveKind::UseExisting;
            out.path = full;
            return out;
        case GroupCollision::Numbered: {
            // Reuse the mod's batch numbering shape ("<base> 2", "-02", ...).
            // Start at 2 so the first sibling of "Prewedding" is "Prewedding 2".
            for (int i = 2; i < 1000000; ++i) {
                std::wstring cand = JoinPath(dir, FormatBatchName(name, i, pattern));
                if (!exists(cand)) {
                    out.kind = GroupResolveKind::Create;
                    out.path = cand;
                    return out;
                }
            }
            out.kind = GroupResolveKind::Cancel;
            return out;
        }
        case GroupCollision::Cancel:
        default:
            out.kind = GroupResolveKind::Cancel;
            return out;
    }
}

// ---------------------------------------------------------------------------
// #3  Destination-name resolution: typed name vs blank -> last target.
// Trims whitespace from the typed name. When the trimmed name is empty and a
// valid last-used target exists, the group goes to that last target; otherwise
// the typed name is used under `dir`. Output is a bare folder name plus whether
// the last target should be reused wholesale.
// ---------------------------------------------------------------------------
struct GroupTargetChoice {
    bool useLastTarget = false;   // true -> reuse lastTargetPath wholesale
    std::wstring folderName;      // trimmed folder name (when !useLastTarget)
    std::wstring lastTargetPath;  // echoed last target (when useLastTarget)
    bool valid = false;           // false -> nothing to do (blank + no last)
};

inline GroupTargetChoice ResolveGroupTarget(const std::wstring& typedName,
                                            const std::wstring& lastTargetPath) {
    // Trim leading/trailing whitespace from the typed name.
    size_t b = 0, e = typedName.size();
    while (b < e && iswspace(typedName[b])) ++b;
    while (e > b && iswspace(typedName[e - 1])) --e;
    std::wstring trimmed = typedName.substr(b, e - b);

    GroupTargetChoice out;
    if (!trimmed.empty()) {
        out.useLastTarget = false;
        out.folderName = trimmed;
        out.valid = true;
        return out;
    }
    // Blank name: fall back to the last used target when one exists.
    if (!lastTargetPath.empty()) {
        out.useLastTarget = true;
        out.lastTargetPath = lastTargetPath;
        out.valid = true;
        return out;
    }
    out.valid = false;  // nothing typed and no last target -> nothing to do
    return out;
}

// ---------------------------------------------------------------------------
// #4  Safety validation of the captured selection against a destination.
// ---------------------------------------------------------------------------
struct GroupItem {
    std::wstring path;            // full filesystem path
    bool isFolder = false;
    bool isVirtual = false;       // non-filesystem / shell-namespace item
};

enum class GroupSafety {
    Ok = 0,
    EmptySelection,      // nothing selected
    VirtualItem,         // an unsupported virtual/non-filesystem item present
    MoveIntoSelf,        // destination is one of the selected folders or beneath it
    DestNotDirectory,    // destination path is occupied by a FILE, not a folder
};

// Validate a captured selection against the resolved destination folder path.
// `moving` is true for a Move (self/child checks matter); a Copy also refuses
// virtual items and empty selections but self/child nesting is still unsafe.
inline GroupSafety ValidateGroup(const std::vector<GroupItem>& items,
                                 const std::wstring& destPath,
                                 bool moving) {
    if (items.empty()) {
        return GroupSafety::EmptySelection;
    }
    // Any virtual / non-filesystem item makes the whole gesture unsafe.
    for (const auto& it : items) {
        if (it.isVirtual) {
            return GroupSafety::VirtualItem;
        }
    }
    (void)moving;  // self/child nesting is unsafe for both move and copy
    // Never let the destination be one of the selected folders or nested in it.
    for (const auto& it : items) {
        if (it.isFolder && IsSameOrDescendant(it.path, destPath)) {
            return GroupSafety::MoveIntoSelf;
        }
    }
    return GroupSafety::Ok;
}

// Type-aware overload: `destExistsAsFile` is true when the destination path
// already exists and is NOT a directory (a plain file). It is checked FIRST --
// before the self/descendant guard and, in the mod, before any filesystem side
// effect -- so a file occupying the destination is reported (DestNotDirectory)
// and the gesture aborts without creating or queuing anything (fix thse
// isseue.md P1). Mirrored into ctrlg-group.wh.cpp PerformGroupOperation.
inline GroupSafety ValidateGroup(const std::vector<GroupItem>& items,
                                 const std::wstring& destPath,
                                 bool moving,
                                 bool destExistsAsFile) {
    if (items.empty()) {
        return GroupSafety::EmptySelection;
    }
    for (const auto& it : items) {
        if (it.isVirtual) {
            return GroupSafety::VirtualItem;
        }
    }
    // Destination-type check BEFORE the self/descendant check and before any
    // side effect: a file at the destination path can never be a group folder.
    if (destExistsAsFile) {
        return GroupSafety::DestNotDirectory;
    }
    (void)moving;  // self/child nesting is unsafe for both move and copy
    for (const auto& it : items) {
        if (it.isFolder && IsSameOrDescendant(it.path, destPath)) {
            return GroupSafety::MoveIntoSelf;
        }
    }
    return GroupSafety::Ok;
}

// ---------------------------------------------------------------------------
// #5  Trigger classification (Task 5a).
// The dedicated Ctrl+G "Group Selection" gate. Ctrl+G is a HARDWIRED group key
// with no place in the New Folder `shortcut` $options; it is gated only by its
// own on/off toggle. This function owns exactly one decision -- "is this the
// Group gesture?" -- and by construction never routes Ctrl+G to New Folder.
// New Folder keeps its own configured-shortcut decision elsewhere in the mod.
// ---------------------------------------------------------------------------
enum class TriggerKind {
    None = 0,          // not a recognised gesture here
    GroupSelection,    // the dedicated Ctrl+G group gesture
    NewFolder,         // reserved: New Folder is decided by the configured shortcut
};

// 'G' virtual-key code (VK 0x47). Mirrors the mod; named locally so the pure
// header needs no Win32.
static const int kVkGroupKey = 0x47;  // 'G'

// Decide whether a keystroke is the dedicated Group gesture.
//   ctrlDown           -- Ctrl is physically held.
//   otherModifierDown  -- any non-Ctrl modifier (Shift/Alt/Win) is held.
//   groupEnabled       -- the groupSelection on/off toggle.
// Only an exact Ctrl+G (no extra modifier) with the toggle ON is the Group
// gesture. Everything else is None; this function never returns NewFolder.
inline TriggerKind ClassifyTrigger(int vk, bool ctrlDown,
                                   bool otherModifierDown, bool groupEnabled) {
    if (vk == kVkGroupKey && ctrlDown && !otherModifierDown && groupEnabled) {
        return TriggerKind::GroupSelection;
    }
    return TriggerKind::None;
}

// ---------------------------------------------------------------------------
// #6  Group operation plan (Task 6a).
// Turns an already-resolved destination (from GroupCollisionResolve) plus a
// Move/Copy choice into the concrete operation the worker performs via
// IFileOperation. Copy is the DEFAULT. This is pure -- it decides WHAT to do;
// the Win32 side (Task 6b) mirrors it. `createDestFirst` tells the worker to
// create the destination folder before moving/copying items into it.
// ---------------------------------------------------------------------------
struct GroupOperationPlan {
    bool valid = false;          // false -> no-op (Cancel / empty dest)
    bool isMove = false;         // false = Copy (default), true = Move
    bool createDestFirst = false;// create the destination folder before the op
    std::wstring destPath;       // destination folder full path
};

inline GroupOperationPlan MakeGroupOperationPlan(const std::wstring& destPath,
                                                 bool isMove,
                                                 GroupResolveKind resolution) {
    GroupOperationPlan out;
    // A Cancel resolution or a missing destination is never a runnable plan.
    if (resolution == GroupResolveKind::Cancel || destPath.empty()) {
        out.valid = false;
        return out;
    }
    out.valid = true;
    out.isMove = isMove;                                   // Copy is the default caller-side
    out.destPath = destPath;
    out.createDestFirst = (resolution == GroupResolveKind::Create);
    return out;
}

// ---------------------------------------------------------------------------
// #7  Popup summary formatting (Task 8a).
// Pure text the Ctrl+G popup shows so the Win32 side can mirror it verbatim.
// No shell / Win32 dependency: every string below is asserted in
// tests\group_popup_test.cpp and must match exactly.
// ---------------------------------------------------------------------------

// Count line: 0 -> "No items selected"; 1 -> "1 item selected";
// N>1 -> "N items selected" (plural).
inline std::wstring FormatGroupCountLine(size_t count) {
    if (count == 0) {
        return L"No items selected";
    }
    std::wstring line = std::to_wstring(count);
    line += (count == 1) ? L" item selected" : L" items selected";
    return line;
}

// Item-name list: newline-joined names. When names.size() > maxShown, show the
// first maxShown names then a "+K more" overflow line (K = names.size()-maxShown).
// Empty vector -> empty string.
inline std::wstring FormatGroupItemList(const std::vector<std::wstring>& names,
                                        size_t maxShown) {
    if (names.empty()) {
        return std::wstring();
    }
    std::wstring out;
    size_t shown = (names.size() > maxShown) ? maxShown : names.size();
    for (size_t i = 0; i < shown; ++i) {
        if (i > 0) out += L'\n';
        out += names[i];
    }
    if (names.size() > maxShown) {
        size_t more = names.size() - maxShown;
        out += L'\n';
        out += L'+';
        out += std::to_wstring(more);
        out += L" more";
    }
    return out;
}

// Move/Copy label. Copy is the DEFAULT (isMove == false).
inline std::wstring FormatGroupOperationLabel(bool isMove) {
    return isMove ? L"Move" : L"Copy";
}

// Destination display line. Blank destination -> prompt to type a name.
inline std::wstring FormatGroupDestinationLine(const std::wstring& destPath) {
    if (destPath.empty()) {
        return L"Destination: (choose a folder name)";
    }
    return L"Destination: " + destPath;
}

// Last-target display line. Blank -> none remembered.
inline std::wstring FormatGroupLastTargetLine(const std::wstring& lastTarget) {
    if (lastTarget.empty()) {
        return L"Last target: none";
    }
    return L"Last target: " + lastTarget;
}

// Effective destination preview. A non-blank typed name joins under `parent`;
// a blank (or whitespace-only) typed name reuses the last target verbatim.
// Blank name + no last target -> empty (nothing to do).
inline std::wstring FormatGroupEffectiveDestination(const std::wstring& typedName,
                                                    const std::wstring& parent,
                                                    const std::wstring& lastTarget) {
    // Trim leading/trailing whitespace from the typed name.
    size_t b = 0, e = typedName.size();
    while (b < e && iswspace(typedName[b])) ++b;
    while (e > b && iswspace(typedName[e - 1])) --e;
    std::wstring trimmed = typedName.substr(b, e - b);

    if (!trimmed.empty()) {
        return JoinPath(parent, trimmed);
    }
    // Blank typed name -> reuse the last target (may be empty -> nothing to do).
    return lastTarget;
}

// ---------------------------------------------------------------------------
// #8  Ctrl+G destination precedence (Task 9) + open-after decision.
// The live WM_APP_GROUP handler mirrors these so a fixed destination, a typed
// name, and the remembered last target resolve in ONE documented order, and so
// the open-after setting cannot be silently ignored again.
// ---------------------------------------------------------------------------

// Trim leading/trailing whitespace (pure helper for the resolver below).
inline std::wstring TrimGroupWs(const std::wstring& s) {
    size_t b = 0, e = s.size();
    while (b < e && iswspace(s[b])) ++b;
    while (e > b && iswspace(s[e - 1])) --e;
    return s.substr(b, e - b);
}

struct GroupDestChoice {
    bool valid = false;            // false -> nothing to do (no name, no target)
    bool useNamedInParent = false; // true  -> run the folder-collision policy on
                                   //          (parentDir, folderName)
                                   // false -> use wholePath verbatim (fixed dest
                                   //          or last target -- an exact folder)
    std::wstring parentDir;        // when useNamedInParent
    std::wstring folderName;       // when useNamedInParent
    std::wstring wholePath;        // when !useNamedInParent
};

// Destination precedence for Ctrl+G:
//   1. fixedDest set    -> group straight into fixedDest (exact folder).
//   2. else typed name  -> <commonParent>\<name> (collision policy applies).
//   3. else last target -> reuse it verbatim (exact folder).
//   4. else             -> invalid (blank name, nothing remembered).
// fixedDest and lastTarget are trimmed of whitespace and one trailing separator.
inline GroupDestChoice ResolveGroupDestination(const std::wstring& fixedDest,
                                               const std::wstring& commonParent,
                                               const std::wstring& typedName,
                                               const std::wstring& lastTarget) {
    GroupDestChoice out;
    std::wstring fixed = TrimTrailingSep(TrimGroupWs(fixedDest));
    if (!fixed.empty()) {
        out.valid = true;
        out.useNamedInParent = false;
        out.wholePath = fixed;
        return out;
    }
    std::wstring name = TrimGroupWs(typedName);
    if (!name.empty()) {
        out.valid = true;
        out.useNamedInParent = true;
        out.parentDir = commonParent;
        out.folderName = name;
        return out;
    }
    std::wstring last = TrimTrailingSep(TrimGroupWs(lastTarget));
    if (!last.empty()) {
        out.valid = true;
        out.useNamedInParent = false;
        out.wholePath = last;
        return out;
    }
    return out;  // invalid
}

// "Open destination after grouping". A group always lands in exactly ONE
// destination (unlike a New Folder batch), so it opens whenever the setting is
// on AND the plan actually ran -- no createdCount gate.
inline bool ShouldOpenGroupDestination(bool openAfter, bool planValid) {
    return openAfter && planValid;
}

// ---------------------------------------------------------------------------
// #9  Honest partial Group-operation result accounting (fix thse isseue.md
//     P1, lines 13565-13620 / 14299-14320).
//
// PerformGroupOperation used to return a bare bool, and the Ctrl+G caller then
// logged the ORIGINAL selection size (sel.size()) on success -- even when the
// shell batch grouped FEWER items than were selected. Items disappear from the
// grouped count for four reasons:
//   * skipped        -- dropped by the skip-existing collision policy
//   * failedToQueue  -- SHCreateItemFromParsingName or Move/CopyItem failed
//   * aborted        -- the user/shell aborted PerformOperations() mid-flight
//   * performed==false -- the batch call itself failed (nothing landed)
//
// These pure helpers fold the raw per-item tallies into an HONEST grouped
// count plus a Complete / Partial / Failed / NothingToDo classification. The
// mod MUST mirror this (namespace groupresult in ctrlg-group.wh.cpp) and log
// the computed count -- never the raw selection size. Exercised by
// tests\group_result_test.cpp.
// ---------------------------------------------------------------------------
namespace groupresult {

// Raw tallies gathered by PerformGroupOperation. Invariant (the caller's
// contract): requested == queued + skipped + failedToQueue -- every considered
// item lands in exactly one bucket.
struct GroupOpOutcome {
    int requested = 0;       // non-virtual, non-empty items that were considered
    int skipped = 0;         // dropped by the skip-existing collision policy
    int failedToQueue = 0;   // SHCreateItemFromParsingName / Move|Copy call failed
    int queued = 0;          // operations actually handed to the shell batch
    bool performed = false;  // PerformOperations() returned SUCCEEDED(hr)
    bool aborted = false;    // GetAnyOperationsAborted() reported an abort
};

enum class GroupResultKind {
    NothingToDo,  // requested == 0 (nothing survived upstream filtering)
    Complete,     // every requested item was grouped; no skip/fail/abort
    Partial,      // some items grouped, but at least one skipped/failed/aborted
    Failed,       // nothing was grouped at all
};

// Honest, log-ready summary. `grouped` is the number that actually landed in
// the destination; `groupedExact` is false only when the batch aborted
// mid-flight (then `grouped` is an upper bound and the caller must phrase it as
// "up to N").
struct GroupResultSummary {
    GroupResultKind kind = GroupResultKind::NothingToDo;
    int grouped = 0;
    bool groupedExact = true;
    int requested = 0;
    int skipped = 0;
    int failed = 0;  // == failedToQueue
};

// The caller's bucketing contract. Not a hard requirement for ClassifyGroupResult
// (which trusts its inputs), but mirrored into the mod and asserted by tests so
// a future change that forgets to tally one bucket is caught.
inline bool GroupOutcomeConsistent(const GroupOpOutcome& o) {
    return o.requested == o.queued + o.skipped + o.failedToQueue;
}

inline GroupResultSummary ClassifyGroupResult(const GroupOpOutcome& o) {
    GroupResultSummary s;
    s.requested = o.requested;
    s.skipped = o.skipped;
    s.failed = o.failedToQueue;

    if (o.requested <= 0) {
        s.kind = GroupResultKind::NothingToDo;
        s.grouped = 0;
        s.groupedExact = true;
        return s;
    }
    // The shell batch never ran, or nothing was handed to it: nothing landed.
    if (!o.performed || o.queued <= 0) {
        s.kind = GroupResultKind::Failed;
        s.grouped = 0;
        s.groupedExact = true;
        return s;
    }
    // The batch started but was aborted: an UNKNOWN subset of the queued items
    // completed. Report queued as an upper bound and flag the count inexact --
    // never claim everything landed.
    if (o.aborted) {
        s.kind = GroupResultKind::Partial;
        s.grouped = o.queued;
        s.groupedExact = false;
        return s;
    }
    // Performed cleanly: every queued item completed.
    s.grouped = o.queued;
    s.groupedExact = true;
    const bool everything =
        (o.queued == o.requested && o.skipped == 0 && o.failedToQueue == 0);
    s.kind = everything ? GroupResultKind::Complete : GroupResultKind::Partial;
    return s;
}

}  // namespace groupresult
