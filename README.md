# libharu-pdfua

[![CI](https://github.com/pbeerli/libharu-pdfua/actions/workflows/ci.yml/badge.svg)](https://github.com/pbeerli/libharu-pdfua/actions/workflows/ci.yml)

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

**Milestones 1-5 done, Milestone 6 (public-release prerequisites) under
way, 2026-09-17 -- nineteen demos, seventeen of them fully PDF/UA-1
compliant (106/106 checks under veraPDF, "PASS," not just a lower
failure count); CI, a CMake install target, and an API reference are
now in place too.** See `docs/roadmap.md` for the full milestone list
and `CHANGES.md` for exactly what exists right now. Real, working:
document-level metadata (`/Lang`, `/DisplayDocTitle`,
`/MarkInfo`+`/StructTreeRoot`, a real XMP `/Metadata` stream declaring
PDF/UA-1 conformance), a real structure tree and marked-content tagging
(`HPDF_UA_Context`, `HPDF_UA_BeginStructureElement()`,
`HPDF_UA_BeginMarkedContent()`, table-header `/Scope`, figure `/Alt`,
artifact marking, automatic `/Tabs /S`), a real document outline tied
to each demo's actual page (including a real multi-entry outline
tree), tagged link and non-link annotations (`HPDF_UA_TagAnnotation()`:
`/OBJR`, `/StructParent`, `/Contents`, and a real `HPDF_UA_ROLE_ANNOT`
role for non-Link annotations per ISO 14289-1:2014 7.18.1), and an
embedded font (DejaVu Sans) in place of never-embedded Standard-14
Helvetica. Beyond the original three report-shaped demos (table,
histogram/figure, multi-series skyline plot), Milestone 6 is
recreating libharu's own original demo set as tagged examples -- so
far a TrueType font demo, a raw-image (`Figure`) demo, a
link-annotation demo, a pie chart, a line/curve reference sheet, a
transparency/blend-mode figure, a Standard-14 font list (deliberately
not fully compliant -- see `demo/tagged_font_list_demo.c`), a
text-features demo, an encoding-selection demo, a multi-entry outline
demo, an encrypted/permission-restricted document, a Text-annotation
demo, a file-attachment demo, and a slide-show demo -- see `demo/` and
`tests/run_all_demos.sh`, which builds and PDF/UA-1-validates all
nineteen demos automatically (also run in CI on Linux and macOS for
every push/PR, see `.github/workflows/ci.yml`); the remaining ~9 demos
from libharu's own original set (CJK/Type1 fonts, PNG/JPEG images, and
PDF/A conformance) are not yet ported, a known, tracked gap (see
`docs/roadmap.md`'s Milestone 6 section). This project can also now be
installed and consumed from another CMake project via
`find_package(hpdf_ua)` instead of only vendored wholesale -- see
`docs/api.md` and this README's "Using this library in your own
project" section. Migrate integration is recommended (as a new,
additive `report_pdf_tagged.c` backend, not a rewrite of Migrate's
existing PDF path) -- see `docs/roadmap.md`'s Milestone 5 section;
timing of that integration is a separate, still-open scheduling call.

## Requirements

- CMake 3.16+
- A C99 compiler
  - **macOS**: Xcode Command Line Tools (`xcode-select --install`). If you
    already have Xcode installed but never opened it or accepted its
    license, `cc` will fail with "You have not agreed to the Xcode license
    agreements" -- run `sudo xcodebuild -license` (from a real terminal,
    not a non-interactive script) to fix that.
  - **Linux**: gcc or clang, plus `make`.
- Optional: zlib and libpng dev headers, auto-detected by CMake, for
  libharu's own (unused by this project) compressed-stream/PNG support.
- [veraPDF](https://software.verapdf.org/) (CLI or Docker) if you want to
  validate output -- see "Validating output" below.

## Building

```sh
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
cd build
./docmeta_demo          # exercises the Milestone 0 document-metadata functions
./tagged_table_demo     # produces a real, PDF/UA-1-checked tagged table
./tagged_histogram_demo # produces a real, PDF/UA-1-checked tagged figure
./tagged_skyline_demo   # produces a real, PDF/UA-1-checked multi-series plot
./tagged_example_demo   # text + table + figure together, one document
./tagged_font_demo      # embedded TrueType font, tagged text (ttfont_demo.c port)
./tagged_image_demo     # tagged Figure from a raw computed image (raw_image_demo.c port)
./tagged_annotation_demo # tagged link annotations (link_annotation.c port)
./tagged_arc_demo       # tagged pie chart (arc_demo.c port)
./tagged_line_demo      # tagged line/dash/cap/join/curve reference sheet (line_demo.c port)
./tagged_ext_gstate_demo # tagged transparency/blend-mode figure (ext_gstate_demo.c port)
./tagged_font_list_demo # tagged Standard-14 font list (font_demo.c port; deliberately not fully compliant, see file header)
./tagged_text_demo      # tagged text features: size/rendering-mode/rotate/spacing/alignment (text_demo.c + text_demo2.c port)
./tagged_encoding_list_demo # tagged text in three encodings on one embedded font (encoding_list.c port)
./tagged_outline_demo   # tagged 3-page document with a multi-entry outline tree (outline_demo.c port)
./tagged_encryption_demo # tagged, password-protected, permission-restricted document (encryption.c + permission.c port)
./tagged_text_annotation_demo # tagged Text (popup/note) annotations (text_annotation.c port)
./tagged_attach_demo    # tagged document with an embedded file attachment (attach.c port)
./tagged_slide_show_demo # tagged 4-page slide show with a Next/Prev link chain (slide_show_demo.c port)
cd ..
```

Each demo `HPDF_SaveToFile()`s to a plain relative filename (e.g.
`"docmeta_demo.pdf"`), so it lands wherever your shell's current
directory is when you run it -- `cd build` first, as above, so the
output PDFs land there instead of scattered into the repo root.
`tests/run_all_demos.sh` (see "Validating output" below) does this
`cd` for you automatically.

Unix/Linux and macOS are the supported platforms; on Windows, build under a
POSIX-compatible layer (WSL, Cygwin, or MSYS2) using these same instructions
-- there is no native Windows/MSVC build path, matching Migrate's own
current platform decision.

## Validating output

To check all nineteen demos against their recorded PDF/UA-1 baselines at
once (this is what CI runs):

```sh
tests/run_all_demos.sh build
```

Seventeen of the nineteen demos are fully PDF/UA-1 compliant -- veraPDF
reports 0 failed rules. Two are deliberately not, by design, not a bug,
each with its own recorded baseline `tests/run_all_demos.sh` checks
against instead of full compliance: `docmeta_demo` (Milestone 0
scaffolding: untagged body text and a non-embedded Standard-14 font,
"at most 3 failed rules") and `tagged_font_list_demo` (exists
specifically to show libharu's non-embeddable Standard-14 fonts, "at
most 2 failed rules" -- see that file's own top comment). Both are
explained in `docs/roadmap.md`.

To check a single PDF by hand instead:

```sh
validate/run_verapdf.sh build/tagged_table_demo.pdf
```

Requires veraPDF (either a local `verapdf` CLI install from
https://software.verapdf.org/, or Docker -- the script falls back to the
published `verapdf/cli` image automatically). See
`docs/pdf_ua_requirements.md` for what's actually being checked.

## Using this library in your own project

```sh
cmake --install build --prefix /your/chosen/prefix
```

then, from your own project's `CMakeLists.txt`:

```cmake
find_package(hpdf_ua REQUIRED)
target_link_libraries(your_target PRIVATE hpdf_ua::hpdf_ua)
```

See `docs/api.md` for the full API reference and a minimal complete
example, or vendor this whole tree (as this project itself does with
libharu) if you'd rather not install anything system-wide.

## Layout

- `vendor/libharu/` -- libharu 2.4.5, unmodified (see `NOTICE.md`).
- `include/hpdf_ua/`, `src/ua/` -- this project's own additions.
- `demo/` -- demo programs, one per roadmap milestone's target output.
- `fonts/` -- an embeddable demo font (DejaVu Sans; see its own
  `DejaVuSans-LICENSE.txt`), used by the demos so their output PDFs pass
  PDF/UA-1's embedded-fonts requirement.
- `validate/` -- veraPDF wrapper script.
- `docs/` -- the PDF/UA-1 requirements checklist, the roadmap, and the
  API reference (`docs/api.md`).
- `tests/` -- `run_all_demos.sh`: builds every demo and validates each
  against veraPDF, asserting each one matches or beats its recorded
  PDF/UA-1 baseline. `test_install.sh`: verifies the CMake install
  target and `find_package(hpdf_ua)` actually work end-to-end.
- `.github/workflows/ci.yml` -- runs both test scripts above on Linux
  and macOS for every push/PR.

## License

zlib/libpng-style license (same as upstream libharu), see `LICENSE`. This is
a permissive, MIT-compatible license that predates and is independent of
this project.
