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

`HPDF_UA_SetAlternateText()` itself is already done (brought forward into
Milestone 1, see above -- a one-line `/Alt` addition once
`HPDF_UA_StructElem` existed). What remains:

- Exercise the same structure-tree/marked-content machinery from Milestone 1
  against a bar-chart-style figure (matching Migrate's own
  `plot_svg.c`-equivalent histogram rendering), not just a table -- confirms
  the API generalizes rather than being table-shaped only.
- A real demo (`demo/tagged_histogram_demo.c` or similar), veraPDF-validated
  the same way Milestone 1's table demo was.

## Milestone 3 -- one tagged line/skyline-style plot page

- Same as Milestone 2, for a multi-series line plot with axes/legend/
  captions -- the shape Migrate's skyline figures need. Confirms axis labels,
  legends, and captions all have a sensible structure-element home (this is
  the point where the role map likely needs `Caption`, and where reading
  order across multiple overlaid series needs a real decision, not just
  "whatever order the code happens to draw them in").

## Milestone 4 -- outline/bookmarks, document-level polish

`HPDF_UA_BeginArtifact()`/`HPDF_UA_EndArtifact()` are already done (brought
forward into Milestone 1, see above -- a real veraPDF failure on
Milestone 1's own demo made this a present, not hypothetical, need). What
remains:

- Tie `HPDF_CreateOutline()`'s existing bookmark mechanism to real structure
  elements (a PDF/UA requirement libharu's existing outline support doesn't
  yet satisfy on its own).
- `/Tabs /S` on every page.
- The two real gaps Milestone 1's veraPDF run found but left unfixed
  (XMP `/Metadata` stream; embedded fonts) belong here too, alongside
  `HPDF_UA_SetTableDataHeaders()` (`/Headers`, for irregular tables --
  still stubbed, `/Scope` covers Migrate's actual simple-table needs so
  far).

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
