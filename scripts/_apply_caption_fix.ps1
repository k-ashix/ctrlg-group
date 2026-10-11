# One-shot applier for fix thse isseue.md caption/top + bottom asks:
#   * square Maximize (Expand) / overlapping-squares Restore (Collapse) glyph
#   * red press feedback on the bottom Cancel button (Close X already red)
#   * regression source-invariants pinning the above
# Deleted right after it runs.
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$cpp = "ctrlg-group.wh.cpp"
$t = [IO.File]::ReadAllText($cpp)

# ---- 1. Maximize/Restore square glyphs in DrawIcon --------------------------
$expandNew = @'
case IconId::Expand:
            // fix thse isseue.md: Maximize affordance -- a single SQUARE window
            // outline (replaces the old corner-bracket mark).
            Rectangle(hdc, x + 3, y + 3, x + u - 3, y + u - 3);
            break;
'@
$collapseNew = @'
case IconId::Collapse:
            // fix thse isseue.md: Restore affordance -- two overlapping squares
            // (front bottom-left + back top-right), the native Restore glyph.
            Rectangle(hdc, x + 2, y + 5, x + u - 5, y + u - 2);
            line(x + 5, y + 2, x + u - 2, y + 2);
            line(x + u - 2, y + 2, x + u - 2, y + u - 5);
            line(x + 5, y + 2, x + 5, y + 5);
            break;
'@
$t = [regex]::Replace($t, 'case IconId::Expand:[\s\S]*?break;', $expandNew)
$t = [regex]::Replace($t, 'case IconId::Collapse:[\s\S]*?break;', $collapseNew)

# ---- 2. Red press feedback on the bottom Cancel button ----------------------
$anchor = "// fix thse isseue.md #3: NO window-region clipping."
$n = ([regex]::Matches($t, [regex]::Escape($anchor))).Count
if ($n -ne 1) { throw "cancel-red anchor count = $n (expected 1)" }
$cancelRed = @'
// fix thse isseue.md: the Cancel button gives RED press feedback when
                // clicked, matching the Close (X) caption button. Scoped to
                // IDCANCEL + pressed so the inactive Copy/Move segment (which
                // shares the same secondary styling) is NOT affected.
                if (dis->CtlID == IDCANCEL && pressed) {
                    fill   = RGB(193, 15, 30);
                    txt    = RGB(255, 255, 255);
                    border = RGB(193, 15, 30);
                }

                // fix thse isseue.md #3: NO window-region clipping.
'@
$t = $t.Replace($anchor, $cancelRed)
[IO.File]::WriteAllText($cpp, $t, (New-Object System.Text.UTF8Encoding($true)))

# ---- 3. Regression source-invariants ---------------------------------------
$reg = "scripts\regression_check.ps1"
$r = [IO.File]::ReadAllText($reg)
$resultAnchor = "Write-Host `"`"`r`nif (`$fail -eq 0) {"
if (($r.IndexOf($resultAnchor)) -lt 0) { throw "regression result anchor not found" }
$regAsserts = @'
Head "PART 2 (fix thse isseue.md): caption square glyph + red Cancel"
AssertPresent "resize Maximize glyph is a single square window outline" 'Rectangle\(hdc, x \+ 3, y \+ 3, x \+ u - 3, y \+ u - 3\)'
AssertPresent "resize Restore glyph is two overlapping squares"         'Rectangle\(hdc, x \+ 2, y \+ 5, x \+ u - 5, y \+ u - 2\)'
AssertPresent "Cancel button gives RED press feedback like Close"       'dis->CtlID == IDCANCEL && pressed'
AssertPresent "Close (X) stays the danger-red caption button"           'chromebtn::IsDanger\(dis->CtlID == 105'

'@
$r = $r.Replace($resultAnchor, $regAsserts + $resultAnchor)
[IO.File]::WriteAllText($reg, $r, (New-Object System.Text.UTF8Encoding($false)))

Write-Host "DONE apply_caption_fix"
