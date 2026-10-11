#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_emoji_corruption.py  --  Emoji / symbol corruption guard + comment hygiene
report for the explorer-ctrlfn-newfolder repo.

WHY THIS EXISTS
---------------
A UTF-8 file that is re-saved (or pasted into) through a Windows-1252 (CP1252)
round trip turns every multi-byte symbol into "mojibake": the Explorer legend
emoji in the ==WindhawkModReadme== block became things like:

    `âœ…`  (should be  ✅)      `âŒ`  (❌)       `âž–`  (➖)
    `âŒ¨ï¸` (⌨️)                `âœï¸` (✍️)       `Â·`  (·)       `â€”` (—)

Windhawk strips the readme comment before it compiles, so the mod still builds,
but the published readme is garbled and clangd can trip over the stray bytes.

WHAT THIS DOES
--------------
1. DETECT  mojibake runs (and unrecoverable U+FFFD) in the codebase + markdown.
2. FIX     them back to the correct symbol via a CP1252 -> UTF-8 round trip
           (``--fix``), restricted to NON-ASCII runs only.
3. SAFE    every fix is proven comment/code-safe: the repair NEVER changes a
           single ASCII (0x00-0x7F) byte, so code, delimiters and the readme
           block's ``*/`` terminator cannot be touched. Also re-checks that each
           Windhawk metadata block still has exactly one ``*/``.
4. REPORT  an inventory of the real emoji/symbols in use, and a list of
           duplicate / near-duplicate comments that could be refined or removed.

USAGE
-----
    python scripts/check_emoji_corruption.py            # scan + report (exit 1 if corrupt)
    python scripts/check_emoji_corruption.py --fix      # repair in place (exit 0)
    python scripts/check_emoji_corruption.py --selftest # run the built-in tests
    python scripts/check_emoji_corruption.py --no-comments   # skip the comment report

Exit codes: 0 = clean (or fixed / selftest OK), 1 = corruption found (scan) or a
selftest failure, 2 = usage/IO error.
"""

import argparse
import os
import re
import sys
import unicodedata

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Files we scan. The main TU + every header/markdown/powershell doc, minus
# build output and THIS script (its self-test strings are deliberate mojibake).
SCAN_SUFFIXES = (".cpp", ".h", ".md", ".ps1")
SKIP_DIR_PARTS = (os.path.join("tests", "build"),)
SELF_NAME = os.path.basename(__file__)

# --- mojibake engine --------------------------------------------------------
# A char is "CP1252-class" when CP1252 can encode it to a single 0x80-0xFF byte
# -- i.e. it is exactly the kind of char a UTF-8-as-CP1252 misdecode produces.
# CP1252 "undefined" byte slots (0x81, 0x8D, 0x8F, 0x90, 0x9D). A mojibake
# decoder round-trips these as the raw C1 control of the SAME code point, so we
# must map them back to their byte ourselves -- Python's cp1252 codec refuses to
# encode them, which otherwise drops a trailing byte (e.g. the 0x8F of an emoji
# variation selector) out of the run.
_C1_RAW = {0x81, 0x8D, 0x8F, 0x90, 0x9D}


def _mojibyte(ch):
    """The single 0x80-0xFF byte that mojibake-decodes (via CP1252) to `ch`,
    or None when `ch` is not a CP1252-class artifact char."""
    o = ord(ch)
    if o in _C1_RAW:
        return o
    try:
        b = ch.encode("cp1252")
    except UnicodeEncodeError:
        return None
    return b[0] if len(b) == 1 and 0x80 <= b[0] <= 0xFF else None


def _is_class(ch):
    return _mojibyte(ch) is not None


def _is_lead(ch):
    # A genuine UTF-8 lead byte (0xC2-0xF4) proves the run is really mojibake,
    # not an isolated legitimate CP1252-range glyph (a lone '·' or '—').
    b = _mojibyte(ch)
    return b is not None and 0xC2 <= b <= 0xF4


def _try_decode(seg):
    """The UTF-8 string `seg` mojibake-decodes to, or None if `seg` is not a
    clean CP1252 -> UTF-8 round trip."""
    bs = bytearray()
    for ch in seg:
        b = _mojibyte(ch)
        if b is None:
            return None
        bs.append(b)
    try:
        return bytes(bs).decode("utf-8")
    except UnicodeDecodeError:
        return None


def _repair_run(run):
    """Greedy CP1252 -> UTF-8 round trip. Repairs the longest decodable segment
    at each position; leaves genuinely-undecodable chars untouched. Returns the
    repaired string (== run when nothing recovered)."""
    whole = _try_decode(run)
    if whole is not None and whole != run:
        return whole
    out, i, n = [], 0, len(run)
    while i < n:
        hit = None
        for j in range(n, i, -1):
            dec = _try_decode(run[i:j])
            if dec is not None and dec != run[i:j]:
                hit = (dec, j)
                break
        if hit:
            out.append(hit[0])
            i = hit[1]
        else:
            out.append(run[i])
            i += 1
    return "".join(out)


_RUN_RE = None


def _find_runs(text):
    """Yield (start, end) of maximal CP1252-class runs that contain a UTF-8
    lead char (so lone legit glyphs like '—'/'·'/'→' are never matched)."""
    i, n = 0, len(text)
    while i < n:
        if _is_class(text[i]):
            j = i + 1
            while j < n and _is_class(text[j]):
                j += 1
            run = text[i:j]
            if any(_is_lead(c) for c in run):
                yield (i, j, run)
            i = j
        else:
            i += 1


def repair_text(text):
    """Return (new_text, fixes) where fixes is a list of (badrun, goodrun)."""
    fixes = []
    out = []
    last = 0
    for (s, e, run) in _find_runs(text):
        fixed = _repair_run(run)
        out.append(text[last:s])
        if fixed != run:
            fixes.append((run, fixed))
            out.append(fixed)
        else:
            out.append(run)
        last = e
    out.append(text[last:])
    return "".join(out), fixes


def _cps(s):
    return " ".join("U+%04X" % ord(c) for c in s)


# --- emoji / symbol inventory ----------------------------------------------
def _is_pictograph(ch):
    o = ord(ch)
    return (
        0x2190 <= o <= 0x2BFF        # arrows, math, misc technical, dingbats
        or 0x2600 <= o <= 0x27BF     # misc symbols + dingbats
        or o == 0xFE0F               # emoji variation selector
        or 0x1F000 <= o <= 0x1FAFF   # emoji planes
    )


def emoji_inventory(text):
    seen = {}
    for ch in text:
        if _is_pictograph(ch):
            seen.setdefault(ch, 0)
            seen[ch] += 1
    return seen


# --- comment extraction + redundancy ---------------------------------------
_LINE_COMMENT = {
    ".cpp": r"//(.*)$",
    ".h": r"//(.*)$",
    ".ps1": r"#(.*)$",
}


def _comment_texts(path, text):
    ext = os.path.splitext(path)[1].lower()
    pat = _LINE_COMMENT.get(ext)
    if not pat:
        return []
    rx = re.compile(pat)
    out = []
    for n, line in enumerate(text.splitlines(), 1):
        m = rx.search(line)
        if m:
            body = m.group(1).strip()
            if body:
                out.append((n, body))
    return out


def _normalize_comment(body):
    s = body.lower().strip()
    s = s.strip("-=*/<> ").strip()
    s = re.sub(r"\s+", " ", s)
    return s


def redundant_comments(files, min_len=12, min_count=3):
    """Group identical (normalized) comment bodies that repeat across the repo
    -- candidates to refine or hoist into one place. Report-only."""
    groups = {}
    for path, text in files:
        for (n, body) in _comment_texts(path, text):
            norm = _normalize_comment(body)
            if len(norm) < min_len:
                continue
            groups.setdefault(norm, []).append((os.path.basename(path), n, body))
    dupes = {k: v for k, v in groups.items() if len(v) >= min_count}
    return dupes


# --- file IO (byte-exact: preserves CRLF + BOM) -----------------------------
_BOM = b"\xef\xbb\xbf"


def read_file(path):
    raw = open(path, "rb").read()
    had_bom = raw.startswith(_BOM)
    body = raw[len(_BOM):] if had_bom else raw
    return body.decode("utf-8"), had_bom


def write_file(path, text, had_bom):
    data = text.encode("utf-8")
    if had_bom:
        data = _BOM + data
    open(path, "wb").write(data)


def ascii_invariant_ok(before, after):
    """The fixer must only ever rewrite non-ASCII bytes: every ASCII (<0x80)
    char must be byte-identical before and after. This is what makes a fix
    provably code/comment-safe (it cannot touch a brace, quote, ';' or '*/')."""
    return [c for c in before if ord(c) < 0x80] == [c for c in after if ord(c) < 0x80]


def metadata_block_safe(text):
    """Each ==WindhawkMod*== metadata block must still hold exactly one '*/'."""
    ok = True
    for name in ("WindhawkModReadme", "WindhawkModSettings"):
        o = text.find("// ==%s==" % name)
        c = text.find("// ==/%s==" % name, o + 1) if o >= 0 else -1
        if o < 0 or c < 0:
            continue
        start = text.find("/*", o)
        if start < 0 or start > c:
            ok = False
            continue
        if text.count("*/", start, c) != 1:
            ok = False
    return ok


# --- target discovery -------------------------------------------------------
def target_files():
    out = []
    for base, dirs, names in os.walk(REPO_ROOT):
        rel = os.path.relpath(base, REPO_ROOT)
        if any(part in rel for part in SKIP_DIR_PARTS):
            continue
        for nm in names:
            if nm == SELF_NAME:
                continue
            if nm.lower().endswith(SCAN_SUFFIXES):
                out.append(os.path.join(base, nm))
    out.sort()
    return out


# --- self test --------------------------------------------------------------
SELFTEST = [
    ("\u00e2\u0153\u2026", "\u2705"),                     # âœ…  -> ✅
    ("\u00e2\u009d\u0152", "\u274c"),                     # âŒ   -> ❌
    ("\u00e2\u017e\u2013", "\u2796"),                     # âž–  -> ➖
    ("\u00c2\u00b7", "\u00b7"),                           # Â·   -> ·
    ("\u00e2\u20ac\u201d", "\u2014"),                     # â€”  -> —
    ("\u00e2\u0152\u00a8\u00ef\u00b8\u008f",
     "\u2328\ufe0f"),                                     # âŒ¨ï¸ -> ⌨️
]
DO_NOT_TOUCH = ["\u2014", "\u00b7", "a \u2192 b", "16-dpi -> device", "plain ascii;{}"]


def run_selftest():
    bad = 0
    for src, want in SELFTEST:
        got, fixes = repair_text(src)
        if got != want:
            bad += 1
            print("[FAIL] repair %s -> %s (wanted %s)" % (_cps(src), _cps(got), _cps(want)))
    for s in DO_NOT_TOUCH:
        got, fixes = repair_text(s)
        if got != s:
            bad += 1
            print("[FAIL] touched a safe string: %r -> %r" % (s, got))
    # ascii-invariant must hold for every repair
    for src, _ in SELFTEST:
        got, _f = repair_text(src)
        if not ascii_invariant_ok(src, got):
            bad += 1
            print("[FAIL] ascii-invariant broken repairing %s" % _cps(src))
    if bad:
        print("selftest: %d failure(s)" % bad)
        return 1
    print("selftest: OK (%d repairs, %d safe strings)" % (len(SELFTEST), len(DO_NOT_TOUCH)))
    return 0


# --- main -------------------------------------------------------------------
def main(argv):
    ap = argparse.ArgumentParser(description="emoji corruption guard + comment hygiene")
    ap.add_argument("--fix", action="store_true", help="repair corruption in place")
    ap.add_argument("--selftest", action="store_true", help="run built-in tests and exit")
    ap.add_argument("--no-comments", action="store_true", help="skip the comment report")
    args = ap.parse_args(argv)

    if args.selftest:
        return run_selftest()

    files = []
    for p in target_files():
        try:
            text, bom = read_file(p)
        except (OSError, UnicodeDecodeError) as e:
            print("[WARN] cannot read %s: %s" % (p, e))
            continue
        files.append((p, text, bom))

    total_corrupt = 0
    fixed_files = 0
    print("== corruption scan ==")
    for p, text, bom in files:
        new, fixes = repair_text(text)
        fffd = text.count("\ufffd")
        if fffd:
            print("  [UNRECOVERABLE] %s: %d x U+FFFD replacement char(s) -- needs manual review"
                  % (os.path.relpath(p, REPO_ROOT), fffd))
            total_corrupt += fffd
        if fixes:
            total_corrupt += len(fixes)
            rel = os.path.relpath(p, REPO_ROOT)
            uniq = sorted(set(fixes))
            print("  [CORRUPT] %s: %d run(s), %d distinct:" % (rel, len(fixes), len(uniq)))
            for bad, good in uniq:
                print("      %-26s ->  %s   (%s -> %s)"
                      % (repr(bad), good, _cps(bad), _cps(good)))
            if args.fix:
                if not ascii_invariant_ok(text, new):
                    print("      [ABORT] fix would change ASCII bytes -- refusing (not comment-safe)")
                    continue
                if p.lower().endswith((".cpp", ".h")) and not metadata_block_safe(new):
                    print("      [ABORT] fix would break a Windhawk metadata block -- refusing")
                    continue
                write_file(p, new, bom)
                fixed_files += 1
                print("      [FIXED] wrote %d repair(s)" % len(fixes))

    print("\n== emoji / symbol inventory (post-fix view) ==")
    inv = {}
    for p, text, bom in files:
        src = repair_text(text)[0] if args.fix else text
        for ch, c in emoji_inventory(src).items():
            inv[ch] = inv.get(ch, 0) + c
    if inv:
        for ch in sorted(inv, key=lambda c: ord(c)):
            try:
                name = unicodedata.name(ch)
            except ValueError:
                name = "<no name>"
            print("  %s  U+%04X  x%-3d %s" % (ch, ord(ch), inv[ch], name))
    else:
        print("  (none)")

    if not args.no_comments:
        print("\n== duplicate / refinable comments (>=3 identical, report-only) ==")
        dupes = redundant_comments([(p, t) for p, t, _ in files])
        if dupes:
            for norm, hits in sorted(dupes.items(), key=lambda kv: -len(kv[1]))[:20]:
                locs = ", ".join("%s:%d" % (f, n) for f, n, _ in hits[:6])
                more = "" if len(hits) <= 6 else " (+%d more)" % (len(hits) - 6)
                print("  x%-2d \"%s\"" % (len(hits), hits[0][2][:70]))
                print("       %s%s" % (locs, more))
        else:
            print("  (no comment repeated >=3 times)")

    print("\n== summary ==")
    if args.fix:
        print("  fixed %d file(s); %d corruption run(s) addressed." % (fixed_files, total_corrupt))
        return 0
    if total_corrupt:
        print("  %d corruption item(s) found. Re-run with --fix to repair." % total_corrupt)
        return 1
    print("  no corruption found.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
