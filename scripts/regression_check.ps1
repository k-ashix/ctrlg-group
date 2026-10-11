<#
    regression_check.ps1  --  One-command regression gate for ctrlg-group.

    Location: scripts\. Run from anywhere:

        powershell -ExecutionPolicy Bypass -File scripts\regression_check.ps1

    --------------------------------------------------------------------------
    What this does (and why it exists on top of folder_run_tests.ps1)
    --------------------------------------------------------------------------
    ctrlg-group.wh.cpp is a Windhawk mod: it #includes Win32 /
    Windhawk headers and has no standalone main(), so the FULL translation unit
    cannot be compiled or executed here. folder_run_tests.ps1 covers every pure
    decision that was MIRRORED into tests\*.h. But some properties are STRUCTURAL
    facts about the mod source itself (it must create a directory, not a file;
    the low-level hook must stay non-blocking; all three trigger options must
    exist). A unit test cannot see those, so a regression there would slip past.

    This script is the gate for BOTH kinds of check:

      PART 1  Run the unit-test suite (delegates to folder_run_tests.ps1).
      PART 2  Assert source-level invariants over ctrlg-group.wh.cpp.

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
$ModFile    = Join-Path $RepoRoot "ctrlg-group.wh.cpp"

$fail = 0
function Ok   ($m) { Write-Host ("  [PASS] " + $m) -ForegroundColor Green }
function Bad  ($m) { Write-Host ("  [FAIL] " + $m) -ForegroundColor Red; $script:fail++ }
function Head ($m) { Write-Host ""; Write-Host $m -ForegroundColor Cyan }

Write-Host "=== ctrlg-group regression gate ===" -ForegroundColor White

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
    Bad "ctrlg-group.wh.cpp not found at: $ModFile"
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
# A setting that is only declared + loaded + logged still does NOTHING. Use this
# to prove a group setting is also READ in the live handler: its global must
# appear MORE times than its decl/set/log sites.
function AssertAtLeast ($label, $pattern, $min) {
    $n = CountMatches $pattern
    if ($n -ge $min) { Ok ("{0} -- {1} occurrence(s) (>= {2})" -f $label, $n, $min) }
    else             { Bad ("{0} -- expected >= {1}, found {2}" -f $label, $min, $n) }
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
AssertPresent "chord replay is actually wired (called, not just defined)" 'ReplayCtrlF\(\)\s*;'
AssertPresent "hook tracks a pending chord key"              'g_chordPending'

# Settings metadata must use Windhawk's one-key-per-list-item schema.
AssertPresent "custom shortcut string setting"               '(?m)^\s*- shortcut: "Ctrl\+F\+N"\r?$'
AssertPresent "custom shortcut is read from settings"        'Wh_GetStringSetting\(L"NewFolder\.shortcut"\)'
AssertPresent "custom shortcut parser present"               'ParseShortcut'
AssertPresent "Ctrl+Q option present"                        '(?m)^\s*- "Ctrl\+Q":'
# Ctrl+G is NO LONGER a New Folder trigger. It is a dedicated, hardwired Group
# Selection key gated by its own on/off toggle, so it must NOT appear as a
# New Folder `shortcut` $options entry.
AssertAbsent  "Ctrl+G removed from New Folder shortcut options" '(?m)^\s*- "Ctrl\+G":'
# ...but Ctrl+G Group Selection must exist as its own on/off toggle setting,
# read into the hardwired hook gate.
AssertPresent "groupSelection toggle setting present"        '(?m)^\s*- groupSelection:'
AssertPresent "groupSelection read into the hook gate"       'Wh_GetIntSetting\(L"Group\.groupSelection"\)'
AssertPresent "Ctrl+G group action wired (WM_APP_GROUP)"     'WM_APP_GROUP'
AssertAbsent  "legacy forced trigger choices are gone"       '(?m)^- trigger:'
AssertAbsent  "invalid multi-key template object is gone"    '(?ms)^- folderTemplates:\s*\r?\n\s+- name:.*?\r?\n\s+folders:'
AssertPresent "template key parsed independently of name"    'key=Name=Folder'
AssertPresent "template line split on two = signs"           'line\.find\(L''=''\, firstEq \+ 1\)'

# Multi-folder creation (v0.2.0) must stay wired up.
AssertPresent "multi-folder toggle setting"                 'multiFolder'
AssertPresent "max-folders cap setting"                      'maxFolders'
AssertPresent "hard cap constant (100)"                      'kFolderHardCap'
AssertPresent "count clamp helper"                           'ResolveFolderCount'
AssertPresent "batch name helper"                            'MakeUniqueFolderNames'
AssertPresent "count-entry capture state"                    'g_countActive'
AssertPresent "count-entry commit path"                      'CommitCountCapture'
AssertPresent "auto-resolve-names toggle setting"            'autoResolveNames'

# v0.2.2 -- the count timeout must match the SetTimer-RETURNED handle, not the
# constant id that SetTimer(NULL, ...) silently ignores. If this regresses, the
# count-entry window never auto-commits and the trigger gets stuck: it does
# nothing the first time and leaks the key into Explorer the second time.
# (The old dead chord-timer machinery was removed in wh(8); only the live
# count-entry timer remains.)
AssertPresent "count timeout matches stored timer id"       'msg\.wParam == g_countTimer'

# The mod must NOT regress back to the file-creating reference behaviour.
AssertAbsent  "old file-creation API is gone"                'CreateFileW'
AssertAbsent  "old file-name helper is gone"                 'MakeUniqueFilename'

# --------------------------------------------------------------------------
# Ctrl+G Group Selection: popup-FIRST + settings actually applied (Task 8/9).
# Before this work the WM_APP_GROUP handler hard-coded a "New folder"
# destination and Copy and never showed a popup, so the Move/Copy, folder-
# collision, fixed-destination and open-after settings were inert (loaded and
# logged, never read). These pin the popup wiring and that each setting is honored.
# --------------------------------------------------------------------------
# The popup must be shown to name the folder BEFORE anything is created.
AssertPresent "Ctrl+G asks via the popup before creating"        'GroupPromptSeed'
# The destination is resolved by precedence (fixed > typed > last), not hardcoded.
AssertPresent "Ctrl+G resolves destination by precedence"        'ResolveGroupDestination'
# The folder-exists collision policy (UseExisting / Numbered / Cancel) is applied.
AssertPresent "Ctrl+G applies folder-collision policy"           'GroupResolveKind::'
# The old Task 6b placeholder (auto "New folder" + forced Copy) must be gone.
AssertAbsent  "Ctrl+G no longer auto-creates a 'New folder' group dest" 'destFolder \+= L"New folder";'
AssertAbsent  "Ctrl+G no longer hardcodes Copy"                  'const bool isMove = false;'
# Move/Copy, folder-collision, fixed-destination and open-after must be READ in
# the handler, not merely declared/loaded/logged (each already appears 2-3x).
AssertAtLeast "Ctrl+G honors Move/Copy setting"          'g_groupIsMove'      4
AssertAtLeast "Ctrl+G honors folder-collision setting"   'g_groupCollision'   4
AssertAtLeast "Ctrl+G honors fixed-destination setting"  'g_groupDestination' 3
AssertAtLeast "Ctrl+G honors open-after setting"         'g_groupOpenAfter'   4

# --------------------------------------------------------------------------
# Selected Items scrolling (waht to create.md #1/#8, Phase 2). Before this the
# panel was a STATIC pinned at listTopRow == 0 with no way to move it, so the
# unit-tested listscroll model was inert in the live popup (the "no UI change"
# report). These pin that the popup actually consumes wheel + nav-key scrolling
# through listscroll:: and rebuilds the panel from the shared builder.
# --------------------------------------------------------------------------
AssertPresent "Selected Items scrolls on mouse wheel"        'WM_MOUSEWHEEL'
AssertPresent "wheel scroll uses tested listscroll model"    'listscroll::OnWheel'
AssertPresent "key scroll uses tested listscroll model"      'listscroll::OnKey'
AssertPresent "scroll refreshes the Selected Items panel"    'RefreshGroupPanel'

# --------------------------------------------------------------------------
# Compact/Expanded runtime toggle (waht to create.md #15, Phase 7). d->expanded
# existed but nothing ever flipped it, so the popup only rendered compact. These
# pin that ONE centralized relayout path exists and a key actually toggles the
# mode live through it.
# --------------------------------------------------------------------------
AssertPresent "compact/expanded relayout path exists"       'ApplyPromptLayout'
AssertPresent "relayout path is actually called"            'ApplyPromptLayout\(hwnd'
AssertPresent "a key toggles compact/expanded live"         'expanded = !'
# fix thse isseue.md #4: a FRESH popup must auto-size PURELY by selection count
# (<=4 compact, >4 expanded). Only a MANUAL toggle (button 103 / Ctrl+E) sets the
# remembered session mode, so a stale Expanded cannot leak into the next fresh
# popup. Pinned: the session flag DEFAULTS to "no manual choice", and the initial
# size is seeded through the tested mode::InitialMode with that flag (the pure
# auto-vs-remembered contract is unit-tested in mode_logic_test).
AssertPresent "session mode defaults to no manual choice (first popup auto-expands, #4)" 'g_sessionHasMode = false'
AssertPresent "fresh popup size seeded via tested mode::InitialMode (#4)" 'mode::InitialMode\(\s*[\r\n ]*g_sessionHasMode'

# --------------------------------------------------------------------------
# Sectioned cards (waht to create.md #3/#5, Phase 3). The Group popup must give
# Destination its OWN control (a labelled read-only field placed by the layout
# model's dest rects) instead of a text line buried in the Selected Items blob.
# --------------------------------------------------------------------------
AssertPresent "layout model has a Destination field rect"      'Rect destField;'
AssertPresent "Destination field control is created"           'd->destField = CreateWindowExW'
AssertPresent "Destination field text uses tested dest:: model" 'GroupDestinationFieldText'
AssertAbsent  "old destination text-line helper removed"       'GroupDestinationLineText'
# fix thse isseue.md #2: the popup must PREVIEW the captured parent as the
# effective destination (fixed > effective > last). The worker seeds the parent
# into GroupPromptSeed.effectiveDest, the popup copies it into NamePromptData,
# and the field text passes it to dest::ComputeDestination -- so an empty fixed
# dest shows the real parent instead of the "Enter a folder name below"
# placeholder. The pure precedence is unit-tested (destination_logic_test); the
# worker->popup->field wiring is clang-only, hence these source invariants.
AssertPresent "worker seeds the captured parent as the destination preview (#2)" 'seed\.effectiveDest = parent'
AssertPresent "popup threads effectiveDest into the display model (#2)"          'data\.effectiveDest = group->effectiveDest'
AssertAbsent  "destination preview no longer hard-codes an empty effective dest (#2)" 'ComputeDestination\(destPath, std::wstring\(\)'

# fix thse isseu e.md #1: each info card stacks title -> description -> field.
# The supporting description is its OWN layout rect BETWEEN the label and the
# field (it used to be painted UNDER the field, which cramped/clipped it). The
# paint must target the description rects, not field.bottom().
AssertPresent "layout model has a Destination description rect" 'Rect destDesc;'
AssertPresent "layout model has a Folder-name description rect" 'Rect nameDesc;'
AssertPresent "dest description painted in its own row"         'L\.destDesc'
AssertPresent "name description painted in its own row"         'L\.nameDesc'
AssertAbsent  "description no longer painted under the field"   'L\.destField\.bottom\(\) \+ 1'

# fix thse isseu e.md #2 (path pass): the Destination field reads as a read-only
# INFO field showing the path PLAINLY -- NO folder glyph ("no icon path"). The
# path text begins at the edge pad and a separate "Copy" button sits to its
# right. Geometry + policy come from the tested destfield:: + layout:: models.
AssertPresent "destfield info-field model present"              'namespace destfield'
AssertPresent "field text inset uses destfield::TextLeftInset"  'destfield::TextLeftInset'
AssertPresent "field is not keyboard-focusable (info only)"     'destfield::NoKeyboardFocus'
AssertPresent "read-only path field keeps its native-EDIT subclass hook" 'DestFieldSubclassProc'
AssertAbsent  "no folder-glyph icon column in the path field (#2 no-icon)" 'destfield::IconCell'
# fix thse isseue.md #2 (path pass): a compact "Copy" button to the right of the
# reduced path field copies the resolved path to the clipboard (id 111).
AssertPresent "layout model has a copy-path button rect (#2)"   'Rect destCopyBtn;'
AssertPresent "copy-path button control is created (#2)"        'd->destCopyBtn = CreateWindowExW'
AssertPresent "copy-path button relayout target (#2)"           'deferMove\(d->destCopyBtn'
AssertPresent "copy-path button writes the path to the clipboard (#2)" 'CopyPathToClipboard'
# fix thse isseue.md (path pass): the Copy button is theme-aware -- owner-drawn
# by the SAME renderer as Create/Cancel (id 111 routed into that WM_DRAWITEM
# branch), so it matches their rounded, themed visuals in light + dark.
AssertPresent "copy button owner-drawn like Create/Cancel (#2)" 'BS_OWNERDRAW'
AssertPresent "copy button themed by the shared button renderer (#2)" 'dis->CtlID == 111'

# Copy/Move segmented control (Phase 4): the tested segmented:: keyboard model
# (Left -> Copy, Right -> Move when focused) must be consumed live so keyboard
# and mouse drive the SAME Copy/Move state; the layout gives it segment rects.
AssertPresent "Copy/Move keyboard uses tested segmented:: model" 'segmented::OnKey'
AssertPresent "layout model has Copy/Move segment rects"         'Rect segCopy;'

# Phase 6/D (fix thse isseu e.md #7): the single Copy/Move toggle button is
# replaced by TWO coordinated owner-drawn segments (ids 107 Copy, 108 Move).
# Clicking a segment goes through segmented::OnClickSegment; both segments are
# created + repainted; the retired single toggle is no longer created as the
# visual control. d->isMove stays the one logical state.
AssertPresent "Copy segment control created (id 107)"           'd->segCopyBtn = CreateWindowExW'
AssertPresent "Move segment control created (id 108)"           'd->segMoveBtn = CreateWindowExW'
AssertPresent "segment labels use tested segmented:: model"     'segmented::SegmentLabel'
AssertPresent "segment click uses tested segmented:: model"     'segmented::OnClickSegment'
AssertPresent "active segment tracks segmented::FromIsMove"     'segmented::FromIsMove'
# fix thse isseu e.md #4/#6: C/M mnemonics drive the segment through the tested
# segmented::Mnemonic model, NOT a raw literal compare, and only when a segment
# owns focus (gated the same as the arrows). The modal loop must call Mnemonic.
AssertPresent "C/M mnemonic uses tested segmented:: model (#6)"  'segmented::Mnemonic'
AssertPresent "segments owner-drawn in WM_DRAWITEM (107/108)"   'dis->CtlID == 107 \|\| dis->CtlID == 108'
AssertAbsent  "single Copy/Move toggle is no longer created"    'd->groupOpToggle = CreateWindowExW'

# fix thse isseue.md #3: the Copy/Move segments + Create/Cancel are owner-draw
# controls that are NO longer region-clipped. Their owner-draw fills the HWND
# with the matching parent surface (card for the segments, panel for the
# buttons) then paints a rounded fill inside, so there is no HRGN clip and no
# corner/seam artifact. The segments still read as ONE pill: Copy rounds the
# LEFT, Move the RIGHT, meeting at a straight seam.
AssertPresent "owner-draw buttons fill the matching parent surface (#3)" 'COLORREF parentBg =.*pal\.cardBg : pal\.panelBg'
AssertPresent "Copy/Move painted as a left/right half-pill fill (#3)"    'rc.right \+ rad'
AssertAbsent  "segment region-union builder removed (#3)"                'MakeSegRgn'
AssertAbsent  "Copy/Move segments are not region-clipped (#3)"           'SetWindowRgn\(d->seg(Copy|Move)Btn'
AssertAbsent  "Create/Cancel buttons are not region-clipped (#3)"        'SetWindowRgn\((ok|cancel),'
# fix thse isseue.md #1: the Copy/Move control is ONE owner-drawn segmented
# control -- active segment accent-filled (segmented::FromIsMove, above). The
# old 1px divider line that was painted ON TOP of the sliding pill (it cut a
# visible seam through the blue pill, so the control read as two layered
# halves) is REMOVED. Separation now comes from the accent pill vs the neutral
# track, and the pushed-out single-side rounding already clips both halves to a
# clean borderless straight seam -- one continuous pill, no line through it.
AssertAbsent  "no divider line painted through the sliding pill (fix thse isseue.md #1)" 'const int seamX = \(dis->CtlID == 107\)'
AssertAbsent  "segments no longer both fully rounded"          'CreateRoundRectRgn\(0, 0, S\(L\.segMove\.w\) \+ 1, S\(L\.segMove\.h\) \+ 1, S\(14\), S\(14\)\)'

# Phase 7/E (fix thse isseu e.md #4/#5/#6): reworded section labels. The name
# label uses the tested seclbl:: wording ("Folder name", NOT the old heavy
# "Group into folder name"). Per request, NEITHER the Destination NOR the
# Folder-name card paints a supporting description row -- each card is just
# icon + title + its field/control (Destination also gets the Copy button).
AssertPresent "name label uses tested seclbl:: wording"        'seclbl::NameLabel'
AssertAbsent  "Folder-name card paints no description row (not needed)" 'seclbl::NameDescription'
AssertAbsent  "Destination card paints no description row (not needed)" 'dest::SectionDescription'
AssertAbsent  "old heavy 'Group into folder name' label gone"  'L"Group into folder name"'

# fix thse isseu e.md #6: a real ACTION card -- title + description + the
# Copy/Move segments + a keyboard hint -- with Create/Cancel moved to their own
# bottom action row. Geometry comes from the layout model; wording from seclbl::.
AssertPresent "layout model has an Action card rect"          'Rect actionCard;'
AssertPresent "layout model has an Action title rect"         'Rect actionTitle;'
AssertPresent "layout model has a keyboard-hint rect"         'Rect keyboardHint;'
AssertPresent "Action card is painted in the card stack"      'L\.actionCard'
AssertPresent "Action title uses tested seclbl:: wording"     'seclbl::ActionLabel'
AssertPresent "Action description uses tested seclbl:: wording" 'seclbl::ActionDescription'
AssertPresent "keyboard hint uses tested seclbl:: wording"    'seclbl::ActionKeyboardHint'

# fix thse isseue.md #1: the Status/Ready feature was REMOVED. There is no
# WM_APP_PROMPT_STATUS message, no SetPromptStatus helper, no status:: state
# machine, no status card (icon/label/progress track) and no per-state status
# glyphs. The Group operation now runs on the worker AFTER the popup closes and
# its real result is logged there (honest success/failure, no fabrication).
AssertAbsent  "no WM_APP_PROMPT_STATUS message (#1)"            'WM_APP_PROMPT_STATUS'
AssertAbsent  "no SetPromptStatus helper (#1)"                 'SetPromptStatus'
AssertAbsent  "no status:: state machine namespace (#1)"       'namespace status \{'
AssertAbsent  "no status card label control (#1)"              'd->statusLabel = CreateWindowExW'
AssertAbsent  "no progress-bar rect in the layout model (#1)"  'Rect progressBar;'
AssertAbsent  "no progress track control (id 106) (#1)"        'dis->CtlID == 106'
AssertAbsent  "no per-state status glyph icon ids (#1)"        'StatusRing, StatusSpinner'
AssertAbsent  "no StatusGlyphIcon mapper (#1)"                 'StatusGlyphIcon'
# the one-submit / duplicate-Create lockout is preserved (btn:: model).
AssertPresent "duplicate Create is locked out (btn:: model)"   'btn::AcceptCreate'

# fix thse isseue.md #4: the primary button caption is decided by the tested
# seclbl::SubmitLabel model ("Group" in Ctrl+G, "Create" in New Folder) and
# wired on d->isGroup at the single IDOK button-creation site.
AssertPresent "primary button caption uses tested seclbl::SubmitLabel (#4)"  'seclbl::SubmitLabel\(d && d->isGroup\)'
AssertPresent "submit-caption model distinguishes Group vs Create (#4)"      'SubmitLabel\(bool isGroup\) \{ return isGroup \? L"Group" : L"Create"; \}'

# Expanded / DPI reflow moves the NEW section controls too (Phase 7): the one
# relayout path repositions the Destination + Status controls, not just the
# original form, so expand and monitor-DPI changes keep every card aligned.
AssertPresent "relayout moves the Destination field"           'deferMove\(d->destField'
AssertAbsent  "relayout no longer moves a Status label (#1)"   'deferMove\(d->statusLabel'

# fix thse isseue.md P0 #2 + S12 (v0.12.52): NEITHER EDIT control is HRGN-clipped
# anymore. The Destination field never had a region; S12 also removed the New
# Folder EDIT's SetWindowRgn (a GDI clip = stair-stepped corners at high DPI).
# Every field now gets its rounded look from a parent-painted FILLED rounded
# surface in editBg + a border (group fields AND the New Folder popup), so the
# rectangular EDIT reads rounded without any hard-edged region clip.
AssertAbsent  "Destination field is NOT region-clipped (P0 #2)"           'roundRgn\(d->destField'
AssertAbsent  "Destination field has no create-time SetWindowRgn (P0 #2)"  'SetWindowRgn\(d->destField'
AssertAbsent  "New Folder EDIT is no longer region-clipped (S12)"          'SetWindowRgn\(d->edit'
AssertAbsent  "the roundRgn relayout clip helper is gone (S12)"            'roundRgn\(d->edit'
AssertPresent "group name field surface filled in editBg (P0 #2)"          'RoundRect\(hdc, nfr.left'
AssertPresent "group Destination field surface filled in editBg (P0 #2)"   'RoundRect\(hdc, dfr.left'
AssertPresent "New Folder popup paints its own AA field surface fill (S12)" 'DrawRoundSurfaceAA\(hdc, nfr, aarender::Surface::Button,\s*nfRad, npal\.editBg'
AssertPresent "New Folder field border via the AA stroke layer (S12)"       'DrawRoundStrokeAA\(hdc, nfr, aarender::Surface::FocusRing,\s*nfRad, nfBorder'
AssertAbsent  "no progress track RoundRect fill (#1)"          'RoundRect\(hdc, fill.left'

# fix thse isseue.md #4: ApplyWindowPolish sets the custom borderless Group
# popup's DWM border to DWMWA_COLOR_NONE (transparent), so the compositor draws
# NO perimeter -- removing the stray ~1-2px white/black outline that used to
# track the theme. The pure attribute/policy is unit-tested (dwm_border_test);
# the Win32 call is clang-only, hence these source invariants.
AssertPresent "dwmborder:: mirrored into the mod (P0 #5)"                 'namespace dwmborder'
AssertPresent "ApplyWindowPolish takes an isGroup border flag (P0 #5)"   'ApplyWindowPolish\(HWND hwnd, bool dark, bool glass, bool isGroup\)'
AssertPresent "DWM border colour set only for the Group popup (P0 #5)"    'dwmborder::AppliesTo\(isGroup\)'
AssertPresent "DWM border is transparent via DWMWA_COLOR_NONE (#4)"       'dwmborder::kColorNone'
AssertPresent "DWM border uses the tested DWMWA_BORDER_COLOR attr (P0 #5)" 'dwmborder::kAttrBorderColor'
AssertPresent "polish call threads the Group flag (P0 #5)"                'ApplyWindowPolish\(hwnd, IsDarkMode\(\), data.glass, data.isGroup\)'
# fix thse isseue.md P0 #3 / #1: the FOUR Group cards (Selected, Destination,
# Folder name, Action) are painted by the ONE shared DrawSectionCard helper in a
# single loop at the DPI-scaled card:: radius -- no per-card bespoke painting, no
# extra border pass, no square rectangle over the rounded card. The Status card
# was removed (#1). card:: is unit-tested; the single-helper loop is pinned here.
AssertPresent "all four Group cards share the one card renderer loop (P0 #3)"  'L.selectedCard, L.destCard, L.nameCard,'
AssertPresent "shared card loop uses the DPI-scaled card:: radius (P0 #3)"     'ScaleDpi\(card::kCardRadius, cdpi\)'

# fix thse isseue.md P0 #4: the Group EDITs must NOT lay an opaque rectangle
# over the parent-painted rounded host. WM_CTLCOLOREDIT returns a HOLLOW brush
# (transparent bg) for the Group popup, and the two EDIT subclasses erase a
# ROUNDED surface themselves (EraseRoundedFieldSurface), so corners stay rounded
# AFTER the control's own paint. inputfield:: is unit-tested (input_field_test);
# the Win32 wiring is pinned here. New Folder keeps its opaque fill (unchanged).
AssertPresent "inputfield:: surface policy mirrored into the mod (P0 #4)"       'namespace inputfield'
AssertPresent "Group EDIT uses the opaque field-surface brush, not a hollow one (#4)"     'return reinterpret_cast<LRESULT>\(ThemeBrush\(pal.editBg\)\);'
AssertPresent "reusable inset-EDIT frame helper defined (#4)"                      'FrameRect InnerEditRect'
AssertAtLeast "all three prompt EDITs are inset inside their frame (#4)"    'inputfield::InnerEditRect' 3
AssertAbsent  "EDIT-self-erased rounded surface helper is gone (#4)"           'EraseRoundedFieldSurface'
AssertPresent "transparent EDIT bg scoped to the Group popup (P0 #4)"           'if \(d && d->isGroup\) \{'

# fix thse isseue.md P0 #2: the DWMWA_BORDER_COLOR (DWMWA_COLOR_NONE) call is
# best-effort -- older Win10 (pre-22000) rejects it. ApplyWindowPolish now
# CHECKS + records its HRESULT (never assumes DWMWA_COLOR_NONE took effect) via
# the tested dwmborder::BorderColorAccepted, and PromptForFolderName logs it, so
# the chosen rendering is explainable from the log alone.
AssertPresent "DWM border-colour HRESULT captured, not assumed (P0 #2)"       'result.borderColorHr ='
AssertPresent "border acceptance via tested dwmborder::BorderColorAccepted (P0 #2)" 'dwmborder::BorderColorAccepted'
AssertPresent "DWM border-colour result is logged per popup (P0 #2)"          'borderColorHRESULT=0x%08X'

# fix thse isseue.md P0 #2: the custom borderless Group popup must NOT carry
# WS_EX_DLGMODALFRAME -- that raised 3-D modal frame painted the stray bright
# outer perimeter the issue reports. ONE shared ex-style helper (outeroutline::,
# unit-tested in outer_outline_test) feeds BOTH the outer-size math and the real
# CreateWindowExW, so the assumed frame can never disagree with the created one.
AssertPresent "outeroutline:: shared ex-style mirrored into the mod (P0 #2)"   'namespace outeroutline'
AssertPresent "prompt ex-style comes from the tested shared helper (P0 #2)"    'outeroutline::PromptExStyle\(data.isGroup\)'
AssertPresent "shared ex-style feeds the outer-size math (P0 #2)"              'exStyle, data.dpi\);'
AssertPresent "shared ex-style feeds the real CreateWindowExW (P0 #2)"         'exStyle, kClass,'
AssertAbsent  "Group popup no longer hard-codes WS_EX_DLGMODALFRAME (P0 #2)"   'WS_EX_DLGMODALFRAME \| WS_EX_TOPMOST'

# fix thse isseue.md #7: the popup lets the Win11 ACRYLIC backdrop show through
# (a transparent, premium, minimal window instead of a flat opaque "boxed"
# panel) only when it will truly composite -- Win11 + backdrop requested + the
# user's Transparency effects ON -- else it keeps the solid themed panel (safe
# fallback). backdrop:: is unit-tested; these pin the mirror + the Win32 wiring.
AssertPresent "backdrop:: Win11 translucency policy mirrored (#7)"        'namespace backdrop'
AssertPresent "acrylicActive is driven by the honest paint-mode decision (#1/#7)" 'data.acrylicActive = paintmode::UsesTitleBandBackdrop\(paintMode\)'
AssertPresent "backdrop material is the Win11 acrylic DWMSBT (#7)"        'backdrop::kBackdropAcrylic'
AssertPresent "Transparency-effects user setting is honoured (#7)"        'EnableTransparency'

# fix thse isseue.md #1/#2/#3: the honest-HRESULT backdrop wiring. The mod must
# NOT treat the OS version + the Transparency setting alone as proof the backdrop
# composited: ApplyWindowPolish now RETURNS the DwmSetWindowAttribute result,
# records SUCCEEDED(hr), extends the DWM frame into the DPI-scaled title band,
# and acrylicActive is ANDed with that real result. WM_ERASEBKGND keeps the
# content solid below the band (#2). fix thse isseue.md v0.12.60: the caption
# buttons no longer paint a PERMANENT grey/black backplate -- the idle surface
# now MATCHES the band beneath it (transparent BLACK under a confirmed DWM
# backdrop so the Win11 material composites through, else the solid themed
# panel), so an idle button blends into the title bar while the child HWND stays
# fully hit-testable. These are clang-only Win32 wiring, hence source invariants.
AssertPresent "ApplyWindowPolish returns the explicit backdrop result (#1/#2)" 'static BackdropResult ApplyWindowPolish'
AssertPresent "backdrop request HRESULT is recorded, not assumed (#1)"    'result.backdropAttributeAccepted = SUCCEEDED\(result.backdropHr\)'
AssertPresent "DWM frame extended into the client title band (#1)"        'DwmExtendFrameIntoClientArea'
AssertPresent "title-band frame extent uses the tested TitleBandH (#1)"   'ScaleDpi\(layout::TitleBandH\(\), DpiForWindow\(hwnd\)\)'
AssertPresent "HRESULT-checked backdrop decision mirrored into the mod (#1)" 'inline bool SkipSolidFillChecked'
AssertPresent "backdrop keeps the content solid below the band (#2)"      'FillRect\(hdc, &below, PanelBrush'
AssertPresent "idle caption fill MATCHES the band -- no permanent grey/black backplate (#1)" 'const bool bandBackdrop = d->isGroup && d->acrylicActive;'
AssertPresent "idle caption fill is transparent BLACK under a confirmed backdrop, else solid panel (#1)" 'HBRUSH bg = bandBackdrop\s*\?\s*static_cast<HBRUSH>\(GetStockObject\(BLACK_BRUSH\)\)\s*:\s*CreateSolidBrush\(pal.panelBg\)'
AssertPresent "caption fill is painted edge-to-edge so the child HWND stays hit-testable (#1)" 'FillRect\(dis->hDC, &dis->rcItem, bg\)'

# fix thse isseue.md P0 #1/#2/#3/#4: the HONEST background-paint-mode contract.
# The ambiguous single-bool backdrop result was replaced by an explicit
# BackdropResult whose DwmExtendFrameIntoClientArea HRESULT is CHECKED (no longer
# discarded), and the pure SolidTheme/DwmTitleBand decision lives in paintmode::
# (mirrored from tests\paint_mode_logic.h, unit-tested by paint_mode_logic_test).
# The New Folder popup ALWAYS paints a solid themed client area (no white body);
# the Group title band drops its fill ONLY when BOTH DWM calls succeeded (no
# white title strip). A per-popup diagnostic makes the chosen mode explainable
# from the log alone.
AssertPresent "paintmode:: paint-mode policy mirrored into the mod (#1/#2/#3)" 'namespace paintmode'
AssertPresent "explicit SolidTheme/DwmTitleBand paint modes exist (#1/#2/#3)"  'enum class PaintMode \{ SolidTheme, DwmTitleBand \}'
AssertPresent "New Folder (non-group) ALWAYS falls back to SolidTheme (#1)"    'if \(!isGroup\) \{\s*return PaintMode::SolidTheme;'
AssertPresent "paint mode is chosen from the confirmed DWM state (#1/#2/#3)"   'paintmode::ChoosePaintMode\('
AssertPresent "frame-extension HRESULT is CHECKED, not discarded (#1)"         'result.frameExtensionSucceeded =\s*SUCCEEDED\(result.frameExtensionHr\)'
AssertPresent "Group band backdrop requires BOTH DWM calls (#2)"              'paintmode::GroupBandCanUseBackdrop\(\s*isGroup, result.backdropAttributeAccepted'
AssertPresent "WM_ERASEBKGND gates the band on the confirmed backdrop (#3)"    'const bool groupBandBackdrop = d && d->isGroup && d->acrylicActive;'
# fix thse isseue.md P0 #1: the CONFIRMED-backdrop Group title band must be handed
# TRANSPARENT pixel data for the DWM extended frame. GDI writes no alpha, so a
# BLACK (0,0,0) fill reads back as fully transparent and the Win11 material
# composites through it; leaving the band untouched is what left a WHITE caption
# strip (Microsoft "Custom Window Frame Using DWM"). The black band fill is Win32
# wiring, hence a source invariant; the themed panel BELOW the band stays solid
# (FillRect ... &below ... PanelBrush, pinned above).
AssertPresent "Group title band is filled BLACK for the DWM extended frame (P0 #1)"  'FillRect\(hdc, &band,\s*static_cast<HBRUSH>\(GetStockObject\(BLACK_BRUSH\)\)\)'
# fix thse isseue.md P0 #3: the title band is DPI-scaled, so the DWM custom-frame
# extension MUST be re-applied after a DPI change / activation or the transparent
# region drifts out of sync with the repainted band. A dedicated helper re-extends
# to the LIVE DPI and is wired into WM_DPICHANGED + WM_ACTIVATE (gated on a
# confirmed backdrop). Win32 wiring, hence source invariants.
AssertPresent "DWM frame extension is REAPPLIED on DPI/activation changes (P0 #3)"   'static bool ReextendTitleBandFrame\(HWND hwnd, UINT dpi\)'
AssertPresent "title-band frame re-extension is wired to the live DPI (P0 #3)"       'ReextendTitleBandFrame\(hwnd, d->dpi\)'
AssertPresent "per-popup backdrop diagnostic is logged (#4)"                   'Wh_Log\(L"NamePrompt backdrop:'
AssertPresent "diagnostic logs the explicit backgroundPaintMode (#4)"          'backgroundPaintMode=%ls'
AssertAbsent  "New Folder no longer skips its own client-area fill (#1)"       'if \(!d->isGroup\) \{\s*return 1;'

# Focus COLOUR + contextual validation (fix thse isseue.md #4): the painted
# focus RING on the owner-draw controls was removed; keyboard-focus-visible now
# drives a focus COLOUR change (accent border + kFocusPct fill nudge). The focus
# STATE + keyboard gate (focusvis::) are kept, only the outline is gone. Pre-
# Create validation still uses the tested namevalid:: model on the name label.
AssertAbsent  "owner-drawn focus RING geometry helper removed (#4)"       'focus::ScaleRing'
AssertAbsent  "owner-drawn focus RING outline paint removed (#4)"         'DrawRoundStrokeAA\(hdc, fr, aarender::Surface::FocusRing'
AssertPresent "focus feedback is a COLOUR change, not an outline (#4)"    'if \(focusColor\)'
AssertPresent "focus COLOUR uses the tested kFocusPct accent tint (#4)"   'hovertint::kFocusPct'
AssertPresent "pre-Create validation uses tested namevalid::"  'namevalid::ValidateGroupName'

# Phase 3a clip fix (fix thse isseu e.md #1): ComputePromptLayout() returns a
# CLIENT (content) size. The popup was passing it straight to CreateWindowExW /
# SetWindowPos as the OUTER size, so WS_CAPTION ate the client area and the
# bottom action row was clipped. The mod now converts client -> outer via
# AdjustWindowRectExForDpi through ONE helper used at BOTH the create and the
# relayout sites. If either regresses, the popup clips again.
AssertPresent "popup converts client->outer size (no clip)"    'AdjustWindowRectExForDpi'
AssertAtLeast "outer-size helper used at create + relayout"    'PromptOuterSize\('  3

# Phase 4/B (fix thse isseu e.md #2): the Group popup must PAINT each section as
# its OWN rounded card, not just define layout rects over one flat panel. A
# reusable DrawSectionCard renderer is defined AND called for the card stack, and
# the theme palette carries a distinct card surface + border (light AND dark).
AssertAtLeast "section card renderer defined + called"         'DrawSectionCard'   2
AssertPresent "card surface token in the theme palette"        'cardBg'
AssertPresent "card border token in the theme palette"         'cardBorder'
AssertPresent "card radius mirrors tested card:: model"        'card::kCardRadius'

# Phase 5/C (fix thse isseu e.md #3/#4/#14): the Selected Items panel must be a
# REAL owner-drawn scrollable list -- NOT a multi-line text STATIC pretending to
# be a list. It is an SS_OWNERDRAW control (id 110) whose WM_DRAWITEM paints
# three visual levels (title + counts + rows) with real column geometry from the
# tested listcol:: model, and the scroll refresh invalidates that same control.
AssertPresent "Selected Items list is owner-drawn (id 110)"    'dis->CtlID == 110'
AssertPresent "Selected Items list uses SS_OWNERDRAW"          'SS_OWNERDRAW'
AssertPresent "list rows use tested listcol:: column model"    'listcol::NameWidth'
AssertPresent "list name width derives from the LIVE row width (#1)" 'listcol::NameWidthFrom\(\s*rowRight - left'
AssertAbsent  "list name width no longer uses the fixed compact content width (#1)" 'kContentW - layout::kCardInset'
AssertPresent "list row name cell uses tested sel:: model"     'sel::RowName'
AssertPresent "list counts breakdown uses tested sel:: model"  'sel::SummaryCounts'
AssertPresent "Selected Items summary is an actionable type breakdown (#11)" 'sel::TypeBreakdown'
AssertPresent "Selected Items list draws a visible scrollbar thumb (#4)" 'listscroll::ThumbRect'
AssertPresent "scrollbar gutter reserved via tested listscroll:: model (#4)" 'listscroll::(TrackRect|ContentWidth)'

# --- fix thse isseue.md "Production UI pass" #1: adaptive Selected Items height ---
# The list is CONTENT-SIZED: height = visibleRows * row height, visibleRows =
# min(itemCount, rows-for-mode). adaptive_list_test pins the pure math; these
# invariants pin the clang-only wiring the unit test cannot see.
AssertPresent "Selected list height is content-sized (#1)"            'visRows \* kSelRowH'
AssertPresent "visible row count is min(itemCount, mode cap) (#1)"     'int visRows = itemCount < capRows'
AssertAbsent  "fixed kSelListH + extra viewport band is gone (#1)"     'kSelListH \+ extra'
AssertPresent "content-sized row helper mirrored into the mod (#1)"   'VisibleRowsFor'
AssertPresent "layout is threaded the live selection count (#1)"      'static_cast<int>\(d->items\.size\(\)\)'

# --- fix thse isseue.md "Production UI pass" #2: flicker-free list scrolling ---
# The scroll refresh must NOT erase the background (TRUE), and the owner-draw
# must paint the whole list into a memory DC and BitBlt it once. Pure rendering,
# so it is clang-only -> pinned here as source invariants.
AssertPresent "Selected list scroll refresh does NOT erase background (#2)" 'InvalidateRect\(d->groupInfo, nullptr, FALSE\)'
AssertAbsent  "old background-erasing list refresh is gone (#2)"            'InvalidateRect\(d->groupInfo, nullptr, TRUE\)'
AssertPresent "Selected list is double-buffered into a memory DC (#2)"      'CreateCompatibleBitmap\(target, bbW, bbH\)'
AssertPresent "double-buffer flushed to the control in one BitBlt (#2)"     'BitBlt\(target, rc.left, rc.top, bbW, bbH'
AssertPresent "per-row item icon uses production vector renderer (#3/#10)" 'it\.isFolder\s*\?\s*icons::IconId::Folder\s*:\s*icons::IconId::File'
AssertPresent "section titles use the Windows-native semantic icon renderer (v0.12.57)" 'DrawSemanticSectionIcon\(hdc,\s*semanticicon::Section::(Destination|Name|Action)'
# fix thse isseue.md #1: the Status card + its "Status" heading were removed, so
# the mod no longer paints a seclbl::StatusLabel heading anywhere.
AssertAbsent  "no Status heading paint (seclbl::StatusLabel) (#1)" 'seclbl::StatusLabel'
# fix thse isseue.md #6: every vector icon sizes its stroke from ONE rule
# (icons::IconStrokeWidth) instead of the old per-icon "u > 12 ? 2 : 1" cliff, so
# all icons share a consistent weight at a given DPI. The rule is unit-tested
# (icon_logic_test); these pin that the renderer actually uses it.
AssertPresent "icon stroke width is ONE consistent rule (#6)"   'IconStrokeWidth\(u\)'
AssertAbsent  "old per-icon stroke-width cliff removed (#6)"     'u > 12 \? 2 : 1'
AssertPresent "size control uses the tested window-chrome rect (#5)" 'L\.chromeSizeBtn'
# fix thse isseue.md A: the caption controls keep the >=28px hit floor
# (kChromeBtnSize) but the click area is WIDER (kChromeBtnW ~45) and spans the
# FULL band height (chromeBtnH = TitleBandH()), so no dead strip above or below
# swallows a click. The buttons are CONTIGUOUS (kCaptionButtonGap = 0, the
# dedicated zero gap -- NOT the general kButtonGap), the hover/press colour
# feedback fills the FULL button rectangle (chromebtn::FillArea, guard 0 -- no
# inset, no blank strips between the hit area and the visible fill), and the
# title is clipped BEFORE the leftmost caption button so it never overlaps the
# Minimize control. The title band is the native 32px height (TitleBandH() =
# kGroupTopMargin 14 + kChromeBand 18). The glyph is a deliberate, centered,
# DPI-scaled size via CaptionGlyphBox.
AssertPresent "title-bar hit floor kept at >=28px (#6)"           'kChromeBtnSize = 28'
AssertPresent "caption click area is WIDER (native ~45, fix)"     'kChromeBtnW\s*=\s*44'
AssertPresent "caption buttons span the FULL header height (fix)" 'chromeBtnH = TitleBandH\(\)'
AssertPresent "title band is the native 32px height (fix thse isseue.md A)" 'kChromeBand\s*=\s*18'
AssertPresent "caption buttons are CONTIGUOUS via a dedicated 0 gap (fix thse isseue.md A)" 'kCaptionButtonGap = 0'
AssertPresent "caption inter-button gap uses kCaptionButtonGap, not kButtonGap (fix thse isseue.md A)" 'chromeGap  = kCaptionButtonGap'
AssertPresent "caption hover/press fill is the FULL button rect (no inset, fix thse isseue.md A/D)" 'chromebtn::FillArea\('
AssertPresent "title text clips BEFORE the leftmost caption button (fix thse isseue.md A)" 'ScaleDpi\(L.chromeMinBtn.x, cdpi\)'
AssertPresent "caption glyph is a centered, sized box (fix)"      'CaptionGlyphBox\('
AssertPresent "size control is a flat window-chrome affordance (#5)" 'BS_FLAT'
AssertPresent "compact group popup width is ~410 (#7)"               'kGroupCompactW\s*=\s*410'
AssertPresent "expanded group popup is WIDER ~660 (#8)"             'kGroupExpandedW\s*=\s*660'
AssertPresent "popup width is chosen by compact/expanded mode (#7/#8)" 'expanded \? kGroupExpandedW : kGroupCompactW'
AssertPresent "content/card widths derive from the mode width (#11/#12)" 'contentW = winW - kOuterMargin'

# Pass5 S8 (fix thse isseue.md #9): file/folder ROWS render the REAL Windows
# Shell icon (SHGetFileInfoW + DrawIconEx + DestroyIcon), via a no-disk
# type/extension lookup, with the custom vector kept only as a fallback. The
# UI-semantic vector slots (Status etc.) remain.
AssertPresent "row icons use the real Windows Shell image (#9)"      'SHGetFileInfoW\('
AssertPresent "row Shell lookup is no-disk + folder/file aware (#9)" 'shellicon::AttrFor\(it\.isFolder\)'
AssertPresent "row Shell flags come from the tested helper (#9)"     'shellicon::IconFlags\(\)'
AssertPresent "no-disk type lookup flag is set (#9)"                 'kSHGFI_USEFILEATTRIBUTES\s*=\s*0x000000010'
AssertPresent "row Shell icon is drawn via DrawIconEx (#9)"          'DrawIconEx\(hdc, ix, iy, rsfi\.hIcon'
AssertPresent "row Shell icon handle is freed after draw (#9)"       'DestroyIcon\(rsfi\.hIcon\)'
AssertAbsent  "no status-icon vector ids remain after removal (#1)"  'icons::IconId::Status'

# fix thse isseue.md #1: the Status card ICON and its status::ToneFor colour
# model were removed together with the Status/Ready feature.
AssertAbsent  "no status icon tone colour model (#1)"                'status::ToneFor'

# Pass5 S10 (fix thse isseue.md #11/#12): badge padding X=8 / Y=3 / minW~32, and
# a SOFT per-extension chip (soft fill + darker ink) from the tested badgecolor::
# table, inheriting the themed neutral chip for unknown types.
AssertPresent "badge X padding is 8 (#11)"                           'kPadX   = 8'
AssertPresent "badge Y inset is 3 (#11)"                             'kPadY   = 3'
AssertPresent "badge minimum pill width is 32 (#11)"                 'kMinW   = 32'
AssertPresent "badge fill is a soft per-extension chip (#12)"        'badgecolor::ChipForLabel\(ex, dark\)'
AssertPresent "badge colour categories mirrored into the mod (#12)"  'case Cat::Document:'
AssertPresent "unknown extensions inherit the themed chip (#12)"     'if \(!chip\.inherit\)'

# Pass5 S11 (fix thse isseue.md #13): final 4/8px spacing cleanup -- the one
# off-grid inter-control gap (action row + chrome strip) is cleaned 6 -> 8.
AssertPresent "action-row / chrome gap is on the 4px grid (#13)"     'kButtonGap      = 8'

# Pass5 S12 (fix thse isseue.md #14): Create/Cancel + Copy/Move segments get a
# HOVER affordance (lighten the resolved fill) on top of the existing pressed
# fill + focus ring, via the tested hovertint:: helper.
AssertPresent "action buttons compute hover state via hovertint (#14)"  'hovertint::VisFor\('
AssertPresent "only non-pressed hover repaints the fill (#14)"          'hovertint::PaintsHover\(hvis\)'
AssertPresent "hover lightens the fill channels via tested blend (#14)"  'hovertint::BlendChannel\(GetRValue'
# fix thse isseue.md P0 #1: the ring is now gated on focus-VISIBLE (keyboard
# only) via the tested focusvis:: model, so a mouse click no longer paints it.
# The owner-draw still reads ODS_FOCUS, but ONLY through focusvis::ShouldShowRing
# together with d->focusVisible; the modal loop flips focusVisible on nav keys /
# pointer presses via focusvis::IsNavKey + FromInput.
AssertPresent "focusvis:: mirrored into the mod (P0 #1)"                 'namespace focusvis'
AssertPresent "focus ring drawn ONLY when keyboard-focus-visible (P0 #1)" 'focusvis::ShouldShowRing\('
AssertPresent "owner-draw still reads ODS_FOCUS for the ring (P0 #1)"    'dis->itemState & ODS_FOCUS'
AssertPresent "nav-key / pointer classify focus-visible (P0 #1)"         'focusvis::IsNavKey\('
AssertPresent "pointer press hides the ring via FromInput (P0 #1)"       'focusvis::FromInput\(focusvis::Input::PointerPress\)'
AssertAbsent  "no un-gated ODS_FOCUS ring (mouse outline) remains (P0 #1)" 'if \(dis->itemState & ODS_FOCUS\)'

# Pass5 S13 (fix thse isseue.md #15): DPI 125/150/200% verification pass. The
# WM_DPICHANGED handler relayouts through the tested dpiutil decode (every
# per-DPI metric scales via the one MulDiv rule -- see tests/dpi_verify_test.cpp).
AssertPresent "WM_DPICHANGED relayouts via the tested dpiutil decode (#15)" 'dpiutil::NeedsRelayout\(static_cast<int>\(d->dpi\), newDpi\)'
AssertAbsent  "Selected Items is no longer a text STATIC blob" 'multi-line STATIC: count \+ items \+ dest'

# Pass2 S6 (fix thse isseu e.md #1): the popup's NamePromptData must be
# HEAP-owned (a PromptSession), not a stack local. The window stores &data in
# GWLP_USERDATA and the review's live worker->popup round-trip keeps the popup
# alive AFTER PromptForFolderName() returns -- a stack `NamePromptData data;`
# would then be a use-after-return the instant WM_APP_PROMPT_STATUS arrived.
# A unit test cannot see Win32 object lifetime, so pin it here: the state is
# allocated with make_unique and the old stack-owned local is gone.
AssertPresent "popup state is heap-owned (PromptSession, #1)"  'std::make_unique<NamePromptData>'
AssertAbsent  "popup state is not a stack-owned local (#1)"    '(?m)^\s*NamePromptData data;\s*\r?$'

# fix thse isseue.md #1: the LIVE worker->popup status round-trip was REMOVED.
# On Create the popup now closes normally; the Ctrl+G worker then runs
# PerformGroupOperation and logs the real result. There is no keepOpenOnAccept,
# no GroupPromptSeed::outLiveHwnd/outSession hand-off, no status::PostState
# sender and no WM_APP_PROMPT_STATUS post. These invariants pin the removal.
AssertAbsent  "no keep-open-on-accept arming (#1)"             'keepOpenOnAccept'
AssertAbsent  "no live popup HWND hand-off (#1)"               'outLiveHwnd'
AssertAbsent  "no owning-session hand-off (#1)"                'outSession'
AssertAbsent  "no status::PostState sender (#1)"               'PostState'
AssertAbsent  "no live popup pump loop (#1)"                   'IsWindow\(\s*livePopup'

# fix thse isseue.md #1/#9: the compact<->expanded (and DPI) relayout resizes the
# popup about its CURRENT center and clamps to the monitor work area -- it must
# NOT pin the top-left with SWP_NOMOVE. A unit test cannot see SetWindowPos, so
# the pure math lives in tests/center_logic.h (center_logic_test) and the ONE
# Win32 wiring site is pinned here: the center model is mirrored + used, the old
# window rect + monitor work area are read, SWP_NOMOVE is gone, and a PromptResize
# diagnostic is logged so the centering can be proven from the Windhawk log.
AssertPresent "center-preserving resize model mirrored (#1)"        'namespace center'
AssertPresent "relayout keeps the window center (center::CenteredResize) (#1)" 'center::CenteredResize'
AssertPresent "relayout reads the current window rect (#1)"         'GetWindowRect\(hwnd, &oldRc\)'
AssertPresent "relayout clamps to the monitor work area (#1)"       'GetMonitorInfo\(mon, &mi\)'
AssertAbsent  "relayout no longer pins the top-left (SWP_NOMOVE gone) (#1)" 'SWP_NOMOVE'
AssertPresent "resize is logged for diagnosis (PromptResize) (#9)"  'PromptResize:'

# fix thse isseue.md #2/#1: the Group popup drops the native caption for a
# borderless WS_POPUP and paints its OWN title bar (title + Expand/Minimize/
# Close) that drags via WM_NCHITTEST. The pure hit classification is unit-tested
# (titlebar_logic_test); the Win32 wiring a unit test cannot see is pinned here.
AssertPresent "custom title-bar model mirrored (#2)"                 'namespace titlebar'
AssertPresent "Group popup is a borderless WS_POPUP title bar (#2)"  'data\.isGroup\s*\?\s*WS_POPUP'
AssertPresent "popup create uses the chosen title-bar style (#2)"    'winStyle,'
AssertPresent "title-bar Minimize control created (id 104) (#1/#2)"  'd->minBtn = CreateWindowExW'
AssertPresent "title-bar Close control created (id 105) (#2)"        'd->closeBtn = CreateWindowExW'
AssertPresent "title-bar Minimize/Close owner-drawn (104/105) (#2)"  'dis->CtlID == 104 \|\| dis->CtlID == 105'
# fix thse isseue.md: the three caption buttons are PLAIN (non-layered) owner-draw
# children. A WS_EX_LAYERED + LWA_COLORKEY(black) button was click-through on the
# acrylic band everywhere except its thin glyph, so a press missed the button,
# fell through to the drag-caption and started a window-move loop -- clicks were
# swallowed and the window felt "stuck". fix thse isseue.md v0.12.60: the idle
# fill no longer paints a permanent backplate -- it MATCHES the band (transparent
# black under a confirmed backdrop, else the solid panel) -- but it is still a
# painted, non-null surface filling the whole rect, so the child stays fully
# hit-testable; hover/pressed/red-Close still paint over that idle base.
AssertPresent "caption Close button is a PLAIN (non-layered) child so clicks are not swallowed (fix)" 'd->closeBtn = CreateWindowExW\(0, L"BUTTON"'
AssertAbsent  "caption buttons no longer colour-key to click-through transparency (fix)"             'SetLayeredWindowAttributes\(d->closeBtn'
AssertPresent "custom title bar is draggable (WM_NCHITTEST) (#2)"    'return HTCAPTION;'
AssertPresent "drag decision uses tested titlebar::Classify (#2)"    'titlebar::Classify'
AssertPresent "title-bar Minimize sends popup to the taskbar (#1)"   'ShowWindow\(hwnd, SW_MINIMIZE\)'
AssertPresent "title bar band height from tested layout::TitleBandH (#2)" 'layout::TitleBandH\(\)'

# fix thse isseue.md #3: the per-section icon sits in a reserved column INSIDE
# the card (at the card inner-left), with the title shifted right past it --
# NOT hung off the card's left border. The shifted label geometry is unit-tested
# (section_icon_test); the paint offset is pinned here.
AssertPresent "section icon drawn in the reserved in-card column (#3)" 'label\.x - layout::kSectionIconCol'

# fix thse isseue.md #7: the Folder name field reads as a real input -- a
# placeholder when empty+unfocused (edit subclass, since a de-themed EDIT ignores
# EM_SETCUEBANNER) and an accent border while focused. The pure decisions
# (placeholder text, accent-on-focus) are unit-tested (namefield_logic_test);
# the Win32 paint is pinned here.
AssertPresent "name field placeholder/border model mirrored (#7)" 'namespace namefield'
AssertPresent "name field paints a placeholder when empty (#7)"    'NameFieldSubclassProc'
AssertPresent "name field border turns accent on focus (#7)"       'namefield::BorderIsAccent'
# fix thse isseue.md #3: the Folder name field must NOT flash on click/focus.
# The EN_SETFOCUS/EN_KILLFOCUS repaint targets ONLY the field's border ring with
# erase=FALSE -- it must never fall back to a full-window erase (the old flash).
AssertPresent "name-field focus repaint targets the field rect, no erase (#3)" 'InvalidateRect\(hwnd, &er, FALSE\)'
AssertPresent "name-field focus repaint maps only the field rect (#3)"         'GetWindowRect\(d->edit, &er\)'

# fix thse isseue.md #6: the read-only Destination field reads as a defined info
# field -- a steady NEUTRAL rounded border (never a focus accent, unlike the
# editable name field) and a path ellipsis for long paths. The border policy is
# unit-tested (destfield_logic_test::BorderIsAccent); the Win32 paint + the path
# compaction are pinned here.
AssertPresent "destfield border policy mirrored into the mod (#6)"  'destfield::BorderIsAccent'
AssertPresent "Destination field path is ellipsized to the field (#6)" 'ApplyDestFieldText'
AssertPresent "destination path uses a real path ellipsis (#6)"     'PathCompactPathW'

# fix thse isseue.md #5: the Selected Items extension column is a visual BADGE
# (a rounded pill behind the uppercased ext), so a row reads [icon] name [EXT]
# size. The pill geometry + show/hide are unit-tested (list_badge_logic_test);
# the owner-draw pill paint is pinned here.
AssertPresent "ext column is a tested badge pill (#5)"              'listbadge::BadgeRect'
AssertPresent "ext badge shown only when an ext exists (#5)"        'listbadge::ShowBadge'
AssertPresent "ext badge width uses the tested pill model (#5)"     'listbadge::BadgeWidth'

# fix thse isseue.md #7: the final card-spacing polish aligns the gap BETWEEN
# stacked Group cards to the 4px scale (kSpace3), so the stack breathes evenly.
# The gap value + uniformity are unit-tested (card_spacing_test); the mod mirror
# of the constant is pinned here so it cannot drift from the model.
AssertPresent "inter-card gap aligned to the 4px scale (#7)"        'kCardGap\s*=\s*kSpace4'

# --------------------------------------------------------------------------
# fix thse isseue.md UI polish RE-REVIEW (pass 6). Each slice pins the live
# owner-draw/token wiring a unit test cannot see (the mod is a Windhawk TU).
# --------------------------------------------------------------------------
Head "PART 2 (pass 6): fix thse isseue.md UI polish re-review"

# #1: Selected Items rows get more vertical air -- the owner-draw row stride
# moves S(20) -> S(24) (kSelRowH mirror = 24), the file icon stays 16 and the
# icon->text gap is 8. The tokens are unit-tested (row_rhythm_test); the live
# owner-draw stride + the header separator breathing (#5) are pinned here.
AssertPresent "Selected Items row height is the taller 24px (pass6 #1)"      'const int rowH  = S\(24\)'
AssertPresent "Selected Items row-height mirror is 24 (pass6 #1)"            'kSelRowH\s*=\s*24'
AssertPresent "row icon -> text gap is 8 (pass6 #1)"                         'kRowIconGap = 8'
# fix thse isseue.md #1/#3: the Selected header uses an EVEN breathing rhythm
# (equal kSelHdrGap gaps around the title / type breakdown / a THIN light
# hairline divider). fix #3: the card's 16px padding now comes entirely from the
# padded selectedContent rect, so the owner-draw no longer adds its own top/left
# inset (kSelHdrTop == 0) and the list HWND is created/relayouted on
# selectedContent -- NOT the whole card -- so the parent's rounded card border
# stays intact and the content has consistent 16px padding on all four sides.
AssertPresent "Selected header inset comes from selectedContent (#3)"       'const int kSelHdrTop = 0'
AssertPresent "Selected header uses an even breathing gap (#1/#3)"          'const int kSelHdrGap = S\(4\)'
AssertPresent "Selected header divider is a thin, light hairline (#1)"      'HPEN spen = CreatePen\(PS_SOLID, 1, hairline\)'
AssertPresent "layout derives the padded Selected content rect (#3)"        'L.selectedContent = Rect\{kOuterMargin \+ kSelectedPad'
AssertPresent "Selected card content padding mirror is 20 / kSpace5 (#3)"   'kSelectedPad\s*=\s*kSpace5'
AssertPresent "Selected list HWND is created inside the padded content (#3)" 'S\(L.selectedContent.x\), S\(L.selectedContent.y\)'
AssertPresent "relayout moves the Selected list to selectedContent (#3)"    'deferMove\(d->groupInfo,   L.selectedContent\)'
AssertAbsent  "Selected list HWND no longer spans the whole card (#3)"      'deferMove\(d->groupInfo,   L.groupHeader\)'

# #3: more horizontal padding INSIDE every card -- card outer->content 12 -> 16
# (kCardInset mirror = kSpace4), while the inter-card gap stays kSpace3 (12).
# The inset + the 6px desc->field breathing are unit-tested (card_padding_test);
# the mod mirror of the constant is pinned here so it cannot drift.
AssertPresent "card inner padding is 16 / kSpace4 (pass6 #3)"                'kCardInset\s*=\s*kSpace4'

# #7: the group Folder name field is the strongest input -- a dedicated taller
# height (kNameFieldH = 32) vs the standard 30px field. New Folder is untouched.
# The height + dominance are unit-tested (name_field_proportions_test); the mod
# mirror of the constant + its use on the group name field are pinned here.
AssertPresent "group name field height mirror is 32 (pass6 #7)"              'kNameFieldH\s*=\s*32'
AssertPresent "group name field uses the taller kNameFieldH (pass6 #7)"      'innerW, kNameFieldH\}'

# fix thse isseue.md #1: the Status card (and its kStatusCardH constant) was
# removed; the Action card height mirror (kActionCardH) is unchanged and pinned.
AssertAbsent  "no Status card height constant (#1)"                          'kStatusCardH'
AssertPresent "action card grown to 116 (P0 #4)"                             'kActionCardH\s*=\s*116'

# v0.12.57 (Semantic Section Icon pass): the five section headings dropped the
# hand-drawn glyph + coloured tile for a WINDOWS-NATIVE icon -- Segoe Fluent
# Icons (Win11) -> Segoe MDL2 Assets (Win10) -> a Shell stock icon when no icon
# font is installed. The provider/mapping is the tested pure model semanticicon
# (semantic_icon_test / section_icon_coverage_test); the font/Shell paint + the
# removal of the old tile system are pinned here (the mod is a Windhawk TU).
AssertPresent "semantic-icon namespace mirrored (v0.12.57)"                  'namespace semanticicon'
AssertPresent "section icon resolved via the tested fallback model (v0.12.57)" 'semanticicon::ResolveWithFallback'
AssertPresent "section icon prefers Segoe Fluent Icons on Win11 (v0.12.57)"  'L"Segoe Fluent Icons"'
AssertPresent "section icon falls back to Segoe MDL2 Assets on Win10 (v0.12.57)" 'L"Segoe MDL2 Assets"'
AssertPresent "section icon last-resort Shell stock icon (v0.12.57)"         'SHGetStockIconInfo'
AssertPresent "SIID_* stock mirror tied to ShellApi.h by static_assert (v0.12.57)" 'static_cast<uint32_t>\(SIID_FOLDER\)'
AssertPresent "shared device-px semantic icon helper exists (v0.12.57)"      'static void DrawSemanticSectionIconDev\('

# The old coloured-tile section-heading system must be fully GONE (no dead
# production code): the sectiontile namespace, both tile painters, and the five
# custom section glyph ids are removed; headings never paint a RoundRect tile.
AssertAbsent  "old sectiontile tint system removed (v0.12.57)"               'namespace sectiontile'
AssertAbsent  "old section tile device helper removed (v0.12.57)"            'DrawSectionTileDev'
AssertAbsent  "old GDI section-tile renderer removed (v0.12.57)"             'DrawSectionIcon\('
AssertAbsent  "custom Selected section glyph id removed (v0.12.57)"          'IconId::Selected'
AssertAbsent  "custom Destination section glyph id removed (v0.12.57)"       'IconId::Destination'
AssertAbsent  "custom Name section glyph id removed (v0.12.57)"              'IconId::Name'
AssertAbsent  "custom Action section glyph id removed (v0.12.57)"            'IconId::Action'

# #9: the Copy/Move segments were text-only. Each now shows a small leading
# vector glyph (Copy/Move) before its caption, with the icon+label group centred
# as a unit via the tested segicon:: model. Create/Cancel stay text-only.
AssertPresent "segment icon namespace mirrored (pass6 #9)"                   'namespace segicon'
AssertPresent "segment caption group centred via the tested model (pass6 #9)" 'segicon::GroupLeft'
AssertAbsent  "Copy segment uses an EMOJI not the vector copy glyph (#1)"                  'icons::IconId::Copy'
AssertAbsent  "Move segment uses an EMOJI not the vector move glyph (#1)"                  'icons::IconId::Move'

# #2 / #6 / #7: radius polish. The EXT badge pill corner grows 4 -> 5 (a softer
# chip; padX/padY already 8/3), the read-only Destination field border uses an
# 8px corner, and the Folder name field (the strongest input) a 7-8px corner.
# The tokens are unit-tested (field_radius_test); the mod mirrors + the field
# RoundRect radii are pinned here.
AssertPresent "badge pill corner radius bumped to 5 (pass6 #2)"              'kRadius = 5'
AssertPresent "name field border uses the namefield radius token (pass6 #7)" 'S2\(namefield::kRadius \* 2\)'
AssertPresent "dest field border uses the destfield radius token (pass6 #6)" 'S2\(destfield::kRadius \* 2\)'

# --------------------------------------------------------------------------
# 1 x C+ mod.wh.cpp: UiFont(dpi) is DEFINED near the theme palette but first
# USED earlier, in NameFieldSubclassProc's placeholder paint. In single-pass
# C++ that is an "undeclared identifier 'UiFont'" clang error unless a forward
# declaration precedes the first use. Pin the ORDERING (decl before first call)
# so it cannot regress -- a unit test cannot see this (the mod is a Windhawk
# translation unit, not standalone), so it lives here as a source invariant.
# --------------------------------------------------------------------------
$uiFontDeclIdx = $modText.IndexOf("static HFONT UiFont(UINT dpi);")
$uiFontUseIdx  = ([regex]::Match($modText, 'UiFont\(static_cast<UINT>\(dpi\)\)')).Index
if ($uiFontDeclIdx -ge 0 -and $uiFontUseIdx -ge 0 -and $uiFontDeclIdx -lt $uiFontUseIdx) {
    Ok "UiFont forward-declared before its first use (no undeclared-identifier)"
} else {
    Bad "UiFont used before it is declared -- clang undeclared_var_use_suggest at first use"
}

# --------------------------------------------------------------------------
# fix thse isseue.md (NamePromptData ordering): EditParentIsGroup DEREFERENCES
# NamePromptData (pd->isGroup), so its DEFINITION needs the COMPLETE struct. The
# Dest/Name EDIT subclasses call it from ABOVE the struct, so it is forward-
# DECLARED up there and DEFINED just below struct NamePromptData. If the body is
# ever moved back above the struct, clang reports "unknown type name
# NamePromptData" at the reinterpret_cast (the exact 2-error build break this
# guards). A unit test cannot see this (the mod is a Windhawk TU, not
# standalone), so the ordering is pinned here -- same shape as the UiFont
# decl-before-use guard above.
# --------------------------------------------------------------------------
$epgDeclIdx   = $modText.IndexOf("static bool EditParentIsGroup(HWND hwnd);")
$structDefIdx = $modText.IndexOf("struct NamePromptData {")
$epgDefMatch  = [regex]::Match($modText, '(?m)^static bool EditParentIsGroup\(HWND hwnd\)\s*\{')
$epgDefIdx    = $epgDefMatch.Index
if ($epgDeclIdx -ge 0 -and $structDefIdx -ge 0 -and $epgDefMatch.Success -and
    $epgDeclIdx -lt $structDefIdx -and $structDefIdx -lt $epgDefIdx) {
    Ok "EditParentIsGroup fwd-declared, then defined after struct NamePromptData (complete type at pd->isGroup)"
} else {
    Bad "EditParentIsGroup body must sit AFTER struct NamePromptData -- clang 'unknown type name NamePromptData' at the reinterpret_cast"
}

# --------------------------------------------------------------------------
# "polish exact badge color": the EXT pill gets its OWN tuned chip tokens
# (badgeFill/badgeBorder) instead of borrowing the input-field groove (editBg),
# which rendered as a heavy #545454 slab in dark mode. Pin the dedicated palette
# slots, their tokens, and that the badge paint actually reads them.
# --------------------------------------------------------------------------
AssertPresent "badge chip fill slot in the palette"             'COLORREF badgeFill;'
AssertPresent "badge chip border slot in the palette"           'COLORREF badgeBorder;'
AssertPresent "badge chip tokens defined (light + dark)"        'kColBadgeDark\b'
AssertPresent "ext badge inherit-default fill is the badge chip token (#12)"   'fillCol\s+= pal\.badgeFill'
AssertPresent "ext badge inherit-default border is the badge chip token (#12)" 'borderCol\s+= pal\.badgeBorder'
AssertPresent "ext badge pill is painted from the resolved fill/border (#12)"  'CreateSolidBrush\(fillCol\)'

# --------------------------------------------------------------------------
# Pass7 S8 (fix thse isseue.md #9/#14): the extension pill HAIRLINE must be
# "less dominant" than the text ink and never a hard black outline. The tested
# badgecolor::SoftBorder blends the ink toward the fill; the mod mirrors it and
# the badge paint uses it for the border while the text keeps the full ink.
# --------------------------------------------------------------------------
AssertPresent "soft badge border helper mirrored into the mod (#9/#14)"        'inline unsigned SoftBorder\('
AssertPresent "badge border uses the softer SoftBorder tone (#9/#14)"          'badgecolor::SoftBorder\(chip\)'
AssertAbsent  "badge border is no longer the full-strength text ink (#9/#14)"  'borderCol = ink;'

# --------------------------------------------------------------------------
# Pass7 S9 (fix thse isseue.md #2/#12): custom rounded SURFACES move from GDI
# RoundRect (stair-stepped corners) to Direct2D anti-aliased geometry. The pure
# policy (which surfaces, float radius, AA-mode constant, stroke inset, fallback
# flag) is the unit-tested namespace aarender, mirrored into the mod; the live
# Direct2D paint (namespace d2d) is clang-only, hence these source invariants.
# EDIT controls + the window frame stay pure Win32. GDI RoundRect is RETAINED as
# the per-frame fallback, so the UI degrades to today's behaviour if D2D fails.
# --------------------------------------------------------------------------
AssertPresent "Direct2D is linked (#2/#12)"                                 '-ld2d1'
AssertPresent "Direct2D header is included (#2/#12)"                        '#include <d2d1.h>'
AssertPresent "AA-render policy mirrored into the mod (#2/#12)"             'inline bool UsesAntialiasedGeometry\(Surface s\)'
AssertPresent "tested AA-mode constant tied to the real D2D enum (#2/#12)"  'aarender::kAntialiasPerPrimitive =='
AssertPresent "Direct2D DC render target created (#2/#12)"                  'CreateDCRenderTarget'
AssertPresent "render target bound to the paint HDC (#2/#12)"               'BindDC\(hdc'
AssertPresent "per-primitive anti-aliasing enabled (#2/#12)"                'SetAntialiasMode\(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE\)'
AssertPresent "EndDraw guards device loss (#2/#12)"                         'g_target->EndDraw\(\)'
AssertPresent "shared AA surface wrapper exists (#2/#12)"                   'static bool DrawRoundSurfaceAA\('
AssertPresent "section cards routed through the AA layer (#2/#12)"          'aarender::Surface::Card'
AssertPresent "extension badges routed through the AA layer (#2/#12)"       'aarender::Surface::Badge'
AssertPresent "GDI RoundRect retained as the fallback (#2/#12)"             'RoundRect\(hdc, rc\.left, rc\.top, rc\.right, rc\.bottom, radiusPx'

# Pass7 S9b: the remaining rounded surfaces (Create/Cancel, Copy/Move segments,
# focus ring) route through the SAME AA layer -- a shared core
# plus a stroke-only outline (focus ring) and a single-side-rounded variant
# (segments). GDI stays as the per-frame fallback everywhere.
AssertPresent "AA layer uses one shared rounded-rect core (#2/#12)"         'inline bool RoundRectCore\('
AssertPresent "stroke-only AA outline helper exists (#2/#12)"               'static bool DrawRoundStrokeAA\('
AssertPresent "single-side-rounded AA segment helper exists (#2/#12)"       'static bool DrawRoundSegmentAA\('
AssertPresent "Create/Cancel buttons routed through the AA layer (#2/#12)"  'aarender::Surface::Button'
AssertPresent "Copy/Move segments routed through the AA layer (#2/#12)"     'aarender::Surface::Segment'
AssertAbsent  "no progress track routed through the AA layer (removed, #1)" 'aarender::Surface::Progress'
AssertPresent "focus ring routed through the AA layer (#2/#12)"             'aarender::Surface::FocusRing'

# --------------------------------------------------------------------------
# S11 (fix thse isseue.md v0.12.51): the Folder name + Destination FIELD surfaces
# (the dominant painted elements) now route fill + hairline border through the
# SAME AA layer -- fill via Surface::Button, border via the stroke-only
# Surface::FocusRing path -- instead of raw GDI RoundRect. GDI RoundRect stays
# inside each `if (!DrawRound...AA(...))` block as the per-frame fallback (pinned
# by the P0 #2 "field surface filled" invariants above). The EDIT controls stay
# pure Win32 (Surface::EditField -> not AA).
AssertPresent "name field surface FILL routed through the AA layer (S11)"         'DrawRoundSurfaceAA\(hdc, nfr, aarender::Surface::Button'
AssertPresent "Destination field surface FILL routed through the AA layer (S11)"  'DrawRoundSurfaceAA\(hdc, dfr, aarender::Surface::Button'
AssertPresent "name field BORDER routed through the AA stroke layer (S11)"        'DrawRoundStrokeAA\(hdc, nfr, aarender::Surface::FocusRing'
# fix thse isseue.md #2: the Destination field SURFACE + hairline BORDER now share
# ONE anti-aliased rounded-rect pass (DrawRoundSurfaceAA with hasBorder=true), so
# the fill and the border are concentric and their corners coincide. The old
# two-pass fill (full rect, stroke inset 0) + separately-inset stroke of the SAME
# radius had non-concentric corner arcs, so the lighter editBg surface bulged past
# the neutral border at the four corners (the square-corner spill / thick doubled
# edge), worse at 125/150/200% DPI. The dest field no longer uses a separate
# stroke-only pass; the editable Folder name field keeps its own two-pass border.
AssertPresent "Destination field surface+border is ONE AA fill+border pass (fix thse isseue.md #2)" 'dsrad, cpal\.editBg, true,'
AssertAbsent  "Destination field border is no longer a separate AA stroke pass (fix thse isseue.md #2)" 'DrawRoundStrokeAA\(hdc, dfr, aarender::Surface::FocusRing'
# fix thse isseue.md #2: the dead ThemeFieldSurface / ThemeFieldCorner wrappers
# (editBg / cardBg accessors for the old EDIT self-erase) were removed -- they
# were unused and tripped clang -Wunused-function. Callers read pal.editBg /
# pal.cardBg directly.
AssertAbsent  "unused ThemeFieldSurface accessor removed (fix thse isseue.md #2 warning)" 'COLORREF ThemeFieldSurface'
AssertAbsent  "unused ThemeFieldCorner accessor removed (fix thse isseue.md #2 warning)" 'COLORREF ThemeFieldCorner'

# S-icon (fix thse isseue.md v0.12.57): all FIVE sections carry the Windows-
# native semantic icon. Each paint site is pinned INDIVIDUALLY, so dropping ANY
# single section icon fails the gate. Selected paints via the device-px helper
# (DrawSemanticSectionIconDev); the other four via DrawSemanticSectionIcon.
AssertAbsent  "Selected header reverted to EMOJI no semantic icon (#1)"    'DrawSemanticSectionIconDev\(hdc, semanticicon::Section::Selected'
AssertPresent "Destination section semantic icon painted (v0.12.57)" 'DrawSemanticSectionIcon\(hdc, semanticicon::Section::Destination'
AssertPresent "Folder name section semantic icon painted (v0.12.57)" 'DrawSemanticSectionIcon\(hdc, semanticicon::Section::Name'
AssertAbsent  "Status heading reverted to chosen symbol no Info icon (#1)"      'DrawSemanticSectionIcon\(hdc, semanticicon::Section::Status'
AssertPresent "Action section semantic icon painted (v0.12.57)"      'DrawSemanticSectionIcon\(hdc, semanticicon::Section::Action'

# S14 (fix thse isseue.md v0.12.54): the section TITLES + the title-bar heading
# use a heavier SEMIBOLD font (UiFontSemibold), distinct from the regular body
# UiFont, matching the target Win11 dialog. The weight policy is the tested
# namespace typeweight, mirrored into the mod and tied to the real Win32 FW_*
# constants by static_asserts. The semibold font must be CREATED, USED across the
# title sites, and FREED on unload (so it can't leak like a stray GDI handle).
AssertPresent "semibold title font helper exists (S14)"            'static HFONT UiFontSemibold\('
AssertPresent "semibold font uses the FW_SEMIBOLD weight (S14)"    'FW_SEMIBOLD, FALSE'
AssertPresent "title weight policy mirrored + tied to Win32 (S14)" 'static_assert\(FW_SEMIBOLD == typeweight::kSemibold'
AssertAtLeast "semibold font used across the title sites (S14)"    'UiFontSemibold\(' 5
AssertPresent "semibold title font freed on unload (S14)"          'DeleteObject\(g_uiFontSemibold\)'

# --------------------------------------------------------------------------
# Slice 1 (fix thse isseue.md "Next exact fix order" #1-#4): the compact/expanded
# +/- resize is now ONE atomic redraw transaction, NOT a sequence of
# MoveWindow(..., TRUE) that each repainted mid-flight (the wiggly/staggered
# redraw the review called out). The pure flag policy is unit-tested
# (redraw_logic_test); the Win32 transaction is a clang-only Windhawk TU a unit
# test cannot see, so the live wiring is pinned here.
# --------------------------------------------------------------------------
AssertPresent "redraw policy mirrored into the mod (#1-#4)"               'namespace redraw'
AssertPresent "redraw flags tied to the real Win32 RDW_* bits (#1-#4)"    'static_assert\(redraw::kRdwAllChildren == RDW_ALLCHILDREN'
AssertPresent "relayout suppresses intermediate painting (#3)"           'SendMessageW\(hwnd, WM_SETREDRAW, FALSE, 0\)'
AssertPresent "relayout re-enables painting after the batch (#3)"        'SendMessageW\(hwnd, WM_SETREDRAW, TRUE, 0\)'
AssertPresent "relayout children move atomically (#2)"                   'BeginDeferWindowPos\('
AssertPresent "defer batch is committed (#2)"                            'EndDeferWindowPos\('
AssertAtLeast "relayout children routed through DeferWindowPos (#2)"     'DeferWindowPos\(' 3
AssertPresent "ONE final repaint reaches all children (#4)"              'RedrawWindow\(hwnd, nullptr, nullptr, redraw::FinalRepaintFlags\(\)\)'
AssertAbsent  "no per-child MoveWindow(..., TRUE) left in relayout (#2)"  'MoveWindow\(d->'
AssertAbsent  "redundant pre-relayout size-toggle repaint removed"       'InvalidateRect\((?:d->|data\.)sizeToggle'

# --------------------------------------------------------------------------
# Slice 2 (fix thse isseue.md #5 "keep hybrid rendering" + the RoundRect scan):
# the Selected Items scrollbar THUMB was the one remaining RAW GDI RoundRect.
# It now routes through the shared AA layer (new aarender::Surface::ScrollThumb,
# unit-tested in aa_render_test) with the GDI RoundRect kept as the per-frame
# fallback, so no custom rounded surface stair-steps its corners anymore.
# --------------------------------------------------------------------------
AssertPresent "ScrollThumb is a mirrored AA surface (#5)"                'Surface::ScrollThumb'
AssertPresent "scrollbar thumb routed through the AA layer (#5)"         'DrawRoundSurfaceAA\(hdc, thumbRc, aarender::Surface::ScrollThumb'
AssertPresent "scrollbar thumb keeps the GDI RoundRect fallback (#5)"    'RoundRect\(hdc, thumbRc\.left, thumbRc\.top'

Write-Host ""
Head "PART 2 (fix thse isseue.md): caption square glyph + red Cancel"
# fix thse isseue.md C: the PRIMARY caption glyphs are verified Segoe FONT glyphs
# (E921 Minimize, E922 Maximize, E923 Restore, E8BB Close) drawn via
# DrawCaptionGlyph with Segoe Fluent Icons (Win11) -> Segoe MDL2 Assets (Win10).
# Glyph presence is VERIFIED with GetGlyphIndicesW (not assumed from a mere font
# install); when no font/glyph is available the hand-drawn vector marks below are
# the DELIBERATE geometric fallback (never an empty / notdef box).
AssertPresent "caption glyphs drawn via the native-font helper (fix thse isseue.md C)" 'DrawCaptionGlyph\('
AssertPresent "caption glyph model: Minimize = Segoe ChromeMinimize E921 (C)"   '0xE921u'
AssertPresent "caption glyph model: Maximize = Segoe ChromeMaximize E922 (C)"   '0xE922u'
AssertPresent "caption glyph model: Restore = Segoe ChromeRestore E923 (C)"     '0xE923u'
AssertPresent "caption glyph model: Close = Segoe ChromeClose E8BB (C)"         '0xE8BBu'
AssertPresent "caption glyph provider falls back Fluent -> MDL2 -> geometric (C)" 'captionglyph::ResolveProvider\('
AssertPresent "caption glyph presence VERIFIED, not assumed from font install (C)" 'GGI_MARK_NONEXISTING_GLYPHS'
# Hand-drawn vector marks RETAINED as the deliberate geometric fallback:
AssertPresent "resize Maximize glyph is a rounded-corner square (geometric fallback)" 'RoundRect\(hdc, x \+ 3, y \+ 3, x \+ u - 3, y \+ u - 3, r, r\)'
AssertPresent "resize Restore glyph is two overlapping rounded squares (geometric fallback)" 'RoundRect\(hdc, x \+ 2, y \+ 5, x \+ u - 5, y \+ u - 2, r, r\)'
AssertAbsent  "old corner-bracket resize glyph is gone"                 'line\(x \+ 2, y \+ 6, x \+ 2, y \+ 2\)'
AssertPresent "Cancel button gives RED press feedback like Close"       'dis->CtlID == IDCANCEL && pressed'
# fix thse isseue.md #2/#5: Create (IDOK) + the Copy-path button (id 111) give
# GREEN press feedback, the confirm-colour counterpart to the Cancel/Close red.
# btn::PressTone (unit-tested) owns the semantic; kColConfirmPressed is the one
# green source. These source invariants pin the mirror + the wiring + the token.
AssertPresent "btn::PressTone press-feedback model mirrored (#2/#5)"    'Tone PressTone\(Button'
AssertPresent "Create+Copy GREEN press feedback via btn::PressTone (#2/#5)" 'btn::PressTone\(pbtn\) == btn::Tone::Green'
AssertPresent "confirm-green press token exists (#2/#5)"                'kColConfirmPressed'
AssertPresent "Close (X) stays the danger-red caption button"           'chromebtn::IsDanger\(dis->CtlID == 105'
# fix thse isseue.md #2/#5 (v0.12.60): ONE shared theme-aware caption foreground
# drives BOTH the title text and all three caption glyphs -- near-black in light
# RGB(24,24,24), near-white in dark RGB(245,245,245), NEVER a hard-coded black in
# dark mode. The pure decision is unit-tested (caption_fg_test); these pin the
# resolver, its two tokens, and that the paint sites actually use it.
AssertPresent "shared caption foreground resolver exists (#2)"            'static COLORREF CaptionForeground\(bool dark\)'
AssertPresent "caption foreground light token is near-black RGB(24,24,24) (#2)"   'kColCaptionFgLight\s+= RGB\(24, 24, 24\)'
AssertPresent "caption foreground dark token is near-white RGB(245,245,245) (#2)" 'kColCaptionFgDark\s+= RGB\(245, 245, 245\)'
AssertPresent "caption foreground is theme-aware, not hard-coded per theme (#2)"  'return dark \? kColCaptionFgDark : kColCaptionFgLight;'
AssertAtLeast "title text + all three glyphs use the shared caption foreground (#2)" 'CaptionForeground\(IsDarkMode\(\)\)' 4
# fix thse isseue.md #1/#3 (v0.12.60): idle caption buttons draw NO permanent
# backplate (idle fill matches the band); hover/pressed/red-Close are the ONLY
# painted feedback and are transient (chromebtn:: + caption_fill_test).
AssertPresent "caption feedback is painted ONLY on hover/press, never idle (#3)" 'if \(chromebtn::PaintsBackground\(cf\)\) \{'
# fix thse isseue.md #4 (v0.12.60): the explicit title-band separator line was
# removed; no border / opaque strip replaces it.
AssertAbsent  "title-band separator line is gone (#4)"                    'MoveToEx\(hdc, rc.left, bandH - 1, nullptr\)'
# fix thse isseue.md #1: the window title keeps ONLY its single 📂 emoji; the
# duplicate drawn folder vector icon (and its tIconRc setup) was removed -- no
# icon on/off, the emoji is always the one title glyph.
AssertAbsent  "window title no longer draws a DUPLICATE folder vector icon (#1)" 'IconId::Folder, tIconRc'
AssertPresent "window title still paints its single emoji folder caption (#1)"   'Group into folder", -1, &tText'
# fix thse isseue.md #1: Selected Items + Copy/Move revert to EMOJI and the
# Status heading shows the chosen status SYMBOL in its state tone, instead of the
# v0.12.57 generic semantic glyphs. Destination/Name/Action headings KEEP the
# native Windows semantic icon; file rows KEEP the real Shell icon.
AssertAbsent  "Selected header no longer uses the semantic List glyph (#1)"       'semanticicon::Section::Selected'
AssertAbsent  "Status heading no longer uses the generic Info semantic glyph (#1)" 'semanticicon::Section::Status'
AssertPresent "Selected header paints its emoji mark in the icon column (#1)"     'selIcoR'
AssertAbsent  "no Status heading status-symbol paint (#1)"                       'RECT sir ='
AssertPresent "Copy/Move segments paint an emoji glyph before the caption (#1)"   'CtlID == 107 \? L"'
Head "PART 2 (fix thse isseue.md): pixelated-text fix (natural ClearType + per-monitor DPI)"
AssertPresent "UI/icon fonts use natural ClearType (smoother text)"        'CLEARTYPE_NATURAL_QUALITY'
AssertAbsent  "no plain CLEARTYPE_QUALITY left (upgraded to natural)"       '[^_]CLEARTYPE_QUALITY'
AssertPresent "popup is created PER-MONITOR DPI aware v2 (native, crisp)"   'DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2'
AssertPresent "DPI awareness is resolved dynamically (Win10 1703+ safe)"    'GetProcAddress\(u32dpi, "SetThreadDpiAwarenessContext"\)'
AssertPresent "popup DPI awareness context is restored after create"       'if \(setDpiCtx && prevDpiCtx\) setDpiCtx\(prevDpiCtx\);'
Head "PART 2 (fix thse isseue.md #1): Destination path native monospace font"
AssertPresent "Destination path uses a cached monospace UI font"        'UiFontMono\('
AssertPresent "monospace path font is Consolas (Windows-bundled)"       'L"Consolas"'
AssertPresent "Destination field is wired to the monospace font"        'h == d->destField \? UiFontMono'
AssertPresent "monospace path font freed on unload"                     'DeleteObject\(g_uiFontMono\)'
Head "PART 2 (fix thse isseue.md #5 C): Copy/Move SLIDING ACTIVE PILL (not a per-half blend)"
# The Copy|Move selection is shown by a SINGLE accent pill that SLIDES between
# the two halves, driven by the tested segglide:: model + a repaint timer (id
# 110). The old independent per-half colour blend (AccentFracForSeg) is GONE.
AssertPresent "Copy/Move slide model mirrored into the mod (#5)"           'namespace segglide'
AssertPresent "sliding pill position from the tested PillBandPct model (#5)" 'segglide::PillBandPct'
AssertPresent "PillBand span struct mirrored into the mod (#5)"            'struct PillBand \{ int startPct; int endPct; \}'
AssertPresent "the sliding pill band is actually painted (#5)"             'band.endPct > band.startPct'
AssertAbsent  "old per-half accent-blend helper is gone (#5)"              'inline int AccentFracForSeg'
# fix thse isseue.md #6: the pill is filled at a MUTED accent tint
# (segglide::MutedAccentFrac) so the Copy/Move slide no longer reads flashy.
AssertPresent "sliding pill is a muted calm accent tint (#6)"             'segglide::MutedAccentFrac'
AssertPresent "glide advances via the tested eased step (#5)"             'segglide::Advance'
AssertPresent "glide seeds the resting side on open (#5)"                 'segglide::RestProgress'
AssertPresent "glide is driven by a repaint timer (id 110) (#5)"          'SetTimer\(hwnd, 110'
AssertPresent "glide timer tick handled in WM_TIMER (#5)"                 'wParam == 110'
AssertAbsent  "Copy/Move no longer FLIPS the fill instantly (#5)"         'else if \(isSeg && segActive\)'

Head "PART 2 (fix thse isseue.md #5 A): underlined C/M access-key mnemonics"
# The Copy/Move labels show the standard Windows access-key underline on C / M
# (the keyboard mnemonics segmented::Mnemonic already handles). The '&' prefix
# label is tested in segmented_logic_test; the paint draws it WITH prefix
# processing so the letter is underlined.
AssertPresent "C/M access-key label model mirrored into the mod (#5)"     'inline std::wstring MnemonicLabel\(Seg s\)'
AssertPresent "Copy shows the underlined C access key (#5)"               'L"&Copy"'
AssertPresent "Move shows the underlined M access key (#5)"               'L"&Move"'
AssertPresent "segment caption drawn from the access-key label (#5)"      'segmented::MnemonicLabel\('

Head "PART 2 (fix thse isseue.md #5 B): Copy/Move animation does not flash"
# The animation ticks must NOT erase the segment background (that was the flash);
# every segment invalidation is no-erase (FALSE) and the owner-draw composites
# the whole surface off-screen, flushed in ONE BitBlt.
AssertAbsent  "segment glide tick no longer erases the background (#5)"    'seg(Copy|Move)Btn, nullptr, TRUE'
AssertPresent "segment invalidations are no-erase (#5)"                   'InvalidateRect\(d->segCopyBtn, nullptr, FALSE\)'
AssertPresent "Copy/Move segment is double-buffered (#5)"                 'segMemDC = CreateCompatibleDC\(realDC\)'
AssertPresent "segment backbuffer flushed in one blit (#5)"               'BitBlt\(realDC, rc.left, rc.top, segBBW, segBBH, segMemDC'
Head "ADD-ON (fix thse isseue.md): rounded field corners + clipping (both themes/DPI)"
# Layered rounded-field rendering: paint the rounded AA SURFACE (background)
# first, then the rounded AA border, with the surface drawn 1px LARGER on each
# side so the rectangular inner EDIT never protrudes past the rounded corner (no
# square-corner / rectangular-patch artifact). Verified for the editable
# Folder-name field and the read-only Destination field; the field GEOMETRY is
# re-verified across the DPI ladder by tests/field_round_test.cpp.
AssertPresent "Name field paints a LAYERED AA rounded surface then border (add-on)"      'DrawRoundSurfaceAA\(hdc, nfr, aarender::Surface::Button'
AssertPresent "Dest field paints a LAYERED AA rounded surface then border (add-on)"      'DrawRoundSurfaceAA\(hdc, dfr, aarender::Surface::Button'
AssertPresent "Name field frame aligned to the field bounds, no spill (fix thse isseue.md #2)"  'RECT nfr = \{S2\(L.nameField.x\), S2\(L.nameField.y\),'
AssertPresent "Dest field frame aligned to the field bounds, no spill (fix thse isseue.md #2)"  'RECT dfr = \{S2\(L.destField.x\), S2\(L.destField.y\),'
AssertPresent "field frame = field bounds, zero spill mirrored into the mod (#2)"             'inline int FrameSpillPx\(\) \{ return 0; \}'
AssertPresent "field OuterFrameRect identity helper mirrored into the mod (#2)"               'inline FrameRect OuterFrameRect\(int fx, int fy, int fw, int fh\)'
if ($fail -eq 0) {
    Write-Host "RESULT : PASS" -ForegroundColor Green
    exit 0
} else {
    Write-Host ("RESULT : FAIL ({0} issue(s))" -f $fail) -ForegroundColor Red
    exit 1
}
