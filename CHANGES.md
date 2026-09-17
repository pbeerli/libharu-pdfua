# Changes

Version numbering: `MAJOR.MINOR.PATCH`, starting at `0.1.0` (pre-1.0,
milestone-driven -- see `docs/roadmap.md`). Bump `MINOR` when a roadmap
milestone completes, `PATCH` for fixes within a milestone.

## 0.6.4 (2026-09-17) -- CI, CMake install target, API reference

- Added `.github/workflows/ci.yml`: builds on Linux and macOS for every
  push/PR, installs veraPDF via Homebrew, and runs both
  `tests/run_all_demos.sh` (PDF/UA-1 regression coverage) and the new
  `tests/test_install.sh` below.
- Added a real CMake install target: `cmake --install build` now
  installs `hpdf_ua` (and the vendored `hpdf` it depends on) plus a
  `hpdf_ua-config.cmake` package config, so a downstream project can
  `find_package(hpdf_ua)` and link `hpdf_ua::hpdf_ua` instead of
  vendoring this whole tree. Previously there were zero `install()`
  rules anywhere in this project's own `CMakeLists.txt`, and
  `vendor/libharu`'s own (EXCLUDE_FROM_ALL) install rules never ran --
  confirmed directly: `cmake --install build` was a complete no-op
  before this change.
- **A real, latent build bug found and fixed along the way, unrelated
  to installation itself**: `hpdf_ua`'s target include directories only
  ever exposed `vendor/libharu/include` as `PRIVATE` (for compiling
  `hpdf_ua`'s own `.c` files), never `PUBLIC`/`INTERFACE` -- even though
  `include/hpdf_ua/hpdf_ua.h` itself does `#include "hpdf.h"` and is a
  public header every consumer (including this project's own `demo/`
  programs) includes directly. This "worked" on the development machine
  only because of an unrelated, differently-versioned system-wide
  libharu install at `/usr/local/include` that the compiler's default
  search path silently fell back to (confirmed with `cc -H`) -- on a
  clean machine with no such stray install, every demo would fail to
  compile with "hpdf.h: No such file or directory", a real,
  environment-dependent "works on my machine" gap between the vendored
  and whatever-happens-to-be-installed libharu. Fixed by making
  `hpdf_ua`'s vendor include directories `PUBLIC` (via
  `BUILD_INTERFACE`/`INSTALL_INTERFACE` generator expressions, so both
  the in-tree build and an installed package get the right headers).
  Confirmed via `compile_commands.json`: demo compile commands now
  explicitly carry `vendor/libharu/include`, independent of whatever
  else is or isn't installed system-wide.
- Added `tests/test_install.sh` and `tests/consume_package/` (a minimal
  separate CMake project, not part of the main build): installs to a
  throwaway prefix, configures and builds `consume_package` against it
  with `find_package(hpdf_ua)`, and runs the resulting binary -- proving
  the install target actually works end-to-end, not just that
  `cmake --install` exits zero. Verified directly before being wired
  into CI.
- Added `docs/api.md`: a grouped map of every function in `hpdf_ua.h`
  plus a minimal complete usage example, since the demos were previously
  the only usage documentation.
- README: added a "Requirements" section (CMake/compiler prerequisites,
  including the macOS Xcode-license gotcha that blocks `cc` entirely
  until `sudo xcodebuild -license` is accepted) and a "Using this
  library in your own project" section covering the new install target.

## 0.6.3 (2026-09-14) -- implement HPDF_UA_SetActualText() (was a stub)

- `HPDF_UA_SetActualText()` had been a documented no-op stub since
  Milestone 4. Implemented for real: sets `/ActualText` (PDF 32000-1
  14.9.4) on a structure element's dict, the same `HPDF_String_New()`
  pattern `HPDF_UA_SetAlternateText()` already uses for `/Alt`, but
  reusing whichever "UTF-8" encoder the caller registered via
  `HPDF_UseUTFEncodings()` (via `HPDF_Doc_FindEncoder()`, which -- unlike
  `HPDF_GetEncoder()` -- returns NULL with no error side effect when
  nothing is registered, so plain-ASCII callers with no UTF-8 encoder set
  up fall back to a plain PDFDocEncoding string) so non-ASCII
  `actual_text` (e.g. Greek letters) round-trips as real UTF-16BE instead
  of being misread byte-for-byte.
- Also added `HPDF_UA_BeginMarkedContentWithActualText()`, writing the
  same `/ActualText` directly into the BDC operand dictionary in the
  content stream (PDF 32000-1 14.9.4 also allows it there, not just on
  the structure element) -- a checker walking the content stream itself
  for a text equivalent may look for it at that level rather than (or in
  addition to) the structure element's own copy.
- Motivating case, found integrating this project into Migrate-n: a
  real-world PDF/UA checker (avalpdf, backed by PDFix SDK) reported a
  TD/TH's content as empty whenever that content was drawn through the
  Identity-H/CID "UTF-8" font -- suspected at the time to be a gap this
  checker's own structure-tree text extraction had for CID-font content,
  which `/ActualText` (on the structure element and/or the marked-content
  span) would sidestep by giving it a direct, tool-independent Unicode
  text equivalent. **Correction, same investigation**: the real cause was
  unrelated to this library -- the consuming project's own Autotools
  build was silently linking a libharu build from before 0.6.2's
  `/ToUnicode` fix even existed (a stale prebuilt static library its
  Makefile never knew to rebuild), so the checker was correctly reporting
  a font it was actually still seeing the pre-0.6.2 broken CMap for.
  Once that got rebuilt, the checker read the CID font's own `/ToUnicode`
  content just fine, `/ActualText` or not. Both functions above are kept
  regardless -- they are correct, real PDF/UA-1 features on their own
  merits (some checkers or assistive technology genuinely do prefer
  `/ActualText` over walking a content stream) -- just not, in the end,
  what actually explained this specific symptom.

## 0.6.2 (2026-09-13) -- fix: invalid /ToUnicode CMap for UTF-8/CID fonts

- `vendor/libharu/src/hpdf_font_cid.c` -- the one exception to this
  project's "vendor/libharu is unmodified" rule, plainly marked here and
  in `NOTICE.md`/the file itself. `HPDF_Type0Font_New()`'s "Identity-H"
  branch (used only by `HPDF_UseUTFEncodings()`'s "UTF-8" encoder -- no
  CJK encoder in this codebase uses that ordering) reused `CreateCMap()`
  for the font's `/ToUnicode` entry, which emits `cidrange`/`cidchar`
  operators -- valid for a font's `/Encoding` CMap, not for `/ToUnicode`
  (PDF32000-1:2008 9.10.3 requires `bfchar`/`bfrange`); real consumers
  (confirmed with veraPDF) correctly refuse to resolve any glyph through
  the resulting stream. Found and fixed while integrating this project's
  tagging module into Migrate-n's own report code, which draws Greek
  letters (Theta, Delta, alpha, mu, sigma) as real Unicode characters
  through an embedded Liberation Sans font reused as a UTF-8/CID font,
  rather than the old Symbol-font ASCII-remapping trick. New
  `CreateToUnicodeCMap()` emits a real `bfrange`-based CMap instead, split
  into 256-code single-row chunks (a bfrange whose low byte overflows its
  own row does not resolve correctly in practice -- confirmed directly:
  ASCII digits worked through one `<0000> <FFFF>` range, U+0398 GREEK
  CAPITAL LETTER THETA did not, until splitting fixed both). Verified via
  veraPDF against a real embedded-Liberation-Sans-as-Unicode-CID-font
  page: PDF/UA-1 clause `7.21.7` ("glyph cannot be mapped to Unicode")
  went from 64 failing checks to 0, full document compliance
  (`isCompliant="true"`); this project's own `tests/run_all_demos.sh`
  re-run clean afterward, all 8 demos at their recorded baseline
  (unaffected -- none of them exercise the UTF-8/CID path yet).

## 0.6.1 (2026-09-13) -- fix: missing H1 heading in four demos

- `tagged_table_demo.c`, `tagged_image_demo.c`, `tagged_histogram_demo.c`,
  `tagged_skyline_demo.c` (all predating the H1 convention `font`/
  `annotation`/`example` demos already had) now each open with a real H1
  title. Found via `avalpdf`, which flagged "Document has no headings" as
  an issue on all four; each now scores 100% clean under `avalpdf` with no
  change to its recorded veraPDF baseline (still 106/106).

## 0.6.0 (2026-09-13) -- Milestone 6 first pass: font/image/annotation demos, test coverage

- New `demo/tagged_font_demo.c` (ported from libharu's `ttfont_demo.c`):
  embedded DejaVu Sans specimen text tagged as `Document > [H1, P, H2,
  P, H2, P]`. **106/106 PDF/UA-1 checks, `PASS` on the first attempt**
  (1025/1025 individual checks); `leaks --atExit` clean.
- New `demo/tagged_image_demo.c` (ported from libharu's
  `raw_image_demo.c`, not `png_demo.c`/`jpeg_demo.c` -- this project's
  build disables libpng discovery): two runtime-computed raw images (an
  RGB gradient, a grayscale ramp), each a tagged `Figure` with accurate
  `/Alt` text plus its own `Caption`. No new vendored binary asset, so
  no new `NOTICE.md`/license entry needed. **106/106, `PASS` on the
  first attempt** (469/469 checks); `leaks --atExit` clean.
- New `demo/tagged_annotation_demo.c` (ported from libharu's
  `link_annotation.c`): three real link annotations (two internal, one
  URI), each tagged via a `Link` structure element. Needed a real new
  addition, not just porting existing calls: **`HPDF_UA_TagAnnotation()`**
  (`include/hpdf_ua/hpdf_ua.h`, `src/ua/hpdf_ua_structure.c`) -- adds a
  real `/OBJR` structure-tree kid, a `/StructParent` key into the
  existing `/ParentTree`, and normalizes `/F` to Print-set/NoView-clear
  (ISO 14289-1:2014 7.18). A real veraPDF run caught one further gap not
  found by inspection: link annotations also need their own `/Contents`
  (ISO 14289-1:2014 7.18.5, PDF 32000-1 14.9.3) -- a `/Alt` on the
  structure element alone doesn't satisfy it. Fixed by having
  `HPDF_UA_TagAnnotation()` copy the element's `/Alt` onto the
  annotation's `/Contents`. **Result: 106/106, `PASS`** (787/787
  checks, up from 105/106 before the `/Contents` fix); `leaks --atExit`
  clean.
- All five pre-existing demos re-verified unchanged: `tagged_table_demo`,
  `tagged_histogram_demo`, `tagged_skyline_demo`, `tagged_example_demo`
  still 106/106 with 0 leaks each; `docmeta_demo` unchanged at its own
  known 103/106 Milestone-0 baseline.
- New `tests/run_all_demos.sh`: builds all eight demos and validates
  each against `validate/run_verapdf.sh`, asserting a per-demo recorded
  baseline (0 failed rules for the seven fully tagged demos, at most 3
  for `docmeta_demo`'s own documented partial scope), failing loudly
  and naming the regressed demo(s) otherwise. Verified to actually catch
  a regression: a temporary no-op patch to `HPDF_UA_SetAlternateText()`
  made the script correctly fail exactly the five demos that depend on
  `/Alt`, reverted afterward with a clean rebuild reconfirming all eight
  demos back at baseline.
- Full writeup, including the deliberately out-of-scope remainder (~22
  more upstream demos not yet ported), in `docs/roadmap.md`'s Milestone
  6 section.

## 0.5.1 (2026-09-12) -- combined text+table+figure example

New `demo/tagged_example_demo.c`: a single document mixing ordinary
paragraph text (H1 + P, using `HPDF_Page_TextRect()` for real word-wrapped
body text -- the per-shape demos through Milestone 3 only ever used
single-line `HPDF_Page_ShowText()`), a table, and a figure together under
one shared `Document` root, plus a real multi-entry outline -- the one
combination the earlier, single-shape demos never tested together. Uses
the full Milestone 4 recipe (real metadata, embedded font, outline) from
the start. **Result: 106/106 PDF/UA-1 checks, veraPDF prints `PASS`, on
the first attempt** (1063/1063 individual checks); `leaks --atExit`
clean. Confirms the shared tagging machinery composes correctly across
mixed content types on one page/document, not just one shape at a time.

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
