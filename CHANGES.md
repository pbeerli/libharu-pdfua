# Changes

Version numbering: `MAJOR.MINOR.PATCH`, starting at `0.1.0` (pre-1.0,
milestone-driven -- see `docs/roadmap.md`). Bump `MINOR` when a roadmap
milestone completes, `PATCH` for fixes within a milestone.

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
