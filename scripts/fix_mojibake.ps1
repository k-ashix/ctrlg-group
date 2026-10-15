<#
    fix_mojibake.ps1 -- Controlled repair of UTF-8-as-CP1252 double-encoded
    ("mojibake") text in a source/markdown file.

    HOW IT WORKS (and why it is SAFE / controlled):
      The corruption is classic: original UTF-8 bytes were decoded as Windows-1252
      and re-saved as UTF-8. To reverse it we:
        1. Read the file as UTF-8.
        2. Find MAXIMAL RUNS of consecutive "CP1252 upper" characters (code points
           that encode to a single 0x80-0xFF byte under CP1252, i.e. the only
           characters a 1252 mis-decode can produce).
        3. For each run: re-encode via CP1252 -> original bytes -> STRICT UTF-8
           decode. If (and only if) that run decodes as valid UTF-8, replace it
           with the decoded text; otherwise leave the run untouched.
      Pure ASCII (code, identifiers, normal comments, markdown) is NEVER in a run
      and is never touched. A lone legitimately-encoded char that does not form a
      valid UTF-8 sequence fails the strict decode and is left as-is. This avoids
      the "uncontrolled global byte replacement" the spec forbids.

    USAGE:
        powershell -ExecutionPolicy Bypass -File scripts\fix_mojibake.ps1 -Path <file>          # dry-run (default)
        powershell -ExecutionPolicy Bypass -File scripts\fix_mojibake.ps1 -Path <file> -Apply   # write changes
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Path,
    [switch]$Apply
)
$ErrorActionPreference = "Stop"

$cp1252 = [System.Text.Encoding]::GetEncoding(1252)
$utf8strict = New-Object System.Text.UTF8Encoding($false, $true)   # no BOM, throwOnInvalid

# Is this char one that a CP1252 mis-decode can emit? (round-trips to a single 0x80-0xFF byte)
function Is-Cp1252Upper([char]$c) {
    if ([int]$c -lt 0x80) { return $false }
    try {
        $b = $cp1252.GetBytes([string]$c)
        if ($b.Length -ne 1) { return $false }
        if ($b[0] -lt 0x80) { return $false }
        # ensure it round-trips (not a '?' substitution)
        $back = $cp1252.GetString($b)
        return ($back -eq [string]$c)
    } catch { return $false }
}

# Detect current BOM so we can preserve the file's encoding choice.
$raw = [System.IO.File]::ReadAllBytes($Path)
$hasBom = ($raw.Length -ge 3 -and $raw[0] -eq 0xEF -and $raw[1] -eq 0xBB -and $raw[2] -eq 0xBF)
$text = [System.IO.File]::ReadAllText($Path, [System.Text.Encoding]::UTF8)

$sb = New-Object System.Text.StringBuilder
$i = 0
$n = $text.Length
$changes = @{}
$changeCount = 0
while ($i -lt $n) {
    $c = $text[$i]
    if (Is-Cp1252Upper $c) {
        $j = $i
        while ($j -lt $n -and (Is-Cp1252Upper $text[$j])) { $j++ }
        $run = $text.Substring($i, $j - $i)
        $decoded = $null
        try {
            $bytes = $cp1252.GetBytes($run)
            $decoded = $utf8strict.GetString($bytes)
        } catch { $decoded = $null }
        if ($decoded -ne $null -and $decoded -ne $run) {
            [void]$sb.Append($decoded)
            $changeCount++
            $key = $run
            if (-not $changes.ContainsKey($key)) {
                $cps = (($decoded.ToCharArray() | ForEach-Object { 'U+{0:X4}' -f [int]$_ }) -join ' ')
                $changes[$key] = ('{0}   ->   "{1}"   [{2}]' -f ($changeCount), $decoded, $cps)
            }
        } else {
            [void]$sb.Append($run)
        }
        $i = $j
    } else {
        [void]$sb.Append($c)
        $i++
    }
}

$result = $sb.ToString()

Write-Host ("File        : {0}" -f $Path)
Write-Host ("Has BOM     : {0}" -f $hasBom)
Write-Host ("Runs fixed  : {0}" -f $changeCount)
Write-Host "Distinct repairs:"
$seen = @{}
foreach ($k in $changes.Keys) {
    $cpsIn = (($k.ToCharArray() | ForEach-Object { 'U+{0:X4}' -f [int]$_ }) -join ' ')
    $line = ('  [{0}] -> {1}' -f $cpsIn, $changes[$k])
    if (-not $seen.ContainsKey($line)) { Write-Host $line; $seen[$line] = $true }
}

if ($Apply) {
    $enc = New-Object System.Text.UTF8Encoding($hasBom, $false)
    [System.IO.File]::WriteAllText($Path, $result, $enc)
    Write-Host "APPLIED (written as UTF-8$([string]::Empty))"
} else {
    Write-Host "DRY-RUN (no file written). Re-run with -Apply to write."
}
