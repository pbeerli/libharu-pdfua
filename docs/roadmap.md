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

- Implement real `/StructTreeRoot` population: `HPDF_UA_BeginStructureElement()`/
  `HPDF_UA_EndStructureElement()` building an actual structure-element tree
  (not the current empty placeholder), with a role map covering `Table`/
  `TR`/`TH`/`TD` plus the generic `Document`/`Sect`/`P` types.
- Implement real marked-content tagging: `HPDF_UA_BeginMarkedContent()`/
  `HPDF_UA_EndMarkedContent()` emitting `BDC .../EMC` into the page content
  stream with a real, page-unique MCID, and building the `/ParentTree`
  entries that tie each MCID back to its structure element.
- Implement `/Scope` (or `/Headers`) table-header association.
- Prototype output: one page, one table, with real headers, validated
  end-to-end with veraPDF (`--flavour ua1`) -- the first milestone where
  "does this pass PDF/UA-1 validation" is a real yes/no answer, not aspirational.

## Milestone 2 -- one tagged figure/histogram page

- `HPDF_UA_SetAlternateText()` on a `Figure` structure element.
- Exercise the same structure-tree/marked-content machinery from Milestone 1
  against a bar-chart-style figure (matching Migrate's own
  `plot_svg.c`-equivalent histogram rendering), not just a table -- confirms
  the API generalizes rather than being table-shaped only.

## Milestone 3 -- one tagged line/skyline-style plot page

- Same as Milestone 2, for a multi-series line plot with axes/legend/
  captions -- the shape Migrate's skyline figures need. Confirms axis labels,
  legends, and captions all have a sensible structure-element home (this is
  the point where the role map likely needs `Caption`, and where reading
  order across multiple overlaid series needs a real decision, not just
  "whatever order the code happens to draw them in").

## Milestone 4 -- artifacts, outline/bookmarks, document-level polish

- `HPDF_UA_MarkArtifact()` for decorative/non-content marks (page borders,
  repeated headers).
- Tie `HPDF_CreateOutline()`'s existing bookmark mechanism to real structure
  elements (a PDF/UA requirement libharu's existing outline support doesn't
  yet satisfy on its own).
- `/Tabs /S` on every page.

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
