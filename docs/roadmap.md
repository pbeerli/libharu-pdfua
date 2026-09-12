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

Per the original plan's own framing: "If the prototype validates and remains
maintainable, Migrate can vendor the layer, use it as a submodule, or import
the small API as a local PDF backend. If it fails, Migrate can still keep
Markdown/SVG output without carrying a half-working PDF/UA implementation."
Revisit at this point, with three real validated prototypes (table, figure,
plot) in hand, not before.

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
