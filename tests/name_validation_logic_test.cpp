// name_validation_logic_test.cpp
// Pins the PURE folder-name validation used for the popup's contextual,
// pre-Create validation (waht to create.md #6 / #14, plan Phase 3 + Phase 9).
//
// Rules (must respect existing behavior, NOT change the resolver):
//   New Folder: a blank/whitespace name is VALID (it defaults to "New folder");
//               only an actually-invalid typed name is rejected.
//   Group:      mirrors ResolveGroupDestination precedence (fixed > typed >
//               last). A blank name is valid when a fixed destination or a
//               remembered last target will still produce a destination; it is
//               invalid only when nothing can result. A fixed destination wins,
//               so a bad typed name is irrelevant (and not rejected) then.
//
// Self-contained: own main(), exit 0 = pass. RED before name_validation_logic.h
// exists; GREEN once it mirrors the mod.

#include "name_validation_logic.h"

#include <cstdio>

using namespace namevalid;

static int g_fail = 0;
static void check(bool cond, const char* what) {
    if (!cond) { printf("[FAIL] %s\n", what); ++g_fail; }
}

int main() {
    // ============================================================
    // REGRESSION FIX (waht-to-create #1): the popup validation must
    // NOT reject names the existing SanitizeFolderBase() would have
    // silently repaired. Validation now sanitizes first (strip the
    // Windows-forbidden chars, control chars, and trailing dots/
    // spaces) and only rejects a name that CANNOT produce a valid
    // creation state: a reserved device name, or an empty sanitized
    // base with no other destination. Everything the old creation
    // pipeline accepted-after-sanitizing stays accepted.
    // ============================================================

    // ---- New Folder: blank is fine (defaults to "New folder") ----
    check(ValidateNewFolderName(L"") == NameCheck::Ok, "NF blank ok");
    check(ValidateNewFolderName(L"   ") == NameCheck::Ok, "NF whitespace ok");
    check(ValidateNewFolderName(L"My Folder") == NameCheck::Ok, "NF normal name ok");
    check(ValidateNewFolderName(L"folder.name") == NameCheck::Ok, "NF internal dot ok");

    // ---- forbidden characters are STRIPPED, not rejected ----
    // Old pipeline: SanitizeFolderBase("a/b") -> "ab" -> created. The popup
    // must match: a name that sanitizes to a non-empty base is Ok.
    check(ValidateNewFolderName(L"a/b") == NameCheck::Ok, "slash sanitizes -> ok");
    check(ValidateNewFolderName(L"a\\b") == NameCheck::Ok, "backslash sanitizes -> ok");
    check(ValidateNewFolderName(L"My:Folder") == NameCheck::Ok, "colon sanitizes -> ok");
    check(ValidateNewFolderName(L"a<b") == NameCheck::Ok, "lt sanitizes -> ok");
    check(ValidateNewFolderName(L"My<test>") == NameCheck::Ok, "angle brackets sanitize -> ok");
    check(ValidateNewFolderName(L"a>b") == NameCheck::Ok, "gt sanitizes -> ok");
    check(ValidateNewFolderName(L"a|b") == NameCheck::Ok, "pipe sanitizes -> ok");
    check(ValidateNewFolderName(L"a?b") == NameCheck::Ok, "question sanitizes -> ok");
    check(ValidateNewFolderName(L"a*b") == NameCheck::Ok, "star sanitizes -> ok");
    check(ValidateNewFolderName(L"a\"b") == NameCheck::Ok, "quote sanitizes -> ok");

    // A name that is ONLY forbidden characters sanitizes to empty -> treated
    // like blank (New Folder defaults to "New folder", so still Ok).
    check(ValidateNewFolderName(L"???") == NameCheck::Ok, "all-forbidden -> blank -> NF ok");
    check(ValidateNewFolderName(L"<>|") == NameCheck::Ok, "all-forbidden 2 -> blank -> NF ok");

    // ---- reserved device names still rejected (they survive sanitizing and
    //      genuinely fail CreateDirectoryW) ----
    check(ValidateNewFolderName(L"CON") == NameCheck::ReservedName, "CON reserved");
    check(ValidateNewFolderName(L"con") == NameCheck::ReservedName, "con reserved (case)");
    check(ValidateNewFolderName(L"NUL") == NameCheck::ReservedName, "NUL reserved");
    check(ValidateNewFolderName(L"COM1") == NameCheck::ReservedName, "COM1 reserved");
    check(ValidateNewFolderName(L"LPT9") == NameCheck::ReservedName, "LPT9 reserved");
    check(ValidateNewFolderName(L"CON.txt") == NameCheck::ReservedName, "CON.txt reserved");
    check(ValidateNewFolderName(L"COM0") == NameCheck::Ok, "COM0 not reserved");
    check(ValidateNewFolderName(L"COM10") == NameCheck::Ok, "COM10 not reserved");
    check(ValidateNewFolderName(L"CONSOLE") == NameCheck::Ok, "CONSOLE not reserved");

    // ---- dots-only and trailing dot/space are STRIPPED, not rejected ----
    // SanitizeFolderBase strips trailing dots/spaces; "..." -> "" (blank),
    // "foo." -> "foo" (ok). Matches the old creation behavior exactly.
    check(ValidateNewFolderName(L".") == NameCheck::Ok, "single dot -> blank -> ok");
    check(ValidateNewFolderName(L"..") == NameCheck::Ok, "double dot -> blank -> ok");
    check(ValidateNewFolderName(L"...") == NameCheck::Ok, "triple dot -> blank -> ok");
    check(ValidateNewFolderName(L"foo.") == NameCheck::Ok, "trailing dot stripped -> ok");
    check(ValidateNewFolderName(L"foo ") == NameCheck::Ok, "trailing space stripped -> ok");

    // ---- Group: blank respects the resolver's precedence ----
    check(ValidateGroupName(L"", false, false) == NameCheck::EmptyNoDestination,
          "group blank + nothing -> needs a name");
    check(ValidateGroupName(L"", true, false) == NameCheck::Ok,
          "group blank + fixed dest -> ok");
    check(ValidateGroupName(L"", false, true) == NameCheck::Ok,
          "group blank + last target -> ok");
    check(ValidateGroupName(L"   ", false, false) == NameCheck::EmptyNoDestination,
          "group whitespace + nothing -> needs a name");
    check(ValidateGroupName(L"Trip", false, false) == NameCheck::Ok,
          "group typed name -> ok");
    // Group: a typed name full of forbidden chars sanitizes down. "a?b" -> "ab"
    // (a usable base), so it is NOT rejected -- matches the old pipeline.
    check(ValidateGroupName(L"a?b", false, false) == NameCheck::Ok,
          "group typed name sanitizes -> ok");
    // A typed name that sanitizes to EMPTY behaves like blank: rejected only
    // when nothing else can produce a destination.
    check(ValidateGroupName(L"???", false, false) == NameCheck::EmptyNoDestination,
          "group all-forbidden + nothing -> needs a name");
    check(ValidateGroupName(L"???", false, true) == NameCheck::Ok,
          "group all-forbidden + last target -> ok");
    // Fixed destination wins: the resolver ignores a typed name, so a bad typed
    // name is irrelevant and must NOT be reported.
    check(ValidateGroupName(L"a?b", true, false) == NameCheck::Ok,
          "group fixed dest overrides typed name");
    // Reserved name is still reported for Group when it is the only destination.
    check(ValidateGroupName(L"CON", false, false) == NameCheck::ReservedName,
          "group reserved typed name rejected");

    // ---- messages: Ok is empty; the two live failures are non-empty ----
    check(ValidationMessage(NameCheck::Ok).empty(), "ok has no message");
    check(!ValidationMessage(NameCheck::EmptyNoDestination).empty(), "empty msg");
    check(!ValidationMessage(NameCheck::ReservedName).empty(), "reserved msg");

    if (g_fail) { printf("name_validation_logic_test: %d check(s) FAILED\n", g_fail); return 1; }
    printf("name_validation_logic_test: OK\n");
    return 0;
}
