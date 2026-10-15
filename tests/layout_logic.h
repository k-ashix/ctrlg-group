// layout_logic.h
//
// PURE layout / spacing system for the New Folder + Group popups, mirrored from
// ctrlg-group.wh.cpp so it can be unit tested with a plain g++
// build (no Win32 runtime). The mod is a Windhawk translation unit with no
// standalone main(), so the geometry decisions live here and are exercised by
// tests\layout_*_test.cpp -- the same contract the rest of the suite uses.
//
// This is the "spacing system" from waht to create.md #9: ONE 4px base scale and
// ONE ComputePromptLayout() that returns every control's design-space rectangle,
// so the popup stops placing controls at ad-hoc absolute coordinates. The mod
// keeps a byte-for-byte copy of this module and applies its own DPI ScaleDpi()
// (MulDiv) to each rect when creating the controls, so on-screen geometry is
// identical to the layout computed here.
//
#pragma once

namespace layout {

// ---- 4px base spacing scale (96-dpi design pixels) ----
inline constexpr int kUnit   = 4;
inline constexpr int kSpace2 = kUnit * 2;   // 8   label -> control
inline constexpr int kSpace3 = kUnit * 3;   // 12  row padding
inline constexpr int kSpace4 = kUnit * 4;   // 16  related controls
inline constexpr int kSpace5 = kUnit * 5;   // 20  section separation / outer margin
inline constexpr int kSpace6 = kUnit * 6;   // 24  generous outer margin

// A DPI-independent design rectangle (96-dpi pixels).
struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    int right()  const { return x + w; }
    int bottom() const { return y + h; }
    bool empty() const { return w <= 0 || h <= 0; }
};

// Everything the popup needs to place its controls, in 96-dpi design pixels.
// The Group popup is a stack of sectioned CARDS (waht to create.md #3/#5/#9)
// so every component -- Selected Items, Destination, Folder name, Status and
// the Copy/Move segmented control -- has its OWN rectangle instead of sharing
// one STATIC text blob. New Folder leaves the group-only rects empty.
struct PromptLayout {
    bool isGroup = false;
    bool expanded = false;   // group only: the taller Selected Items viewport
    int windowW = 0;
    int windowH = 0;

    // Selected Items card (group only): summary header + scrollable list.
    Rect selectedCard;
    Rect selectedHeader;
    Rect selectedList;
    // fix thse isseue.md #3: the padded CONTENT rectangle inside selectedCard
    // (16px on all four sides). The owner-drawn list HWND lives here, NOT over
    // the whole card, so the rounded card background + border stay visible.
    Rect selectedContent;
    // groupHeader stays an ALIAS of the whole selectedCard rect (card-level
    // wiring only). The list HWND now uses selectedContent, so there is no
    // competing geometry model (fix thse isseue.md #3).
    Rect groupHeader;

    // Destination card (group only): title + description + read-only field.
    // fix thse isseu e.md #1: the description is its OWN row BETWEEN the label
    // and the field (title -> description -> field), not painted under the field.
    Rect destCard;
    Rect destLabel;
    Rect destDesc;
    Rect destField;
    Rect destCopyBtn;  // fix thse isseue.md #2 (path pass): "copy path" button

    // Folder name card (group uses the card; New Folder keeps its original
    // bare label/field geometry and leaves nameCard empty). Same
    // title -> description -> field stack as the Destination card.
    Rect nameCard;
    Rect nameLabel;
    Rect nameDesc;
    Rect nameField;

    // Action card (group only, fix thse isseu e.md #6): title + description +
    // the Copy/Move segmented control + a keyboard hint. The segments live
    // INSIDE this card; Create/Cancel move to their own bottom action row.
    Rect actionCard;
    Rect actionTitle;
    Rect actionDesc;
    Rect keyboardHint;

    // Compact/expand size control (group only, fix thse isseu e.md #5): a
    // window-CHROME affordance in the top-right strip ABOVE the first card, not
    // a child button sitting inside the Selected Items card. Right-aligned to
    // the window edge; its bottom stays above selectedCard.y.
    Rect chromeSizeBtn;
    // fix thse isseue.md #2/#1: Minimize + Close sit to the RIGHT of the expand
    // control in the custom title bar (Close is the rightmost control).
    Rect chromeMinBtn;
    Rect chromeCloseBtn;

    // Action row.
    Rect segCopy;        // group only: Copy segment (inside the Action card)
    Rect segMove;        // group only: Move segment (inside the Action card)
    Rect opToggle;       // group only: single Copy|Move toggle (alias of segCopy)
    Rect createButton;   // bottom action row
    Rect cancelButton;   // bottom action row
    Rect autoGroupButton; // bottom action row (ADD-ON): timed Auto Group button,
                          // left of Create/Group. Zero-size when Auto Group is
                          // disabled in settings (button not created).
    // fix thse isseue.md UI-03: footer INFO / help button (square, pinned to the
    // LEFT end of the action row). Drives the themed help panel (infohelp model).
    Rect infoButton;
};

// Scale a 96-dpi design value to device pixels for `dpi`. Rounds to nearest,
// matching the mod's ScaleDpi() = MulDiv(value, dpi, 96). MulDiv rounds half
// away from zero; for the non-negative design values here that is (v*dpi+48)/96.
inline int ScaleForDpi(int value96, int dpi) {
    return static_cast<int>(
        (static_cast<long long>(value96) * dpi + 48) / 96);
}

inline Rect ScaleRect(const Rect& r, int dpi) {
    return Rect{ScaleForDpi(r.x, dpi), ScaleForDpi(r.y, dpi),
                ScaleForDpi(r.w, dpi), ScaleForDpi(r.h, dpi)};
}

// ---- Client -> outer window size (Phase 3a bug fix, fix thse isseu e.md #1) ----
// ComputePromptLayout().windowW/windowH are CLIENT (content) sizes. A popup
// created WS_CAPTION | WS_SYSMENU has a non-client frame (title bar + borders)
// that AdjustWindowRectExForDpi() reports; the OUTER window must be that much
// LARGER or the bottom controls fall outside the client area and are clipped
// (the exact symptom in the screenshot). These helpers add the device-pixel
// non-client extents to the (already device-pixel) client size. The mod's
// original bug -- passing the client size straight to CreateWindowExW /
// SetWindowPos -- is exactly ncExtra == 0.
inline int OuterWidthFromClient(int clientW, int ncExtraW) {
    return clientW + ncExtraW;
}
inline int OuterHeightFromClient(int clientH, int ncExtraH) {
    return clientH + ncExtraH;
}

// ---- Popup geometry constants (96-dpi design pixels) ----
// One place for the window box and the shared control metrics. The vertical
// rhythm is expressed as offsets from the top of the form area so the New
// Folder and Group popups share the exact same form layout; the Group popup
// simply pushes the form down by a header band to make room for the
// Selected Items / destination block above it.
inline constexpr int kWindowW        = 344;
inline constexpr int kGroupCompactW  = 410;   // fix #7: compact group width (~400-420)
inline constexpr int kGroupExpandedW = 660;   // fix #8: expanded is WIDER (~640-680)
inline constexpr int kFormWindowH    = 178;   // New Folder height (no header band)
inline constexpr int kOuterMargin    = kSpace5;  // 20  left/right content margin
inline constexpr int kContentW       = kWindowW - kOuterMargin * 2;  // 304
inline constexpr int kLabelH         = 22;
inline constexpr int kFieldH         = 30;
inline constexpr int kNameFieldH     = 32;  // fix #7 (pass6): group Folder name field dominates (taller than kFieldH)
inline constexpr int kButtonW        = 74;
inline constexpr int kButtonH        = 30;
inline constexpr int kButtonGap      = 8;   // fix #13: 4px-grid gap (was 6) for the action row + chrome strip

// Form vertical offsets, measured from the top of the form area.
inline constexpr int kLabelTop       = 18;
inline constexpr int kFieldTop       = 46;   // below the label (+ small gap)
inline constexpr int kActionRowTop   = 92;   // Create/Cancel (+ Move/Copy) row

// Group header band: the vertical space the Selected Items / destination block
// consumes above the shared form.
// fix thse isseue.md E: kGroupHeaderBand (legacy fixed header band) unused -- removed.
inline constexpr int kGroupHeaderTop    = kSpace4;  // 16
inline constexpr int kGroupHeaderBottomGap = kSpace2; // 8 -> header block is band-8 tall

// Extra vertical room the Group header band (its Selected Items list) gains in
// expanded mode. The form below it shifts down by this amount and the window
// grows by it. New Folder has no list, so expanded leaves it unchanged.
inline constexpr int kGroupHeaderExpandedExtra = 120;

// ---- Group sectioned-card layout (Phase 3a, waht to create.md #3/#5/#9) ----
// Each Group section is a rounded CARD; controls sit inside with kCardInset
// padding and cards are separated by kCardGap. Expanded adds
// kGroupHeaderExpandedExtra to the Selected Items list; every card below it
// shifts down by the same amount (see ComputePromptLayout).
inline constexpr int kCardInset         = kSpace4;   // 16  padding inside a card (fix #3 pass6: more internal breathing)
inline constexpr int kCardGap           = kSpace4;   // 12  gap between stacked cards (fix #7: aligned to the 4px scale)
inline constexpr int kGroupTopMargin    = 14;        // top margin above card A
inline constexpr int kGroupBottomMargin = kSpace4;   // 16  below the action row
inline constexpr int kSelHeaderH        = 34;        // Selected Items summary header
inline constexpr int kSelListH          = 68;        // Selected Items list (compact, legacy)
// fix thse isseue.md #1: content-sized Selected Items list. The viewport shows
// min(itemCount, rows-for-mode) rows; these mirror mode::CompactRows()/
// ExpandedRows() and the owner-draw row height S(20).
inline constexpr int kListCompactRows   = 4;         // compact row cap (mode::CompactRows)
inline constexpr int kListExpandedRows  = 9;         // expanded row cap (mode::ExpandedRows)
inline constexpr int kSelRowH           = 24;        // fix #1: taller list row for vertical air (owner-draw S(24))
inline constexpr int kSelListPadV       = 8;         // list top+bottom padding (legacy, superseded by kSelectedPad)
// fix thse isseue.md #3: Selected card content padding + the exact owner-draw
// header rhythm, so the card height is DERIVED from the inset content block.
inline constexpr int kSelectedPad       = kSpace5;   // 20  content inset on all 4 sides (fix thse isseue.md #3: was 16/kSpace4)
inline constexpr int kSelTitleH         = 24;        // title row height (owner-draw S(24))
inline constexpr int kSelCountsH        = 16;        // type-breakdown row (owner-draw S(16))
inline constexpr int kSelDividerH       = 1;         // 1px hairline divider
inline constexpr int kSelHdrGap         = 4;         // even gap: title/counts/divider/rows (fix thse isseue.md #3: 4px rhythm, was 3; owner-draw S(4))
inline constexpr int kDestCardH         = 100;        // Destination card: title+10 h18, desc+32 h16, field+56 h26 (ends 82) -> +12 bottom (P2 #13 rhythm)
inline constexpr int kNameCardH         = 108;       // Folder name card (title + desc + field)
inline constexpr int kDescH             = 16;        // supporting description row (fix #1)
inline constexpr int kActionCardH       = 116;      // Action card: title+10..28, desc+30..46, segs+52..82, hint+86..100 (ends 100) -> 116 breathing (P0 #4)
inline constexpr int kHintH             = 14;        // keyboard-hint row height

// fix thse isseue.md #3: reserved icon column at the card inner-left; each
// section TITLE starts this many px to the right so the icon sits INSIDE the
// card content instead of hanging off the card's left border.
inline constexpr int kSectionIconCol    = 24;

// Window-chrome strip for the size control (fix thse isseu e.md #5). The group
// card stack drops by kChromeBand so the compact/expand button lives in its own
// top strip, right-aligned to the window edge, visually separate from card A.
inline constexpr int kChromeBtnSize = 28;   // fix #6: comfortable caption hit target (glyph stays small)
inline constexpr int kChromeBtnW    = 44;   // fix thse isseue.md: caption click-area WIDTH (native ~45); buttons also span the full band height
static_assert(kChromeBtnW >= kChromeBtnSize, "caption click area must not shrink below the legacy 28px hit floor");
// fix thse isseue.md E: kChromeTop (legacy chrome-strip inset) unused -- removed.
inline constexpr int kChromeBand    = 18;   // fix thse isseue.md A: TitleBandH()=kGroupTopMargin(14)+18 = 32px native title-band height
// fix thse isseue.md A: caption buttons are CONTIGUOUS (native title-bar style);
// the inter-button gap is its OWN zero constant. The general kButtonGap (action
// row + copy-path button) is intentionally left untouched.
inline constexpr int kCaptionButtonGap = 0;

// fix thse isseue.md #2: a deliberate gap between the custom title band and the
// first (Selected Items) card. The first card top is TitleBandH() + this gap, so
// the card no longer butts straight up against the header band. 12px @96dpi.
inline constexpr int kHeaderToSelectedGap = kSpace4;  // 16  title band -> first card (fix thse isseue.md #3: was 12/kSpace3)

// fix thse isseue.md #2: the Group popup is borderless (WS_POPUP) and paints its
// OWN title bar across the top band (title + Expand/Minimize/Close). The band
// runs from y=0 down to the title band bottom; the first card starts a
// kHeaderToSelectedGap below it (see cardTop in ComputePromptLayout).
inline int TitleBandH() { return kGroupTopMargin + kChromeBand; }

// Compute the full popup layout for a New Folder (isGroup=false) or Group
// (isGroup=true) prompt, in 96-dpi design pixels. `expanded` grows the Group
// Selected Items list; it has no effect on New Folder. Compact
// (expanded=false) is byte-identical to the original geometry.
inline PromptLayout ComputePromptLayout(bool isGroup, bool expanded = false,
                                        int itemCount = kListExpandedRows) {
    PromptLayout L;
    L.isGroup = isGroup;
    L.expanded = expanded;
    L.windowW = kWindowW;

    const int cancelX = kWindowW - kOuterMargin - kButtonW;       // right-aligned
    const int createX = cancelX - kButtonW - kButtonGap;          // left of Cancel

    if (!isGroup) {
        // ---- New Folder: original bare-form geometry, unchanged (no cards) ----
        L.nameLabel = Rect{kOuterMargin, kLabelTop, kContentW, kLabelH};
        L.nameField = Rect{kOuterMargin, kFieldTop, kContentW, kFieldH};
        L.createButton = Rect{createX, kActionRowTop, kButtonW, kButtonH};
        L.cancelButton = Rect{cancelX, kActionRowTop, kButtonW, kButtonH};
        L.windowH = kFormWindowH;
        return L;
    }

    // ---- Group: sectioned card stack (top -> bottom) ----
    // fix thse isseue.md #1: the Selected Items list is CONTENT-SIZED (see
    // selectedList below); every card below it shifts down with the list and
    // the window grows to fit, so a small selection no longer leaves a gap.
    // fix #9/#10: expanded mode is WIDER, not just taller. Width (and every
    // width derived from it) is chosen by mode so cards/fields/segments grow
    // horizontally. Compact stays 344 -> byte-identical to the previous build.
    const int winW     = expanded ? kGroupExpandedW : kGroupCompactW;
    const int contentW = winW - kOuterMargin * 2;
    const int innerX   = kOuterMargin + kCardInset;      // 32  content inside a card
    const int innerW   = contentW - kCardInset * 2;
    L.windowW = winW;
    const int cancelX2 = winW - kOuterMargin - kButtonW;   // right-aligned
    const int createX2 = cancelX2 - kButtonW - kButtonGap; // left of Cancel

    // fix thse isseu e.md #5: a window-CHROME strip ABOVE the first card holds
    // the compact/expand size control, so it reads as a window affordance and
    // not as part of the Selected Items card. The whole card stack drops by
    // kChromeBand to make room; the button is right-aligned to the window edge.
    const int chromeBtn  = kChromeBtnW;       // wider click area (native ~45)
    const int chromeBtnH = TitleBandH();      // FULL band height: no dead strip above/below so clicks are not swallowed (fix thse isseue.md)
    const int chromeGap  = kCaptionButtonGap;   // fix thse isseue.md A: contiguous caption buttons (0 gap), NOT the general kButtonGap
    // fix thse isseue.md #5: Windows caption order Minimize | Expand/Restore |
    // Close -- Close rightmost, Expand/Restore middle, Minimize leftmost. Each
    // control fills the band top-to-bottom; the gap between them stays caption.
    const int closeX  = winW - kOuterMargin - chromeBtn;
    const int expandX = closeX - chromeGap - chromeBtn;
    const int minX    = expandX - chromeGap - chromeBtn;
    L.chromeCloseBtn = Rect{closeX,  0, chromeBtn, chromeBtnH};
    L.chromeSizeBtn  = Rect{expandX, 0, chromeBtn, chromeBtnH};
    L.chromeMinBtn   = Rect{minX,    0, chromeBtn, chromeBtnH};
    const int cardTop = TitleBandH() + kHeaderToSelectedGap;  // first card top: a gap below the title band (#2)

    // Card A -- Selected Items (summary header + CONTENT-SIZED list viewport).
    // fix thse isseue.md #1: show min(itemCount, rows-for-mode) rows instead of
    // a fixed viewport, so a 5-item selection no longer reserves 9 rows. Height
    // is visibleRows * row height + a small top/bottom padding; the old fixed
    // kSelListH / kGroupHeaderExpandedExtra band no longer drives the height.
    const int capRows = expanded ? kListExpandedRows : kListCompactRows;
    int visRows = itemCount < capRows ? itemCount : capRows;
    if (visRows < 1) visRows = 1;   // always at least one row tall
    // fix thse isseue.md #3: the Selected card content is inset kSelectedPad
    // (16px) on ALL four sides. The content block is the owner-draw rhythm:
    //   title + gap + counts + gap + 1px divider + gap + visRows rows.
    // The card height is DERIVED from that content + the two paddings, so a
    // small selection leaves no empty space, and the parent keeps the rounded
    // card border intact (the list HWND sits inside selectedContent).
    const int selContentH =
        kSelTitleH + kSelHdrGap + kSelCountsH + kSelHdrGap +
        kSelDividerH + kSelHdrGap + visRows * kSelRowH;
    L.selectedContent = Rect{kOuterMargin + kSelectedPad, cardTop + kSelectedPad,
                             contentW - kSelectedPad * 2, selContentH};
    const int selCardH = selContentH + kSelectedPad * 2;
    L.selectedCard = Rect{kOuterMargin, cardTop, contentW, selCardH};
    L.groupHeader  = L.selectedCard;   // alias: the whole card rect (card wiring only)
    // Header (title + counts + divider) and the rows region, both INSIDE the
    // padded content rect -- used by the layout tests.
    L.selectedHeader = Rect{L.selectedContent.x, L.selectedContent.y,
                            L.selectedContent.w,
                            kSelTitleH + kSelHdrGap + kSelCountsH + kSelHdrGap +
                                kSelDividerH};
    L.selectedList   = Rect{L.selectedContent.x,
                            L.selectedHeader.bottom() + kSelHdrGap,
                            L.selectedContent.w, visRows * kSelRowH};

    // Card B -- Destination (title -> description -> read-only field).
    // fix thse isseu e.md #1: the supporting description is its OWN row sitting
    // BETWEEN the label and the field, so the field is no longer cramped by a
    // description painted beneath it.
    const int destY = L.selectedCard.bottom() + kCardGap;
    L.destCard  = Rect{kOuterMargin, destY, contentW, kDestCardH};
    L.destLabel = Rect{innerX + kSectionIconCol, destY + 12,
                       innerW - kSectionIconCol, 18};
    L.destDesc  = Rect{innerX, L.destLabel.bottom() + 4, innerW, kDescH};
    // fix thse isseue.md #2 (path pass): the path field is a REDUCED rect that
    // leaves room for a compact "Copy path" button on its right, same row, so
    // the path reads plainly and the button never overlaps the text.
    const int copyBtnW = 54;
    L.destField   = Rect{innerX, L.destDesc.bottom() + 8, innerW - copyBtnW - kButtonGap, 26};
    L.destCopyBtn = Rect{L.destField.right() + kButtonGap, L.destField.y, copyBtnW, 26};

    // Card C -- Folder name (title -> description -> edit).
    const int nameY = L.destCard.bottom() + kCardGap;
    L.nameCard  = Rect{kOuterMargin, nameY, contentW, kNameCardH};
    L.nameLabel = Rect{innerX + kSectionIconCol, nameY + 12,
                       innerW - kSectionIconCol, 18};
    L.nameDesc  = Rect{innerX, L.nameLabel.bottom() + 4, innerW, kDescH};
    L.nameField = Rect{innerX, L.nameDesc.bottom() + 8, innerW, kNameFieldH};

    // Card D -- Action (fix thse isseu e.md #6): title -> description ->
    // Copy/Move segments -> keyboard hint. The segments now live INSIDE this
    // card instead of sharing the bottom button row. fix thse isseue.md #1: the
    // Status card was removed, so Action follows the Folder name card directly.
    const int actionY = L.nameCard.bottom() + kCardGap;
    L.actionCard  = Rect{kOuterMargin, actionY, contentW, kActionCardH};
    L.actionTitle = Rect{innerX + kSectionIconCol, actionY + 12,
                         innerW - kSectionIconCol, 18};
    L.actionDesc  = Rect{innerX, L.actionTitle.bottom() + 4, innerW, kDescH};
    const int segY = L.actionDesc.bottom() + 8;
    // fix thse isseu e.md #7: the two segments meet edge-to-edge (segMove starts
    // exactly at segCopy.right()) so they read as ONE pill with a straight
    // shared middle boundary, not two separate rounded buttons.
    // fix #12: segments fill the inner width so they GROW when expanded.
    const int segHalf = innerW / 2;
    L.segCopy  = Rect{innerX, segY, segHalf, kButtonH};
    L.segMove  = Rect{innerX + segHalf, segY, innerW - segHalf, kButtonH};
    L.opToggle = Rect{innerX, segY, kButtonW, kButtonH};   // single-toggle alias
    L.keyboardHint = Rect{innerX, segY + kButtonH + 6, innerW, kHintH};

    // Bottom action row -- Create/Cancel only (right-aligned), below the card.
    const int rowY = L.actionCard.bottom() + kSpace3;
    L.createButton = Rect{createX2, rowY, kButtonW, kButtonH};
    L.cancelButton = Rect{cancelX2, rowY, kButtonW, kButtonH};

    L.windowH = rowY + kButtonH + kGroupBottomMargin;
    return L;
}

// fix thse isseue.md UI-01 / UI-05: responsive reflow. ComputePromptLayout()
// returns the popup at its PREFERRED compact/expanded size; when the user
// maximizes or drag-resizes the Group popup the window procedure's WM_SIZE
// handler reflows the children into the ACTUAL client rect through THIS pure
// transform, so the form never stays stranded at the fixed size in the
// upper-left corner. Contract:
//   * the content stretches to the client WIDTH (cards/fields widen; the
//     right-anchored chrome + action controls stay flush with the new edge),
//   * the Selected Items card absorbs the extra HEIGHT and every section below
//     it shifts down by the same amount, so the Destination / Folder-name /
//     Action sections and the footer track the client bottom and stay VISIBLE,
//   * the client is clamped to (minW,minH) so controls are never crushed,
//   * New Folder is never reflowed (its bare geometry is preserved).
// Pure: no Win32, no DPI (device scaling happens at the DeferWindowPos site,
// exactly like ComputePromptLayout's output). Mirrored into ctrlg-group.wh.cpp
// (the source copy additionally shifts the Auto Group add-on button).
inline PromptLayout ReflowToClient(const PromptLayout& base,
                                   int clientW, int clientH,
                                   int minW, int minH,
                                   int selRowH = kSelRowH,
                                   int selCapRowsOverride = -1,
                                   int rowsNeededOverride = -1) {
    PromptLayout L = base;
    if (!base.isGroup) return L;              // New Folder keeps its fixed form
    if (clientW < minW) clientW = minW;
    if (clientH < minH) clientH = minH;
    // fix thse isseue.md Phase 2: the Selected-Items row height is MODE-AWARE.
    // List passes the 24px list row (the default); a GRID view (Large / Extra
    // large) passes its WHOLE cell height (viewmode::CellH) so the reflow floor
    // and the row-cap math below reserve a complete cell -- never a 24px row
    // that clips the icon/filename.
    if (selRowH <= 0) selRowH = kSelRowH;

    const int dx = clientW - base.windowW;    // extra width to distribute
    int dy = clientH - base.windowH;          // extra height to distribute
    // Never shrink the Selected Items card below a readable floor: keep the
    // footer reachable rather than letting the lower cards overlap it.
    // v0.12.68 #3: derive the Selected Items reflow FLOOR from its ACTUAL header
    // geometry plus one complete list row, NOT a magic 40px -- so a shrink keeps
    // the header and at least one whole row visible and the lower cards + footer
    // reachable (no overlap). Grid modes reserve their taller whole cell upstream.
    const int selHeaderBlockH = base.selectedList.y - base.selectedContent.y;
    const int kMinSelReflowH = selHeaderBlockH + selRowH;
    if (base.selectedContent.h + dy < kMinSelReflowH)
        dy = kMinSelReflowH - base.selectedContent.h;

    // fix thse isseue.md Issue 4: do NOT pour ALL the extra height into the
    // Selected Items card. Only a selection that was ROW-CAPPED in the preferred
    // layout (more items than its viewport shows) uses the extra height (and then
    // scrolls, issue 9/10); a small selection that already shows every row keeps
    // its content height, so a maximized/zoomed popup no longer leaves a big
    // blank band below the last row and the lower sections keep their natural
    // spacing. Shrink (dy < 0) is unchanged so the readable floor above still
    // holds. selGrow is what the card grows by (and what the lower cards shift).
    const int baseVisRows = (selRowH > 0) ? base.selectedList.h / selRowH : 0;
    const int selCapRows  = (selCapRowsOverride >= 0)
                                ? selCapRowsOverride
                                : (base.expanded ? kListExpandedRows : kListCompactRows);
    // fix thse isseue.md P1 "exact fit treated as capped": overflow -- the only
    // reason to pour extra height into the Selected card and start scrolling --
    // means MORE rows are needed than the preferred viewport actually shows,
    // NOT mere equality with the declared row cap. When the caller supplies the
    // true rows-needed (rowsNeededOverride >= 0), decide from it; otherwise keep
    // the legacy cap heuristic for callers that cannot compute it.
    const bool selRowCapped = (rowsNeededOverride >= 0)
                                  ? (rowsNeededOverride > baseVisRows)
                                  : (baseVisRows >= selCapRows);
    int selGrow = dy;
    if (dy > 0 && !selRowCapped) selGrow = 0;

    L.windowW = clientW;
    L.windowH = clientH;

    const auto widen  = [dx](Rect& r) { if (r.w > 0) r.w += dx; };
    const auto moveX  = [dx](Rect& r) { if (r.w > 0) r.x += dx; };
    const auto shiftY = [selGrow](Rect& r) { if (r.w > 0) r.y += selGrow; };

    // Stretch the full-width cards / fields / descriptions to the client width.
    widen(L.selectedCard);  widen(L.selectedHeader); widen(L.selectedList);
    widen(L.selectedContent); widen(L.groupHeader);
    widen(L.destCard); widen(L.destLabel); widen(L.destDesc); widen(L.destField);
    widen(L.nameCard); widen(L.nameLabel); widen(L.nameDesc); widen(L.nameField);
    widen(L.actionCard); widen(L.actionTitle); widen(L.actionDesc); widen(L.keyboardHint);

    // Right-anchored controls move right so they stay flush with the new edge.
    moveX(L.chromeSizeBtn); moveX(L.chromeMinBtn); moveX(L.chromeCloseBtn);
    moveX(L.destCopyBtn); moveX(L.createButton); moveX(L.cancelButton);

    // Copy/Move segmented control: a LEFT-anchored, full-inner-width pair, NOT a
    // right-anchored control. It stays flush with the card's left inset and
    // STRETCHES to the new inner width (widening with the card) instead of
    // floating off to the right edge and leaving a gap. The two halves still
    // meet edge-to-edge so they read as one seamless pill. opToggle (the compact
    // single-button variant) just stays left-anchored.
    {
        const int segLeft   = base.segCopy.x;                     // card inner-left inset
        const int newInnerW = (base.segMove.right() - segLeft) + dx;
        const int newHalf   = newInnerW / 2;
        L.segCopy.x = segLeft;             L.segCopy.w = newHalf;              L.segCopy.y += selGrow;
        L.segMove.x = segLeft + newHalf;   L.segMove.w = newInnerW - newHalf;  L.segMove.y += selGrow;
        L.opToggle.x = segLeft;            L.opToggle.y += selGrow;
    }

    // The Selected Items card absorbs the (content-capped) extra height; every
    // section below it shifts down by the SAME amount so spacing stays uniform.
    L.selectedCard.h    += selGrow;
    L.selectedList.h    += selGrow;
    L.selectedContent.h += selGrow;
    L.groupHeader.h     += selGrow;
    shiftY(L.destCard); shiftY(L.destLabel); shiftY(L.destDesc);
    shiftY(L.destField); shiftY(L.destCopyBtn);
    shiftY(L.nameCard); shiftY(L.nameLabel); shiftY(L.nameDesc); shiftY(L.nameField);
    shiftY(L.actionCard); shiftY(L.actionTitle); shiftY(L.actionDesc);
    shiftY(L.keyboardHint);   // segCopy/segMove/opToggle shifted in the block above
    shiftY(L.createButton); shiftY(L.cancelButton);
    return L;
}

// fix thse isseue.md Phase 4: which repaint a Ctrl+wheel view-mode switch needs.
// A switch only changes the Selected Items VIEWPORT content (grid vs list). If
// every section OUTSIDE that card -- the window size and the destination,
// folder-name, action and footer rects -- is geometrically identical between
// the OLD and NEW effective layouts, nothing around the card moved, so repaint
// JUST the Selected Items viewport (no child reposition, no full-window erase ->
// no title-bar / card / footer flash). Otherwise a section genuinely moved
// (e.g. a preferred popup must resize for a taller mode) and ONE atomic relayout
// is required. Comparing the two computed layouts keeps this robust across every
// window state instead of guessing from flags.
enum class ViewSwitchRepaint { SelectedOnly, FullRelayout };

inline bool SectionsUnchanged(const PromptLayout& a, const PromptLayout& b) {
    const auto eq = [](const Rect& p, const Rect& q) {
        return p.x == q.x && p.y == q.y && p.w == q.w && p.h == q.h;
    };
    return a.windowW == b.windowW && a.windowH == b.windowH &&
           eq(a.destCard, b.destCard)         && eq(a.destField, b.destField)     &&
           eq(a.nameCard, b.nameCard)         && eq(a.nameField, b.nameField)     &&
           eq(a.actionCard, b.actionCard)     && eq(a.keyboardHint, b.keyboardHint) &&
           eq(a.segCopy, b.segCopy)           && eq(a.segMove, b.segMove)         &&
           eq(a.createButton, b.createButton) && eq(a.cancelButton, b.cancelButton);
}

inline ViewSwitchRepaint ViewSwitchRepaintFor(const PromptLayout& oldL,
                                              const PromptLayout& newL) {
    return SectionsUnchanged(oldL, newL) ? ViewSwitchRepaint::SelectedOnly
                                         : ViewSwitchRepaint::FullRelayout;
}

}  // namespace layout
