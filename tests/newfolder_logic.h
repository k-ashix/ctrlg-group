// newfolder_logic.h
//
// PURE logic mirrored from ctrlg-group.wh.cpp so it can be unit
// tested with a plain g++ build (no Win32 runtime required). The mod is a
// Windhawk translation unit with no standalone main() and Windhawk/Win32
// headers, so it cannot be compiled here directly -- each pure decision is
// mirrored into this header and exercised by tests\*_test.cpp.
//
// Two decisions live here:
//   1. MakeUniqueFolderName  -- "New folder", "New folder (2)", ... numbering
//   2. TriggerDecide         -- the Ctrl+F+N / Ctrl+N / Ctrl+N+F state machine
//
// >>> RED STUB <<< : the bodies below are intentionally unimplemented so the
// tests fail first. They are filled in during the GREEN step.

#pragma once

#include <string>
#include <functional>
#include <cwchar>
#include <vector>

// Virtual-key constants (avoid pulling in <windows.h> for the g++ test build).
static const int kVkF = 'F';
static const int kVkN = 'N';
static const int kVkControl = 0x11;  // VK_CONTROL
static const int kVkShift   = 0x10;  // VK_SHIFT
static const int kVkAlt     = 0x12;  // VK_MENU (Alt)
static const int kVkLWin    = 0x5B;  // VK_LWIN
static const int kVkRWin    = 0x5C;  // VK_RWIN

// True if `vk` is a modifier key that can be HELD to keep a gesture alive
// (Ctrl / Shift / Alt / Win). Used to pick which key the count/template capture
// window must watch -- NOT always Ctrl. Left/right specific codes fold onto the
// generic modifier so a configured shortcut's held keys are recognised.
inline bool IsHoldModifierVk(int vk) {
    return vk == kVkControl || vk == 0xA2 || vk == 0xA3 ||   // Ctrl, L/R Ctrl
           vk == kVkShift   || vk == 0xA0 || vk == 0xA1 ||   // Shift, L/R Shift
           vk == kVkAlt     || vk == 0xA4 || vk == 0xA5 ||   // Alt,   L/R Alt
           vk == kVkLWin    || vk == kVkRWin;                // Win
}

// ---------------------------------------------------------------------------
// #3  Atomic shortcut snapshot.
//
// The configurable shortcut (ShortcutConfig) is READ on the low-level keyboard
// hook thread while LoadSettings WRITES it on the settings thread. A plain
// multi-field struct assignment is not atomic, so the hook can observe a torn
// half-updated shortcut. Packing the config into ONE 64-bit word lets the mod
// publish it with a single InterlockedExchange64 and read it with a single
// atomic load -- no torn read, and the LL hook stays lock-free.
//
// Layout (little end):
//   bits  0-7   triggerVk       (0-255)
//   bits  8-10  heldCount       (0-4)
//   bits 16-23  heldVks[0]
//   bits 24-31  heldVks[1]
//   bits 32-39  heldVks[2]
//   bits 40-47  heldVks[3]
struct ShortcutSnapshot {
    int triggerVk = 0;
    int heldVks[4] = {0, 0, 0, 0};
    int heldCount = 0;
};

inline long long PackShortcut(int triggerVk, const int* heldVks, int heldCount) {
    if (heldCount < 0) heldCount = 0;
    if (heldCount > 4) heldCount = 4;
    unsigned long long p = 0;
    p |= static_cast<unsigned long long>(triggerVk & 0xFF);
    p |= static_cast<unsigned long long>(heldCount & 0x7) << 8;
    for (int i = 0; i < 4; ++i) {
        int vk = (heldVks && i < heldCount) ? (heldVks[i] & 0xFF) : 0;
        p |= static_cast<unsigned long long>(vk) << (16 + i * 8);
    }
    return static_cast<long long>(p);
}

inline ShortcutSnapshot UnpackShortcut(long long packed) {
    unsigned long long p = static_cast<unsigned long long>(packed);
    ShortcutSnapshot s;
    s.triggerVk = static_cast<int>(p & 0xFF);
    s.heldCount = static_cast<int>((p >> 8) & 0x7);
    if (s.heldCount > 4) s.heldCount = 4;
    for (int i = 0; i < 4; ++i) {
        s.heldVks[i] = static_cast<int>((p >> (16 + i * 8)) & 0xFF);
    }
    return s;
}

// ---------------------------------------------------------------------------
// #4  Release-wait key set for the F2-rename fallback.
//
// Before injecting F2 (the rename accelerator) the mod waits for the gesture
// keys to be physically released, so the injected F2 is not combined with a
// still-held modifier. The old wait watched a FIXED set (Ctrl/Alt/Shift/N/F).
// Once the shortcut became user-configurable that set was wrong: e.g. for
// Alt+F2 the trigger key F2 itself was never waited for, so the injected F2
// could merge with the user's still-down F2.
//
// The correct wait set is the three standard modifiers PLUS every key in the
// configured shortcut (held keys + trigger), de-duplicated.
inline std::vector<int> ShortcutReleaseWaitVks(int triggerVk,
                                               const int* heldVks,
                                               int heldCount) {
    std::vector<int> out = { kVkControl, kVkAlt, kVkShift };
    auto add = [&out](int vk) {
        if (vk == 0) return;
        for (int e : out) {
            if (e == vk) return;
        }
        out.push_back(vk);
    };
    if (heldCount > 4) heldCount = 4;
    for (int i = 0; i < heldCount; ++i) {
        if (heldVks) add(heldVks[i]);
    }
    add(triggerVk);
    return out;
}

// ---------------------------------------------------------------------------
// #8-2  Which modifier keeps the count/template capture window alive.
//
// After a trigger fires, the count/template capture window stays open only
// while the gesture's HOLD MODIFIER remains physically down. The old code
// hard-coded that modifier to Ctrl (GetAsyncKeyState(VK_CONTROL)), so a
// non-Ctrl shortcut (Alt+F2, Win+N) reported "released" on the very first
// follow-up key and committed the default single folder -- batch counts and
// template letters were unreachable.
//
// The correct hold-modifier set is every MODIFIER key in the configured
// shortcut's held keys (Ctrl for Ctrl+F+N, Alt for Alt+F2, Win for Win+N).
// keyDown(vk) reports whether a key is currently physically down (the mod wraps
// GetAsyncKeyState). The window stays alive while ANY of those modifiers is
// held. A shortcut with NO held modifier (e.g. a bare F-key trigger) has no key
// to hold, so capture is treated as always-held for its duration (it ends by
// Enter / Escape / timeout instead).
inline std::vector<int> GestureHoldModifierVks(const int* heldVks,
                                               int heldCount) {
    std::vector<int> out;
    if (heldCount > 4) heldCount = 4;
    for (int i = 0; i < heldCount; ++i) {
        int vk = heldVks ? heldVks[i] : 0;
        if (IsHoldModifierVk(vk)) {
            bool dup = false;
            for (int e : out) { if (e == vk) { dup = true; break; } }
            if (!dup) out.push_back(vk);
        }
    }
    return out;
}

// True if the capture window should stay open: EVERY configured hold-modifier
// is still down. The gesture spec is "all preceding keys remain held", so for
// Ctrl+Shift+N both Ctrl AND Shift must stay down -- releasing EITHER ends the
// window. (An OR check here was a bug: it kept the window alive when only one
// of several modifiers was still held.) With no hold-modifier configured,
// returns true (the window is not modifier-gated; it ends by Enter / Escape /
// timeout).
inline bool IsGestureModifierHeld(const int* heldVks, int heldCount,
                                  const std::function<bool(int)>& keyDown) {
    std::vector<int> mods = GestureHoldModifierVks(heldVks, heldCount);
    if (mods.empty()) {
        return true;   // no modifier to hold -> window not modifier-gated
    }
    for (int vk : mods) {
        if (!keyDown || !keyDown(vk)) {
            return false;   // this configured modifier was released -> end
        }
    }
    return true;   // all configured hold-modifiers still down
}

// ---------------------------------------------------------------------------
// #B  Capture-window key routing: template letter vs count-commit vs cancel.
//
// BUG (Ctrl+Shift+N + template letter): after the trigger fires, the capture
// window is open. To type a lowercase template letter (e.g. 'i') the user must
// RELEASE Shift -- letters are lowercase. But the window was gated on EVERY
// configured hold-modifier staying down (IsGestureModifierHeld: Ctrl AND Shift
// for Ctrl+Shift+N). So releasing Shift closed the window and committed a
// single default folder BEFORE the template letter arrived: first attempt made
// nothing/one folder, a second attempt (keys held differently) fired the
// template and created all its folders. The template letter must NOT be held
// hostage to a still-pressed modifier.
//
// The correct rule: once the capture window is open, a letter BOUND to a
// template routes to that template REGARDLESS of which hold-modifiers are still
// physically down. Modifier release only ever commits an accumulated COUNT --
// it must never swallow a pending, bound template letter. Digits still commit
// the count; a bound letter fires its template; an unbound letter commits the
// (default single) folder as before.
enum class CaptureKey {
    Digit,           // 0-9
    BoundLetter,     // a-z that IS bound to a template
    UnboundLetter,   // a-z that is NOT bound to a template
    Enter,           // commit count
    Escape,          // cancel: create nothing
    ModifierRelease, // a configured hold-modifier went up (no other key)
    Other,           // any other key
};

enum class CaptureRoute {
    Continue,       // still capturing (keep the window open)
    FireTemplate,   // route to the template action; create NO count folders
    CommitCount,    // commit the accumulated count (>=1) as folders
    Cancel,         // create nothing
};

// Decide what a key does inside the count/template capture window.
//
// `sawDigit`      : has at least one count digit been typed already?
// `modifierHeld`  : are ALL configured hold-modifiers still physically down?
//                   (false once the user releases e.g. Shift to type a letter.)
//
// Template letters are the crux of the bug: a BoundLetter fires its template
// whether or not `modifierHeld` is still true -- the release of a modifier can
// NEVER pre-empt a bound template letter. Only a Digit-then-release or a plain
// ModifierRelease commits the count.
inline CaptureRoute CaptureLetterDecide(CaptureKey key, bool sawDigit,
                                        bool modifierHeld) {
    switch (key) {
        case CaptureKey::BoundLetter:
            // Fires the template no matter the modifier state. THIS is the fix:
            // releasing Shift to type the lowercase letter must not have already
            // committed a folder and closed the window.
            return CaptureRoute::FireTemplate;

        case CaptureKey::Escape:
            return CaptureRoute::Cancel;

        case CaptureKey::Digit:
            // A digit keeps (or starts) count entry. If a modifier was released
            // on this very event, the count commits; otherwise keep capturing.
            return modifierHeld ? CaptureRoute::Continue
                                : CaptureRoute::CommitCount;

        case CaptureKey::Enter:
            return CaptureRoute::CommitCount;

        case CaptureKey::ModifierRelease:
            // Releasing a hold-modifier finishes count entry: commit typed
            // digits, or the default single folder when none were typed.
            return CaptureRoute::CommitCount;

        case CaptureKey::UnboundLetter:
        case CaptureKey::Other:
        default:
            // Not a template, not a count key: commit what we have (default
            // single folder). Same behaviour whether or not a modifier is held.
            (void)sawDigit;
            return CaptureRoute::CommitCount;
    }
}

// ---------------------------------------------------------------------------
// #B2  Lowercase normalization + capture-window gating for template letters.
//
// Requirement: a template key must
//   (a) normalize to its LOWERCASE letter even when Shift is held (typing an
//       uppercase 'I' still resolves the template bound to 'i'), and
//   (b) fire ONLY while the capture window is open (right after Ctrl+Shift+N).
//       During normal typing anywhere else the SAME letters must NEVER fire a
//       template -- no accidental folder creation while editing code.
//
// A keyboard VK code for a letter is Shift-INDEPENDENT (both 'i' and Shift+'i'
// arrive as VK 'I' == 0x49). Folding VK->lowercase is therefore inherently
// case-normalizing; this helper makes that explicit and returns 0 for anything
// that is not an A-Z letter.
inline wchar_t NormalizeTemplateLetter(int vk) {
    if (vk >= 'A' && vk <= 'Z') {
        return static_cast<wchar_t>(vk - 'A' + L'a');
    }
    return 0;  // not a letter -> never a template key
}

// Bit-mask membership test mirrored from the mod's IsTemplateKeyBound: bit
// (c-'a') of `mask` is set iff lowercase letter `c` is bound to a template.
// Case-folds first, so 'I' and 'i' test the same bit.
inline bool TemplateKeyBoundInMask(unsigned long mask, wchar_t typed) {
    wchar_t c = typed;
    if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
    if (c < L'a' || c > L'z') return false;
    return (mask & (1UL << (c - L'a'))) != 0;
}

// The single decision the hook must make for a letter keydown: does it fire a
// template?  It fires ONLY when ALL of these hold:
//   - the capture window is open (captureActive) -- so nothing fires during
//     normal typing, and
//   - the VK normalizes to an A-Z letter (Shift state irrelevant), and
//   - that lowercase letter is bound to a template in the mask.
// Returns the lowercase template key to fire, or 0 for "not a template action".
inline wchar_t ResolveTemplateLetterKey(bool captureActive, int vk,
                                        unsigned long templateKeyMask) {
    if (!captureActive) {
        return 0;   // (b) never fire outside the capture window
    }
    wchar_t c = NormalizeTemplateLetter(vk);   // (a) lowercase, Shift-independent
    if (c == 0) {
        return 0;
    }
    if (!TemplateKeyBoundInMask(templateKeyMask, c)) {
        return 0;   // unbound letter -> not a template
    }
    return c;
}

// Chord window: max gap between the two chord keys (and the replay timeout).
static const unsigned long long kChordTimeoutMs = 1200;

// ---------------------------------------------------------------------------
// Pure path join (mirror of the mod's JoinPath).
// ---------------------------------------------------------------------------
inline std::wstring JoinPath(const std::wstring& a, const std::wstring& b) {
    if (a.empty()) {
        return b;
    }

    wchar_t back = a[a.size() - 1];
    if (back == L'\\' || back == L'/') {
        return a + b;
    }

    return a + L'\\' + b;
}

// ---------------------------------------------------------------------------
// Create-collision decision (mirror of the folder-create loop in the mod).
// #5: a check-then-create (PathFileExistsW then CreateDirectoryW) is not
// atomic -- another process can create the same directory in between, so
// CreateDirectoryW fails with ERROR_ALREADY_EXISTS. That is NOT a real failure:
// the create loop should treat it as a collision and advance to the next
// candidate name. Every other error is a genuine failure. ERROR_ALREADY_EXISTS
// is 183 on Windows; define it locally so this header needs no <windows.h>.
#ifndef ERROR_ALREADY_EXISTS
#define ERROR_ALREADY_EXISTS 183
#endif
inline bool ShouldRetryOnCollision(unsigned long createError) {
    return createError == ERROR_ALREADY_EXISTS;
}

// ---------------------------------------------------------------------------
// Unique folder-name generator.
// exists(fullPath) -> true if a file/folder already exists at that path.
// Returns the first free path: "<dir>\New folder", then "New folder (2)", ...
// ---------------------------------------------------------------------------
using NameExists = std::function<bool(const std::wstring&)>;

inline std::wstring MakeUniqueFolderName(const std::wstring& dir,
                                         const NameExists& exists) {
    std::wstring base = JoinPath(dir, L"New folder");
    if (!exists(base)) {
        return base;
    }

    for (int i = 2; i < 10000; ++i) {
        wchar_t buf[64] = {0};
        std::swprintf(buf, 64, L"New folder (%d)", i);

        std::wstring candidate = JoinPath(dir, buf);
        if (!exists(candidate)) {
            return candidate;
        }
    }

    return base;
}

// ---------------------------------------------------------------------------
// Trigger / chord state machine.
// ---------------------------------------------------------------------------
enum class TriggerMode {
    CtrlFN     = 0,  // hold Ctrl, tap F, tap N        (default; preserves search)
    CtrlN      = 1,  // Ctrl+N                         (swallows new-window)
    CtrlNF     = 2,  // hold Ctrl, tap N, tap F        (swallows new-window)
    CtrlShiftN = 3,  // Ctrl+Shift+N                   (overlays native New folder)
};

enum class TriggerAction {
    None = 0,
    CreateFolder = 1,
    ReplayCtrlF = 2,  // re-inject Ctrl+F so Explorer search still works
};

struct TriggerState {
    bool pending = false;                    // waiting for the 2nd chord key
    int pendingVk = 0;                       // key swallowed while pending (F or N)
    unsigned long long pendingSinceMs = 0;   // when pending began
    int swallowUpVk = 0;                     // vk whose next keyup must be swallowed
};

struct KeyEvent {
    int vk = 0;                     // 'F', 'N', kVkControl, ...
    bool isDown = true;             // keydown vs keyup
    bool isTimeout = false;         // synthetic chord-timeout tick
    bool ctrlDown = false;          // modifier snapshot at event time
    bool altDown = false;
    bool shiftDown = false;
    bool explorerForeground = false;
    bool injected = false;          // event was synthesized by us
    unsigned long long nowMs = 0;
};

struct TriggerResult {
    bool swallow = false;                        // true => hook returns 1 (suppress)
    TriggerAction action = TriggerAction::None;
    TriggerState next;
};

inline TriggerResult TriggerDecide(TriggerMode mode,
                                   const TriggerState& st,
                                   const KeyEvent& ev) {
    TriggerResult r;
    r.next = st;

    // 1. Timeout tick: resolve a pending chord that expired.
    if (ev.isTimeout) {
        if (st.pending && (ev.nowMs - st.pendingSinceMs) > kChordTimeoutMs) {
            r.next.pending = false;
            r.next.pendingVk = 0;
            // Only CtrlFN swallowed a real Ctrl+F (search) it now owes back.
            if (mode == TriggerMode::CtrlFN && st.pendingVk == kVkF) {
                r.action = TriggerAction::ReplayCtrlF;
            }
        }
        return r;
    }

    // 2. Never react to our own injected input (avoids a feedback loop).
    if (ev.injected) {
        return r;
    }

    // 3. Swallow the key-up of a key whose key-down we swallowed (no stray char).
    if (!ev.isDown) {
        if (st.swallowUpVk != 0 && ev.vk == st.swallowUpVk) {
            r.swallow = true;
            r.next.swallowUpVk = 0;
        }
        return r;
    }

    // --- key DOWN from here ---
    const bool cleanCtrl = ev.ctrlDown && !ev.altDown && !ev.shiftDown;

    // 4. Complete (or abandon) a pending chord.
    if (st.pending) {
        const bool expired = (ev.nowMs - st.pendingSinceMs) > kChordTimeoutMs;
        const int wantSecond = (mode == TriggerMode::CtrlFN) ? kVkN
                             : (mode == TriggerMode::CtrlNF) ? kVkF
                             : 0;

        if (!expired && wantSecond != 0 && ev.vk == wantSecond && ev.ctrlDown) {
            r.swallow = true;
            r.action = TriggerAction::CreateFolder;
            r.next.pending = false;
            r.next.pendingVk = 0;
            r.next.swallowUpVk = ev.vk;
            return r;
        }

        // Auto-repeat of the first chord key: keep swallowing, stay pending.
        if (!expired && ev.vk == st.pendingVk && ev.ctrlDown) {
            r.swallow = true;
            return r;
        }

        // Chord broken or expired: abandon, replaying Ctrl+F only for CtrlFN.
        r.next.pending = false;
        r.next.pendingVk = 0;
        if (mode == TriggerMode::CtrlFN && st.pendingVk == kVkF) {
            r.action = TriggerAction::ReplayCtrlF;
        }
        return r;  // let the current key through
    }

    // 5a. Ctrl+Shift+N: single-press, fires immediately. This overlays the
    // shortcut Windows itself already binds to "New folder", so we just take
    // it over to add naming + batch. It needs Ctrl AND Shift held (Alt clear),
    // which is a DIFFERENT modifier gate than cleanCtrl -- so it is handled
    // before the cleanCtrl passthrough guard below. There is nothing to replay:
    // we are enhancing the native gesture, not suppressing a competing one.
    if (mode == TriggerMode::CtrlShiftN) {
        const bool ctrlShift = ev.ctrlDown && ev.shiftDown && !ev.altDown;
        if (ctrlShift && ev.explorerForeground && ev.vk == kVkN) {
            r.swallow = true;
            r.action = TriggerAction::CreateFolder;
            r.next.swallowUpVk = kVkN;
        }
        return r;
    }

    // 5. Not pending: react to the FIRST trigger key for this mode.
    if (!cleanCtrl || !ev.explorerForeground) {
        return r;  // passthrough
    }

    switch (mode) {
        case TriggerMode::CtrlN:
            if (ev.vk == kVkN) {
                r.swallow = true;
                r.action = TriggerAction::CreateFolder;
                r.next.swallowUpVk = kVkN;
            }
            break;

        case TriggerMode::CtrlFN:
            if (ev.vk == kVkF) {
                r.swallow = true;  // suppress search until we learn if N follows
                r.next.pending = true;
                r.next.pendingVk = kVkF;
                r.next.pendingSinceMs = ev.nowMs;
                r.next.swallowUpVk = kVkF;
            }
            break;

        case TriggerMode::CtrlNF:
            if (ev.vk == kVkN) {
                r.swallow = true;  // suppress new-window
                r.next.pending = true;
                r.next.pendingVk = kVkN;
                r.next.pendingSinceMs = ev.nowMs;
                r.next.swallowUpVk = kVkN;
            }
            break;
    }

    return r;
}

// ===========================================================================
// #C  Config-driven trigger state machine (mirror of LowLevelKeybdProc).
//
// The MOD is config-driven: a shortcut is a ShortcutSnapshot {triggerVk,
// heldVks[], heldCount}, NOT the fixed TriggerMode enum above. Every supported
// gesture flows through ONE path:
//
//   Single-press (Ctrl+N, Ctrl+Q, Ctrl+G, Ctrl+Shift+N)
//     hold every heldVk, tap triggerVk -> FIRE immediately (open capture
//     window for batch/template; leaving it makes one folder).
//
//   Two-key chord (Ctrl+F+N: hold Ctrl, tap F, tap N; Ctrl+N+F: tap N, tap F)
//     the ONE non-modifier held key is the "middle" key. It is SWALLOWED on its
//     keydown (so Ctrl+N never opens a new window, Ctrl+F never opens search),
//     the gesture goes pending, and completes only when triggerVk follows while
//     the modifiers stay held. A broken/expired chord replays Ctrl+F only for
//     the Ctrl+F+N case (that is the search key it owed back).
//
// After a fire, the capture window (CountDecide / CaptureLetterDecide, above)
// takes over so the user can freely type a batch count OR a template letter, or
// leave it to create a single folder -- identical for ALL shortcut modes.

// The single non-modifier held key of a chord (F for Ctrl+F+N, N for Ctrl+N+F).
// 0 when the shortcut is single-press (Ctrl+N/Q/G) or a pure modifier+trigger
// combo (Ctrl+Shift+N) -- only a non-zero value uses the pending-chord path.
inline int ChordIntermediateVkFromSnapshot(const ShortcutSnapshot& s) {
    int heldCount = s.heldCount;
    if (heldCount > 4) heldCount = 4;
    int found = 0;
    for (int i = 0; i < heldCount; ++i) {
        if (!IsHoldModifierVk(s.heldVks[i])) {
            if (found != 0) return 0;  // >1 non-modifier: not a simple chord
            found = s.heldVks[i];
        }
    }
    return found;
}

// Are ALL configured hold-modifiers of the shortcut currently down?
// `keyDown(vk)` wraps GetAsyncKeyState. Mirrors IsGestureModifierHeldNow: for a
// multi-modifier shortcut (Ctrl+Shift+N) EVERY modifier must be held.
inline bool SnapshotModifiersHeld(const ShortcutSnapshot& s,
                                  const std::function<bool(int)>& keyDown) {
    int heldCount = s.heldCount;
    if (heldCount > 4) heldCount = 4;
    for (int i = 0; i < heldCount; ++i) {
        int vk = s.heldVks[i];
        if (!IsHoldModifierVk(vk)) continue;
        if (!keyDown || !keyDown(vk)) return false;
    }
    return true;
}

// The config-driven decision the hook makes for one key event. Mirrors the
// LowLevelKeybdProc trigger path (NOT the count/capture window, which is
// CountDecide / CaptureLetterDecide). `keyDown` reports whether a vk is
// physically down now (for the "all held keys still down" check).
inline TriggerResult SnapshotTriggerDecide(const ShortcutSnapshot& sc,
                                           const TriggerState& st,
                                           const KeyEvent& ev,
                                           const std::function<bool(int)>& keyDown) {
    TriggerResult r;
    r.next = st;

    const int chordVk = ChordIntermediateVkFromSnapshot(sc);

    // Timeout tick: expire a pending chord; replay Ctrl+F only if that was the
    // swallowed middle key (Ctrl+F+N owes search back).
    if (ev.isTimeout) {
        if (st.pending && (ev.nowMs - st.pendingSinceMs) > kChordTimeoutMs) {
            r.next.pending = false;
            r.next.pendingVk = 0;
            if (st.pendingVk == kVkF) r.action = TriggerAction::ReplayCtrlF;
        }
        return r;
    }

    if (ev.injected) return r;   // never react to our own injected input

    // Swallow the key-up of a key whose key-down we swallowed.
    if (!ev.isDown) {
        if (st.swallowUpVk != 0 && ev.vk == st.swallowUpVk) {
            r.swallow = true;
            r.next.swallowUpVk = 0;
        }
        return r;
    }

    // --- key DOWN ---
    // Resolve a pending chord.
    if (st.pending) {
        const bool expired = (ev.nowMs - st.pendingSinceMs) > kChordTimeoutMs;
        if (!expired && ev.vk == sc.triggerVk && SnapshotModifiersHeld(sc, keyDown)) {
            r.swallow = true;
            r.action = TriggerAction::CreateFolder;
            r.next.pending = false;
            r.next.pendingVk = 0;
            r.next.swallowUpVk = ev.vk;
            return r;
        }
        // Auto-repeat of the held middle key while modifiers stay down.
        if (!expired && ev.vk == st.pendingVk && SnapshotModifiersHeld(sc, keyDown)) {
            r.swallow = true;
            return r;
        }
        // Chord broken/expired: abandon, replaying Ctrl+F only for the F case.
        r.next.pending = false;
        r.next.pendingVk = 0;
        if (st.pendingVk == kVkF) r.action = TriggerAction::ReplayCtrlF;
        // fall through: let this key be evaluated as a possible fresh start
    }

    // Start a pending chord: the middle key pressed with modifiers held and
    // Explorer in front. Swallow it now (no new window / no search leak).
    if (chordVk != 0 && ev.explorerForeground && ev.vk == chordVk &&
        SnapshotModifiersHeld(sc, keyDown)) {
        r.swallow = true;
        r.next.pending = true;
        r.next.pendingVk = chordVk;
        r.next.pendingSinceMs = ev.nowMs;
        r.next.swallowUpVk = chordVk;
        return r;
    }

    // Single-press fire: trigger key with EVERY held key still down, Explorer
    // in front. Covers Ctrl+N, Ctrl+Q, Ctrl+G, Ctrl+Shift+N.
    if (ev.explorerForeground && ev.vk == sc.triggerVk &&
        SnapshotModifiersHeld(sc, keyDown)) {
        r.swallow = true;
        r.action = TriggerAction::CreateFolder;
        r.next.swallowUpVk = ev.vk;
        return r;
    }

    return r;  // passthrough
}

// ===========================================================================
// Multi-folder creation (v0.2.0) -- pure logic, mirrored into the mod.
// ===========================================================================
#include <vector>

// Folder-count caps. Default per-run cap is 25; the hard cap is 100 and cannot
// be exceeded even if the user configures maxFolders higher.
static const int kFolderDefaultMax = 25;
static const int kFolderHardCap    = 100;

// Default state of the "Multiple folders in one go" toggle. It ships ON so
// batch counts and folder templates work out of the box (a template key is only
// captured while the count window is open, which the toggle controls).
static const bool kMultiFolderDefaultOn = true;

// Default state of the "Name folders with a popup box" toggle (useDialog) on
// the Ctrl+F+N New-Folder path. It ships OFF -> the native inline rename box is
// used by default; the user can turn the themed popup ON. BOTH modes stay
// available. (Ctrl+G Group Selection has its own always-on popup, unrelated to
// this toggle.) Mirrors g_useDialog's shipped default; pinned by
// tests/popup_default_test.cpp.
static const bool kUsePopupDefaultOn = false;

// Clamp the user's maxFolders setting into the allowed [1, kFolderHardCap] range.
inline int ClampFolderMax(int maxSetting) {
    if (maxSetting < 1) {
        return 1;
    }
    if (maxSetting > kFolderHardCap) {
        return kFolderHardCap;
    }
    return maxSetting;
}

// Decide whether a trigger opens the key-capture window (which listens for
// count digits AND template letters) or goes straight to a single folder.
//
// BUG FIX: templates were unreachable when "Multiple folders in one go" was OFF,
// because the capture window -- the ONLY place a template letter is caught --
// only opened when multiEnabled was true. The window must open when EITHER
// multi-folder is on OR at least one template is configured, so a chosen
// template key always works regardless of the multi-folder toggle.
inline bool ShouldOpenCaptureWindow(bool multiEnabled, bool hasTemplates) {
    return multiEnabled || hasTemplates;
}

// Breakdown of a template run for clear logging. `requested` is how many folder
// names the template lists; `planned` is how many survived the collision policy
// (Skip drops names that already exist, so planned < requested); `created` is
// how many were actually made on disk. From those:
//   skippedExisting = requested - planned  (dropped because they already exist)
//   failed          = planned  - created   (a create call failed, e.g. denied)
struct TemplateRunSummary {
    int requested = 0;
    int planned = 0;
    int created = 0;
    int skippedExisting = 0;
    int failed = 0;
};

inline TemplateRunSummary SummarizeTemplateRun(int requested, int planned,
                                               int created) {
    TemplateRunSummary s;
    s.requested = requested;
    s.planned = planned;
    s.created = created;
    s.skippedExisting = (requested > planned) ? (requested - planned) : 0;
    s.failed = (planned > created) ? (planned - created) : 0;
    return s;
}

// Resolve how many folders to create for one trigger:
//   multiEnabled == false        -> always 1 (feature off)
//   requested   <= 0 (no digits) -> 1
//   otherwise                    -> min(requested, ClampFolderMax(maxSetting))
inline int ResolveFolderCount(int requested, bool multiEnabled, int maxSetting) {
    if (!multiEnabled) {
        return 1;
    }

    int effMax = ClampFolderMax(maxSetting);
    if (requested < 1) {
        return 1;
    }
    if (requested > effMax) {
        return effMax;
    }
    return requested;
}

// Generate `count` unique folder paths, reserving each within the batch and
// skipping any that already exist on disk. Uses the Explorer-native sequence
// "New folder", "New folder (2)", "New folder (3)", ...
//
// PERFORMANCE: names are only ever emitted in strictly increasing numeric
// order, and an index we pass is never revisited. So instead of restarting the
// "(2), (3), ..." scan for every slot (the old O(count * (disk + batch))
// behaviour -- thousands of stats at the 100-folder cap in a busy directory),
// we keep a monotonic `nextIndex` cursor. Each existence probe advances the
// cursor, giving O(count + preexisting) probes total. Because every candidate
// index is fresh and unpicked, the batch is inherently collision-free -- no
// per-slot rescan of `out` is needed. Output is byte-identical to the old
// generator (multi_name_test.cpp pins the exact names).
inline std::vector<std::wstring> MakeUniqueFolderNames(
        const std::wstring& dir, int count, const NameExists& exists,
        bool autoResolve = true) {
    std::vector<std::wstring> out;
    if (count < 1) {
        return out;
    }

    // Monotonic cursor over the numeric suffix. Index 1 is the bare
    // "New folder"; 2+ are "New folder (N)". It only ever moves forward.
    int nextIndex = 1;

    auto pathForIndex = [&](int i) -> std::wstring {
        if (i <= 1) {
            return JoinPath(dir, L"New folder");
        }
        wchar_t buf[64] = {0};
        std::swprintf(buf, 64, L"New folder (%d)", i);
        return JoinPath(dir, buf);
    };

    for (int k = 0; k < count; ++k) {
        if (!autoResolve) {
            // Numbering disabled: only the bare "New folder" is ever a
            // candidate. Probe it once; free -> take it (and reserve it so a
            // second slot in the same batch is skipped), taken -> skip slot.
            std::wstring base = pathForIndex(1);
            if (nextIndex <= 1 && !exists(base)) {
                out.push_back(base);
            }
            nextIndex = 2;  // "New folder" can be claimed at most once
            continue;
        }

        // Advance the cursor to the next free index. Each iteration is a single
        // probe; the cursor never rewinds, so total probes stay linear.
        bool placed = false;
        for (; nextIndex < 100000; ++nextIndex) {
            std::wstring candidate = pathForIndex(nextIndex);
            if (!exists(candidate)) {
                out.push_back(candidate);
                ++nextIndex;   // reserve it; next slot starts past it
                placed = true;
                break;
            }
        }
        if (!placed) {
            break;
        }
    }

    return out;
}

// ---------------------------------------------------------------------------
// Batch naming pattern (mirror of the namingPattern setting in the mod).
// Chooses ONLY how the batch NUMBER is formatted; the base name and the
// skip-existing collision resolution are identical for every pattern.
//   Space  : "<base> 1"      (default; unchanged legacy behaviour)
//   Dash2  : "<base>-01"      (dash separator, 2-digit zero-pad)
//   Dash3  : "<base>-001"     (dash separator, 3-digit zero-pad)
//   Paren2 : "<base> (01)"    (space + parenthesised 2-digit zero-pad)
// Zero-pad only pads; a number wider than the pad width is written in full
// (e.g. Dash2 of 100 -> "-100"), so the sequence never collides by truncation.
// ---------------------------------------------------------------------------
enum class NamePattern {
    Space  = 0,   // "<base> N"
    Dash2  = 1,   // "<base>-0N"
    Dash3  = 2,   // "<base>-00N"
    Paren2 = 3,   // "<base> (0N)"
    Under2 = 4,   // "<base>_0N"
    Under3 = 5,   // "<base>_00N"
};

// Left-pad a positive integer to at least `width` digits (never truncates).
inline std::wstring PadNumber(int n, int width) {
    std::wstring s = std::to_wstring(n);
    while (static_cast<int>(s.size()) < width) {
        s.insert(s.begin(), L'0');
    }
    return s;
}

// Render one batch member name (without the directory) for a given index.
inline std::wstring FormatBatchName(const std::wstring& base, int index,
                                    NamePattern pattern) {
    switch (pattern) {
        case NamePattern::Dash2:
            return base + L"-" + PadNumber(index, 2);
        case NamePattern::Dash3:
            return base + L"-" + PadNumber(index, 3);
        case NamePattern::Paren2:
            return base + L" (" + PadNumber(index, 2) + L")";
        case NamePattern::Under2:
            return base + L"_" + PadNumber(index, 2);
        case NamePattern::Under3:
            return base + L"_" + PadNumber(index, 3);
        case NamePattern::Space:
        default:
            return base + L" " + std::to_wstring(index);
    }
}

// ---------------------------------------------------------------------------
// Named batch generator (used by the auto-resolve-OFF path once the user has
// typed a base name). Produces `count` folders whose number is formatted by
// `pattern` (default Space -> "<base> 1", "<base> 2", ...), skipping any that
// already exist on disk. A monotonic cursor advances past existing numbers, so
// the batch never clobbers and still yields `count` new names. Empty base or
// count < 1 -> no names (caller falls back to a single default folder).
// ---------------------------------------------------------------------------
inline std::vector<std::wstring> MakeNamedFolderNames(
        const std::wstring& dir, const std::wstring& base, int count,
        const NameExists& exists, NamePattern pattern = NamePattern::Space) {
    std::vector<std::wstring> out;
    if (count < 1 || base.empty()) {
        return out;
    }

    auto pathForIndex = [&](int i) -> std::wstring {
        return JoinPath(dir, FormatBatchName(base, i, pattern));
    };

    int nextIndex = 1;
    for (int k = 0; k < count; ++k) {
        bool placed = false;
        for (; nextIndex < 1000000; ++nextIndex) {
            std::wstring candidate = pathForIndex(nextIndex);
            if (!exists(candidate)) {
                out.push_back(candidate);
                ++nextIndex;  // reserve it; the next slot starts past it
                placed = true;
                break;
            }
        }
        if (!placed) {
            break;
        }
    }

    return out;
}

// ---------------------------------------------------------------------------
// Count-entry state machine (runs after a trigger fires while multi is on).
// ---------------------------------------------------------------------------
enum class CountKey {
    Digit,    // 0-9 (value passed separately)
    Enter,    // commit
    Escape,   // cancel (create nothing)
    Other,    // any other key: commit accumulated; swallowed while Ctrl held
              // (never leak Ctrl+key), passes through only after Ctrl release
    Timeout,  // count-entry window elapsed: commit
};

enum class CountOutcome {
    Continue,  // still capturing
    Commit,    // create `count` folders
    Cancel,    // create none
};

struct CountState {
    bool active = false;    // a count-entry window is open
    int value = 0;          // digits accumulated so far
    bool sawDigit = false;  // at least one digit typed
};

struct CountResult {
    bool swallow = false;              // hide this key from Explorer
    CountOutcome outcome = CountOutcome::Continue;
    int count = 0;                     // valid when outcome == Commit (clamped)
    CountState next;
};

// Decide what a key does during count entry. `digit` is 0-9 only when key==Digit.
// `ctrlHeld` must be true for the whole gesture: the count window only lives
// while Ctrl stays down. The moment Ctrl is released, capture ends -- committing
// the typed digits, or cancelling if none were typed yet.
inline CountResult CountDecide(CountState st, CountKey key, int digit,
                               bool multiEnabled, int maxSetting,
                               bool ctrlHeld = true) {
    CountResult r;
    r.next = st;

    // Helper: end the capture window and reset accumulation.
    auto endCapture = [](CountResult& res) {
        res.next.active = false;
        res.next.value = 0;
        res.next.sawDigit = false;
    };

    // Ctrl-held-throughout: releasing Ctrl ends entry immediately. A synthetic
    // Timeout tick is exempt (it isn't a key and can arrive after release).
    if (!ctrlHeld && key != CountKey::Timeout) {
        // A DIGIT is explicit intent and must be honoured even if Ctrl reads
        // released on this same event: the low-level hook can see the digit
        // keydown after GetAsyncKeyState(VK_CONTROL) already flipped to up
        // (observed as "sawDigit=0 -> requested=0 -> count=1"). Fold the digit
        // into the accumulator, THEN commit -- so "Ctrl+F+N then 9" makes 9.
        int accumulated = st.sawDigit ? st.value : 0;
        // Only the FIRST digit of a fresh capture can arrive on the same event
        // Ctrl reads released (the observed race). Once a digit was already
        // captured (st.sawDigit), the release event just commits what's there
        // -- a stray digit on release is not appended.
        if (key == CountKey::Digit && !st.sawDigit) {
            int v = digit;
            if (v > kFolderHardCap) {
                v = kFolderHardCap;
            }
            accumulated = v;
            r.swallow = true;  // this digit is ours; don't leak it to Explorer
        } else {
            r.swallow = false;  // nothing to hide: the releasing key isn't ours
        }
        // Releasing Ctrl finishes the gesture: commit the typed digits (incl.
        // the digit on this very event), or the default single folder when
        // none were typed. Only Esc cancels now.
        r.outcome = CountOutcome::Commit;
        r.count = ResolveFolderCount(accumulated, multiEnabled, maxSetting);
        endCapture(r);
        return r;
    }

    switch (key) {
        case CountKey::Digit: {
            int v = st.value * 10 + digit;
            if (v > kFolderHardCap) {
                v = kFolderHardCap;  // never accumulate past the hard cap
            }
            r.next.value = v;
            r.next.sawDigit = true;
            r.swallow = true;
            r.outcome = CountOutcome::Continue;
            return r;
        }

        case CountKey::Enter:
        case CountKey::Timeout: {
            // Enter is a real key we hide; a timeout is synthetic (nothing to hide).
            r.swallow = (key == CountKey::Enter);
            r.outcome = CountOutcome::Commit;
            r.count = ResolveFolderCount(st.sawDigit ? st.value : 0,
                                         multiEnabled, maxSetting);
            endCapture(r);
            return r;
        }

        case CountKey::Escape: {
            r.swallow = true;
            r.outcome = CountOutcome::Cancel;
            endCapture(r);
            return r;
        }

        case CountKey::Other:
        default: {
            // A non-count key while Ctrl is STILL held (the ctrlHeld==false
            // release path returned above). Commit what we have (or 1) but
            // SWALLOW the key: Ctrl is down, so letting it reach Explorer would
            // fire a Ctrl+<key> shortcut -- Ctrl+N (new window) / Ctrl+F
            // (search) -- which is the "a new window opens during the gesture"
            // bug. Keys only reach Explorer AFTER Ctrl is released (handled by
            // the ctrlHeld==false branch above, which passes through safely).
            r.swallow = true;
            r.outcome = CountOutcome::Commit;
            r.count = ResolveFolderCount(st.sawDigit ? st.value : 0,
                                         multiEnabled, maxSetting);
            endCapture(r);
            return r;
        }
    }
}

// ---------------------------------------------------------------------------
// "Create and open" decision (mirror of the open-after-create gate in the mod).
//
// The setting is OFF by default. When ON, after a SINGLE folder is created the
// mod navigates the active Explorer tab INTO that folder (same window, no new
// window). It must NEVER open when a batch (>1) was created -- there is no one
// folder to open, and opening the first would be surprising. It also must not
// open when nothing was created (createdCount == 0), e.g. a cancelled rename or
// a clash that was skipped.
//
// openAfterCreate : the useOpenAfterCreate setting.
// createdCount    : how many folders were actually created this run.
inline bool ShouldOpenAfterCreate(bool openAfterCreate, int createdCount) {
    return openAfterCreate && createdCount == 1;
}

// ---------------------------------------------------------------------------
// Type-speed timeouts (mirror of the typeSpeed setting in the mod).
//
// One setting scales BOTH time windows a slow typist can hit:
//   * the count-entry window (how long to type the batch number), and
//   * the inline-rename watch (how long to type the folder name).
// The chord gap (F->N) is deliberately NOT scaled -- it is a keystroke gap, not
// typing, so it stays fixed.
//
// Three presets only (0=Normal default, 1=Relaxed, 2=Patient). No free-form
// number entry: an arbitrary value (e.g. 50 ms, or 10 minutes) would just break
// the gesture, so the choices are constrained to sane, tested pairs. An unknown
// value falls back to Normal.
enum class TypeSpeed {
    Normal  = 0,  // default: current behavior (1200 ms / 30 s)
    Relaxed = 1,  // 2500 ms / 60 s
    Patient = 2,  // 4000 ms / 120 s
};

// Count-entry window in milliseconds for a given speed.
inline unsigned long CountWindowMsForSpeed(int speed) {
    switch (speed) {
        case 1:  return 2500;
        case 2:  return 4000;
        case 0:
        default: return 1200;
    }
}

// Inline-rename watch window in milliseconds for a given speed.
inline unsigned long RenameWatchMsForSpeed(int speed) {
    switch (speed) {
        case 1:  return 60000;
        case 2:  return 120000;
        case 0:
        default: return 30000;
    }
}
