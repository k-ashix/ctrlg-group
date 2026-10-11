<#
    check_comment_safety.ps1  --  Guard against premature C block-comment
    termination inside the Windhawk metadata blocks of the .wh.cpp mod.

    WHY THIS EXISTS
    ---------------
    A Windhawk mod embeds its README and Settings as YAML/Markdown wrapped in
    C block comments:

        // ==WindhawkModReadme==
        /*
        ...markdown...
        */
        // ==/WindhawkModReadme==

        // ==WindhawkModSettings==
        /*
        ...yaml...
        */
        // ==/WindhawkModSettings==

    Windhawk strips these blocks by their `// ==...==` markers before it
    compiles, so a stray `*/` inside the prose still builds under Windhawk.
    But clangd (and any real C++ tokenizer used by an editor) sees the raw
    file: a `*/` inside the prose CLOSES the comment early, and everything
    after it is parsed as C++ -- producing a flood of false errors such as:

        A type specifier is required for all declarations
        Unknown type name 'Move'
        Unexpected character <U+2705> / <U+274C> / <U+2796>
        Missing terminating ' character

    The classic trigger is Markdown like `**Copy**/**Move**`: the `**/` in it
    contains the sequence `*/`.

    WHAT THIS CHECKS
    ----------------
    For each metadata block, the inner text (between the opening `/*` and the
    LAST `*/` before the closing `// ==/...==` marker) must contain NO `*/`.
    Exactly one `*/` -- the real terminator -- is allowed per block.

    EXIT: 0 = safe, 1 = a premature `*/` (or a malformed block) was found.
#>

[CmdletBinding()]
param(
    [string]$Source
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot  = Split-Path -Parent $ScriptDir
if (-not $Source) {
    $Source = Join-Path $RepoRoot "ctrlg-group.wh.cpp"
}

if (-not (Test-Path $Source)) {
    Write-Host "[FAIL] comment-safety: source not found: $Source" -ForegroundColor Red
    exit 1
}

$text = Get-Content -Raw -LiteralPath $Source

# Blocks to validate: opening marker -> closing marker.
$blocks = @(
    @{ Name = "WindhawkModReadme";   Open = "// ==WindhawkModReadme==";   Close = "// ==/WindhawkModReadme==" },
    @{ Name = "WindhawkModSettings"; Open = "// ==WindhawkModSettings=="; Close = "// ==/WindhawkModSettings==" }
)

$problems = New-Object System.Collections.Generic.List[string]

function LineOfIndex([string]$s, [int]$idx) {
    if ($idx -lt 0) { return -1 }
    return ($s.Substring(0, $idx) -split "`n").Count
}

foreach ($b in $blocks) {
    $openMarker = $text.IndexOf($b.Open)
    if ($openMarker -lt 0) {
        $problems.Add("$($b.Name): opening marker '$($b.Open)' not found")
        continue
    }
    $closeMarker = $text.IndexOf($b.Close, $openMarker)
    if ($closeMarker -lt 0) {
        $problems.Add("$($b.Name): closing marker '$($b.Close)' not found")
        continue
    }

    # First `/*` after the opening marker.
    $commentOpen = $text.IndexOf("/*", $openMarker)
    if ($commentOpen -lt 0 -or $commentOpen -gt $closeMarker) {
        $problems.Add("$($b.Name): no '/*' between the marker and its close")
        continue
    }

    # Region that must be a single, well-formed block comment:
    # from `/*` up to (not including) the closing `// ==/...==` marker.
    $regionStart = $commentOpen + 2
    $region = $text.Substring($regionStart, $closeMarker - $regionStart)

    # The region should contain exactly ONE `*/` -- the real terminator.
    $count = ([regex]::Matches($region, [regex]::Escape("*/"))).Count
    if ($count -eq 0) {
        $problems.Add("$($b.Name): block comment is never closed before '$($b.Close)'")
    }
    elseif ($count -gt 1) {
        # Report the FIRST premature `*/` (all but the last are premature).
        $firstStar = $region.IndexOf("*/")
        $absIdx    = $regionStart + $firstStar
        $line      = LineOfIndex $text $absIdx
        $snippetStart = [Math]::Max(0, $firstStar - 30)
        $snippet   = $region.Substring($snippetStart, [Math]::Min(70, $region.Length - $snippetStart)) -replace "`r?`n", " "
        $problems.Add("$($b.Name): premature '*/' at line $line closes the comment early -> clangd parses prose as C++. Near: ...$snippet...")
    }
}

if ($problems.Count -gt 0) {
    Write-Host "[FAIL] comment-safety guard" -ForegroundColor Red
    foreach ($p in $problems) { Write-Host "        - $p" -ForegroundColor Red }
    Write-Host "        Fix: remove the stray '*/' (e.g. write '**Copy** / **Move**' instead of '**Copy**/**Move**')." -ForegroundColor Yellow
    exit 1
}

Write-Host "[PASS] comment-safety guard (no premature '*/' in Windhawk metadata blocks)" -ForegroundColor Green
exit 0
