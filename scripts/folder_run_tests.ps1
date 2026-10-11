<#
    folder_run_tests.ps1  --  Build & run all unit tests for ctrlg-group,
    then run the source/guard checks.

    HOW TO RUN (no arguments needed):

        powershell -ExecutionPolicy Bypass -File .\scripts\folder_run_tests.ps1

    Success prints "OVERALL : PASS" and exits 0. Any failing unit test or guard
    prints "OVERALL : FAIL" and exits non-zero, so this doubles as the CI gate.

    The script finds the repo root and tests\ directory from its own path, so it
    does not depend on the current working directory.

    --------------------------------------------------------------------------
    Why the tests are NOT linked against ctrlg-group.wh.cpp
    --------------------------------------------------------------------------
    ctrlg-group.wh.cpp is a Windhawk mod: it #includes Windhawk /
    Win32 headers and has no standalone main(), so it cannot be compiled on its
    own. The pure decisions (unique folder-name numbering; the Ctrl+F+N /
    Ctrl+N / Ctrl+N+F trigger state machine) are MIRRORED into
    tests\newfolder_logic.h, and each tests\*_test.cpp is a self-contained
    program that includes that header. The tests validate the logic without the
    un-compilable Win32 translation unit.

    This script:
      1. Compiles every tests\*_test.cpp into tests\build\
      2. Runs each resulting .exe (exit 0 == pass)
      3. Auto-chains the guard scripts (log-layer, empty-body, balance,
         version/INIT) and the source-invariant regression gate
      4. Writes a full transcript to test_result_run_<HH_MM_tt>.log in the repo
         root, and error.log ONLY when something fails (stale error.log is
         removed on a clean run)
      5. Cleans up tests\build\ unless -KeepBinaries

    Options:
        -KeepBinaries          keep compiled exes in tests\build\
        -SkipLogLayerCheck     skip the log-layer guard
        -SkipEmptyBodyCheck    skip the empty-body guard
        -SkipBalanceCheck      skip the delimiter balance check
        -SkipCommentSafetyCheck skip the Windhawk comment-safety guard
        -SkipVersionInitCheck  skip the version / INIT consistency check
        -SkipRegressionCheck   skip the source-invariant regression gate
        -ShowPass              print each passing unit test to the console
        -Compiler g++          C++ compiler (default: g++)
        -Std c++17             C++ standard (default: c++17)
        -Threads 4             parallel compile+run workers (default: 4, max: 6)
#>

[CmdletBinding()]
param(
    [switch]$KeepBinaries,
    [switch]$SkipLogLayerCheck,
    [switch]$SkipEmptyBodyCheck,
    [switch]$SkipBalanceCheck,
    [switch]$SkipCommentSafetyCheck,
    [switch]$SkipVersionInitCheck,
    [switch]$SkipRegressionCheck,
    [switch]$ShowPass,
    [string]$Compiler = "g++",
    [string]$Std = "c++17",
    [ValidateRange(1, 6)]
    [int]$Threads = 5
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot  = Split-Path -Parent $ScriptDir
$TestsDir  = Join-Path $RepoRoot "tests"
$BuildDir  = Join-Path $TestsDir "build"
$LogFile   = Join-Path $RepoRoot ("test_result_run_{0}.log" -f (Get-Date -Format "hh_mm_tt"))
$ErrLog    = Join-Path $RepoRoot "error.log"

$script:transcript = New-Object System.Collections.Generic.List[string]
$script:fails      = New-Object System.Collections.Generic.List[string]

function Emit($msg, $color) {
    $script:transcript.Add([string]$msg)
    if ($color) { Write-Host $msg -ForegroundColor $color } else { Write-Host $msg }
}
function Section($msg) { Emit "" $null; Emit $msg Cyan }

# Run a native command (g++, a test exe) capturing stdout+stderr and its exit
# code WITHOUT letting $ErrorActionPreference = "Stop" promote native stderr to
# a thrown error. Exit code is left in $script:nativeExit.
function Invoke-Native([scriptblock]$block) {
    $prev = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $out = & $block
    } finally {
        $script:nativeExit = $LASTEXITCODE
        $ErrorActionPreference = $prev
    }
    return $out
}

Emit "=== ctrlg-group :: test + guard run ===" White
Emit ("Repo   : {0}" -f $RepoRoot) $null
Emit ("Tests  : {0}" -f $TestsDir) $null
Emit ("Compiler: {0} (std={1})" -f $Compiler, $Std) $null
Emit ("Threads : {0} (max 6)" -f $Threads) $null

# ---------------------------------------------------------------------------
# 1. Unit tests: compile & run every tests\*_test.cpp
# ---------------------------------------------------------------------------
Section "UNIT TESTS"

if (-not (Test-Path $TestsDir)) {
    Emit ("  [FAIL] tests directory not found: {0}" -f $TestsDir) Red
    $script:fails.Add("tests directory missing")
} else {
    # Compile into a fresh per-run sub-directory. A stale .exe left locked by a
    # previous run (AV scan, a still-exiting process) can otherwise make the
    # linker fail with "cannot open output file ... Permission denied".
    $BuildRoot = $BuildDir   # the tests\build parent; removed whole at the end
    $BuildDir = Join-Path $BuildDir (Get-Date -Format "yyyyMMdd_HHmmssfff")
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

    $testSources = Get-ChildItem -Path $TestsDir -Filter "*_test.cpp" | Sort-Object Name
    if ($testSources.Count -eq 0) {
        Emit "  [FAIL] no *_test.cpp files found" Red
        $script:fails.Add("no unit tests found")
    }

    # Each test is independent (self-contained .cpp -> own .exe), so compile+run
    # is fanned out across up to $Threads parallel workers via a runspace pool.
    # Workers share nothing and write to their own exe path; the parent collects
    # every result and emits them in a STABLE (sorted-by-name) order so the
    # transcript, the [PASS]/[FAIL] lines, and the CI gate stay deterministic
    # regardless of which worker finishes first.
    $workerBody = {
        param($Src, $Name, $Exe, $Compiler, $Std)

        function Invoke-NativeWorker([scriptblock]$block) {
            $prev = $ErrorActionPreference
            $ErrorActionPreference = "Continue"
            try   { $out = & $block }
            finally { $script:workerExit = $LASTEXITCODE; $ErrorActionPreference = $prev }
            return $out
        }

        $buildOut = Invoke-NativeWorker { & $Compiler "-std=$Std" "-Wall" $Src "-o" $Exe 2>&1 }
        if ($script:workerExit -ne 0) {
            return [pscustomobject]@{
                Name = $Name; Kind = "compile"; Ok = $false; Exit = $script:workerExit
                Output = ($buildOut | Out-String)
            }
        }

        $runOut = Invoke-NativeWorker { & $Exe 2>&1 }
        return [pscustomobject]@{
            Name = $Name; Kind = "test"; Ok = ($script:workerExit -eq 0); Exit = $script:workerExit
            Output = ($runOut | Out-String)
        }
    }

    $pool = [runspacefactory]::CreateRunspacePool(1, $Threads)
    $pool.Open()
    $handles = New-Object System.Collections.Generic.List[object]

    foreach ($src in $testSources) {
        $name = [IO.Path]::GetFileNameWithoutExtension($src.Name)
        $exe  = Join-Path $BuildDir ($name + ".exe")

        $ps = [powershell]::Create()
        $ps.RunspacePool = $pool
        [void]$ps.AddScript($workerBody).
            AddArgument($src.FullName).AddArgument($name).AddArgument($exe).
            AddArgument($Compiler).AddArgument($Std)

        $handles.Add([pscustomobject]@{ PS = $ps; Async = $ps.BeginInvoke(); Name = $name })
    }

    # Collect results keyed by test name, then emit in stable name order.
    $results = @{}
    foreach ($h in $handles) {
        try   { $res = $h.PS.EndInvoke($h.Async) | Select-Object -First 1 }
        catch { $res = [pscustomobject]@{ Name = $h.Name; Kind = "worker"; Ok = $false; Exit = -1; Output = ($_ | Out-String) } }
        finally { $h.PS.Dispose() }
        if (-not $res) { $res = [pscustomobject]@{ Name = $h.Name; Kind = "worker"; Ok = $false; Exit = -1; Output = "no result from worker" } }
        $results[$h.Name] = $res
    }

    $pool.Close()
    $pool.Dispose()

    foreach ($src in $testSources) {
        $name = [IO.Path]::GetFileNameWithoutExtension($src.Name)
        $res  = $results[$name]
        if ($res.Ok) {
            if ($ShowPass) { Emit ("  [PASS] {0}" -f $name) Green }
            else { $script:transcript.Add("  [PASS] " + $name) }
        } elseif ($res.Kind -eq "compile") {
            Emit ("  [FAIL] compile: {0}.cpp" -f $name) Red
            Emit $res.Output $null
            $script:fails.Add("compile " + $name + ".cpp")
        } else {
            Emit ("  [FAIL] {0} (exit {1})" -f $name, $res.Exit) Red
            Emit $res.Output $null
            $script:fails.Add("test " + $name)
        }
    }
    Emit ("  {0} unit test file(s) compiled & run across {1} worker(s)" -f $testSources.Count, $Threads) $null
}

# ---------------------------------------------------------------------------
# 2. Guard scripts + regression gate
# ---------------------------------------------------------------------------
function Invoke-Guard($fileName, $label, $extraArgs) {
    $path = Join-Path $ScriptDir $fileName
    if (-not (Test-Path $path)) {
        Emit ("  [FAIL] {0}: not found ({1})" -f $label, $fileName) Red
        $script:fails.Add($label + " missing")
        return
    }

    $argList = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $path)
    if ($extraArgs) { $argList += $extraArgs }

    $out = Invoke-Native { & powershell @argList 2>&1 }
    $code = $script:nativeExit
    foreach ($line in ($out -split "`r?`n")) { $script:transcript.Add([string]$line) }

    if ($code -eq 0) {
        Emit ("  [PASS] {0}" -f $label) Green
    } else {
        Emit ("  [FAIL] {0} (exit {1})" -f $label, $code) Red
        $script:fails.Add($label)
    }
}

Section "GUARDS"
if (-not $SkipLogLayerCheck)    { Invoke-Guard "check_log_layer.ps1"    "log-layer guard"     @("-Quiet") }
if (-not $SkipEmptyBodyCheck)   { Invoke-Guard "check_empty_body.ps1"   "empty-body guard"    @("-Quiet") }
if (-not $SkipBalanceCheck)     { Invoke-Guard "check_balance.ps1"      "balance check"       $null }
if (-not $SkipCommentSafetyCheck) { Invoke-Guard "check_comment_safety.ps1" "comment-safety guard" $null }
if (-not $SkipVersionInitCheck) { Invoke-Guard "check_version_init.ps1" "version/INIT check"  $null }
if (-not $SkipRegressionCheck)  { Invoke-Guard "regression_check.ps1"   "regression gate"     @("-SkipUnitTests") }

# ---------------------------------------------------------------------------
# 3. Verdict + logs
# ---------------------------------------------------------------------------
Emit "" $null
Emit "----------------------------------------------------" $null
if ($script:fails.Count -eq 0) {
    Emit "OVERALL : PASS" Green
    $exit = 0
} else {
    Emit ("OVERALL : FAIL ({0})" -f ($script:fails -join ", ")) Red
    $exit = 1
}

Set-Content -Path $LogFile -Value ($script:transcript -join [Environment]::NewLine) -Encoding UTF8
Emit ("transcript: {0}" -f $LogFile) $null

if (Test-Path $ErrLog) { Remove-Item -Force $ErrLog }
if ($exit -ne 0) {
    $errLines = @("last run FAILED: " + ($script:fails -join ", "),
                  "see transcript: $LogFile")
    Set-Content -Path $ErrLog -Value ($errLines -join [Environment]::NewLine) -Encoding UTF8
}

if (-not $KeepBinaries) {
    # Remove the ENTIRE tests\build tree (this run's timestamped dir, any stale
    # dirs/exes left by an interrupted run, and the empty parent) so nothing is
    # left behind. Best-effort: a freshly built exe may still be locked by an AV
    # scan; a failed cleanup must not fail the run.
    $cleanupTarget = if ($BuildRoot) { $BuildRoot } else { $BuildDir }
    if ($cleanupTarget -and (Test-Path $cleanupTarget)) {
        Remove-Item -Recurse -Force $cleanupTarget -ErrorAction SilentlyContinue
    }

    # Sweep EVERY leftover .exe anywhere under tests\ (e.g. exes built straight
    # into tests\ by an older run or a manual g++ invocation). This guarantees
    # no .exe is left behind in the test folder after verification.
    if (Test-Path $TestsDir) {
        $strayExes = Get-ChildItem -Path $TestsDir -Filter "*.exe" -File -Recurse -ErrorAction SilentlyContinue
        foreach ($stray in $strayExes) {
            Remove-Item -Force $stray.FullName -ErrorAction SilentlyContinue
            if (Test-Path $stray.FullName) {
                Emit ("  [WARN] could not remove leftover exe (locked?): {0}" -f $stray.FullName) Yellow
            } else {
                $script:transcript.Add("  [CLEAN] removed leftover exe: " + $stray.FullName)
            }
        }
    }
}

exit $exit
