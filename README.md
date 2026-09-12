# libharu-pdfua

Tagged-PDF (PDF/UA-1 / ISO 14289-1) extensions for [libharu](https://github.com/libharu/libharu),
built as a small, generic module on top of an unmodified vendored copy of
the library -- no dependency on any particular application.

## Why

[Migrate-n](https://github.com/pbeerli/migrate-5.0.7) (population-genetics
MCMC software) generates its graphical report as a PDF via libharu. libharu
can write basic PDF/A metadata but has no tagged-PDF support at all -- no
populated structure tree, no marked content, no alternate text, no table
header associations. That isn't enough for a screen reader to make sense of
the document, and it won't pass PDF/UA-1 validation. Two libharu upstream
issues asking for exactly this (#99, opened 2015; #175, opened 2018) have
sat with no maintainer response for years. This project builds it as a
standalone extension instead of inside Migrate itself, so it's reusable by
anyone using libharu, and so Migrate's own report code stays uncoupled from
this project's design churn while it's worked out.

**Terminology note**: WCAG 2.1 AA is a web-content (HTML/CSS/JS) standard;
there's no PDF-specific WCAG conformance level. The PDF-specific standard
that operationalizes the same accessibility intent, and the one an actual
validator checks, is **PDF/UA-1**. That's this project's real, concrete
target -- see `docs/pdf_ua_requirements.md`.

## Status

**Milestones 1-3 done, 2026-09-12.** See `docs/roadmap.md` for the full
milestone list and `CHANGES.md` for exactly what exists right now. In
short: real, working document-level metadata (`/Lang`, `/DisplayDocTitle`,
`/MarkInfo`+`/StructTreeRoot`), a real structure tree and marked-content
tagging (`HPDF_UA_Context`, `HPDF_UA_BeginStructureElement()`,
`HPDF_UA_BeginMarkedContent()`, table-header `/Scope`, figure `/Alt`,
artifact marking) that works for text content
(`demo/tagged_table_demo.c`), path-painting content like chart bars/axes
(`demo/tagged_histogram_demo.c`), and multi-series polylines/dashed
strokes with a real reading-order decision for legend text
(`demo/tagged_skyline_demo.c`) -- all three verified against the actual
standard: **104 of 106 PDF/UA-1 checks pass under veraPDF** for each,
clean on the first attempt for the second and third demos. Two real,
understood gaps remain (no XMP `/Metadata` stream yet; the demos'
Standard-14 font isn't embedded), both deferred to Milestone 4 -- **this
project's demos are not yet fully PDF/UA-1 conformant**, but "does this
pass PDF/UA-1 validation" is now a real, mostly-yes answer, not
aspirational.

## Building

```sh
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/docmeta_demo          # exercises the Milestone 0 document-metadata functions
./build/tagged_table_demo     # produces a real, PDF/UA-1-checked tagged table
./build/tagged_histogram_demo # produces a real, PDF/UA-1-checked tagged figure
./build/tagged_skyline_demo   # produces a real, PDF/UA-1-checked multi-series plot
```

Unix/Linux and macOS are the supported platforms; on Windows, build under a
POSIX-compatible layer (WSL, Cygwin, or MSYS2) using these same instructions
-- there is no native Windows/MSVC build path, matching Migrate's own
current platform decision.

## Validating output

```sh
validate/run_verapdf.sh build/docmeta_demo.pdf
```

Requires veraPDF (either a local `verapdf` CLI install from
https://software.verapdf.org/, or Docker -- the script falls back to the
published `verapdf/cli` image automatically). See
`docs/pdf_ua_requirements.md` for what's actually being checked.

## Layout

- `vendor/libharu/` -- libharu 2.4.5, unmodified (see `NOTICE.md`).
- `include/hpdf_ua/`, `src/ua/` -- this project's own additions.
- `demo/` -- demo programs, one per roadmap milestone's target output.
- `validate/` -- veraPDF wrapper script.
- `docs/` -- the PDF/UA-1 requirements checklist and the roadmap.
- `tests/` -- placeholder; real CTest wiring lands with Milestone 1.

## License

zlib/libpng-style license (same as upstream libharu), see `LICENSE`. This is
a permissive, MIT-compatible license that predates and is independent of
this project.
