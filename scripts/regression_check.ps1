<#
    regression_check.ps1  --  One-command regression gate for explorer-ctrlfn-newfolder.

    Location: scripts\. Run from anywhere:

        powershell -ExecutionPolicy Bypass -File scripts\regression_check.ps1

    --------------------------------------------------------------------------
    What this does (and why it exists on top of folder_run_tests.ps1)
    --------------------------------------------------------------------------
    explorer-ctrlfn-newfolder.wh.cpp is a Windhawk mod: it #includes Win32 /
    Windhawk headers and has no standalone main(), so the FULL translation unit
    cannot be compiled or executed here. folder_run_tests.ps1 covers every pure
    decision that was MIRRORED into tests\*.h. But some properties are STRUCTURAL
    facts about the mod source itself (it must create a directory, not a file;
    the low-level hook must stay non-blocking; all three trigger options must
    exist). A unit test cannot see those, so a regression there would slip past.

    This script is the gate for BOTH kinds of check:

      PART 1  Run the unit-test suite (delegates to folder_run_tests.ps1).
      PART 2  Assert source-level invariants over explorer-ctrlfn-newfolder.wh.cpp.

    Exit code is 0 only when EVERYTHING passes, so it drops straight into CI as a
    required status check.

    Options:
        -SkipUnitTests   run only the source-invariant checks (PART 2)
        -Compiler g++    forwarded to folder_run_tests.ps1 (default: g++)
        -Std c++17       forwarded to folder_run_tests.ps1 (default: c++17)
#>

[CmdletBinding()]
param(
    [switch]$SkipUnitTests,
    [string]$Compiler = "g++",
    [string]$Std = "c++17"
)

$ErrorActionPreference = "Stop"

$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot   = Split-Path -Parent $ScriptDir
$ModFile    = Join-Path $RepoRoot "explorer-ctrlfn-newfolder.wh.cpp"

$fail = 0
function Ok   ($m) { Write-Host ("  [PASS] " + $m) -ForegroundColor Green }
function Bad  ($m) { Write-Host ("  [FAIL] " + $m) -ForegroundColor Red; $script:fail++ }
function Head ($m) { Write-Host ""; Write-Host $m -ForegroundColor Cyan }

Write-Host "=== explorer-ctrlfn-newfolder regression gate ===" -ForegroundColor White

# --------------------------------------------------------------------------
# PART 1  --  unit-test suite
# --------------------------------------------------------------------------
if (-not $SkipUnitTests) {
    Head "PART 1: unit-test suite (folder_run_tests.ps1)"
    $runner = Join-Path $ScriptDir "folder_run_tests.ps1"
    if (-not (Test-Path $runner)) {
        Bad "folder_run_tests.ps1 not found -- cannot run unit tests"
    } else {
        # Run the tests + guards but NOT this gate again (avoid recursion).
        & powershell -NoProfile -ExecutionPolicy Bypass -File $runner `
            -SkipRegressionCheck -Compiler $Compiler -Std $Std
        if ($LASTEXITCODE -eq 0) { Ok "unit-test suite passed" }
        else { Bad "unit-test suite failed (see its output above)" }
    }
} else {
    Head "PART 1: unit-test suite  (skipped via -SkipUnitTests)"
}

# --------------------------------------------------------------------------
# PART 2  --  source-level invariants over the mod
# --------------------------------------------------------------------------
if (-not (Test-Path $ModFile)) {
    Bad "explorer-ctrlfn-newfolder.wh.cpp not found at: $ModFile"
    Write-Host ""
    Write-Host ("RESULT : FAIL ({0} issue(s))" -f $fail) -ForegroundColor Red
    exit 1
}

$modText = [IO.File]::ReadAllText($ModFile)
function CountMatches ($pattern) { return ([regex]::Matches($modText, $pattern)).Count }
function AssertPresent ($label, $pattern) {
    $n = CountMatches $pattern
    if ($n -ge 1) { Ok ("{0} -- present ({1})" -f $label, $n) }
    else          { Bad ("{0} -- expected >=1, found 0" -f $label) }
}
function AssertAbsent ($label, $pattern) {
    $n = CountMatches $pattern
    if ($n -eq 0) { Ok ("{0} -- absent (0)" -f $label) }
    else          { Bad ("{0} -- expected 0, found {1}" -f $label, $n) }
}

Head "PART 2: source invariants"

# Folder creation (not file creation) is the whole point of this mod.
AssertPresent "creates a real directory (CreateDirectoryW)"   'CreateDirectoryW'
AssertPresent "elevated create uses FILE_ATTRIBUTE_DIRECTORY" 'FILE_ATTRIBUTE_DIRECTORY'
AssertPresent "unique folder-name helper present"            'MakeUniqueFolderName'
AssertPresent "default base name 'New folder'"               'New folder'

# The keyboard hook must stay fast: it only offloads work to the worker thread.
AssertPresent "hook offloads work (non-blocking)"            'PostThreadMessageW'
AssertPresent "ignores its own injected input"               'LLKHF_INJECTED'
AssertPresent "installs a low-level keyboard hook"           'WH_KEYBOARD_LL'
AssertPresent "Ctrl+F search-preserving replay path"         'ReplayCtrlF'

# All three configurable trigger options must exist.
AssertPresent "trigger option ctrl_f_n"                      'ctrl_f_n'
AssertPresent "trigger option ctrl_n_f"                      'ctrl_n_f'
AssertPresent "trigger option ctrl_n"                        'ctrl_n:'

# Multi-folder creation (v0.2.0) must stay wired up.
AssertPresent "multi-folder toggle setting"                 'multiFolder'
AssertPresent "max-folders cap setting"                      'maxFolders'
AssertPresent "hard cap constant (100)"                      'kFolderHardCap'
AssertPresent "count clamp helper"                           'ResolveFolderCount'
AssertPresent "batch name helper"                            'MakeUniqueFolderNames'
AssertPresent "count-entry capture state"                    'g_countActive'
AssertPresent "count-entry commit path"                      'CommitCountCapture'
AssertPresent "auto-resolve-names toggle setting"            'autoResolveNames'

# v0.2.2 -- chord/count timeouts must match the SetTimer-RETURNED handle, not
# the constant id that SetTimer(NULL, ...) silently ignores. If this regresses,
# the count-entry window never auto-commits and the trigger gets stuck: it does
# nothing the first time and leaks the key into Explorer the second time.
AssertPresent "chord timeout matches stored timer id"       'msg\.wParam == g_chordTimer'
AssertPresent "count timeout matches stored timer id"       'msg\.wParam == g_countTimer'

# The mod must NOT regress back to the file-creating reference behaviour.
AssertAbsent  "old file-creation API is gone"                'CreateFileW'
AssertAbsent  "old file-name helper is gone"                 'MakeUniqueFilename'

# --------------------------------------------------------------------------
Write-Host ""
if ($fail -eq 0) {
    Write-Host "RESULT : PASS" -ForegroundColor Green
    exit 0
} else {
    Write-Host ("RESULT : FAIL ({0} issue(s))" -f $fail) -ForegroundColor Red
    exit 1
}
