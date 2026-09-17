# Roadmap

Adapted from migrate-n's own modernization plan (`plan.md`, "Independent Haru
project" / "Near-term report prototype" / "Haru tagging feasibility"
sections), refined for this standalone project.

## Milestone 0 -- scaffolding (this commit)

- Vendor libharu 2.4.5 unmodified under `vendor/libharu/`.
- Stand up the `hpdf_ua` module skeleton (`include/hpdf_ua/hpdf_ua.h`,
  `src/ua/`) with a small number of real, working, low-risk additions:
  - `HPDF_UA_SetDocumentLanguage()` -- real `/Lang` catalog entry (a genuine
    gap in libharu 2.4.5, not present in any form).
  - `HPDF_UA_SetDisplayDocTitle()` -- real `/ViewerPreferences
    /DisplayDocTitle` support (extends `HPDF_Catalog_SetViewerPreference`'s
    existing bitflag mechanism with a new flag).
  - The structure-tree/marked-content API surface declared (function
    signatures fixed, matching the target design below) but not yet
    implemented -- calling them returns `HPDF_UA_NOT_YET_IMPLEMENTED` rather
    than silently doing nothing, so callers can't mistake a stub for a
    working tag.
- CMake build producing a static `hpdf_ua` library (vendored libharu plus the
  new module) and a demo executable.
- `docs/pdf_ua_requirements.md`: the concrete technical checklist.
- `validate/run_verapdf.sh`: a wrapper script (documents veraPDF install,
  runs it against a generated PDF, reports pass/fail) -- ready to use as soon
  as there's real tagged output to validate.

## Milestone 1 -- one tagged table page (first real structure-tree work)

**Done and veraPDF-validated, 2026-09-12.** `HPDF_UA_Context` (a new,
explicit resource handle, since the tagging bookkeeping needs a home that
isn't the vendored `HPDF_Doc` struct -- see `hpdf_ua.h`'s design note),
`HPDF_UA_BeginStructureElement()`/`HPDF_UA_EndStructureElement()` (real
`/StructTreeRoot` population, tree linkage happens immediately at Begin
time), `HPDF_UA_BeginMarkedContent()`/`HPDF_UA_EndMarkedContent()` (real
`BDC .../EMC` + MCID + `/ParentTree`), `HPDF_UA_SetTableHeaderScope()`
(`/Scope`, brought forward from Milestone 2 since it's a one-line addition
once a struct element exists), `HPDF_UA_SetAlternateText()` (`/Alt`,
likewise brought forward from Milestone 2), and
`HPDF_UA_BeginArtifact()`/`HPDF_UA_EndArtifact()` (brought forward from
Milestone 4 -- see below for why) are all real and working.

`demo/tagged_table_demo.c` produces a real `Document > Table > TR >
TH/TD` tree, one MCID per cell (15 for a 3-column/4-row-plus-header
table), verified two ways: direct inspection of the generated PDF's bytes
(role names, `/Scope`, MCID count, `/ParentTree`/`/StructParents` all
present and correct), and a real `validate/run_verapdf.sh` run --
**104 of 106 PDF/UA-1 checks pass.** `leaks --atExit` confirms zero memory
leaks for a full create-tag-save-free cycle.

**Two real bugs found and fixed by that first actual veraPDF run, not by
inspection** -- concrete evidence for why "validate end-to-end with
veraPDF" was the right bar to set, not just "looks right by eye":
1. The demo's decorative table-border rectangle was drawn with no BDC/EMC
   wrapping at all -- untagged-by-omission, correctly flagged by veraPDF
   (ISO 14289-1:2014 7.1/3, "content shall be marked as Artifact or
   tagged as real content"). Fixed by implementing
   `HPDF_UA_BeginArtifact()`/`HPDF_UA_EndArtifact()` for real now, instead
   of leaving them stubbed until Milestone 4 as originally planned -- a
   present, validated gap couldn't reasonably wait for a future milestone
   once it was real rather than hypothetical.
2. The first `HPDF_UA_BeginArtifact()` implementation wrote `/Artifact
   BDC` (one operand) -- syntactically invalid PDF, since `BDC` always
   requires a second (properties dict or name) operand; `BMC` is the
   correct one-operand form for "no properties." veraPDF correctly
   treated the malformed token as not-a-valid-tag, still failing the same
   check. Fixed by using `BMC` instead. This was caught only by actually
   running the real validator a second time after the first fix, not by
   re-reading the code -- the bytes looked plausible at a glance.

**Two real, understood gaps remain, deliberately not fixed in this pass
because they are outside this milestone's actual scope (structure tree /
marked content), not because they don't matter:**
- ISO 14289-1:2014 7.1/8: the document catalog has no `/Metadata` (XMP)
  stream. libharu's existing PDF/A support (`hpdf_pdfa.c`'s
  `HPDF_PDFA_AddXmpMetadata()`) has the machinery to write one, but reusing
  it directly would re-create its own (non-xref-registered)
  `/MarkInfo`+`/StructTreeRoot` alongside this project's own, exactly
  reintroducing the `HPDF_OTYPE_DIRECT` bug found and fixed below unless
  specifically guarded against -- a real integration task, not a one-line
  fix, left for a dedicated pass.
- ISO 14289-1:2014 7.21.4.1/1: the demo uses libharu's Standard-14
  `Helvetica` (never embedded, by PDF convention) -- PDF/UA-1 requires all
  rendering fonts to be embedded. This is a font-provisioning concern
  (needs a real TTF file with a permissive license bundled and loaded via
  `HPDF_LoadTTFontFromFile(..., HPDF_TRUE)`), unrelated to tagging; left
  for whichever real consumer (this project's own future demos, or
  Migrate's eventual integration) actually needs to ship a real embedded
  font.

**One more real bug found and fixed along the way, unrelated to PDF/UA
mechanics**: `HPDF_UA_EnableTagging()`'s `struct_tree_root` was never
`HPDF_Xref_Add()`-registered (a direct carry-over from the PDF/A code
path it was adapted from, which never needed to reference
`struct_tree_root` a second time). The moment Milestone 1's top-level
structure elements each tried to point their own `/P` back at
`struct_tree_root`, libharu's object model -- which marks any
non-xref-registered object `HPDF_OTYPE_DIRECT` ("owned by exactly one
container") the instant it is first added anywhere, silently refusing a
second `HPDF_Dict_Add()`/`HPDF_Array_Add()` with `HPDF_INVALID_OBJECT` --
made every single top-level `HPDF_UA_BeginStructureElement()` call fail.
Found via direct, deliberate instrumentation (temporary debug prints,
removed after use) once the first few debugging attempts themselves had
bugs (missing braces after scripted edits produced misleadingly
always-failing code) -- worth remembering: a debugging aid can have its
own bugs, re-check it before trusting its output. Fixed by registering
`struct_tree_root` in the xref immediately after creation, matching every
other shared object this project creates.

## Milestone 2 -- one tagged figure/histogram page

**Done and veraPDF-validated, 2026-09-12** (same day as Milestone 1;
`HPDF_UA_SetAlternateText()` itself had already been brought forward into
Milestone 1 as a one-line `/Alt` addition). `demo/tagged_histogram_demo.c`
draws a real bar-chart histogram (matching Migrate's own `plot_svg.c`
`REPORT_FIGURE_HISTOGRAM` renderer: one filled rectangle per bin) as
`Document > Figure` (axis + 10 bars + tick labels, all inside one
marked-content span, with real `/Alt` text describing the chart's actual
shape/peak) followed by `Document > Caption` (its own separately tagged
text, not folded into the `/Alt`).

This milestone's actual point -- confirming the API generalizes beyond
table cells -- is now empirically confirmed: Milestone 1's marked-content
machinery had so far only ever wrapped text-showing operators
(`BT`/`Tj`/`ET`); this demo wraps **path-painting operators** (`re`/`f` for
each bar, `m`/`l`/`S` for the axis lines) inside `BDC`/`EMC` too, with no
code change needed to `HPDF_UA_BeginMarkedContent()`/`EndMarkedContent()`
-- they were already content-agnostic, just writing the same operators
around whatever the caller draws in between.

Verified the same way as Milestone 1: direct byte inspection (10 `re`/`f`
pairs with heights correctly proportional to the fake density data, 2
`BDC`/`EMC`/MCID pairs, `/Alt` present once), a real
`validate/run_verapdf.sh` run (**104/106 PDF/UA-1 checks pass -- clean
on the first attempt**, no new failures beyond the same two Milestone-1
gaps below), and `leaks --atExit` (zero leaks). No new bugs found this
time -- both real defects Milestone 1 uncovered (the malformed
`/Artifact BDC`/`BMC` mixup and the un-registered `struct_tree_root`)
were structural/shared, not table-specific, so fixing them once already
covered this milestone too.

## Milestone 3 -- one tagged line/skyline-style plot page

**Done and veraPDF-validated, 2026-09-12** (same day as Milestones 1-2;
`Caption` had already entered the role map in Milestone 2, ahead of this
milestone's original expectation that it would be needed here first).
`demo/tagged_skyline_demo.c` draws a two-population "Theta through time"
line plot (`Document > Figure`: axes, two color/dash-distinguished
polylines, and decorative legend swatches, all in one marked-content span
with rich `/Alt` text describing both series' trends) followed by two
`Document > P` elements (the first real use of the `P` role in this
project) for the legend's text labels, and finally `Document > Caption`.

New content shapes exercised beyond Milestones 1-2: **polylines**
(`moveto`/`lineto` chains, not single closed rectangles) as the plotted
data itself, and a **dashed stroke** (`HPDF_Page_SetDash()`) distinguishing
the two series without relying on color alone. `HPDF_UA_BeginMarkedContent()`/
`EndMarkedContent()` needed no changes for either -- confirms, for the
second milestone in a row, that they are genuinely content-agnostic.

**The real reading-order decision this milestone was scoped to explore**:
rather than fold the legend's text labels into the Figure's own `/Alt`
(as Milestone 2 did for axis meaning) or leave them implicit in whatever
order drawing code happened to touch them, they are pulled out as
separate, real `P` structure elements, placed as `Document` children
**after** the Figure in a fixed order matching their top-to-bottom visual
position next to the legend swatches -- a deliberate choice, not an
accident of call order (since this project's design links tree order to
`HPDF_UA_BeginStructureElement()` call order, the caller controls it
directly). Axis titles ("Theta"/"Time") stayed folded into the Figure's
`/Alt` text, a legitimate alternative pattern, kept different from the
legend case deliberately so this demo exercises both approaches rather
than picking one uniformly.

Verified the same way as Milestones 1-2: direct byte inspection (4
`BDC`/`EMC`/`MCID` pairs -- Figure, two `P`s, Caption -- correct dash-array
tokens on Population 2's line and its legend swatch, solid reset
afterward), a real `validate/run_verapdf.sh` run (**104/106 PDF/UA-1
checks pass, clean on the first attempt** -- same two known gaps, no new
ones), and `leaks --atExit` (zero leaks). Three real demos in a row now
building clean on the first veraPDF attempt confirms the two bugs found
during Milestone 1 were the only structural defects in the shared
machinery -- new content shapes exercise it without surfacing anything
new.

## Milestone 4 -- outline/bookmarks, document-level polish

**Done and veraPDF-validated, 2026-09-12** (same day as Milestones 1-3;
`HPDF_UA_BeginArtifact()`/`HPDF_UA_EndArtifact()` had already been brought
forward into Milestone 1). **All three tagged demos now pass PDF/UA-1
validation outright -- 106/106 checks, veraPDF prints `PASS`, not just
"fewer failures."** This closes the two real gaps every demo through
Milestone 3 had:

- **XMP `/Metadata` stream**: `HPDF_UA_AddMetadata()`, a new, minimal,
  purpose-built XMP writer (not a reuse of libharu's own PDF/A
  `HPDF_PDFA_AddXmpMetadata()`, which would have unconditionally
  recreated `/MarkInfo`/`/StructTreeRoot` and collided with this
  project's already-tagged tree). Writes `dc:title` (from the document's
  existing `/Title`) and declares PDF/UA-1 conformance
  (`pdfuaid:part=1`) -- real, working, idempotent.
- **Embedded fonts**: vendored `fonts/DejaVuSans.ttf` (DejaVu fonts
  license, based on Bitstream Vera -- explicitly permissive and intended
  for redistribution; `fonts/DejaVuSans-LICENSE.txt` included) in place
  of the Standard-14 Helvetica every demo used through Milestone 3. All
  three demos now load it via `HPDF_LoadTTFontFromFile(pdf, ..., HPDF_TRUE)`.

Also done, per the roadmap's original scope:

- **`/Tabs /S` on every tagged page**: made automatic, not a separate call
  -- baked into `hpdf_ua_find_or_create_page_entry()` (the same internal
  helper `HPDF_UA_BeginMarkedContent()`/`HPDF_UA_BeginArtifact()` already
  use to register a page's first use), so every page any of this
  project's tagging functions ever touch gets it for free.
- **Document outline tied to real content**: each of the three demos now
  creates a real `HPDF_CreateOutline()` entry (e.g. "Table", "Histogram",
  "Skyline plot") with a real page destination
  (`HPDF_Page_CreateDestination()`/`HPDF_Destination_SetXYZ()`). One real
  limitation, documented rather than solved: libharu's outline API (like
  the base PDF outline model itself) is page/destination-based, not
  structure-element-based -- there is no PDF-standard mechanism to point
  an outline entry directly at a `/StructElem` object, so this ties each
  entry to the closest real thing available, the actual page the content
  lives on, not a deeper structural link.

**Not done, deliberately out of this pass's scope** (not requested this
round): `HPDF_UA_SetTableDataHeaders()` (`/Headers`, for irregular
tables) remains stubbed -- `/Scope` already covers this project's actual
simple-table needs, and no consumer has needed irregular-table support
yet.

Verified the same way as every prior milestone, now with a stronger bar:
direct byte inspection (`/Outlines`, `/Metadata`+`pdfuaid`, `FontFile2`+
`DejaVuSans`, `/Tabs` all present and correct across all three demos), a
clean rebuild from scratch, a real `validate/run_verapdf.sh` run on each
demo (**106/106 PDF/UA-1 checks pass -- veraPDF's own text-format output
now prints the literal word `PASS`, not just a lower failure count**),
and `leaks --atExit` on each (zero leaks, all three).

## Milestone 5 -- decide

**Decided 2026-09-12: the prototype succeeded (it's the first branch of
the original either/or, not the second) -- integrate, but narrowly, as a
new additive backend, not a `pretty.c` rewrite. Timing/scheduling of the
actual integration work is a separate, still-open call, not decided here.**

The original framing ("if it fails, keep Markdown/SVG without carrying a
half-working implementation") assumed failure was a live possibility.
It wasn't, by the time this milestone was reached: three genuinely
different content shapes (table, bar-chart figure, multi-series line
plot), all independently veraPDF-validated to full PDF/UA-1 conformance
(106/106), zero memory leaks. So the real question at this milestone
turned out to be *how* to integrate, not *whether* the prototype earned
it.

**Checked directly against Migrate's actual source, not assumed:**
Migrate's Phase 5 report work already built exactly the right seam for
this, apparently without knowing it would matter this precisely later.
`report_model.h`'s own top comment: *"a generic document made of tables
and figures, with no knowledge of MCMC/Bayesian specifics and no
knowledge of any particular output backend (Markdown/SVG today; HTML/PDF
are meant to reuse the same document unchanged later)."* Its
`report_figure_fmt` already carries a 256-byte `alt_text` field --
accessibility text was already a first-class part of the model before
this project existed, mapping directly onto
`HPDF_UA_SetAlternateText()`. Its `report_figure_kind_t` is exactly
`{HISTOGRAM, LINE_SERIES}` -- precisely this project's Milestone 2 and
Milestone 3 shapes. And `report_markdown.c` (the existing Markdown
backend) is a genuinely small, generic ~35-line walker over
`report_document_fmt`'s sections, dispatching once on
`section->kind == TABLE` vs. `FIGURE` -- a real, working precedent for
exactly the size and shape a `report_pdf_tagged.c` counterpart would
need, not a hypothetical one.

**What this means concretely, if/when this integration is scheduled:**

- A new file, `report_pdf_tagged.c` (matching this project's `core/report/`
  target module map's already-reserved `report_pdf_haru.c` slot almost
  exactly), walking the *same* `report_document_fmt` object every other
  backend already consumes -- no changes needed to `report_model.c`
  itself, or to any of the 16 `report_add_*_table`/`report_add_*_figure`
  builder functions, or to `generate_markdown_report_prototype()` beyond
  one added call once the new writer exists.
- `pretty.c` (the current, legacy, actually-shipped Haru PDF path) is not
  touched at all -- this is a genuinely new, additive output option,
  exactly mirroring how the Markdown/SVG report itself was added
  alongside the legacy PDF without disturbing it.
- Vendor this project's `hpdf_ua` module (`include/hpdf_ua/`, `src/ua/`)
  directly into `source/migrate-codex-7/`, linking against Migrate's
  *own already-vendored* libharu copy (`lib/haru`) rather than adding a
  second, redundant one -- the module was deliberately built using only
  libharu's public/semi-public headers for exactly this kind of reuse.
- **The real remaining engineering gap is narrower than "port every
  report type" might suggest, and is concentrated in two places, not
  spread across all 16 table/figure builders**: every table already
  shares one generic shape (`report_table_fmt`: column names + a cells
  grid) regardless of which builder produced it, so one table-rendering
  routine (mirroring `write_table_section()`) covers all 16, the same
  way it already does for Markdown. The two real open pieces are (1)
  **page layout** -- deciding where each section lands on which page,
  when to start a new one, margins/font sizing -- something no existing
  backend needs (Markdown flows; SVG figures are self-contained), so
  this is genuinely new work, not a port; and (2) **confidence-band
  rendering** -- `report_figure_series_fmt.y_err` (an optional per-point
  error-band half-width) was never exercised by this project's own
  Milestone 3 skyline demo, which deliberately used two plain series
  with no bands, so that drawing case (a filled band or paired
  offset lines) still needs a first real implementation.
- Two real, unresolved decisions of Migrate's own making, not this
  project's to answer alone: whether a fully tagged PDF is meant to
  *replace* `pretty.c`'s output eventually, or exist permanently
  alongside it as a third format (next to Markdown and the legacy PDF);
  and whether Phase 5.5's still-open report-graphics-quality gap (the
  user's own "my PDF plotting looks 100x better than your .md -> PDF"
  reaction) should be resolved on `plot_svg.c` first, since a
  `report_pdf_tagged.c` backend would likely reuse or closely mirror
  whatever plotting primitives that work settles on.

**Not decided here, deliberately**: *when* to schedule this integration
work against Migrate's other active priorities (Phase 6 performance,
Phase 7 refactoring). That is a real scope/timing call for Migrate's own
project owner, recorded as explicitly open in `plan.md`, not assumed
either way by this side project.

## Research findings (2026-09-12) that inform this roadmap

- **Upstream libharu status**: trickle-maintained (bug-fix/build/security
  commits from rotating drive-by contributors), explicitly described on its
  own site as needing a new maintainer. Latest upstream release is
  **2.4.6** (2026-03-26); this project vendored **2.4.5**, one release
  behind -- worth diffing and rebasing onto 2.4.6 early, before this
  project's own additions grow large enough to make that diff painful.
  Two long-open upstream issues (#99, opened 2015; #175, opened 2018) both
  request tagged-PDF support with zero maintainer engagement in either --
  real, long-standing demand this project is finally acting on, and
  plausibly worth eventually offering the result back upstream once it
  proves out here, rather than staying a permanent fork.
- **No prior art found for this specific effort** (adding PDF/UA to
  libharu, or a comparable from-scratch tagged-PDF-in-C project, 2024-2026).
  This is genuinely under-documented territory -- no roadmap to copy, so
  treat early milestone estimates as uncertain, and expect the structure-
  tree/content-stream synchronization problem (keeping the logical
  structure tree and the physical content stream in agreement as content is
  added) to be the main source of real design difficulty, per the closest
  comparable writeup found (a commercial Java PDF library's account of the
  same problem, cited in `pdf_ua_requirements.md`'s reference-material
  section).
- **A close, actively-maintained alternative exists and is worth a second
  look before this goes too far: PDFio** (michaelrsweet/pdfio, Apache-2.0, C,
  single expert maintainer, active as of 2026). It already has
  `pdfioContentBeginMarked`/`EndMarked` (BDC/EMC + MCID) and a `/Lang`
  setter merged (v1.6.0, 2025-10-06) -- ahead of where this project starts.
  It's unclear from documentation alone whether PDFio has any
  `/StructTreeRoot`/role-map/`/ParentTree` authoring API beyond raw
  marked-content operators (i.e. whether it has solved more of the actual
  hard problem, or just the same easy part this project can also add to
  libharu quickly). Worth reading `pdfio.h`/`pdfio.md` directly and
  comparing before this project's structure-tree work (Milestone 1) goes
  very far -- if PDFio turns out to already be closer to real PDF/UA
  authoring, extending it instead of libharu could be less total work,
  though it would mean leaving libharu's already-integrated PDF/A support
  and Migrate's existing PDF rendering code behind. Noted here as a
  decision point, not acted on -- the project proceeds on libharu per the
  explicit choice that started it.

## Explicitly out of scope for this project

- Anything specific to Migrate's own data model (`world_fmt`, `bayes_fmt`,
  MCMC state). This project only ever accepts plain structure/content
  descriptions and produces a tagged PDF; a separate adapter layer inside
  Migrate itself (not here) is responsible for turning Migrate's report
  model into calls against this API.
- Non-PDF output formats (this project is PDF/UA-specific; Migrate's
  Markdown/SVG/HTML report work is a separate, already-largely-complete
  track).
- Windows (MSVC) as a native target -- Unix/Linux/macOS, and Windows only via
  a POSIX-compatible layer (WSL/Cygwin/MSYS2), matching Migrate's own current
  platform decision.

## Milestone 6 -- toward a public release (recorded 2026-09-12, not started)

Per the user's stated intent: once this project is further along, make it a
public GitHub repository so others can use the library.

Two concrete prerequisites the user specifically named, neither started yet:

- **More test coverage of libharu's own original functionality**, not just
  this project's own tagging additions. This project's vendored copy
  (`vendor/libharu/`) deliberately dropped upstream libharu's own `demo/`
  directory when first vendored (see `NOTICE.md`) to keep the initial scaffold
  lean. That directory (confirmed still present in migrate-codex-7's own
  untouched `lib/haru/demo/`, ~25 programs) covers real functionality this
  project's five demos never touch: vector graphics (`arc_demo`, `line_demo`),
  fonts (`chfont_demo`, CJK `jpfont_demo`/`ttfont_demo_jp`, TrueType
  `ttfont_demo`, Type1, `character_map`, `encoding_list`), images (`png_demo`,
  `jpeg_demo`, `raw_image_demo`), encryption/permissions (`encryption.c`,
  `permission.c`), annotations (`link_annotation`, `text_annotation`),
  outlines (`outline_demo`/`_jp`), attachments (`attach.c`), PDF/A conformance
  (`pdf_a_conformance.c`), and a slide-show/grid-sheet layout example.
- **Recreate that demo set as tagged examples** using this project's own API
  (`HPDF_UA_Context`, structure elements, marked content) rather than leaving
  them as libharu's original untagged versions -- i.e. this project's real
  test/demo suite should eventually cover the full breadth of what libharu
  itself can do, all of it accessibly tagged, not just the five report-shaped
  examples (table/histogram/skyline/example/docmeta) built so far.

Not scoped further than this list yet -- pick this up as a dedicated pass,
porting a handful of the more representative demos first (a font demo, an
image demo, an annotation demo) rather than all ~25 at once, following this
project's own established milestone-by-milestone discipline.

### First pass, done and veraPDF-validated, 2026-09-13

Picked up exactly the "handful first" scope this section itself asked for:
three new tagged demos (a font demo, an image demo, an annotation demo),
plus the test-coverage script prerequisite. The remaining ~22 upstream
demos (vector graphics, CJK/Type1 fonts, encryption/permissions, PDF/A
conformance, attachments, slide-show/grid-sheet layout) are a deliberate,
explicit scope cut for a later pass -- see "Not done" below.

**`demo/tagged_font_demo.c`** (`demo/tagged_font_demo.c`) -- ported from
libharu's original `ttfont_demo.c` (found at
`source/migrate-codex-7/lib/haru/demo/ttfont_demo.c` in the sibling
Migrate tree; this project's own `vendor/libharu/` dropped `demo/` when
first vendored, see `NOTICE.md`). Embeds the same `fonts/DejaVuSans.ttf`
this project already vendors (`HPDF_LoadTTFontFromFile(..., HPDF_TRUE)`)
and tags the specimen content -- font name, alphabet/digits, and the
sample sentence at three sizes -- as `Document > [H1, P, H2, P, H2, P]`
instead of the original's plain, untagged text. A decorative divider
rule is marked as an Artifact, matching `tagged_table_demo.c`'s
border-rectangle precedent. **Result: 106/106 PDF/UA-1 checks, veraPDF
prints `PASS`, on the first attempt** (1025/1025 individual checks);
`leaks --atExit` clean (0 leaks).

**`demo/tagged_image_demo.c`** (`demo/tagged_image_demo.c`) -- ported
from libharu's original `raw_image_demo.c`, not `png_demo.c`/
`jpeg_demo.c`: this project's `CMakeLists.txt` deliberately disables
libpng discovery (`CMAKE_DISABLE_FIND_PACKAGE_PNG ON`, see its own
comment), so a PNG-based port would have needed a new build dependency
this project doesn't otherwise carry. `raw_image_demo.c`'s approach
needs no image-decoding library at all, matching this section's own
"or raw_image_demo if that's simpler" suggestion. Rather than vendor a
new binary test-image asset (which would have meant a new
`NOTICE.md`/license entry, same rigor as `fonts/DejaVuSans-LICENSE.txt`
-- deliberately avoided since it wasn't needed), this demo computes two
small images at runtime (a 64x64 RGB gradient and a 64x16 grayscale
ramp) and tags each as its own `Document > Figure` with real, accurate
`/Alt` text describing the actual computed pixel pattern, followed by
its own `Document > Caption` -- matching `tagged_histogram_demo.c`'s
Figure+Caption pattern exactly, now exercising `HPDF_LoadRawImageFromMem()`
or `HPDF_Page_DrawImage()` (a real libharu code path no prior demo in
this project touched) instead of only path-painting/text operators.
**Result: 106/106 PDF/UA-1 checks, veraPDF prints `PASS`, on the first
attempt** (469/469 individual checks); `leaks --atExit` clean.

**`demo/tagged_annotation_demo.c`** (`demo/tagged_annotation_demo.c`) --
ported from libharu's original `link_annotation.c`: an index page with
three real link annotations (two internal page-jump links via
`HPDF_Page_CreateLinkAnnot()`, one external URI link via
`HPDF_Page_CreateURILinkAnnot()`) plus two minimal tagged destination
pages. This is the one demo in this pass that needed real, new engineering,
not just porting existing calls onto existing tagging API: annotations are
not part of any page content stream, so `HPDF_UA_BeginMarkedContent()`
cannot wrap them the way it wraps ordinary drawing operators. Added
**`HPDF_UA_TagAnnotation()`** (`include/hpdf_ua/hpdf_ua.h`,
`src/ua/hpdf_ua_structure.c`) -- a new, real function, not a stub --
which:
- appends a real `/OBJR` (object-reference) kid to a `HPDF_UA_ROLE_LINK`
  element's own `/K` array, the PDF mechanism for a structure element to
  "contain" an annotation (PDF 32000-1 14.7.4.3);
- assigns the annotation a fresh `/StructParent` key into this context's
  existing `/ParentTree` `/Nums` (the same flat number tree
  `HPDF_UA_BeginMarkedContent()` already uses for pages' `/StructParents`
  keys -- one shared monotonic counter, so the two key domains never
  collide, confirmed by direct testing, not just by inspection);
- normalizes the annotation's `/F` flags to Print-set/NoView-clear (ISO
  14289-1:2014 7.18) -- none of libharu's own annotation constructors set
  `/F` by default, confirmed by reading `hpdf_annotation.c` directly.

**Two real, veraPDF-caught findings this demo produced, not found by
inspection alone** (matching this project's own established pattern from
every prior milestone -- see Milestone 1's own "found by veraPDF, not by
inspection" bugs):
1. The first `HPDF_UA_TagAnnotation()` pass (OBJR + `/StructParent` +
   `/F` only) still failed ISO 14289-1:2014 7.18.5 ("Links shall contain
   an alternate description via their Contents key", PDF 32000-1 14.9.3)
   with 3 failed checks (one per link annotation) -- a real, separate
   PDF/UA-1 requirement from the structure-tree association itself: a
   `/Alt` on the *structure element* does not satisfy it, only a
   `/Contents` entry on the *annotation* does. Fixed by having
   `HPDF_UA_TagAnnotation()` copy the associated element's own `/Alt`
   text (if `HPDF_UA_SetAlternateText()` was already called on it) onto
   the annotation's `/Contents` key -- one real description serving both
   purposes, documented in `hpdf_ua.h`'s own comment on the function so
   a future caller understands why setting `/Alt` first matters here.
2. Confirmed, via the same real run, that this fix actually closed the
   gap: re-running `validate/run_verapdf.sh` after the `/Contents` fix
   moved this demo from 105/106 rules (784 passed / 3 failed checks) to
   **106/106 (787/787 checks, veraPDF prints `PASS`)** -- verified
   directly, not assumed.

`leaks --atExit` clean (0 leaks) on the fixed version.

**All five pre-existing demos re-verified unchanged after these library
additions** (the new `HPDF_UA_TagAnnotation()` code is purely additive,
touching no existing function): `tagged_table_demo` 106/106 (595/595
checks), `tagged_histogram_demo` 106/106 (252/252), `tagged_skyline_demo`
106/106 (379/379), `tagged_example_demo` 106/106 (1063/1063), each
re-run through `leaks --atExit` (0 leaks). `docmeta_demo` (Milestone 0
scaffolding, not a fully tagged demo, never claimed full PDF/UA-1
compliance) unchanged at its own known 103/106 baseline.

**Test coverage (`tests/run_all_demos.sh`, this pass's other named
prerequisite)**: a new script -- `tests/run_all_demos.sh` -- builds all
eight demos (the original five plus these three new ones), runs each to
regenerate its PDF, and runs `validate/run_verapdf.sh` against every one,
asserting a per-demo recorded baseline (0 failed rules for the seven
fully tagged demos; at most 3 for `docmeta_demo`'s own documented,
deliberate Milestone-0 partial scope) rather than a flat "must be
`PASS`" that would incorrectly fail `docmeta_demo` for something it was
never scoped to fix. Fails loudly (nonzero exit, one `FAIL:` line per
regressed demo naming the demo and how many rules/checks regressed) if
any demo's real veraPDF result gets worse than its baseline. **Verified
to actually catch a regression, not just written and assumed to work**:
temporarily made `HPDF_UA_SetAlternateText()` a no-op (simulating a
real future refactor bug), rebuilt, and re-ran the script -- it
correctly reported `FAIL` for exactly the five demos that depend on
`/Alt` (histogram, skyline, example, image, and annotation -- the last
because `HPDF_UA_TagAnnotation()`'s `/Contents` copy also depends on
`/Alt`) while correctly leaving `tagged_table_demo` and
`tagged_font_demo` (neither of which calls `HPDF_UA_SetAlternateText()`)
passing; then reverted the injected bug and reconfirmed a clean rebuild
puts all eight demos back at their recorded baselines. This is real
coverage of libharu's own original TrueType-embedding, raw-image, and
annotation code paths -- exercised through this project's tagging layer
on every build, not a no-op placeholder.

**Not done in this pass, deliberately, per this section's own "handful
first" scope**:
- The other ~22 upstream demos this section's own list named (vector
  graphics `arc_demo`/`line_demo`, CJK/Type1 fonts, `encoding_list`,
  encryption/permissions, PDF/A conformance, attachments, outline demos,
  slide-show/grid-sheet layout) are not ported. A real, explicit scope
  cut, not an oversight -- pick up the next representative slice in a
  future pass the same way this one did.
- `README.md`'s own "how to run" demo list (lines ~57-61) was not
  updated to mention the three new binaries -- left alone deliberately
  per this pass's explicit instructions not to touch `README.md` unless
  a license-file addition was genuinely required (it wasn't: the image
  demo computes its images at runtime rather than vendoring a new
  binary asset, specifically to avoid that). A real, small, known gap:
  `README.md` is accurate about the original five demos but silent on
  the three new ones.
- No lighter-weight direct-metadata check (e.g. `qpdf --qdf` + grep, as
  `tests/README.md` originally sketched for document-level-only demos)
  was added -- `tests/run_all_demos.sh`'s real veraPDF run already
  covers every demo this project actually ships, including
  `docmeta_demo`, so the lighter-weight variant would have been
  redundant coverage, not new coverage.
- Milestone 6's own second prerequisite ("test coverage of libharu's own
  original functionality") is now covered for exactly the functionality
  these three new demos exercise (TrueType embedding/subsetting, raw
  RGB/grayscale image drawing, link/URI annotations, destinations) --
  not for the untouched ~22-demo remainder (CJK fonts, encryption,
  PDF/A, attachments, etc.), which still has zero coverage in this
  project. A real, bounded gap, not a completed prerequisite.

### Second pass, CI + install target + API docs, 2026-09-17

Picked up three concrete public-release-readiness gaps identified while
diagnosing a "cannot build from a fresh clone" report (see `CHANGES.md`'s
`0.6.4` entry for the full detail on each):

- **CI** (`.github/workflows/ci.yml`): builds on Linux and macOS for
  every push/PR and runs both test scripts below. Nothing previously
  caught a regression before a user hit it.
- **CMake install target**: `cmake --install build` was a complete
  no-op before this pass (zero `install()` rules anywhere in this
  project's own `CMakeLists.txt`; `vendor/libharu`'s own were
  `EXCLUDE_FROM_ALL`'d out along with the rest of that subdirectory).
  Now installs `hpdf_ua` + `hpdf` + headers + a `hpdf_ua-config.cmake`
  package config, so `find_package(hpdf_ua)` works for a downstream
  consumer. Verified end-to-end, not just written and assumed: a new
  `tests/test_install.sh` installs to a throwaway prefix and builds a
  separate minimal consumer project (`tests/consume_package/`) against
  it, now wired into CI.
- **A real bug this work surfaced, not sought out**: fixing the install
  target's include-directory usage requirements (`PRIVATE` &rarr;
  `PUBLIC`/`INTERFACE` for `vendor/libharu/include`) revealed that every
  demo's own compile had been silently resolving `hpdf.h` through an
  unrelated, differently-versioned system-wide libharu install on the
  development machine (`/usr/local/include`, confirmed via `cc -H`),
  not this project's own vendored copy -- because `hpdf_ua`'s public
  header (`hpdf_ua.h`) does `#include "hpdf.h"` but the target's own
  include-directory usage requirements never actually exposed
  `vendor/libharu/include` to anything outside `hpdf_ua`'s own `.c`
  files. A genuinely clean machine (no stray libharu install) would
  have failed every demo compile with "hpdf.h: No such file or
  directory" -- a real, environment-dependent gap between "builds here"
  and "builds anywhere," and plausibly the actual mechanism behind
  reports of a fresh clone failing to build. Fixed as part of the same
  change that makes the install target correct.
- **API docs** (`docs/api.md`): a grouped map of every `hpdf_ua.h`
  function plus a minimal complete example -- the demos were previously
  the only usage documentation.

Deliberately not done in this pass (see this section's own "Not done"
list above for the pre-existing ~22-demo gap, still open): the actual
outreach comment on libharu's own dead upstream issues (#99, #175) is
being held until Milestone 6's remaining demo-coverage work is further
along, a separate, later decision.

### Third pass, 7 more tagged demos (vector graphics, text/font
features, outline), 2026-09-17

Picked up the largest remaining slice of the ~22-demo gap this
section's first pass explicitly deferred, split into batches by the
user's own request rather than ported all at once, matching this
project's "handful first" discipline. This batch: everything portable
with **no new dependency and no new licensed asset** --
`demo/tagged_arc_demo.c`, `demo/tagged_line_demo.c`,
`demo/tagged_ext_gstate_demo.c`, `demo/tagged_font_list_demo.c`,
`demo/tagged_text_demo.c`, `demo/tagged_encoding_list_demo.c`, and
`demo/tagged_outline_demo.c` -- see `CHANGES.md`'s `0.6.5` entry for
what each one exercises and how it was verified (6 of 7 reach
106/106-check full PDF/UA-1 compliance on the first attempt;
`tagged_font_list_demo` at its own documented, deliberate 2-failed-rule
baseline, the same non-embedded-Standard-14-font gap `docmeta_demo`
already has). All 15 demos this project now ships are wired into
`tests/run_all_demos.sh` and pass or beat their recorded baseline; all
7 new ones confirmed `leaks --atExit` clean.

Two design calls the user made explicitly before this batch started
(see this conversation's own record, not re-litigated here): PNG
demos will get a real libpng dependency when their batch comes up
(reversing this project's current `CMAKE_DISABLE_FIND_PACKAGE_PNG ON`),
and the CJK-font demos will get a real embedded font (Noto Sans CJK,
OFL-licensed) rather than porting them non-compliant.

**Remaining, explicitly deferred to later batches** (this pass's own
scope cut, not an oversight):
- **Images** (needs the libpng decision above): `png_demo.c`,
  `image_demo.c`. `jpeg_demo.c` needs no new dependency (libharu embeds
  JPEG bytes as-is, no decode) but does need the two bundled JPEG
  assets (`demo/images/rgb.jpg`, `gray.jpg`) vendored with their own
  license check -- grouped with the PNG batch since it's the same
  "images" slice, not because it shares PNG's dependency.
- **CJK** (needs the Noto Sans CJK decision above): `chfont_demo.c`,
  `ttfont_demo_jp.c`, `jpfont_demo.c`, `outline_demo_jp.c`,
  `character_map.c` (a CJK glyph-table inspection tool, takes an
  encoding name as a command-line argument -- grouped here since it is
  fundamentally a CJK-font tool, not because it itself needs the new
  font, though using it well benefits from one).
- **Security/annotations/attachments**: `encryption.c`, `permission.c`,
  `text_annotation.c` (needs no new API --
  `HPDF_UA_TagAnnotation()` already works with any annotation role, not
  just `HPDF_UA_ROLE_LINK`, per its own doc comment), `attach.c` (needs
  a small file to attach -- an existing project doc would do, no new
  asset), `slide_show_demo.c`. No new dependency needed for any of
  these; deferred purely for batch-size discipline, not a real blocker.
- **`pdf_a_conformance.c`**: deliberately separated out rather than
  grouped with any batch above -- combining PDF/A conformance
  switching with this project's own PDF/UA-1 tagging in one document
  is a real design question (two conformance regimes' metadata/
  structure requirements interacting), not just a porting exercise,
  and deserves its own dedicated pass.
- `grid_sheet.c`/`make_rawimage.c` are original libharu helper/utility
  files (a background-grid-drawing helper reused by several original
  demos, and a one-off raw-image-file generator), not demos in their
  own right -- not tracked as a remaining port.

### Fourth pass, security/annotations/attachments, 2026-09-17

Picked up the "security/annotations/attachments" batch the third pass's
own list above named as needing no new dependency: `tagged_encryption_demo.c`
(merges `encryption.c` + `permission.c`), `tagged_text_annotation_demo.c`
(`text_annotation.c`, 4 of 8 icons), `tagged_attach_demo.c` (`attach.c`,
attaching this project's own `CHANGES.md` rather than a new binary
asset), and `tagged_slide_show_demo.c` (`slide_show_demo.c`, 4 of 17
transition styles plus a real Next/Prev link chain). See `CHANGES.md`'s
`0.6.6` entry for the full detail on each -- all 4 reach 106/106 full
PDF/UA-1 compliance, and all 19 demos this project now ships pass or
beat their recorded baseline.

**Three real bugs found and fixed in this pass, not sought out** (see
`CHANGES.md`'s `0.6.6` entry for the full detail on each): libharu's
`HPDF_SetPermission()` silently clears the PDF-spec-reserved
accessibility-extraction permission bit unless the caller explicitly
ORs `HPDF_PERMISSION_PAD` back in; ISO 14289-1:2014 7.18.1 requires
non-Link/Widget/PrinterMark annotations to be nested under a structure
element literally named "Annot" (not just any container), which led to
a real, deliberate new addition to this project's own public API --
`HPDF_UA_ROLE_ANNOT` -- a documented exception to the "every role name
is an ISO 32000-1 standard type" design rule, since "Annot" was only
formally standardized in PDF 2.0; and `HPDF_AttachFile()` returns a
`HPDF_EmbeddedFile` pointer, not a `HPDF_STATUS`, so a naive
`!= HPDF_OK` check on it is backwards.

**Remaining, explicitly deferred** (unchanged from the third pass's own
list): images (`png_demo.c`/`image_demo.c`/`jpeg_demo.c`, needs the
already-decided libpng dependency), CJK
(`chfont_demo.c`/`ttfont_demo_jp.c`/`jpfont_demo.c`/`outline_demo_jp.c`/
`character_map.c`, needs the already-decided Noto Sans CJK font), and
`pdf_a_conformance.c` on its own (a real design question, not a batch
item).

### Fifth pass, PNG images, 2026-09-17

Picked up the images batch's `png_demo.c` and `image_demo.c` (the
already-decided libpng dependency): `demo/tagged_png_demo.c` (6 of 15
original images, one representative bit depth per PNG color type) and
`demo/tagged_image_transform_demo.c` (5 of 7 original examples: actual
size, scaling, rotation, image mask, color mask). Both reach 106/106
full PDF/UA-1 compliance. See `CHANGES.md`'s `0.6.7` entry for the full
detail, including **a third real instance of this project's "stray
environment artifact silently shadows the real build" bug class**
(a stale, gitignored `vendor/libharu/include/hpdf_config.h` left behind
by an unrelated tool, disabling PNG support silently) and the
`target_include_directories()` reordering that hardens against a
recurrence.

`jpeg_demo.c` -- this batch's third named item -- is deliberately NOT
ported: its two sample images (`demo/images/rgb.jpg`, `gray.jpg`) have
no stated license anywhere in upstream libharu, unlike PNGSuite's own
explicit README. Left open rather than resolved on an unstated
assumption; revisit if a clearly-licensed replacement image (or an
explicit decision to accept the risk) is decided later.

**Remaining, explicitly deferred**: CJK
(`chfont_demo.c`/`ttfont_demo_jp.c`/`jpfont_demo.c`/`outline_demo_jp.c`/
`character_map.c`, needs the already-decided Noto Sans CJK font),
`jpeg_demo.c` (blocked on the image-license question above), and
`pdf_a_conformance.c` on its own.
