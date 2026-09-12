# PDF/UA-1 (ISO 14289-1) technical requirements checklist

This is the concrete, structural checklist this project is building against.
Note the standards distinction up front: **WCAG 2.1 AA is a web-content
standard (HTML/CSS/JS)**; there is no PDF-specific WCAG conformance level.
**PDF/UA-1 (ISO 14289-1)** is the PDF-specific standard that operationalizes
the same accessibility intent (usable by assistive technology, in particular
screen readers) for PDF documents, and is what a real validator (veraPDF)
actually checks. "Passing WCAG 2.1 AA for a PDF," in the terms a validator
can check, means passing PDF/UA-1. This project targets PDF/UA-1 explicitly,
validated with veraPDF's PDF/UA-1 ruleset.

## Structural requirements

- **`/StructTreeRoot`**: a real, populated logical structure tree, not the
  empty placeholder libharu's existing PDF/A support already creates
  (`hpdf_pdfa.c`). Every piece of real content (headings, paragraphs, table
  cells, figures) needs a corresponding structure element.
- **Marked content**: every content-stream region that structure references
  must be wrapped in `BDC .../EMC` (or `BMC/EMC` for artifacts with no
  properties) operators, each carrying a unique `/MCID` (marked-content ID)
  within its page.
- **`/ParentTree`**: a number tree mapping each page's `/StructParents` entry
  (and each MCID within it) back to its structure-tree parent element, so a
  reader can resolve "this marked content belongs to this structure node."
- **Role map (`/RoleMap`)**: every custom structure type used must map to one
  of the standard PDF structure types (`Document`, `Part`, `Sect`, `Div`,
  `P`, `H1`-`H6`, `L`, `LI`, `Lbl`, `LBody`, `Table`, `TR`, `TH`, `TD`,
  `THead`, `TBody`, `TFoot`, `Figure`, `Formula`, `Caption`, `Artifact`, and
  more) even if this project introduces its own semantic names internally
  (e.g. "SkylineFigure" -> `Figure`).
- **Artifacts**: purely decorative/layout content (page borders, background
  grid lines that don't carry data, repeated headers/footers) must be marked
  as artifacts (`/Type /Pagination`, `/Layout`, `/Background`, etc.), not left
  untagged and not folded into the real structure tree -- an artifact is
  explicitly *outside* the structure tree, not an empty/absent tag.

## Metadata and language

- **`/Lang`** on the document catalog (and optionally overridden per
  structure element for mixed-language content) -- a real BCP 47 language
  tag (e.g. `en-US`). libharu currently has no catalog-level language
  setter at all (checked directly against the vendored 2.4.5 source this
  project forked from `hpdf_catalog.c`/`hpdf_doc.c` -- no `/Lang` anywhere).
- **`/Title`** in both the document Info dictionary (libharu already has
  this via `HPDF_SetInfoAttr(pdf, HPDF_INFO_TITLE, ...)`) and,
  for PDF/UA, `/ViewerPreferences /DisplayDocTitle true` must also be set so
  viewers actually show the title instead of the filename. libharu has
  `HPDF_Catalog_SetViewerPreference()` already but no `DisplayDocTitle` flag
  defined yet (only `HideToolbar`/`HideMenubar` and a few others).
- **`/MarkInfo`** with `/Marked true` -- libharu's PDF/A support already adds
  this.

## Content-level requirements

- **Alternate text (`/Alt`)** on every figure/image structure element --
  required for anything conveying information non-textually (skyline plots,
  posterior histograms).
- **Table structure**: `/Table` > `/TR` > `/TH`/`/TD`, with header
  association via `/Scope` (`Row`/`Column`/`Both`) on `/TH` elements, or
  explicit `/Headers` references on `/TD` elements for irregular tables --
  the posterior-estimate and Bayes-factor tables this project's ultimate
  consumer (Migrate's Phase 5 report) generates are exactly this shape.
- **Reading order**: content and marked-content sequence in the page content
  stream, and the structure tree's element order, must both reflect the
  actual logical reading order -- not just visual left-to-right/top-to-bottom
  placement if that ever diverges (e.g. a multi-column layout).
- **Tab order**: `/Tabs /S` (structure order) on each page, so keyboard/AT
  navigation follows the structure tree rather than an unspecified default.
- **Document outline/bookmarks**: required for any document with distinct
  sections, mirroring the structure tree's top-level sections, so
  screen-reader users can jump between sections the way sighted users would
  via bookmarks. libharu already supports outlines
  (`HPDF_CreateOutline`/`HPDF_Outline_*`) for navigation; PDF/UA additionally
  requires the outline entries to correspond to real structure elements, not
  just page-jump destinations.

## Validation

- **veraPDF** (https://verapdf.org, Apache 2.0, Java-based) is the
  standard open-source PDF/UA-1 validator; the GreenLicht/PDF Association
  reference test suite is its own conformance baseline. Used from a build/
  test pipeline via its CLI (`verapdf --format text --flavour ua1 file.pdf`,
  exit code and machine-readable XML/JSON report). See `validate/
  run_verapdf.sh`.
- Manual review remains necessary for anything a structural validator cannot
  infer (whether alt text is actually *meaningful*, whether reading order is
  actually correct for a sighted reader's expectation, whether color
  contrast in generated figures is adequate) -- veraPDF proves the PDF is
  *structurally* well-formed for assistive technology, not that the content
  itself is good; that needs an actual screen-reader run-through too.

## Additions confirmed by research (2026-09-12), folded into the checklist above

- **Embedded fonts**: all fonts must be embedded (except invisible/OCR text)
  -- a real UA-1 requirement independent of tagging, worth checking against
  libharu's font handling directly (subset fonts, standard 14 fonts without
  embedding, etc.).
- **`/StructParents`/`/StructParent`**: numeric keys on pages/annotations/
  XObjects that resolve into `/ParentTree`, complementary to MCIDs (MCIDs
  identify content *within* a stream; StructParents identify which
  ParentTree entry a whole page/annotation/XObject maps to).
- **`/ActualText`**: needed wherever the actual glyphs are not directly
  Unicode-mappable via ToUnicode/cmap -- a real, concrete concern for this
  project's actual consumer, since Migrate's report content includes Greek
  letters (theta, migration-rate symbols) and mathematical notation that a
  subset/symbol font may not map cleanly; likely needed on more figures/
  labels than a typical document would.
- **Heading nesting**: `H1`-`H6` must not skip levels.
- **List numbering**: `/L` elements should carry a `ListNumbering` attribute.
- **Links**: need a real `Link` structure element associated with the link
  annotation, not just a bare annotation.
- **Encryption/permissions**: PDF/UA forbids security settings that would
  block assistive-technology content extraction (e.g. disabling
  copy-for-accessibility) -- worth a guard if this project's consumer ever
  adds PDF encryption/permissions options.
- **PDF/UA-2 (ISO 14289-2, aligned with PDF 2.0) exists** as of this
  research and reorganizes/extends the tag set (namespaces, a revised
  heading model). Tooling/validator support is newer and thinner than
  PDF/UA-1's as of 2026. **Decision: this project targets PDF/UA-1**
  (matches PDF 1.7, what current tools/validators/screen readers assume),
  but structure-element naming should stay close to PDF/UA-2's model where
  the two don't conflict, so a later UA-2 target isn't a full redesign.

## Standing reference material (not libharu-specific, but the actual spec to build against)

The PDF Association (https://pdfa.org/accessibility/) maintains the current
implementation guidance: the *Tagged PDF Best Practice Guide: Syntax*, the
*Matterhorn Protocol* (the ~31 machine+manual failure conditions veraPDF's
own UA-1 rules are based on), and **Well-Tagged PDF (WTPDF)**, first
published 2024 with an ongoing "Techniques" series (82 published as of
March 2026). Use these as the implementation spec, not just this checklist.

## What libharu (this project's vendored base) already has vs. lacks

Confirmed directly against the vendored source under `vendor/libharu/`:

| Requirement | Status |
| --- | --- |
| `/MarkInfo`, `/Marked true` | Real, working (`HPDF_UA_EnableTagging()`) |
| `/StructTreeRoot`, real population | Real, working (`HPDF_UA_BeginStructureElement()`) -- Milestone 1 |
| `/ParentTree`, MCIDs, `BDC`/`EMC` content-stream tagging | Real, working (`HPDF_UA_BeginMarkedContent()`/`EndMarkedContent()`) -- Milestone 1, veraPDF-validated |
| `/RoleMap` | Not needed -- every role this project uses is a PDF/UA-1 standard structure type already |
| `/Lang` (catalog) | Real, working (`HPDF_UA_SetDocumentLanguage()`) |
| `/Title` (Info dict) | Real (libharu's own `HPDF_SetInfoAttr`, unchanged) |
| `/ViewerPreferences /DisplayDocTitle` | Real, working (`HPDF_UA_SetDisplayDocTitle()`) |
| `/Alt` (figure alternate text) | Real, working (`HPDF_UA_SetAlternateText()`) |
| Table header association: `/Scope` | Real, working (`HPDF_UA_SetTableHeaderScope()`) |
| Table header association: `/Headers` (irregular tables) | Still a stub (`HPDF_UA_SetTableDataHeaders()`) -- `/Scope` covers this project's actual simple-table needs so far |
| Artifacts (`/Artifact` `BMC`/`EMC`) | Real, working (`HPDF_UA_BeginArtifact()`/`EndArtifact()`) -- brought forward from Milestone 4 after a real veraPDF failure |
| `/Tabs /S` (tab/reading order) | Real, working -- automatic on every page any tagging call touches (`hpdf_ua_find_or_create_page_entry()`), Milestone 4 |
| Outline/bookmarks | Real, working (each demo now calls `HPDF_CreateOutline()` + a real page destination) -- Milestone 4. One documented limitation: libharu's outline model (like base PDF) is page/destination-based, not structure-element-based; there is no PDF-standard way to point an outline entry directly at a `/StructElem` object |
| XMP `/Metadata` stream | Real, working (`HPDF_UA_AddMetadata()`, a minimal purpose-built writer -- not a reuse of libharu's PDF/A `HPDF_PDFA_AddXmpMetadata()`, which would collide with this project's already-tagged tree) -- `dc:title` + `pdfuaid:part=1`, Milestone 4 |
| Embedded fonts | Real, working -- `fonts/DejaVuSans.ttf` (DejaVu fonts license, permissive/redistributable) loaded via `HPDF_LoadTTFontFromFile(..., HPDF_TRUE)` in place of Standard-14 Helvetica -- Milestone 4 |

First real, end-to-end veraPDF run (2026-09-12, `demo/tagged_table_demo.c`,
`--flavour ua1`): 104 of 106 checks passed at Milestone 1; both remaining
failures (the metadata and font rows above) were exactly the two gaps
Milestone 4 closed. **As of Milestone 4, all three tagged demos
(`tagged_table_demo`, `tagged_histogram_demo`, `tagged_skyline_demo`)
pass PDF/UA-1 validation outright -- 106/106 checks, veraPDF's
`--format text` output prints the literal word `PASS`.**
