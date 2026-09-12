# Changes

Version numbering: `MAJOR.MINOR.PATCH`, starting at `0.1.0` (pre-1.0,
milestone-driven -- see `docs/roadmap.md`). Bump `MINOR` when a roadmap
milestone completes, `PATCH` for fixes within a milestone.

## Milestone 5 (2026-09-12) -- decision recorded, no code change

Decided: the prototype succeeded (106/106 PDF/UA-1 across three genuinely
different content shapes, zero leaks) -- integrate, but narrowly, as a
new `report_pdf_tagged.c` backend consuming Migrate's existing
`report_model.c` object (`report_document_fmt`/`report_section_fmt`),
not a `pretty.c` rewrite. Checked directly against Migrate's actual
source: `report_model.h` already declares itself backend-agnostic
("HTML/PDF are meant to reuse the same document unchanged later"),
already carries an `alt_text` field on every figure, and its
`report_figure_kind_t` (`HISTOGRAM`/`LINE_SERIES`) matches this project's
Milestone 2/3 shapes exactly. Real remaining gap identified precisely:
page-layout logic (genuinely new work) and confidence-band
(`y_err`) rendering (not yet exercised by any demo here) -- not "one
implementation per table type," since all 16 of Migrate's table builders
already share one generic shape. Scheduling this integration against
Migrate's other priorities is an explicitly open call, not decided here
-- see `plan.md`. Full writeup in `docs/roadmap.md`.

## 0.5.0 (2026-09-12) -- Milestone 4: outline, tabs, metadata, embedded fonts -- full PDF/UA-1 conformance

- New `HPDF_UA_AddMetadata()`: a minimal, purpose-built XMP `/Metadata`
  stream writer (`dc:title` + `pdfuaid:part=1`), deliberately not a reuse
  of libharu's own PDF/A `HPDF_PDFA_AddXmpMetadata()` (which would
  unconditionally recreate `/MarkInfo`/`/StructTreeRoot` and collide with
  this project's already-tagged tree). Fixes ISO 14289-1:2014 7.1/8.
- `/Tabs /S` now set automatically on every page any tagging call
  touches, baked into the shared `hpdf_ua_find_or_create_page_entry()`
  helper -- no separate call needed, applies to all three demos for free.
- Vendored `fonts/DejaVuSans.ttf` (+ `fonts/DejaVuSans-LICENSE.txt`,
  DejaVu fonts license, permissive/redistributable) and switched all
  three tagged demos from Standard-14 Helvetica to this embedded font via
  `HPDF_LoadTTFontFromFile(..., HPDF_TRUE)`. Fixes ISO 14289-1:2014
  7.21.4.1/1.
- All three demos now create a real `HPDF_CreateOutline()` entry with a
  real page destination ("Table", "Histogram", "Skyline plot") -- tied to
  the actual page the content lives on, since libharu's (and base PDF's)
  outline model has no way to reference a `/StructElem` object directly.
- **Result: all three tagged demos now pass PDF/UA-1 validation outright
  -- 106/106 checks, veraPDF's `--format text` output prints the literal
  word `PASS`, not just a lower failure count.** Verified via a clean
  rebuild from scratch, direct byte inspection (`/Outlines`,
  `/Metadata`+`pdfuaid`, `FontFile2`+`DejaVuSans`, `/Tabs` all present),
  and `leaks --atExit` on each (zero leaks).
- `HPDF_UA_SetTableDataHeaders()` (`/Headers`, irregular tables) remains
  the one deliberately-out-of-scope stub -- `/Scope` covers this
  project's actual simple-table needs, no consumer has needed it yet.

## 0.4.0 (2026-09-12) -- Milestone 3: tagged skyline/multi-series plot page

- New `demo/tagged_skyline_demo.c`: a two-population "Theta through time"
  line plot -- `Document > Figure` (axes, two color/dash-distinguished
  polylines, decorative legend swatches, one marked-content span, rich
  `/Alt` text) followed by two `Document > P` elements (first real use of
  the `P` role) for the legend's text labels, then `Document > Caption`.
- New content shapes: polylines (`moveto`/`lineto` chains) and a dashed
  stroke (`HPDF_Page_SetDash()`) -- no changes needed to
  `HPDF_UA_BeginMarkedContent()`/`EndMarkedContent()`, confirming again
  they are content-agnostic.
- Explores the milestone's real design question: a deliberate
  reading-order decision (legend labels pulled out as separate `P`
  elements after the Figure, in a fixed order matching their visual
  position) rather than folding them into `/Alt` or leaving order
  implicit -- contrasted deliberately with axis titles, which stayed in
  `/Alt`, so this demo exercises both patterns.
- Verified: direct byte inspection, `validate/run_verapdf.sh`
  (**104/106 PDF/UA-1 checks pass, clean on the first attempt** -- same
  two known gaps, no new ones), `leaks --atExit` (zero leaks). Third demo
  in a row clean on first attempt.

## 0.3.0 (2026-09-12) -- Milestone 2: tagged figure/histogram page

- New `demo/tagged_histogram_demo.c`: a real bar-chart histogram
  (`Document > Figure`, one marked-content span covering axis + 10 bars +
  tick labels, real `/Alt` text) plus `Document > Caption` (its own
  tagged text). Matches Migrate's own `plot_svg.c`
  `REPORT_FIGURE_HISTOGRAM` renderer shape (one filled rectangle per bin).
- Confirms `HPDF_UA_BeginMarkedContent()`/`EndMarkedContent()` generalize
  to path-painting content (`re`/`f`, `m`/`l`/`S`), not just the
  text-showing content (`BT`/`Tj`/`ET`) Milestone 1's table demo used --
  no code change needed, they were already content-agnostic.
- Verified: direct byte inspection, `validate/run_verapdf.sh`
  (**104/106 PDF/UA-1 checks pass, clean on the first attempt** -- no new
  failures beyond the same two Milestone-1 gaps), `leaks --atExit` (zero
  leaks).

## 0.2.0 (2026-09-12) -- Milestone 1: structure tree / marked content, veraPDF-validated

- New `HPDF_UA_Context` (`HPDF_UA_NewContext()`/`HPDF_UA_FreeContext()`):
  an explicit resource handle owning this project's tagging bookkeeping
  (per-page `/StructParents`/MCID tracking, `/ParentTree`), since libharu's
  own `HPDF_Doc` struct is not modified by this project and a
  module-global registry would be unsafe across multiple documents.
- Real, working: `HPDF_UA_BeginStructureElement()`/`EndStructureElement()`
  (real `/StructTreeRoot` population), `HPDF_UA_BeginMarkedContent()`/
  `EndMarkedContent()` (real `BDC`/`EMC` + MCID + `/ParentTree`),
  `HPDF_UA_SetAlternateText()` (`/Alt`), `HPDF_UA_SetTableHeaderScope()`
  (`/Scope`) -- the latter two brought forward from their original
  Milestone 2 slot since they were one-line additions once
  `HPDF_UA_StructElem` existed.
- `HPDF_UA_BeginArtifact()`/`HPDF_UA_EndArtifact()`: brought forward from
  Milestone 4 after `demo/tagged_table_demo.c`'s first real veraPDF run
  found an actual, present untagged-content failure on its own decorative
  table border (ISO 14289-1:2014 7.1/3) -- not held for a later milestone
  once it was a real, not hypothetical, gap.
- Two real bugs found and fixed via that same veraPDF run (not by
  inspection): a malformed `/Artifact BDC` (missing the properties operand
  `BDC` always requires; `BMC` is correct for "no properties"); and
  `HPDF_UA_EnableTagging()`'s `struct_tree_root` never being
  `HPDF_Xref_Add()`-registered, which made every top-level
  `HPDF_UA_BeginStructureElement()` call fail once more than one
  structure element tried to reference it (libharu marks non-xref
  objects `HPDF_OTYPE_DIRECT`, "owned by exactly one container," the
  instant they're first added anywhere, and refuses a second reference).
- `demo/tagged_table_demo.c` rewritten to produce a real tagged
  `Document > Table > TR > TH/TD` tree (15 MCIDs). Verified: direct byte
  inspection, a real `validate/run_verapdf.sh` run (**104/106 PDF/UA-1
  checks pass**), and `leaks --atExit` (zero leaks).
- Two real, understood gaps remain, out of this milestone's scope: no
  XMP `/Metadata` stream yet (ISO 14289-1:2014 7.1/8); Standard-14 fonts
  (e.g. the demos' Helvetica) are never embedded (ISO 14289-1:2014
  7.21.4.1/1). Both deferred to Milestone 4 -- see `docs/roadmap.md`.

## 0.1.0 (2026-09-12) -- project scaffolded

- Vendored libharu 2.4.5 unmodified under `vendor/libharu/` (one release
  behind current upstream 2.4.6 -- see `docs/roadmap.md`'s research-findings
  note; rebasing onto 2.4.6 is an early to-do, not yet done).
- New `hpdf_ua` module (`include/hpdf_ua/hpdf_ua.h`, `src/ua/`):
  - **Real, working**: `HPDF_UA_SetDocumentLanguage()` (catalog `/Lang`,
    absent from libharu entirely until now), `HPDF_UA_SetDisplayDocTitle()`
    (`/ViewerPreferences /DisplayDocTitle`, also absent), `HPDF_UA_EnableTagging()`
    (idempotent `/MarkInfo` + `/StructTreeRoot` setup).
  - **Stubs, API shape fixed, not yet implemented** (all return
    `HPDF_UA_NOT_YET_IMPLEMENTED`): structure-element and marked-content
    tagging, artifact marking, alternate text, table header association --
    Milestone 1+.
- CMake build (`add_subdirectory` on the vendored libharu, same recipe
  migrate-n's own build uses) producing the `hpdf_ua` static library plus
  two demos (`docmeta_demo`, `tagged_table_demo`).
- `validate/run_verapdf.sh`: veraPDF CLI wrapper (local launcher or Docker
  fallback), ready for Milestone 1's first real validation target.
- `docs/pdf_ua_requirements.md`: the PDF/UA-1 technical checklist, including
  research-confirmed additions (embedded fonts, `/StructParents`,
  `/ActualText` for non-Unicode-mappable glyphs, heading nesting, list
  numbering, link structure elements, encryption restrictions, and a
  PDF/UA-1-vs-UA-2 targeting decision).
- `docs/roadmap.md`: six milestones (scaffolding through "decide whether to
  vendor this into Migrate"), plus research findings on libharu's actual
  maintenance state, two long-open relevant upstream issues, and a flagged
  open question (PDFio as a possible alternative base) not yet acted on.
